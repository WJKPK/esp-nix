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

#include "esp_log.h"
typedef enum {
    LOG_OUTPUT_VERBOSE,
    LOG_OUTPUT_DEBUG,
    LOG_OUTPUT_INFO,
    LOG_OUTPUT_WARNING,
    LOG_OUTPUT_ERROR
} LogLevel;

#ifndef LOGGER_OUTPUT_LEVEL
#define LOGGER_OUTPUT_LEVEL LOG_OUTPUT_INFO
#endif

#if LOGGER_OUTPUT_LEVEL >= LOG_OUTPUT_ERROR
#define log_error( tag, format, ... ) ESP_LOG_LEVEL_LOCAL(ESP_LOG_ERROR,   tag, format, ##__VA_ARGS__)
#else
#define log_error( tag, format, ... )
#endif

#if LOGGER_OUTPUT_LEVEL >= LOG_OUTPUT_WARNING
#define log_warning( tag, format, ... ) ESP_LOG_LEVEL_LOCAL(ESP_LOG_WARN,    tag, format, ##__VA_ARGS__)
#else
#define log_warning( tag, format, ... )
#endif

#if LOGGER_OUTPUT_LEVEL >= LOG_OUTPUT_INFO
#define log_info( tag, format, ... ) ESP_LOG_LEVEL_LOCAL(ESP_LOG_INFO, tag, format, ##__VA_ARGS__)
#else
#define log_info( tag, format, ... )
#endif

#if LOGGER_OUTPUT_LEVEL >= LOG_OUTPUT_DEBUG
#define log_debug( tag, format, ... ) ESP_LOG_LEVEL_LOCAL(ESP_LOG_DEBUG,   tag, format, ##__VA_ARGS__)
#else
#define log_debug( tag, format, ... )
#endif

#if LOGGER_OUTPUT_LEVEL >= LOG_OUTPUT_VERBOSE
#define log_verbose( tag, format, ... ) ESP_LOG_LEVEL_LOCAL(ESP_LOG_VERBOSE, tag, format, ##__VA_ARGS__)
#else
#define log_verbose( tag, format, ... )
#endif

void logger_init(void);

