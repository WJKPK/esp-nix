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
#ifndef _MAIN_HEAT_CONTROLLER_
#define _MAIN_HEAT_CONTROLLER_

#include "utilities/error.h"

typedef unsigned celcius;

typedef enum {
    heating_state_idle,
    heating_state_constant,
    heating_state_multi_stage,
    heating_state_last
} heating_mode_state;

typedef enum {
    multistage_heating_jedec,
    multistage_heating_last
} multistage_heating_type;

//TODO should have some status passed to input
typedef void (*heat_completion_marker)(void);
error_status_t heat_controller_start_multistage_heating_mode(multistage_heating_type type, heat_completion_marker completion_routine);
error_status_t heat_controller_start_constant_heating(celcius temperature, unsigned duration, heat_completion_marker completion_routine);
error_status_t heat_controller_init(void);
unsigned heat_controller_get_temperature(void);
error_status_t heat_controller_cancel_action(void);

#endif
