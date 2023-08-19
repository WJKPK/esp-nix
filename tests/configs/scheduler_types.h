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

#endif  // __SCHEDULER_CUSTOM_TYPES__
