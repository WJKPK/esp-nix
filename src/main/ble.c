/* 
 * Copyright 2023 WJKPK
 *  
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "ble.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"

#include "utilities/logger.h"

static const char* device_name = "ESP32";
uint16_t hrs_hrm_handle;
static uint16_t conn_handle;
static uint8_t ble_addr_type;
static error_t ble_advertise (void);
static int generic_read_access(uint16_t conn_handle, uint16_t attr_handle,
        struct ble_gatt_access_ctxt* ctxt, void* arg);

#define GET_FULL_UUID(simplified_uuid) 0x94, 0x56, simplified_uuid & 0xFF, (simplified_uuid & 0xFF00) >> 8, 0xB4, 0x11, 0x12, 0xFA, \
    simplified_uuid & 0xFF, (simplified_uuid & 0xFF00) >> 8, 0x24, 0x4C, 0x44, 0x14, 0x42, 0x80, 

static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    {
        /* Service: Heart-rate */
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(BASIC_CHARACTERISTIC_SERVICE_UUID),
        .characteristics = (struct ble_gatt_chr_def[])
        { {
              /* Characteristic: Heart-rate measurement */
              .uuid       = BLE_UUID128_DECLARE(GET_FULL_UUID(BASIC_CHARACTERISTIC_UUID)),
              .access_cb  = generic_read_access,
              .val_handle = &hrs_hrm_handle,
              .flags      = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
          },{
              0, /* No more characteristics in this service */
          }, }
    },
    {
        /* Service: Device Information */
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(DEVICE_INFO_UUID),
        .characteristics = (struct ble_gatt_chr_def[])
        { {
              /* Characteristic: * Manufacturer name */
              .uuid      = BLE_UUID128_DECLARE(GET_FULL_UUID(DEVICE_INFO_MANUFACTURER_NAME_UUID)),
              .access_cb = generic_read_access,
              .flags     = BLE_GATT_CHR_F_READ,
          },{
              /* Characteristic: Model number string */
              .uuid      = BLE_UUID128_DECLARE(GET_FULL_UUID(DEVICE_INFO_MODEL_NUMBER_UUID)),
              .access_cb = generic_read_access,
              .flags     = BLE_GATT_CHR_F_READ,
          },{
              0, /* No more characteristics in this service */
          }, }
    },

    {
        0, /* No more services */
    },
};

#define READ_WRITE_CALLBACKS_COUNT 8U
static read_observer_descriptor_t read_observers[READ_WRITE_CALLBACKS_COUNT];
static write_observer_descriptor_t write_observers[READ_WRITE_CALLBACKS_COUNT];


simplified_uuid_t ble_get_simplified_uuid(const ble_uuid_t* full_uuid) {
    ble_uuid_any_t uuid_full_casted = *(ble_uuid_any_t*)full_uuid;
    simplified_uuid_t result = 0;
    switch (uuid_full_casted.u.type) {
        case BLE_UUID_TYPE_128:
            result |= uuid_full_casted.u128.value[8];
            result |= ((uint16_t)uuid_full_casted.u128.value[9]) << 8;
            break;

        default:
            assert(false);
            break;
    }
    return result;
}

error_t ble_add_write_observer(write_observer_descriptor_t observer_descriptor) {
    for (unsigned i = 0; i < READ_WRITE_CALLBACKS_COUNT; i++) {
        if (write_observers[i].observer != NULL)
            continue;

        write_observers[i] = observer_descriptor;
        return error_any;
    }
    return collection_full;
}

error_t ble_add_read_observer(read_observer_descriptor_t observer_descriptor) {
    for (unsigned i = 0; i < READ_WRITE_CALLBACKS_COUNT; i++) {
        if (read_observers[i].observer != NULL)
            continue;

        read_observers[i] = observer_descriptor;
        return error_any;
    }
    return collection_full;
}

static ble_uuid_any_t ble_get_full_uuid(simplified_uuid_t simplified_uuid) {
    ble_uuid_any_t full_uuid = {
        .u128 = {
            .u.type = BLE_UUID_TYPE_128,
            .value = {GET_FULL_UUID(simplified_uuid)},
        }
    };
    return full_uuid;
}

