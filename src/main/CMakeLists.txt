idf_component_register(SRCS "main.c"
                            "ble.c"
                            "utilities/scheduler.c"
                            "utilities/error.c"
                    INCLUDE_DIRS "."
                                 "utilities/configs"
                                 "${IDF_PATH}/components/freertos/FreeRTOS-Kernel/include/freertos")
