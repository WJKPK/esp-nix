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

#include <CppUTest/UtestMacros.h>
#include <memory>

extern "C" {
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
}

static std::unique_ptr<StackType_t[]> uxIdleTaskStack = std::make_unique<StackType_t[]>(configMINIMAL_STACK_SIZE);
static std::unique_ptr<StackType_t[]> uxTimerTaskStack= std::make_unique<StackType_t[]>(configTIMER_TASK_STACK_DEPTH);

void vApplicationMallocFailedHook (void) {
    FAIL("Not enough memory!");
}

void vApplicationGetIdleTaskMemory (StaticTask_t** ppxIdleTaskTCBBuffer,
  StackType_t**                                    ppxIdleTaskStackBuffer,
  uint32_t*                                        pulIdleTaskStackSize) {
    static StaticTask_t xIdleTaskTCB;
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;
    *ppxIdleTaskStackBuffer = uxIdleTaskStack.get();
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

void vApplicationGetTimerTaskMemory( StaticTask_t ** ppxTimerTaskTCBBuffer,
                                     StackType_t ** ppxTimerTaskStackBuffer,
                                     uint32_t * pulTimerTaskStackSize ) {
    static StaticTask_t xTimerTaskTCB;
    *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;
    *ppxTimerTaskStackBuffer = uxTimerTaskStack.get();
    *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}

void vApplicationIdleHook (void) {
}

