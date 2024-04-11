/*
 * Copyright 2024 WJKPK
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <assert.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "ble.h"
#include "nimble/nimble_port.h"
#include "nimble/ble.h"
#include "nimble/nimble_port_freertos.h"

#include "utilities/logger.h"
#include "utilities/addons.h"
#include "utilities/scheduler.h"

_Static_assert(1 == CONFIG_BT_NIMBLE_MAX_CONNECTIONS, "Component tested with only one concurrent connection!");
#define BLE_CONN_HANDLE_INVALID   0x0

static const char* device_name = "ThermoPlate";
static ble_context* ble_ctx = NULL;

static error_status_t ble_advertise (void);
static int generic_access(uint16_t conn_handle, uint16_t attr_handle,
        struct ble_gatt_access_ctxt* ctxt, void* arg);

#define GET_FULL_UUID(simplified_uuid) 0x94, 0x56, simplified_uuid & 0xFF, (simplified_uuid & 0xFF00) >> 8, 0xB4, 0x11, 0x12, 0xFA, \
    simplified_uuid & 0xFF, (simplified_uuid & 0xFF00) >> 8, 0x24, 0x4C, 0x44, 0x14, 0x42, 0x80, 

static struct {
    uint16_t val_handle;
    simplified_uuid_t uuid;
} handle_uuid_mapping[] = {
    {0, HEATER_TEMPERATURE_READ_UUID},
    {0, HEATER_MODE_WRITE_UUID},
    {0, HEATER_CONST_TEMPERATURE_WRITE_UUID},
    {0, HEATER_CONST_TIME_WRITE_UUID},
};

static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(DEVICE_INFO_SERVICE),
        .characteristics = (struct ble_gatt_chr_def[])
        { {
              .uuid      = BLE_UUID128_DECLARE(GET_FULL_UUID(DEVICE_INFO_MANUFACTURER_NAME_UUID)),
              .access_cb = generic_access,
              .flags     = BLE_GATT_CHR_F_READ,
          },{
              .uuid      = BLE_UUID128_DECLARE(GET_FULL_UUID(DEVICE_INFO_MODEL_NUMBER_UUID)),
              .access_cb = generic_access,
              .flags     = BLE_GATT_CHR_F_READ,
          }, {
              0, /* No more characteristics in this service */
          }, }
    },
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(DEVICE_MEASUREMENT_SERVICE),
        .characteristics = (struct ble_gatt_chr_def[])
        { {
              .uuid      = BLE_UUID128_DECLARE(GET_FULL_UUID(HEATER_TEMPERATURE_READ_UUID)),
              .access_cb = generic_access,
              .val_handle = &handle_uuid_mapping[0].val_handle,
              .flags     = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
          },{
              .uuid      = BLE_UUID128_DECLARE(GET_FULL_UUID(HEATER_MODE_WRITE_UUID)),
              .access_cb = generic_access,
              .val_handle = &handle_uuid_mapping[1].val_handle,
              .flags     = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
          },{
              .uuid      = BLE_UUID128_DECLARE(GET_FULL_UUID(HEATER_CONST_TEMPERATURE_WRITE_UUID)),
              .access_cb = generic_access,
              .val_handle = &handle_uuid_mapping[2].val_handle,
              .flags     = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
          },{
              .uuid      = BLE_UUID128_DECLARE(GET_FULL_UUID(HEATER_CONST_TIME_WRITE_UUID)),
              .access_cb = generic_access,
              .val_handle = &handle_uuid_mapping[3].val_handle,
              .flags     = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
          },{
              0, /* No more characteristics in this service */
          }, }
    },

    {
        0, /* No more services */
    },
};

simplified_uuid_t ble_get_simplified_uuid(const ble_uuid_t* full_uuid) {
    ble_uuid_any_t uuid_full_casted = *(ble_uuid_any_t*)full_uuid;
    simplified_uuid_t result = 0;
    switch (uuid_full_casted.u.type) {
        case BLE_UUID_TYPE_128:
            result |= uuid_full_casted.u128.value[8];
            result |= ((uint16_t)uuid_full_casted.u128.value[9]) << 8;
            break;

        case BLE_UUID_TYPE_16:
            result = uuid_full_casted.u16.value;
            break;

        default:
            assert(false);
            break;
    }
    return result;
}

