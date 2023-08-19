#ifndef _UTILITIES_SCHEDULER_
#define _UTILITIES_SCHEDULER_ 

#include <stdbool.h>
#include "scheduler_types.h"

typedef enum {
#define SCHEDULE_QUEUE(name, item_type, size, callbacks_count) SchedulerQueue##name,
  #include "scheduler.scf"
  SchedulerQueueLast
} SchedulerQueueId;
#undef SCHEDULE_QUEUE

typedef void (*SchedulerCallback)(void*);

void SCHEDULER_Init(void);
bool SCHEDULER_Enqueue(SchedulerQueueId queue_id, void* payload);
bool SCHEDULER_Dequeue(SchedulerQueueId queue_id, void* payload);
bool SCHEDULER_Unsubscribe(SchedulerQueueId queue_id, SchedulerCallback callback);
bool SCHEDULER_Subscribe(SchedulerQueueId queue_id, SchedulerCallback callback);
void SCHEDULER_Run(void);

#endif  // _UTILITIES_SCHEDULER_
