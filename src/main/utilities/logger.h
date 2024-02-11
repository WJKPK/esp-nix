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

#ifndef LOGGER_OUTPUT_LEVEL
    #define LOGGER_OUTPUT_LEVEL LOG_OUTPUT_INFO
#endif

#include <stdio.h>

typedef enum {
    LOG_OUTPUT_VERBOSE,
    LOG_OUTPUT_DEBUG,
    LOG_OUTPUT_INFO,
    LOG_OUTPUT_WARNING,
    LOG_OUTPUT_ERROR
} LogLevel;

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

#if LOGGER_OUTPUT_LEVEL >= LOG_OUTPUT_ERROR
#define log_error(format, ... ) printf(ANSI_COLOR_RED format ANSI_COLOR_RESET "\n", ##__VA_ARGS__)
#else
#define log_error(format, ... )
#endif

#if LOGGER_OUTPUT_LEVEL >= LOG_OUTPUT_WARNING
#define log_warning(format, ... ) printf(ANSI_COLOR_YELLOW format ANSI_COLOR_RESET "\n", ##__VA_ARGS__)
#else
#define log_warning(format, ... )
#endif

#if LOGGER_OUTPUT_LEVEL >= LOG_OUTPUT_INFO
#define log_info(format, ... ) printf(ANSI_COLOR_GREEN format ANSI_COLOR_RESET "\n", ##__VA_ARGS__)
#else
#define log_info(format, ... )
#endif

#if LOGGER_OUTPUT_LEVEL >= LOG_OUTPUT_DEBUG
#define log_debug(format, ... ) printf(ANSI_COLOR_BLUE format ANSI_COLOR_RESET "\n", ##__VA_ARGS__)
#else
#define log_debug(format, ... )
#endif

#if LOGGER_OUTPUT_LEVEL >= LOG_OUTPUT_VERBOSE
#define log_verbose(format, ... ) printf(ANSI_COLOR_CYAN format ANSI_COLOR_RESET "\n", ##__VA_ARGS__)
#else
#define log_verbose(format, ... )
#endif

