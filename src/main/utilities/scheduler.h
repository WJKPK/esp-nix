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

#ifndef _UTILITIES_SCHEDULER_
#define _UTILITIES_SCHEDULER_

#include <stdbool.h>
#include "scheduler_types.h"

typedef enum {
#define SCHEDULE_QUEUE(name, item_type, size, callbacks_count) SchedulerQueue##name,
  #include "scheduler.scf"
  SchedulerQueueLast
} scheduler_queue_id_t;
#undef SCHEDULE_QUEUE

typedef void (*scheduler_callback_t)(void*);

void scheduler_init(void);
bool scheduler_enqueue(scheduler_queue_id_t queue_id, void* payload);
bool scheduler_dequeue(scheduler_queue_id_t queue_id, void* payload);
bool scheduler_unsubscribe(scheduler_queue_id_t queue_id, scheduler_callback_t callback);
bool scheduler_subscribe(scheduler_queue_id_t queue_id, scheduler_callback_t callback);
void scheduler_run(void);

#endif  // _UTILITIES_SCHEDULER_
