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

#include "utilities/scheduler.h"

#include <stdatomic.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "queue.h"

#include "utilities/addons.h"

static struct {
    atomic_bool        is_initialized;
    TaskHandle_t       scheduler_task;
    QueueHandle_t      queues_list[SchedulerQueueLast];
    scheduler_callback_t* callbacks[SchedulerQueueLast];
    unsigned           max_no_of_callbacks[SchedulerQueueLast];
} ctx;

#define SCHEDULE_QUEUE(name, item_type, size, callbacks_count) {                \
     static StaticQueue_t _static_##name##_queue;                               \
     static scheduler_callback_t _scheduler_callback_list_##name[callbacks_count]; \
     static uint8_t _queue_##name##_storage_area[size * sizeof(item_type)];     \
     ctx.queues_list[SchedulerQueue##name] = xQueueCreateStatic(size,           \
         sizeof(item_type),                                                     \
         _queue_##name##_storage_area,                                          \
         &_static_##name##_queue);                                              \
     ctx.callbacks[SchedulerQueue##name] = _scheduler_callback_list_##name;     \
     ctx.max_no_of_callbacks[SchedulerQueue##name] = callbacks_count;           \
}

void scheduler_init (void) {
    ctx.scheduler_task = xTaskGetCurrentTaskHandle();
#include "scheduler.scf"
    atomic_store_explicit(&ctx.is_initialized, true, memory_order_relaxed);
}

#undef SCHEDULE_QUEUE

static inline bool IsInitialized (void) {
    return atomic_load_explicit(&ctx.is_initialized, memory_order_relaxed);
}

bool scheduler_enqueue (scheduler_queue_id_t queue_id, void* payload) {
    if (!IsInitialized()) {
        return false;
    }
    return (pdTRUE == xQueueSend(ctx.queues_list[queue_id], payload, 0)) &&
           (pdPASS == xTaskNotifyGive(ctx.scheduler_task));
}

#define SCHEDULE_QUEUE(name, item_type, size, callbacks_count) {                               \
     item_type _buff##name;                                                                    \
     while (pdTRUE == xQueueReceive(ctx.queues_list[SchedulerQueue##name], &_buff##name, 0)) { \
         for (unsigned _i = 0; _i < callbacks_count; _i++) {                                   \
             if (ctx.callbacks[SchedulerQueue##name][_i] != NULL)                              \
             ctx.callbacks[SchedulerQueue##name][_i](&_buff##name);                            \
         }                                                                                     \
     }                                                                                         \
}

void scheduler_run (void) {
    if (0 == ulTaskNotifyTake(pdFALSE, portMAX_DELAY)) {
        return;
    }

#include "scheduler.scf"
}

#undef SCHEDULE_QUEUE

bool scheduler_subscribe (scheduler_queue_id_t queue_id, scheduler_callback_t callback) {
    if (!IsInitialized()) {
        return false;
    }

    if (queue_id >= SchedulerQueueLast) {
        return false;
    }

    for (unsigned i = 0; i < ctx.max_no_of_callbacks[queue_id]; i++) {
        if (NULL == ctx.callbacks[queue_id][i]) {
            ctx.callbacks[queue_id][i] = callback;
            return true;
        }
    }
    return false;
}

bool scheduler_unsubscribe (scheduler_queue_id_t queue_id, scheduler_callback_t callback) {
    if (!IsInitialized()) {
        return false;
    }

    if (queue_id >= SchedulerQueueLast) {
        return false;
    }

    for (unsigned i = 0; i < ctx.max_no_of_callbacks[queue_id]; i++) {
        if (callback == ctx.callbacks[queue_id][i]) {
            ctx.callbacks[queue_id][i] = NULL;
            return true;
        }
    }
    return false;
}

