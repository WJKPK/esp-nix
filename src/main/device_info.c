/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#define LOGGER_OUTPUT_LEVEL LOG_OUTPUT_INFO

#include "ble.h"
#include "utilities/logger.h"

static const char* manuf_name = "WJKPK";
static const char* model_num  = "ESP32C3Generic";

void on_device_info_read(simplified_uuid_t uuid, uint8_t** buff, size_t* size) {
    switch (uuid) {
        case DEVICE_INFO_MODEL_NUMBER_UUID:
            *buff = (uint8_t*)model_num;
            *size = strlen(model_num);
            break;
        case DEVICE_INFO_MANUFACTURER_NAME_UUID:
            *buff = (uint8_t*)manuf_name;
            *size = strlen(manuf_name);
            break;
        default:
            *buff = (uint8_t*)manuf_name;
            *size = 0;
            break;
    }
}

error_t device_info_init(void) {
    static const simplified_uuid_t uuid_filter[] = { DEVICE_INFO_MODEL_NUMBER_UUID,
                                                     DEVICE_INFO_MANUFACTURER_NAME_UUID };
    read_observer_descriptor_t observer_descriptor = {
        .observer = on_device_info_read,
        .uuid_filter = uuid_filter,
        .filter_count = 2,
    };
    return ble_add_read_observer(observer_descriptor);
}
