#include "utilities/error.h"
#include <stdio.h>

#define DEFINE_ERROR(name, description) case name: printf("%s: %s:%u", description, file, line); break;
void _error_print_message(error_t error, char* file, unsigned line) {
    switch(error) {

        #include "error.scf"

        case error_last:
            break;
    }
}
#undef DEFINE_ERROR

