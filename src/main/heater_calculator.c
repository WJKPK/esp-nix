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
#include "FreeRTOS.h"
#include "task.h"
#include "heater_calculator.h"
#include "pid.h"

static pid_state_t previous_state = { 0 };

static float constrain_output(float output) {
    const float min_power_percent = 5.f;
    const float max_power_percent = 100.f;

    return output < min_power_percent ? 0.0f : output > max_power_percent ? max_power_percent : output;
}

float get_heating_power_percent(float actual_temperature, float setpoint) {
    const pid_params_t parameters = {
        .kp = 1.0f,
        .kd = 0,
        .ki = 0.2f,
    };

    previous_state.actual = actual_temperature;
    previous_state.target = setpoint;
    float actual_time = ((float) (xTaskGetTickCount()) / (float) portTICK_PERIOD_MS) / 1000.f;
    previous_state.time_delta = actual_time - previous_state.time_delta;

    previous_state = pid_iterate(parameters, previous_state);
    return constrain_output(previous_state.output);
}