error_status_t ble_add_write_observer(write_observer_descriptor_t observer_descriptor) {
    for (unsigned i = 0; i < BLE_READ_WRITE_CALLBACKS_COUNT; i++) { if (ble_ctx->write_observers[i].observer != NULL) continue;
        ble_ctx->write_observers[i] = observer_descriptor;
        return ERROR_ANY;
    }
    return ERROR_COLLECTION_FULL;
}

error_status_t ble_add_read_observer(read_observer_descriptor_t observer_descriptor) {
    for (unsigned i = 0; i < BLE_READ_WRITE_CALLBACKS_COUNT; i++) {
        if (ble_ctx->read_observers[i].observer != NULL)
            continue;

        ble_ctx->read_observers[i] = observer_descriptor;
        return ERROR_ANY;
    }
    return ERROR_COLLECTION_FULL;
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
    error_status_t ret = ERROR_ANY;
    switch (event->type) {
        case BLE_GAP_EVENT_CONNECT:
            /* A new connection was established or a connection attempt failed */
            log_info("connection %s; status=%d\n",
              event->connect.status == 0 ? "established" : "failed",
              event->connect.status);

            if (event->connect.status != 0) {
                /* Connection failed; resume advertising */
                ret = ble_advertise();
            }
            if (ble_ctx->conn_handle != BLE_CONN_HANDLE_INVALID) {
                ret = (0 == ble_gap_terminate(event->connect.conn_handle, BLE_ERR_CONN_PARMS)) ?
                    ERROR_ANY : ERROR_LIBRARY_ERROR;
                break;
            }
            ble_ctx->conn_handle = event->connect.conn_handle;
            break;

        case BLE_GAP_EVENT_DISCONNECT:
            log_info("disconnect; reason=%d\n", event->disconnect.reason);
            ble_ctx->conn_handle = BLE_CONN_HANDLE_INVALID; //TODO - some mutex required?
            /* Connection terminated; resume advertising */
            ret = ble_advertise();
            break;

        case BLE_GAP_EVENT_ADV_COMPLETE:
            log_info("adv complete\n");
            ret = ble_advertise();
            break;

        case BLE_GAP_EVENT_SUBSCRIBE:
            ESP_LOGI("BLE_GAP_SUBSCRIBE_EVENT", "conn_handle from subscribe=%d", ble_ctx->conn_handle);
            break;

        case BLE_GAP_EVENT_MTU:
            log_info("mtu update event; conn_handle=%d mtu=%d\n",
              event->mtu.conn_handle,
              event->mtu.value);
            break;
    }

    return ret == ERROR_ANY ? 0 : -1;
}

static error_status_t ble_advertise (void) {
    struct ble_gap_adv_params adv_params;
    struct ble_hs_adv_fields fields;

    memset(&fields, 0, sizeof(fields));

    fields.flags = BLE_HS_ADV_F_DISC_GEN
      | BLE_HS_ADV_F_BREDR_UNSUP;
    fields.tx_pwr_lvl_is_present = 1;
    fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;
    fields.name             = (uint8_t*) ble_ctx->device_name;
    fields.name_len         = strlen(ble_ctx->device_name);
    fields.name_is_complete = 1;
    
    error_status_t ret = ERROR_ANY;
    switch(ble_gap_adv_set_fields(&fields)) {
        case BLE_HS_EBUSY:
            ret = ERROR_ACTION_ALREADY_REQUESTED;
            goto error_handler;
        case BLE_HS_EMSGSIZE:
            ret = ERROR_MEMORY_CONSTRAINT_EXCEEDED; 
            goto error_handler;
        default:
            break;
    }

    /* Begin advertising */
    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
    if (0 != ble_gap_adv_start(BLE_ADDR_PUBLIC, NULL, BLE_HS_FOREVER, &adv_params, ble_gap_event, NULL)) {
        ret = ERROR_LIBRARY_ERROR;
        goto error_handler;
    }
    return ERROR_ANY;

error_handler:
    error_print_message(ret);
    return ret;

}