static int ble_gap_event (struct ble_gap_event* event, void* arg) {
    error_t ret = error_any;
    switch (event->type) {
        case BLE_GAP_EVENT_CONNECT:
            /* A new connection was established or a connection attempt failed */ MODLOG_DFLT(INFO, "connection %s; status=%d\n",
              event->connect.status == 0 ? "established" : "failed",
              event->connect.status);

            if (event->connect.status != 0) {
                /* Connection failed; resume advertising */
                ret = ble_advertise();
            }
            conn_handle = event->connect.conn_handle;
            break;

        case BLE_GAP_EVENT_DISCONNECT:
            MODLOG_DFLT(INFO, "disconnect; reason=%d\n", event->disconnect.reason);

            /* Connection terminated; resume advertising */
            ret = ble_advertise();
            break;

        case BLE_GAP_EVENT_ADV_COMPLETE:
            MODLOG_DFLT(INFO, "adv complete\n");
            ret = ble_advertise();
            break;

        case BLE_GAP_EVENT_SUBSCRIBE:
            if (event->subscribe.attr_handle == hrs_hrm_handle) {
            } else if (event->subscribe.attr_handle != hrs_hrm_handle) {
            }
            ESP_LOGI("BLE_GAP_SUBSCRIBE_EVENT", "conn_handle from subscribe=%d", conn_handle);
            break;

        case BLE_GAP_EVENT_MTU:
            MODLOG_DFLT(INFO, "mtu update event; conn_handle=%d mtu=%d\n",
              event->mtu.conn_handle,
              event->mtu.value);
            break;
    }

    return ret == error_any ? 0 : -1;
}

static error_t ble_advertise (void) {
    struct ble_gap_adv_params adv_params;
    struct ble_hs_adv_fields fields;

    memset(&fields, 0, sizeof(fields));

    fields.flags = BLE_HS_ADV_F_DISC_GEN
      | BLE_HS_ADV_F_BREDR_UNSUP;
    fields.tx_pwr_lvl_is_present = 1;
    fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;
    fields.name             = (uint8_t*) device_name;
    fields.name_len         = strlen(device_name);
    fields.name_is_complete = 1;
    
    error_t ret = error_any;
    switch(ble_gap_adv_set_fields(&fields)) {
        case BLE_HS_EBUSY:
            ret = action_already_requested;
            goto error_handler;
        case BLE_HS_EMSGSIZE:
            ret = memory_constraint_exceeded; 
            goto error_handler;
        default:
            break;
    }

    /* Begin advertising */
    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
    if (0 != ble_gap_adv_start(ble_addr_type, NULL, BLE_HS_FOREVER, &adv_params, ble_gap_event, NULL)) {
        ret = library_error;
        goto error_handler;
    }

    return error_any;
error_handler:
    error_print_message(ret);
    return ret;

}

error_t ble_notify_custom(uint16_t handle, uint8_t* buff, size_t len) {
    struct os_mbuf *om = ble_hs_mbuf_from_flat(buff, len);
    return 0 == ble_gatts_notify_custom(conn_handle, handle, om) ? error_any : library_error;
}

static bool is_uuid_in_filter(const ble_uuid_t* uuid, simplified_uuid_t const* filter, unsigned count) {
    for (unsigned i = 0; i < count; i++) {
        const ble_uuid_any_t full_uuid = ble_get_full_uuid(filter[i]);
        if (0 == ble_uuid_cmp(uuid , (const ble_uuid_t *)&full_uuid))
            return true;
    }
    return false;
}

static int generic_read_access(uint16_t conn_handle, uint16_t attr_handle,
  struct ble_gatt_access_ctxt* ctxt, void* arg) {
    const ble_uuid_t* ctx_uuid = ctxt->chr->uuid;
    for (unsigned i = 0; i < READ_WRITE_CALLBACKS_COUNT; i++) {
        if(!is_uuid_in_filter(ctx_uuid, read_observers[i].uuid_filter, read_observers[i].filter_count))
            continue;

        size_t payload_size = 0;
        uint8_t* buffer = NULL;
        read_observers[i].observer(ble_get_simplified_uuid(ctx_uuid), &buffer, &payload_size);
        if (0 != os_mbuf_append(ctxt->om, buffer, payload_size))
            return BLE_ATT_ERR_INSUFFICIENT_RES;
    }
    return 0;
}

