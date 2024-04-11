/*
 * Copyright 2024 WJKPK
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <stdint.h>
#include "encoder_fsm.h"

typedef enum {
    ENCODER_FSM_START = 0,
    ENCODER_FSM_CLOCK_FINAL,
    ENCODER_FSM_CLOCK_BEGIN,
    ENCODER_FSM_CLOCK_NEXT,
    ENCODER_FSM_CONTERCLOCK_BEGIN,
    ENCODER_FSM_COUNTERCLOCK_FINAL,
    ENCODER_FSM_COUNTERCLOCK_NEXT,
    ENCODER_FSM_LAST
} ENCODER_FSM_STATE;

const uint8_t ttable[ENCODER_FSM_LAST][4] = {
    // start
    { ENCODER_FSM_START,             ENCODER_FSM_CLOCK_BEGIN,             ENCODER_FSM_CONTERCLOCK_BEGIN,
      ENCODER_FSM_START                                                  },
    // clockwise_final
    { ENCODER_FSM_CLOCK_NEXT,        ENCODER_FSM_START,                   ENCODER_FSM_CLOCK_FINAL,
      ENCODER_FSM_START | ENCODER_DIRECTION_CLOCKWISE                                              },
    // clockwise_begin
    { ENCODER_FSM_CLOCK_NEXT,        ENCODER_FSM_CLOCK_BEGIN,             ENCODER_FSM_START,
      ENCODER_FSM_START                                                                                                          },
    // clockwise_next
    { ENCODER_FSM_CLOCK_NEXT,        ENCODER_FSM_CLOCK_BEGIN,             ENCODER_FSM_CLOCK_FINAL,
      ENCODER_FSM_START                                                                                                                                        },
    // counterclockwise_begin
    { ENCODER_FSM_COUNTERCLOCK_NEXT, ENCODER_FSM_START,                   ENCODER_FSM_CONTERCLOCK_BEGIN,
      ENCODER_FSM_START                                                                                                                                                                      },
    // counterclockwise_final
    { ENCODER_FSM_COUNTERCLOCK_NEXT, ENCODER_FSM_COUNTERCLOCK_FINAL,      ENCODER_FSM_START,
      ENCODER_FSM_START
      | ENCODER_DIRECTION_COUNTERCLOCKWISE                                                                                                                                                   },
    // counterclockwise_next
    { ENCODER_FSM_COUNTERCLOCK_NEXT, ENCODER_FSM_COUNTERCLOCK_FINAL,      ENCODER_FSM_CONTERCLOCK_BEGIN,
      ENCODER_FSM_START                                                                                                                                                                                        },
};

encoder_fsm_output encoder_fms_process(bool a_state, bool b_state) {
    static uint8_t state = ENCODER_FSM_START;
    uint8_t pinstate     = ((b_state << 1) | a_state) & 0xF;

    state = ttable[state & 0xf][pinstate];
    return (encoder_fsm_output) (state & 0x30);
}
