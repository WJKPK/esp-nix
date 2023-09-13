#include "CppUTest/TestHarness.h"
#include "CppUTest/CommandLineTestRunner.h"

#include <CppUTest/UtestMacros.h>
#include <memory>

extern "C" {
#include "FreeRTOS.h"
#include "task.h"
}

static std::unique_ptr<StackType_t[]> uxIdleTaskStack = std::make_unique<StackType_t[]>(configMINIMAL_STACK_SIZE);

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

void vApplicationIdleHook (void) {
}

