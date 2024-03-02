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

#include <stdbool.h>
#include <driver/gpio.h>
#include "esp_err.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOSConfig.h"
/* BLE */
#include "host/util/util.h"
#include "console/console.h"
#include "services/gap/ble_svc_gap.h"
#include "spi.h"
#include "utilities/scheduler.h"
#include "utilities/error.h"

#include "ble.h"
#include "lcd.h"
#include "device_info.h"
#include "heat_controller.h"
#include "utilities/timer.h"

void app_main (void) {
    scheduler_init();
    FATAL_IF_FAIL(timer_init());

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_ERROR_CHECK(gpio_install_isr_service(0));

    FATAL_IF_FAIL(ble_init_nimble());
    FATAL_IF_FAIL(device_info_init());
    FATAL_IF_FAIL(spi_init());
    FATAL_IF_FAIL(heat_controller_init());
    ldc_init();

    while(true) {
        scheduler_run();
    }
}

