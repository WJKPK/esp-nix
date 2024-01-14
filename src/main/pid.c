/*
 * Modified version of Joshua Saxby (aka @saxbophone) code 
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

#include "pid.h"

pid_state_t pid_iterate(const pid_params_t calibration, pid_state_t state) {
    float error = state.target - state.actual;
    state.integral += (error * state.time_delta);
    float derivative = (error - state.previous_error) / state.time_delta;
    state.output = (
        (calibration.kp * error) + (calibration.ki * state.integral) + (calibration.kd * derivative)
    );
    state.previous_error = error;
    return state;
}

