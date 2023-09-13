#include "CppUTest/TestHarness.h"
#include "CppUTest/CommandLineTestRunner.h"
#include <atomic>

extern "C" {
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "configs/scheduler_types.h"
#include "utilities/scheduler.h"
}

std::atomic<bool> is_test_finished(false);

static inline void set_test_end (void) {
    std::atomic_store_explicit(&is_test_finished, true, std::memory_order_relaxed);
};

static inline void set_test_start (void) {
    std::atomic_store_explicit(&is_test_finished, false, std::memory_order_relaxed);
};

static inline bool get_test_status (void) {
    return std::atomic_load_explicit(&is_test_finished, std::memory_order_relaxed);
};

TEST_GROUP(EventSchedulerTests) {
    void setup () {
        set_test_start();
    }

    void teardown () {
    }
};

TEST(EventSchedulerTests, SendOneEventCustomStructCallOneCallback) {
    SchedulerCallback send_callback = [](void* arg) {
          CustomStruct* casted_arg(reinterpret_cast<CustomStruct*>(arg));
          CHECK_EQUAL(PayloadTwo, casted_arg->payload_type);
          CHECK_EQUAL(-1, casted_arg->payload_two);
          set_test_end();
      };

    CHECK_EQUAL(true, SCHEDULER_Subscribe(SchedulerQueueTestStruct, send_callback));
    CustomStruct custom_struct({
        .payload_type = PayloadTwo,
        .payload_two  = -1
    });
    CHECK_EQUAL(true, SCHEDULER_Enqueue(SchedulerQueueTestStruct, &custom_struct));
    while (!get_test_status());
    CHECK_EQUAL(true, SCHEDULER_Unsubscribe(SchedulerQueueTestStruct, send_callback));
}

static unsigned repetitions_counter = 0;
constexpr unsigned no_of_repetitions(10);

TEST(EventSchedulerTests, SendMultipleEventUnsignedCallOneCallback) {
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
    while (no_of_messages-- > 0)
        CHECK_EQUAL(true, SCHEDULER_Enqueue(SchedulerQueueTest, std::make_unique<unsigned>(0xDEADBEEF).get()));

    while (!get_test_status());
    CHECK_EQUAL(true, SCHEDULER_Unsubscribe(SchedulerQueueTest, send_callback));
}

