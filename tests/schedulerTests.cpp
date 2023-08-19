
#include "CppUTest/TestHarness.h"
#include "CppUTest/CommandLineTestRunner.h"
#include <atomic>
#include <memory>

extern "C" {
#include <stdio.h>
  #include "FreeRTOS.h"
  #include "task.h"
  #include "configs/scheduler_types.h"
  #include "utilities/scheduler.h"
}

static std::unique_ptr<StackType_t[]> uxIdleTaskStack = std::make_unique<StackType_t[]>(configMINIMAL_STACK_SIZE);
std::atomic<bool> is_test_finished(false);

void vApplicationMallocFailedHook( void ) {
}



void vApplicationGetIdleTaskMemory( StaticTask_t ** ppxIdleTaskTCBBuffer,
                                    StackType_t ** ppxIdleTaskStackBuffer,
                                    uint32_t * pulIdleTaskStackSize )
{
/* If the buffers to be provided to the Idle task are declared inside this
 * function then they must be declared static - otherwise they will be allocated on
 * the stack and so not exists after this function exits. */
  static StaticTask_t xIdleTaskTCB;

    /* Pass out a pointer to the StaticTask_t structure in which the Idle task's
     * state will be stored. */
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

    /* Pass out the array that will be used as the Idle task's stack. */
    *ppxIdleTaskStackBuffer = uxIdleTaskStack.get();

    /* Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
     * Note that, as the array is necessarily of type StackType_t,
     * configMINIMAL_STACK_SIZE is specified in words, not bytes. */
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

void vApplicationIdleHook( void ) {
}

static inline void set_test_end(void) {
  std::atomic_store_explicit(&is_test_finished, true, std::memory_order_relaxed);
};

static inline void set_test_start(void) {
  std::atomic_store_explicit(&is_test_finished, false, std::memory_order_relaxed);
};

static inline bool get_test_status(void) {
  return std::atomic_load_explicit(&is_test_finished, std::memory_order_relaxed);
};


TEST_GROUP(EventSchedulerTests) {
  void setup() {
    set_test_start();
  }

  void teardown() {
  }
};

void SendOneUnsigned(void) {
  auto send_callback = [](void* arg) { 
    unsigned* casted_arg(reinterpret_cast<unsigned*>(arg));
    CHECK_EQUAL(0xCAFEBEAA, *casted_arg);
    set_test_end();
  };

  CHECK_EQUAL(true, SCHEDULER_Subscribe(SchedulerQueueTest, send_callback));
  CHECK_EQUAL(true, SCHEDULER_Enqueue(SchedulerQueueTest, std::make_unique<unsigned>(0xCAFEBEAA).get()));
  while(!get_test_status());
  CHECK_EQUAL(true, SCHEDULER_Unsubscribe(SchedulerQueueTest, send_callback));
}

TEST(EventSchedulerTests, SendOneEventUnsignedCallOneCallback) {
  SendOneUnsigned();
}

void SendCustomStruct(void) {
  SchedulerCallback send_callback = [](void* arg) { 
    CustomStruct* casted_arg(reinterpret_cast<CustomStruct*>(arg));
    CHECK_EQUAL(PayloadTwo, casted_arg->payload_type);
    CHECK_EQUAL(-1, casted_arg->payload_two);
    set_test_end();
  };

  CHECK_EQUAL(true, SCHEDULER_Subscribe(SchedulerQueueTestStruct, send_callback));
  CustomStruct custom_struct({
    .payload_type = PayloadTwo,
    .payload_two = -1
  });
  CHECK_EQUAL(true,  SCHEDULER_Enqueue(SchedulerQueueTestStruct, &custom_struct));
  while(!get_test_status());
  CHECK_EQUAL(true, SCHEDULER_Unsubscribe(SchedulerQueueTestStruct, send_callback));

}

TEST(EventSchedulerTests, SendOneEventCustomStructCallOneCallback) {
  SendCustomStruct();
}

static unsigned repetitions_counter = 0;
constexpr unsigned no_of_repetitions(10);

void SendMultipleUnsigned(void) {
  SchedulerCallback send_callback = [](void* arg) { 
    unsigned* casted_arg(reinterpret_cast<unsigned*>(arg));
    CHECK_EQUAL(0xDEADBEEF, *casted_arg);
    repetitions_counter++;
    if (repetitions_counter == no_of_repetitions) {
        set_test_end();
    }

  };

  CHECK_EQUAL(true, SCHEDULER_Subscribe(SchedulerQueueTest, send_callback));
  unsigned no_of_messages(no_of_repetitions);
  while (no_of_messages-->0)
    CHECK_EQUAL(true, SCHEDULER_Enqueue(SchedulerQueueTest, std::make_unique<unsigned>(0xDEADBEEF).get()));

  while(!get_test_status());
  CHECK_EQUAL(true, SCHEDULER_Unsubscribe(SchedulerQueueTest, send_callback));
}

TEST(EventSchedulerTests, SendMultipleEventUnsignedCallOneCallback) {
  SendMultipleUnsigned();
}

typedef struct {
  struct {
    int ac;
    char** av;
  } input_pair;
  TaskHandle_t loop_handle;
} CppuTestContext;

void cpputest_task(void* pvParameters) {
    CppuTestContext* cpputest_ctx =  reinterpret_cast<CppuTestContext*>(pvParameters);

    CHECK_EQUAL(0, CommandLineTestRunner::RunAllTests(cpputest_ctx->input_pair.ac, cpputest_ctx->input_pair.av));

    vTaskEndScheduler();
    vTaskDelete(NULL);
    vTaskDelete(cpputest_ctx->loop_handle);
}

int main(int ac, char** av) {
    auto event_scheduler_loop = [](void * pvParameters) {
    SCHEDULER_Init();
      for( ;; ) {
        SCHEDULER_Run();
      }
    };
    TaskHandle_t loop_task_handle = NULL;
    CHECK_EQUAL(pdPASS, xTaskCreate(event_scheduler_loop, "ESLoop", configMINIMAL_STACK_SIZE, NULL, configMAX_PRIORITIES - 1, &loop_task_handle));

    CppuTestContext cpputest_ctx{{ac,av}, loop_task_handle};
    CHECK_EQUAL(pdPASS, xTaskCreate(cpputest_task, "CppTask", configMINIMAL_STACK_SIZE, &cpputest_ctx, configMAX_PRIORITIES - 2, NULL));

    vTaskStartScheduler();
    return 0;
}

