/*
 * Copyright 2024 WJKPK
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

#ifndef _MAIN_MENU_
#define _MAIN_MENU_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MENU_EVENT_ENCODER_UP,
    MENU_EVENT_ENCODER_DOWN,
    MENU_EVENT_ENCODER_PUSH,
    MENU_EVENT_PREEMPT_REQUEST,
    MENU_EVENT_PREEMPT_TAKE,
    MENU_EVENT_REQUEST_DONE,
    MENU_EVENT_ENCODER_LAST
} menu_event_type;

void menu_init(void);

#ifdef __cplusplus
}
#endif

#endif // _MAIN_MENU_

