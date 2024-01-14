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

#ifndef _MAIN_PID_
#define _MAIN_PID_

#ifdef __cplusplus
extern "C"{
#endif

typedef struct pid_calibration {
    float kp;
    float ki;
    float kd;
} pid_params_t;


typedef struct pid_state {
    float actual; // The actual reading as measured
    float target; // The desired reading
    float time_delta; // Time since last sample/calculation - should be set when updating state
    // The previously calculated error between actual and target (zero initially)
    float previous_error;
    float integral; // Sum of integral error over time
    float output; // the modified output value calculated by the algorithm, to compensate for error
} pid_state_t;

pid_state_t pid_iterate(const pid_params_t calibration, pid_state_t state);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