static bool find_handle(simplified_uuid_t uuid, uint16_t* handle) {
    for (unsigned i = 0; i < COUNT_OF(handle_uuid_mapping); i++) {
        if(handle_uuid_mapping[i].uuid == uuid) {
            *handle = handle_uuid_mapping[i].val_handle;
            return true;
        }
    }
    return false;
}

error_status_t ble_notify_custom(simplified_uuid_t uuid, uint8_t* buff, size_t len) {
    if (BLE_CONN_HANDLE_INVALID == ble_ctx->conn_handle)
        return ERROR_RESOURCE_UNAVAILABLE;

    struct os_mbuf *om = ble_hs_mbuf_from_flat(buff, len);
    uint16_t handle;
    if (!find_handle(uuid, &handle))
        return ERROR_UNKNOWN_RESOURCE;

    return 0 == ble_gatts_notify_custom(ble_ctx->conn_handle, handle, om) ? ERROR_ANY : ERROR_LIBRARY_ERROR;
}

error_status_t ble_notify(simplified_uuid_t uuid) {
    if (BLE_CONN_HANDLE_INVALID == ble_ctx->conn_handle)
        return ERROR_RESOURCE_UNAVAILABLE;

    uint16_t handle;
    if (!find_handle(uuid, &handle))
        return ERROR_UNKNOWN_RESOURCE;

    return 0 == ble_gatts_notify(ble_ctx->conn_handle, handle) ? ERROR_ANY : ERROR_LIBRARY_ERROR;
}

static bool is_uuid_in_filter(const ble_uuid_t* uuid, simplified_uuid_t const* filter, unsigned count) {
    for (unsigned i = 0; i < count; i++) {
        const ble_uuid_any_t full_uuid = ble_get_full_uuid(filter[i]);
        if (0 == ble_uuid_cmp(uuid , (const ble_uuid_t *)&full_uuid))
            return true;
    }
    return false;
}

static bool is_valid_read_descriptor(read_buff_descriptor_t* buff_desc) {
    if (buff_desc->buff == NULL || buff_desc->size == 0)
        return false;
    return true;
}

static int generic_read_access(uint16_t conn_handle, uint16_t attr_handle,
  struct ble_gatt_access_ctxt* ctxt, void* arg) {
    if (BLE_CONN_HANDLE_INVALID == conn_handle)
        return BLE_ATT_ERR_INVALID_HANDLE;

    const ble_uuid_t* ctx_uuid = ctxt->chr->uuid;
    for (unsigned i = 0; i < BLE_READ_WRITE_CALLBACKS_COUNT; i++) {
        if(!is_uuid_in_filter(ctx_uuid, ble_ctx->read_observers[i].uuid_filter, ble_ctx->read_observers[i].filter_count))
            continue;

        read_buff_descriptor_t read_desc =
            ble_ctx->read_observers[i].observer(ble_get_simplified_uuid(ctx_uuid));

        if (!is_valid_read_descriptor(&read_desc))
            return BLE_ATT_ERR_READ_NOT_PERMITTED;

        if (0 != os_mbuf_append(ctxt->om, read_desc.buff, read_desc.size))
            return BLE_ATT_ERR_INSUFFICIENT_RES;
    }
    return 0;
}

static size_t mbuf_size(struct os_mbuf *om) {
    if (OS_MBUF_PKTHDR(om))
        return OS_MBUF_PKTLEN(om);
    return os_mbuf_len(om);
}

static int generic_write_access(uint16_t conn_handle, uint16_t attr_handle,
  struct ble_gatt_access_ctxt* ctxt, void* arg) {
    if (BLE_CONN_HANDLE_INVALID == conn_handle)
        return BLE_ATT_ERR_INVALID_HANDLE;

    const ble_uuid_t* ctx_uuid = ctxt->chr->uuid;
    for (unsigned i = 0; i < BLE_READ_WRITE_CALLBACKS_COUNT; i++) {
        if(!is_uuid_in_filter(ctx_uuid, ble_ctx->write_observers[i].uuid_filter, ble_ctx->write_observers[i].filter_count))
            continue;

        size_t size = mbuf_size(ctxt->om);
        if (BLE_ACL_MAX_PKT_SIZE < size)
            return BLE_ATT_ERR_INSUFFICIENT_RES;
        
        if (0 != os_mbuf_copydata(ctxt->om, 0, size, ble_ctx->data_buffer))
            return BLE_ATT_ERR_INSUFFICIENT_RES;

        ble_ctx->write_observers[i].observer(ble_get_simplified_uuid(ctx_uuid), ble_ctx->data_buffer, size);
    }
    return 0;
}

