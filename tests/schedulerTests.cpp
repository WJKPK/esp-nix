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
    scheduler_callback_t send_callback = [](void* arg) {
          CustomStruct* casted_arg(reinterpret_cast<CustomStruct*>(arg));
          CHECK_EQUAL(PayloadTwo, casted_arg->payload_type);
          CHECK_EQUAL(-1, casted_arg->payload_two);
          set_test_end();
      };

    CHECK_EQUAL(true, scheduler_subscribe(SchedulerQueueTestStruct, send_callback));
    CustomStruct custom_struct({
        .payload_type = PayloadTwo,
        .payload_two  = -1
    });
    CHECK_EQUAL(true, scheduler_enqueue(SchedulerQueueTestStruct, &custom_struct));
    while (!get_test_status());
    CHECK_EQUAL(true, scheduler_unsubscribe(SchedulerQueueTestStruct, send_callback));
}

static unsigned repetitions_counter = 0;
constexpr unsigned no_of_repetitions(10);

TEST(EventSchedulerTests, SendMultipleEventUnsignedCallOneCallback) {
    scheduler_callback_t send_callback = [](void* arg) {
          unsigned* casted_arg(reinterpret_cast<unsigned*>(arg));
          CHECK_EQUAL(0xDEADBEEF, *casted_arg);
          repetitions_counter++;
          if (repetitions_counter == no_of_repetitions) {
              set_test_end();
          }
      };

    CHECK_EQUAL(true, scheduler_subscribe(SchedulerQueueTest, send_callback));
    unsigned no_of_messages(no_of_repetitions);
    while (no_of_messages-- > 0)
        CHECK_EQUAL(true, scheduler_enqueue(SchedulerQueueTest, std::make_unique<unsigned>(0xDEADBEEF).get()));

    while (!get_test_status());
    CHECK_EQUAL(true, scheduler_unsubscribe(SchedulerQueueTest, send_callback));
}

