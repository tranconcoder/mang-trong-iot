/* board.c - Board-specific hooks */

/*
 * SPDX-FileCopyrightText: 2017 Intel Corporation
 * SPDX-FileContributor: 2018-2021 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include "esp_log.h"
#include "iot_button.h"
#include "esp_ble_mesh_sensor_model_api.h"
#include "freertos/task.h"

#define TAG "BOARD"

#define BUTTON_IO_NUM           0
#define BUTTON_ACTIVE_LEVEL     0

extern void example_ble_mesh_send_sensor_message(uint32_t opcode);

#define SEND_DHT_DATA 1
#define SEND_LDR_DATA 2


void send_data_task() {
    while(true) {
        example_ble_mesh_send_sensor_message(SEND_DHT_DATA);
        example_ble_mesh_send_sensor_message(SEND_LDR_DATA);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void button_tap_cb(void* arg)
{
    example_ble_mesh_send_sensor_message(send_opcode[press_count++]);
    press_count = press_count % ARRAY_SIZE(send_opcode);
}

static void board_button_init(void)
{
    button_handle_t btn_handle = iot_button_create(BUTTON_IO_NUM, BUTTON_ACTIVE_LEVEL);
    if (btn_handle) {
        iot_button_set_evt_cb(btn_handle, BUTTON_CB_RELEASE, button_tap_cb, "RELEASE");
    }
}

void board_init(void)
{
    board_button_init();

    xTaskCreate(send_data_task, "send", 4096,  NULL, 5, NULL);
}
