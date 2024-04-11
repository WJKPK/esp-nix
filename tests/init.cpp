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

extern "C" {
#include "FreeRTOS.h"
#include "task.h"
#include "configs/scheduler_types.h"
#include "utilities/scheduler.h"
#include "utilities/timer.h"
}

typedef struct {
    struct {
        int    ac;
        char** av;
    }            input_pair;
    TaskHandle_t loop_handle;
} CppuTestContext;

void cpputest_task(void* pvParameters) {
    CppuTestContext* cpputest_ctx = reinterpret_cast<CppuTestContext*>(pvParameters);

    CHECK_EQUAL(0, CommandLineTestRunner::RunAllTests(cpputest_ctx->input_pair.ac, cpputest_ctx->input_pair.av));

    vTaskEndScheduler();
    vTaskDelete(NULL);
    vTaskDelete(cpputest_ctx->loop_handle);
}

int main(int ac, char** av) {
    auto event_scheduler_loop = [](void* pvParameters) {
          scheduler_init();
          for (;;) {
              scheduler_run();
          }
      };
    TaskHandle_t loop_task_handle = NULL;

    timer_init();
    CHECK_EQUAL(pdPASS,
      xTaskCreate(event_scheduler_loop, "ESLoop", configMINIMAL_STACK_SIZE, NULL, configMAX_PRIORITIES - 1,
      &loop_task_handle));

    CppuTestContext cpputest_ctx{ { ac, av }, loop_task_handle };
    CHECK_EQUAL(pdPASS,
      xTaskCreate(cpputest_task, "CppTask", configMINIMAL_STACK_SIZE, &cpputest_ctx, configMAX_PRIORITIES - 2, NULL));

    vTaskStartScheduler();
    return 0;
}
