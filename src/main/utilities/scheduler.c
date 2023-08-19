#include "utilities/scheduler.h"
#include <stdatomic.h>
#include <stdio.h>
#include "FreeRTOS.h"
#include "queue.h"

#define __same_type(a, b) __builtin_types_compatible_p(typeof(a), typeof(b))
#define __must_be_array(a) BUILD_BUG_ON_ZERO(__same_type((a), &(a)[0]))
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]) + __must_be_array(arr))

static struct {
  atomic_bool is_initialized;
  TaskHandle_t scheduler_task;
  QueueHandle_t queues_list[SchedulerQueueLast];
  SchedulerCallback* callbacks[SchedulerQueueLast];
  unsigned max_no_of_callbacks[SchedulerQueueLast];
} ctx;

void SCHEDULER_Init(void) {
  ctx.scheduler_task = xTaskGetCurrentTaskHandle();
  #define SCHEDULE_QUEUE(name, item_type, size, callbacks_count)  {static StaticQueue_t _static_##name##_queue; \
                                                                  static SchedulerCallback _scheduler_callback_list_##name[callbacks_count]; \
                                                                  static uint8_t _queue_##name##_storage_area[size * sizeof(item_type)]; \
                                                                  ctx.queues_list[SchedulerQueue##name] = xQueueCreateStatic(size, \
                                                                                                      sizeof(item_type), \
                                                                                                      _queue_##name##_storage_area, \
                                                                                                      &_static_##name##_queue); \
                                                                  ctx.callbacks[SchedulerQueue##name] = _scheduler_callback_list_##name; \
                                                                  ctx.max_no_of_callbacks[SchedulerQueue##name] = callbacks_count;}
    #include "scheduler.scf"
  #undef SCHEDULE_QUEUE
  atomic_store_explicit(&ctx.is_initialized, true, memory_order_relaxed);
}

static inline bool IsInitialized(void) {
  return atomic_load_explicit(&ctx.is_initialized, memory_order_relaxed);
}


bool SCHEDULER_Enqueue(SchedulerQueueId queue_id, void* payload) {
  if (!IsInitialized()) {
    return false;
  }
  return (pdTRUE == xQueueSend(ctx.queues_list[queue_id], payload, 0))
    && (pdPASS == xTaskNotifyGive(ctx.scheduler_task));
}

void SCHEDULER_Run(void) {
  if (0 == ulTaskNotifyTake(pdFALSE, portMAX_DELAY)) {
    return;
  }

#define SCHEDULE_QUEUE(name, item_type, size, callbacks_count)  {item_type _buff##name; \
                                                                while (pdTRUE == xQueueReceive(ctx.queues_list[SchedulerQueue##name], &_buff##name, 0)) { \
                                                                  for(unsigned _i = 0; _i < callbacks_count; _i++) { \
                                                                    if (ctx.callbacks[SchedulerQueue##name][_i] != NULL) \
                                                                      ctx.callbacks[SchedulerQueue##name][_i](&_buff##name); \
                                                                  } \
                                                                }}
  #include "scheduler.scf"
  #undef SCHEDULE_QUEUE
}

bool SCHEDULER_Subscribe(SchedulerQueueId queue_id, SchedulerCallback callback) {
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

bool SCHEDULER_Unsubscribe(SchedulerQueueId queue_id, SchedulerCallback callback) {
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

