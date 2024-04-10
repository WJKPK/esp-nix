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

#include "esp_rom_sys.h"
#include "esp_cpu.h"

#define DEFINE_ERROR(name, description) case name: return description;
const char* _error_get_name(error_status_t error) {
    switch(error) {

        #include "error.scf"

        case ERROR_LAST:
            break;
    }
    return NULL;
}
#undef DEFINE_ERROR

void _error_print_message(error_status_t error, char* file, unsigned line) {
    esp_rom_printf("%s: %s:%u", _error_get_name(error), file, line);
}

static void error_print_stack_info(const char *msg, error_status_t rc, const char *file, int line, const char *function, const char *expression, intptr_t addr) {
    esp_rom_printf("%s failed: %s", msg, _error_get_name(rc));
    esp_rom_printf(" at 0x%08x\n", esp_cpu_get_call_addr(addr));
    esp_rom_printf("file: \"%s\" line %d\nfunc: %s\nexpression: %s\n", file, line, function, expression);
}

void _fatal_handler(error_status_t rc, const char *file, int line, const char *function, const char *expression) {
    error_print_stack_info("FATAL", rc, file, line, function, expression, (intptr_t)__builtin_return_address(0));
    abort();
}

