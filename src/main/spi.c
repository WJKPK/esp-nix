/*
 * Copyright 2023 WJKPK
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include "spi.h"

#include <limits.h>
#include <string.h>

#include "driver/spi_common.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"

#define SPI_CLOCK_SOURCE 500000UL
#define SPI_HOST SPI2_HOST
#define SPI_MISO 7
#define AFE_SPI_CS 5
#define SPI_SCLK 10

static spi_device_handle_t spi_handle[SpiDeviceLast];

error_status_t spi_init(void) {
    spi_bus_config_t buscfg = {
        .miso_io_num = SPI_MISO,
        .mosi_io_num = -1,
        .sclk_io_num = SPI_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 32,
    };
    if (ESP_OK != spi_bus_initialize(SPI_HOST, &buscfg, SPI_DMA_CH_AUTO)) {
        return ERROR_RESOURCE_UNAVAILABLE;
    }
    struct {
        unsigned pin;
        uint32_t flags;
    } device_map[SpiDeviceLast] = {
        {AFE_SPI_CS, SPI_DEVICE_HALFDUPLEX}
    };
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = SPI_CLOCK_SOURCE,
        .mode = 0,
        .queue_size = 4,
    };
    for (spi_dev_t dev = SpiDeviceThermocoupleAfe; dev < SpiDeviceLast; dev++) {
        devcfg.spics_io_num = device_map[dev].pin;
        devcfg.flags = device_map[dev].flags;
        if (ESP_OK != spi_bus_add_device(SPI_HOST, &devcfg, &spi_handle[dev])) {
            goto error;
        }
    }
    return ERROR_ANY;

error:
    for (spi_dev_t dev = SpiDeviceThermocoupleAfe; dev < SpiDeviceLast; dev++) {
        spi_handle[dev] != NULL ? ({spi_bus_remove_device(spi_handle[dev]);}) : ({});
    }
    return ERROR_RESOURCE_UNAVAILABLE;
}

typedef bool(*spi_readout_conventer_t)(uint8_t*, size_t, uint16_t*);

bool afe_readout_to_temperature(uint8_t *read_buffer, size_t read_buffer_length, uint16_t* out) {
    read_buffer_length != 2 ? ({return false;}) : ({});
    uint16_t casted_temp = (read_buffer[0] << 8) | read_buffer[1];
    ((casted_temp & 0x4) >> 2) == 1 ? ({return false;}) : ({});

    *out = (((casted_temp & 0x7FF8) >> 3) / 4);
    return true;
}

error_status_t spi_read(spi_dev_t device, void* out_data, size_t size) {
    const struct {
        spi_readout_conventer_t conventer;
        size_t message_size;
    } conveter_map[SpiDeviceLast] = {
        {afe_readout_to_temperature, sizeof(uint16_t)}
    };

    spi_transaction_t t = {
        .cmd = 0,
        .rxlength = conveter_map[device].message_size * CHAR_BIT,
        .flags = SPI_TRANS_USE_RXDATA,
    };
    spi_device_polling_transmit(spi_handle[device], &t) != ESP_OK ? ({return ERROR_COMMUNICATION_ERROR;}) : ({});
    return conveter_map[device].conventer(t.rx_data, conveter_map[device].message_size, out_data) ?
        ERROR_ANY : ERROR_CONVERSION_ERROR;
}