static size_t mbuf_size(struct os_mbuf *om) {
    if (OS_MBUF_PKTHDR(om))
        return OS_MBUF_PKTLEN(om);
    return os_mbuf_len(om);
}

static inline bool is_vla_capable(size_t size) {
    const size_t max_vla_size = 256;
    if (size <= max_vla_size)
        return true;
    return false;
}

static int generic_write_access(uint16_t conn_handle, uint16_t attr_handle,
  struct ble_gatt_access_ctxt* ctxt, void* arg) {
    const ble_uuid_t* ctx_uuid = ctxt->chr->uuid;
    for (unsigned i = 0; i < READ_WRITE_CALLBACKS_COUNT; i++) {
        if(!is_uuid_in_filter(ctx_uuid, write_observers[i].uuid_filter, write_observers[i].filter_count))
            continue;

        size_t size = mbuf_size(ctxt->om);
        if (!is_vla_capable(size))
            return BLE_ATT_ERR_INSUFFICIENT_RES;
        
        uint8_t buffer[size];
        if (0 != os_mbuf_copydata(ctxt->om, 0, size, buffer))
            return BLE_ATT_ERR_INSUFFICIENT_RES;

        write_observers[i].observer(ble_get_simplified_uuid(ctx_uuid),buffer, size);
    }
    return 0;
}

static void ble_on_sync (void) {
    ble_advertise();
}

static void ble_on_reset (int reason)              {
    MODLOG_DFLT(ERROR, "Resetting state; reason=%d", reason);
}

void gatt_svr_register_cb (struct ble_gatt_register_ctxt* ctxt, void* arg) {
    char buf[BLE_UUID_STR_LEN];

    switch (ctxt->op) {
        case BLE_GATT_REGISTER_OP_SVC:
            MODLOG_DFLT(INFO, "registered service %s with handle=%d",
              ble_uuid_to_str(ctxt->svc.svc_def->uuid, buf),
              ctxt->svc.handle);
            break;

        case BLE_GATT_REGISTER_OP_CHR:
            MODLOG_DFLT(INFO, "registering characteristic %s with "
              "def_handle=%d val_handle=%d",
              ble_uuid_to_str(ctxt->chr.chr_def->uuid, buf),
              ctxt->chr.def_handle,
              ctxt->chr.val_handle);
            break;

        case BLE_GATT_REGISTER_OP_DSC:
            MODLOG_DFLT(INFO, "registering descriptor %s with handle=%d",
              ble_uuid_to_str(ctxt->dsc.dsc_def->uuid, buf),
              ctxt->dsc.handle);
            break;

        default:
            assert(0);
            break;
    }
}

static void ble_host_task (void* param) {
    /* This function will return only when nimble_port_stop() is executed */
    nimble_port_run();
    nimble_port_freertos_deinit();
}

static int gatt_svr_init (void) {
    ble_svc_gap_init();
    ble_svc_gatt_init();

    if (ble_gatts_count_cfg(gatt_svr_svcs) != 0)
        goto lib_error;

    if (ble_gatts_add_svcs(gatt_svr_svcs) != 0)
        goto lib_error;

    if(ble_svc_gap_device_name_set(device_name) != 0)
        goto lib_error;

    return error_any;

lib_error:
    error_print_message(library_error);
    return library_error;
}

error_t ble_init_nimble(void) {
    esp_err_t ret = nimble_port_init();
    if (ret != ESP_OK)
        goto lib_error;

    /* Initialize the NimBLE host configuration */
    ble_hs_cfg.sync_cb  = ble_on_sync;
    ble_hs_cfg.reset_cb = ble_on_reset;
    ble_hs_cfg.gatts_register_cb = gatt_svr_register_cb;

    if (gatt_svr_init() != error_any)
        goto lib_error;

    /* Start the task */
    nimble_port_freertos_init(ble_host_task);
    return error_any;

lib_error:
    error_print_message(library_error);
    return library_error;

}

