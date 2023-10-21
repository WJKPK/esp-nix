idf_component_register(SRCS "main.c"
                            "ble.c"
                            "device_info.c"
                            "utilities/scheduler.c"
                            "utilities/error.c"
                            "utilities/logger.c"
                    INCLUDE_DIRS "."
                                 "utilities/configs"
                                 "${IDF_PATH}/components/freertos/FreeRTOS-Kernel/include/freertos")
