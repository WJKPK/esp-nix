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

#include "utilities/timer.h"
#include "utilities/addons.h"
#include "timer_definitions.h"

#include "FreeRTOS.h"
#include "timers.h"

static struct {
    TimerHandle_t handle;
    StaticTimer_t internals;
    UBaseType_t increment;
    periodic_timer_callback_t callbacks[MAX_TIMERS_CALLBACK];
} periodic_timers[periodic_timer_last];

static struct {
    TimerHandle_t handle;
    StaticTimer_t internals;
    UBaseType_t increment;
    oneshot_timer_callback_t callback;
} oneshot_timers[oneshot_timer_last];

#define MATCH_TIMER_INDEX(timer_struct, timer_handle) ({ unsigned i = 0;    \
        do{                                                                 \
            timer_handle == timer_struct[i].handle ? ({ break; }) : ({});   \
        } while(i++ < COUNT_OF(timer_struct));                              \
        i; })

static void on_periodic_timer_tick( TimerHandle_t xTimer ) {
    unsigned timer_index = MATCH_TIMER_INDEX(periodic_timers, xTimer);
    timer_index >= periodic_timer_last ? ({ return; }) : ({});
    periodic_timer_callback_t* callbacks = periodic_timers[timer_index].callbacks;
    for (unsigned i = 0; i < MAX_TIMERS_CALLBACK; i++) {
        NULL != callbacks[i] ? ({ callbacks[i]() ;}) : ({});
    }
}

static void on_oneshot_timer_tick( TimerHandle_t xTimer ) {
    unsigned timer_index = MATCH_TIMER_INDEX(oneshot_timers, xTimer);
    timer_index >= oneshot_timer_last ? ({ return; }) : ({});
    
    oneshot_timer_callback_t callback = oneshot_timers[timer_index].callback;
    NULL != callback ? ({ callback(); }) : ({});
}

error_status_t timer_register_callback(periodic_timer_t timer, periodic_timer_callback_t callback) {
    timer >= periodic_timer_last ? ({ return error_unknown_resource; }) : ({});
    for (unsigned i = 0; i < COUNT_OF(periodic_timers[timer].callbacks); i++) {
        if (periodic_timers[timer].callbacks[i] == NULL) {
            periodic_timers[timer].callbacks[i] = callback;
            return error_any;
        }
    }
    return error_collection_full;
}

error_status_t oneshot_arm(oneshot_timer_t timer, unsigned period_ms, oneshot_timer_callback_t callback) {
    timer >= oneshot_timer_last ? ({ return error_unknown_resource; }) : ({});
    const TimerHandle_t handle = oneshot_timers[timer].handle;

    oneshot_timers[timer].callback = callback;
    pdTRUE == xTimerChangePeriod(handle, period_ms, 0) && xTimerReset(handle, 0) ? ({ return error_any; }) : ({});

    oneshot_timers[timer].callback = NULL;
    return error_collection_full;
}

error_status_t timer_init(void) {
    #define PERIODIC_TIMER(name, period_ms) periodic_timers[name].handle = xTimerCreateStatic(STRINGIFY(name),  \
                                 pdMS_TO_TICKS(period_ms),                                                      \
                                 pdTRUE,                                                                        \
                                 &periodic_timers[name].increment,                                              \
                                 on_periodic_timer_tick,                                                        \
                                 &periodic_timers[name].internals);                                             \
                                 periodic_timers[name].handle != NULL ? ({                                      \
                                    pdFAIL == xTimerStart(periodic_timers[name].handle, 0) ? ({                 \
                                        return error_collection_full;                                           \
                                    }) : ({});                                                                  \
                                 }) : ({ return error_resource_unavailable; });

    #define ONESHOT_TIMER(name) oneshot_timers[name].handle = xTimerCreateStatic(STRINGIFY(name),               \
                                 portMAX_DELAY,                                                                 \
                                 pdFALSE,                                                                       \
                                 &oneshot_timers[name].increment,                                               \
                                 on_oneshot_timer_tick,                                                         \
                                 &oneshot_timers[name].internals);                                              \
                                 oneshot_timers[name].handle != NULL ? ({}) : ({ return error_resource_unavailable; });

    #include "timer.scf"
    #undef PERIODIC_TIMER
    #undef ONESHOT_TIMER 
    return error_any;
}

