#include <stdint.h>
#include "encoder_fsm.h"

typedef enum {
    encoder_fsm_start = 0,
    encoder_fsm_clock_final,
    encoder_fsm_clock_begin,
    encoder_fsm_clock_next,
    encoder_fsm_conterclock_begin,
    encoder_fsm_counterclock_final,
    encoder_fsm_counterclock_next,
    encoder_fsm_last
} encoder_fsm_state;

const uint8_t ttable[encoder_fsm_last][4] = {
  // start
  {encoder_fsm_start, encoder_fsm_clock_begin,  encoder_fsm_conterclock_begin, encoder_fsm_start},
  // clockwise_final
  {encoder_fsm_clock_next, encoder_fsm_start, encoder_fsm_clock_final,  encoder_fsm_start | encoder_direction_clockwise},
  // clockwise_begin 
  {encoder_fsm_clock_next,  encoder_fsm_clock_begin,  encoder_fsm_start, encoder_fsm_start},
  // clockwise_next
  {encoder_fsm_clock_next,  encoder_fsm_clock_begin,  encoder_fsm_clock_final,  encoder_fsm_start},
  // counterclockwise_begin
  {encoder_fsm_counterclock_next, encoder_fsm_start, encoder_fsm_conterclock_begin, encoder_fsm_start},
  // counterclockwise_final
  {encoder_fsm_counterclock_next, encoder_fsm_counterclock_final, encoder_fsm_start, encoder_fsm_start | encoder_direction_counterclockwise},
  // counterclockwise_next
  {encoder_fsm_counterclock_next, encoder_fsm_counterclock_final, encoder_fsm_conterclock_begin, encoder_fsm_start},
};

encoder_fsm_output encoder_fms_process(bool a_state, bool b_state) {
    static uint8_t state = encoder_fsm_start;
    uint8_t pinstate = ((b_state << 1) | a_state) & 0xF;
    state = ttable[state & 0xf][pinstate];
    return (encoder_fsm_output)(state & 0x30);
}

