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
#include "CppUTestExt/MockSupport.h"
#include "encoder.h"
#include <unistd.h>

extern "C" {
#include "FreeRTOS.h"
#include "task.h"
#include "menu.h"
#include "utilities/scheduler.h"
}

TEST_GROUP(MenuTests) {
    void setup() {
        menu_init();
    }

    void teardown() {
    }
};

TEST(MenuTests, GoToJedecTest) {
    encoder_event_type push = ENCODER_EVENT_PUSH;
    encoder_event_type down = ENCODER_EVENT_DOWN;

    mock().expectNCalls(2, "lcd_send_request").andReturnValue(ERROR_ANY);
    scheduler_enqueue(SchedulerQueueMenu, &push);
    scheduler_enqueue(SchedulerQueueMenu, &down);

    mock().checkExpectations();
    mock().clear();
}
