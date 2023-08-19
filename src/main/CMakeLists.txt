idf_component_register(SRCS "main.c"
                            "gatt_svr.c"
                            "utilities/scheduler.c"
                    INCLUDE_DIRS "."
                                 "utilities/configs"
                                 "${IDF_PATH}/components/freertos/FreeRTOS-Kernel/include/freertos")
