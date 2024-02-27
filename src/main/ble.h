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

#ifndef H_BLEHR_SENSOR_
#define H_BLEHR_SENSOR_

#include "nimble/ble.h"
#include "utilities/error.h"

#define DEVICE_MEASUREMENT_SERVICE              0x181A
#define DEVICE_INFO_SERVICE                     0x180A
#define DEVICE_INFO_MANUFACTURER_NAME_UUID      0x2A29
#define DEVICE_INFO_MODEL_NUMBER_UUID           0x2A24
#define HEATER_TEMPERATURE_READ_UUID            0x2A6E
#define HEATER_MODE_WRITE_UUID                  0x2A26

typedef uint16_t simplified_uuid_t;

typedef void(*ble_write_callback_t)(simplified_uuid_t uuid, uint8_t* buff, size_t size);
typedef void(*ble_read_callback_t)(simplified_uuid_t uuid, uint8_t** buff, size_t* size);

typedef struct {
    simplified_uuid_t const* uuid_filter;
    unsigned filter_count;
    ble_write_callback_t observer;
} write_observer_descriptor_t;

typedef struct {
    simplified_uuid_t const* uuid_filter;
    unsigned filter_count;
    ble_read_callback_t observer;
} read_observer_descriptor_t;

error_status_t ble_add_write_observer(write_observer_descriptor_t observer_descriptor);
error_status_t ble_add_read_observer(read_observer_descriptor_t observer_descriptor);
error_status_t ble_notify_custom(simplified_uuid_t uuid, uint8_t* buff, size_t len);

error_status_t ble_init_nimble(void);

#endif

