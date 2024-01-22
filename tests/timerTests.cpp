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
#include "configs/timer_definitions.h"
#include "utilities/timer.h"
}

static std::atomic<bool> is_test_finished(false);

static inline void set_test_end (void) {
    std::atomic_store_explicit(&is_test_finished, true, std::memory_order_relaxed);
};

static inline void set_test_start (void) {
    std::atomic_store_explicit(&is_test_finished, false, std::memory_order_relaxed);
};

static inline bool get_test_status (void) {
    return std::atomic_load_explicit(&is_test_finished, std::memory_order_relaxed);
};

TEST_GROUP(TimerTests) {
    void setup () {
        set_test_start();
    }

    void teardown () {
    }
};

static unsigned repetitions_counter = 0;
constexpr unsigned no_of_repetitions(5);

TEST(TimerTests, OneSecondTimerFireFiveTimes) {
    periodic_timer_callback_t timer_callback = [](void) {
          repetitions_counter++;
          if (repetitions_counter == no_of_repetitions) {
              set_test_end();
          }
      };

    CHECK_EQUAL(error_any, timer_register_callback(periodic_timer_ten_msec, timer_callback));

    while (!get_test_status());
}

