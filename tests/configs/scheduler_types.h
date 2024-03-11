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

#ifndef __SCHEDULER_CUSTOM_TYPES__
#define __SCHEDULER_CUSTOM_TYPES__

typedef enum {
    PayloadOne,
    PayloadTwo,
    PayloadLast
} PayloadType;

typedef struct {
  PayloadType payload_type;
  union {
    unsigned payload_one;
    signed payload_two;
  };
} CustomStruct;

#include "lcd.h"
#include "encoder.h"

#endif  // __SCHEDULER_CUSTOM_TYPES__
