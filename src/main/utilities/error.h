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

#ifndef _UTILITIES_ERROR_
#define _UTILITIES_ERROR_

#include <stdbool.h>

#define DEFINE_ERROR(name, description) name,
typedef enum {
    #include "error.scf"
    error_last
} error_status_t;
#undef DEFINE_ERROR

void _error_print_message(error_status_t error, char* file, unsigned line);
#define error_print_message(error) _error_print_message(error, __FILE__, __LINE__);

static inline bool error_is_success(error_status_t status) {
   return status == error_any; 
}

void _fatal_handler(error_status_t rc, const char *file, int line, const char *function, const char *expression);
#define FATAL_IF_FAIL(x) do {                               \
        error_status_t err_rc_ = (x);                       \
        if (unlikely(err_rc_ != error_any)) {               \
            _fatal_handler(err_rc_, __FILE__, __LINE__,     \
                                    __FUNCTION__, #x);      \
        }                                                   \
    } while(0)

#endif  // _UTILITIES_ERROR_
