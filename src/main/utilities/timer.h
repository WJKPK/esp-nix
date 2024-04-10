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

#include <inttypes.h>
#include "utilities/error.h"
#include "utilities/types.h"

typedef void (*soft_irq_routine)(void *, uint32_t);

typedef enum {
    #define PERIODIC_TIMER(name, period_ms) name,
    #define ONESHOT_TIMER(name)
    #include "timer.scf"
    #undef PERIODIC_TIMER
    #undef ONESHOT_TIMER
    periodic_timer_last
} periodic_timer_t;

typedef enum {
    #define PERIODIC_TIMER(name, period_ms)
    #define ONESHOT_TIMER(name) name,
    #include "timer.scf"
    #undef PERIODIC_TIMER
    #undef ONESHOT_TIMER
    oneshot_timer_last
} oneshot_timer_t;

typedef void (*periodic_timer_callback_t)(void* args);
typedef void (*oneshot_timer_callback_t)(void* args);

static inline miliseconds microseconds_to_miliseconds(microseconds microsec) {
    return microsec / 1000;
}

static inline seconds miliseconds_to_seconds(miliseconds milisec) {
    return milisec / 1000;
}

static inline miliseconds seconds_to_miliseconds(seconds sec) {
    return sec * 1000;
}

error_status_t oneshot_arm(oneshot_timer_t timer, unsigned period_ms, oneshot_timer_callback_t callback, void* args);
miliseconds periodic_get_period(periodic_timer_t timer);
miliseconds periodic_get_expire(periodic_timer_t timer);
miliseconds oneshot_get_expire(oneshot_timer_t timer);
error_status_t timer_register_callback(periodic_timer_t timer, periodic_timer_callback_t callback, void* args);
error_status_t timer_unregister_callback(periodic_timer_t timer, periodic_timer_callback_t callback);
error_status_t timer_init(void);
error_status_t timer_soft_irq(soft_irq_routine routine, void* arg, uint32_t uarg);

#endif  // _UTILITIES_TIMER_
