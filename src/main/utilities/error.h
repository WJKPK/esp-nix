#ifndef _UTILITIES_ERROR_
#define _UTILITIES_ERROR_

#define DEFINE_ERROR(name, description) name,
typedef enum {
    #include "error.scf"
    error_last
} error_t;
#undef DEFINE_ERROR

void _error_print_message(error_t error, char* file, unsigned line);
#define error_print_message(error) _error_print_message(error, __FILE__, __LINE__);

#endif  // _UTILITIES_ERROR_
