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

#include "utilities/error.h"
#include <stdio.h>

#define DEFINE_ERROR(name, description) case name: printf("%s: %s:%u", description, file, line); break;
void _error_print_message(error_status_t error, char* file, unsigned line) {
    switch(error) {

        #include "error.scf"

        case error_last:
            break;
    }
}
#undef DEFINE_ERROR

