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

#ifndef _UTILITIES_TIMER_
#define _UTILITIES_TIMER_

#include "utilities/error.h"

typedef enum {
    #define PERIODIC_TIMER(name, period_ms) name,
    #define ONESHOT_TIMER(name)
    #include "configs/timer.scf"
    #undef PERIODIC_TIMER
    #undef ONESHOT_TIMER
    periodic_timer_last
} periodic_timer_t;

typedef enum {
    #define PERIODIC_TIMER(name, period_ms)
    #define ONESHOT_TIMER(name) name,
    #include "configs/timer.scf"
    #undef PERIODIC_TIMER
    #undef ONESHOT_TIMER
    oneshot_timer_last
} oneshot_timer_t;

typedef void (*periodic_timer_callback_t)(void);
typedef void (*oneshot_timer_callback_t)(void);

error_status_t oneshot_arm(oneshot_timer_t timer, unsigned period_ms, oneshot_timer_callback_t callback);
error_status_t timer_register_callback(periodic_timer_t timer, periodic_timer_callback_t callback);
error_status_t timer_init(void);

#endif  // _UTILITIES_TIMER_
