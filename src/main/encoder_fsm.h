#include <stdbool.h>

typedef enum {
   encoder_direction_any = 0x0,
   encoder_direction_clockwise = 0x10,
   encoder_direction_counterclockwise = 0x20,
} encoder_fsm_output;

encoder_fsm_output encoder_fms_process(bool a_state, bool b_state);