static int generic_access(uint16_t conn_handle, uint16_t attr_handle,
  struct ble_gatt_access_ctxt* ctxt, void* arg) {
    log_info("%s called with op %u", __FUNCTION__, ctxt->op);
    int result;
    switch(ctxt->op) {
        case BLE_GATT_ACCESS_OP_READ_CHR:
            result = generic_read_access(conn_handle, attr_handle, ctxt, arg);
            log_info("result %d", result);
            return result;

        case BLE_GATT_ACCESS_OP_WRITE_CHR:
            return generic_write_access(conn_handle, attr_handle, ctxt, arg);

        case BLE_GATT_ACCESS_OP_READ_DSC:
        case BLE_GATT_ACCESS_OP_WRITE_DSC:
            return 0;
    }
    return 0;
}

static void ble_on_sync (void) {
    ble_advertise();
}

static void ble_on_reset (int reason)              {
    log_info("Resetting state; reason=%d", reason);
}

void gatt_svr_register_cb (struct ble_gatt_register_ctxt* ctxt, void* arg) {
    char buf[BLE_UUID_STR_LEN];

    switch (ctxt->op) {
        case BLE_GATT_REGISTER_OP_SVC:
            log_info("registered service %s with handle=%d",
              ble_uuid_to_str(ctxt->svc.svc_def->uuid, buf),
              ctxt->svc.handle);
            break;

        case BLE_GATT_REGISTER_OP_CHR:
            log_info("registering characteristic %s with "
              "def_handle=%d val_handle=%d",
              ble_uuid_to_str(ctxt->chr.chr_def->uuid, buf),
              ctxt->chr.def_handle,
              ctxt->chr.val_handle);
            break;

        case BLE_GATT_REGISTER_OP_DSC:
            log_info("registering descriptor %s with handle=%d",
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

    if(ble_svc_gap_device_name_set(ble_ctx->device_name) != 0)
        goto lib_error;

    return ERROR_ANY;

lib_error:
    error_print_message(ERROR_LIBRARY_ERROR);
    return ERROR_LIBRARY_ERROR;
}

static error_status_t setup_context(ble_context_allocator ctx_allocator) {
    ble_ctx = ctx_allocator();
    if (NULL == ble_ctx)
        return ERROR_MEMORY_CONSTRAINT_EXCEEDED;

    memset(ble_ctx, 0, sizeof(*ble_ctx));
    ble_ctx->device_name = device_name;
    ble_ctx->conn_handle = BLE_CONN_HANDLE_INVALID;
    return ERROR_ANY;
}

error_status_t ble_init_nimble(ble_context_allocator ctx_allocator) {
    if (NULL == ctx_allocator)
        return ERROR_INVALID_INPUT_PARAMETER;
    
    error_status_t ctx_status = ERROR_ANY;
    if (ERROR_ANY != (ctx_status = setup_context(ctx_allocator)))
        return ctx_status;

    esp_err_t ret = nimble_port_init();
    if (ret != ESP_OK)
        goto lib_error;

    ble_hs_cfg.sync_cb  = ble_on_sync;
    ble_hs_cfg.reset_cb = ble_on_reset;
    ble_hs_cfg.gatts_register_cb = gatt_svr_register_cb;

    if (gatt_svr_init() != ERROR_ANY)
        goto lib_error;

    nimble_port_freertos_init(ble_host_task);
    return ERROR_ANY;

lib_error:
    error_print_message(ERROR_LIBRARY_ERROR);
    return ERROR_LIBRARY_ERROR;

}

