/*
 * Copyright 2023 WJKPK
 *
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
#ifndef _MAIN_HEAT_CONTROLLER_INTERFACE_
#define _MAIN_HEAT_CONTROLLER_INTERFACE_

#include "utilities/error.h"
typedef unsigned celcius;
typedef enum {
    heating_request_constant,
    heating_request_jedec,
    heating_request_last 
} heating_request_type;

typedef struct {
    heating_request_type type;
    struct {
        celcius const_temperature;
        unsigned duration; 
    } constant;
} heater_request;

error_status_t heat_controller_interface_init(void);

#endif
