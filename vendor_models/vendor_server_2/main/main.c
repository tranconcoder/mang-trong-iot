/* main.c - Application main entry point */

/*
 * SPDX-FileCopyrightText: 2017 Intel Corporation
 * SPDX-FileContributor: 2018-2021 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"
#include "esp_timer.h"
#include "esp_random.h"
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_ble_mesh_defs.h"
#include "esp_ble_mesh_common_api.h"
#include "esp_ble_mesh_networking_api.h"
#include "esp_ble_mesh_provisioning_api.h"
#include "esp_ble_mesh_config_model_api.h"
#include "esp_ble_mesh_local_data_operation_api.h"

#include "board.h"
#include "ble_mesh_example_init.h"

#define TAG "LDR_NODE"

#define CID_ESP     0x02E5

#define ESP_BLE_MESH_VND_MODEL_ID_CLIENT    0x0000
#define ESP_BLE_MESH_VND_MODEL_ID_SERVER    0x0001

#define ESP_BLE_MESH_VND_MODEL_OP_SEND      ESP_BLE_MESH_MODEL_OP_3(0x00, CID_ESP)
#define ESP_BLE_MESH_VND_MODEL_OP_STATUS    ESP_BLE_MESH_MODEL_OP_3(0x01, CID_ESP)
#define ESP_BLE_MESH_VND_MODEL_OP_DHT_DATA  ESP_BLE_MESH_MODEL_OP_3(0x02, CID_ESP)
#define ESP_BLE_MESH_VND_MODEL_OP_LDR_DATA  ESP_BLE_MESH_MODEL_OP_3(0x03, CID_ESP)

#define LDR_SEND_INTERVAL_MS    3000  // Send LDR data every 3 seconds
#define TARGET_NODE_ADDR        0x0001  // Address of the target node (client/provisioner)

// I2C LCD Configuration
#define I2C_MASTER_SCL_IO           22    /*!< GPIO number used for I2C master clock */
#define I2C_MASTER_SDA_IO           21    /*!< GPIO number used for I2C master data  */
#define I2C_MASTER_NUM              0     /*!< I2C master i2c port number, the number of i2c peripheral interfaces available will depend on the chip */
#define I2C_MASTER_FREQ_HZ          100000                     /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */
#define I2C_MASTER_TIMEOUT_MS       1000

// LCD 1602 I2C Configuration
#define LCD_ADDR                    0x27  // Common I2C address for LCD1602 with I2C backpack
#define LCD_COLS                    16
#define LCD_ROWS                    2

// DHT sensor data structure (must match server)
typedef struct {
    float temperature;
    float humidity;
    uint32_t timestamp;
} dht_data_t;

// LDR sensor data structure
typedef struct {
    uint16_t light_level;      // Light level (0-4095, 12-bit ADC)
    float voltage;             // Voltage reading (0-3.3V)
    uint8_t light_status;      // 0=Dark, 1=Dim, 2=Bright
    uint32_t timestamp;        // Timestamp in milliseconds
} ldr_data_t;

static uint8_t dev_uuid[ESP_BLE_MESH_OCTET16_LEN] = { 0x33, 0x11 };
static esp_timer_handle_t ldr_timer;
static esp_timer_handle_t lcd_test_timer;
static bool is_provisioned = false;
static uint16_t net_idx = 0;
static uint16_t app_idx = 0;
static uint16_t my_addr = 0; // Store our own address

static esp_ble_mesh_cfg_srv_t config_server = {
    /* 3 transmissions with 20ms interval */
    .net_transmit = ESP_BLE_MESH_TRANSMIT(2, 20),
    .relay = ESP_BLE_MESH_RELAY_DISABLED,
    .relay_retransmit = ESP_BLE_MESH_TRANSMIT(2, 20),
    .beacon = ESP_BLE_MESH_BEACON_ENABLED,
#if defined(CONFIG_BLE_MESH_GATT_PROXY_SERVER)
    .gatt_proxy = ESP_BLE_MESH_GATT_PROXY_ENABLED,
#else
    .gatt_proxy = ESP_BLE_MESH_GATT_PROXY_NOT_SUPPORTED,
#endif
#if defined(CONFIG_BLE_MESH_FRIEND)
    .friend_state = ESP_BLE_MESH_FRIEND_ENABLED,
#else
    .friend_state = ESP_BLE_MESH_FRIEND_NOT_SUPPORTED,
#endif
    .default_ttl = 7,
};

static esp_ble_mesh_model_t root_models[] = {
    ESP_BLE_MESH_MODEL_CFG_SRV(&config_server),
};

static esp_ble_mesh_model_op_t vnd_op[] = {
    ESP_BLE_MESH_MODEL_OP(ESP_BLE_MESH_VND_MODEL_OP_SEND, 2),
    ESP_BLE_MESH_MODEL_OP(ESP_BLE_MESH_VND_MODEL_OP_DHT_DATA, sizeof(dht_data_t)),
    ESP_BLE_MESH_MODEL_OP_END,
};

static esp_ble_mesh_model_t vnd_models[] = {
    ESP_BLE_MESH_VENDOR_MODEL(CID_ESP, ESP_BLE_MESH_VND_MODEL_ID_SERVER,
    vnd_op, NULL, NULL),
};

// Global variables for received DHT data
static float last_temperature = 0.0;
static float last_humidity = 0.0;
static bool dht_data_received = false;

// Forward declarations
static esp_err_t i2c_master_init(void);
static esp_err_t lcd_init(void);
static void lcd_clear(void);
static void lcd_set_cursor(uint8_t col, uint8_t row);
static void lcd_print(const char* str);
static void lcd_write_cmd(uint8_t cmd);
static void lcd_write_data(uint8_t data);
static void lcd_write_nibble(uint8_t nibble, uint8_t rs);
static void update_lcd_display(void);

// LDR simulation functions
static ldr_data_t simulate_ldr_reading(void)
{
    ldr_data_t data;
    
    // Simulate light level (0-4095, 12-bit ADC)
    data.light_level = esp_random() % 4096;
    
    // Convert to voltage (0-3.3V)
    data.voltage = (data.light_level * 3.3f) / 4095.0f;
    
    // Determine light status based on light level
    if (data.light_level < 1000) {
        data.light_status = 0; // Dark
    } else if (data.light_level < 3000) {
        data.light_status = 1; // Dim
    } else {
        data.light_status = 2; // Bright
    }
    
    data.timestamp = (uint32_t)(esp_timer_get_time() / 1000); // Convert to milliseconds
    
    return data;
}

static void send_ldr_data(void)
{
    if (!is_provisioned) {
        ESP_LOGW(TAG, "Node not provisioned yet, skipping LDR data send");
        return;
    }
    
    ldr_data_t ldr_data = simulate_ldr_reading();
    
    const char* status_str[] = {"Dark", "Dim", "Bright"};
    ESP_LOGI(TAG, "Sending LDR data - Light: %d, Voltage: %.2fV, Status: %s, Time: %lu ms", 
             ldr_data.light_level, ldr_data.voltage, status_str[ldr_data.light_status], ldr_data.timestamp);
    ESP_LOGI(TAG, "Opcode: 0x%06x, Size: %d bytes, Target: 0x%04x", 
             ESP_BLE_MESH_VND_MODEL_OP_LDR_DATA, sizeof(ldr_data_t), TARGET_NODE_ADDR);
    
    esp_ble_mesh_msg_ctx_t ctx = {
        .net_idx = net_idx,
        .app_idx = app_idx,
        .addr = TARGET_NODE_ADDR,
        .send_ttl = 3,
        .send_rel = false,
    };
    
    esp_err_t err = esp_ble_mesh_server_model_send_msg(&vnd_models[0], &ctx, 
                                                       ESP_BLE_MESH_VND_MODEL_OP_LDR_DATA,
                                                       sizeof(ldr_data_t), (uint8_t *)&ldr_data);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send LDR data: %s", esp_err_to_name(err));
    }
}

static void ldr_timer_callback(void* arg)
{
    send_ldr_data();
}

static void lcd_test_timer_callback(void* arg)
{
    static int counter = 0;
    counter++;
    
    ESP_LOGI(TAG, "LCD test update #%d", counter);
    
    if (!dht_data_received) {
        // Update LCD with test message if no DHT data received
        char line1[17], line2[17];
        snprintf(line1, sizeof(line1), "Test #%d", counter);
        snprintf(line2, sizeof(line2), "Addr: 0x%04X", my_addr);
        
        lcd_clear();
        lcd_set_cursor(0, 0);
        lcd_print(line1);
        lcd_set_cursor(0, 1);
        lcd_print(line2);
        
        ESP_LOGI(TAG, "LCD updated with test message");
    }
}

static esp_ble_mesh_elem_t elements[] = {
    ESP_BLE_MESH_ELEMENT(0, root_models, vnd_models),
};

static esp_ble_mesh_comp_t composition = {
    .cid = CID_ESP,
    .element_count = ARRAY_SIZE(elements),
    .elements = elements,
};

static esp_ble_mesh_prov_t provision = {
    .uuid = dev_uuid,
};

static void prov_complete(uint16_t net_idx_param, uint16_t addr, uint8_t flags, uint32_t iv_index)
{
    ESP_LOGI(TAG, "=== PROVISIONING COMPLETE ===");
    ESP_LOGI(TAG, "net_idx 0x%03x, addr 0x%04x", net_idx_param, addr);
    ESP_LOGI(TAG, "flags 0x%02x, iv_index 0x%08" PRIx32, flags, iv_index);
    ESP_LOGI(TAG, "My address is: 0x%04x", addr);
    ESP_LOGI(TAG, "=============================");
    
    net_idx = net_idx_param;
    my_addr = addr; // Store our address
    is_provisioned = true;
    
    board_led_operation(LED_G, LED_OFF);
    
    // Start LDR data sending timer
    esp_timer_create_args_t timer_args = {
        .callback = ldr_timer_callback,
        .name = "ldr_timer"
    };
    
    esp_err_t err = esp_timer_create(&timer_args, &ldr_timer);
    if (err == ESP_OK) {
        err = esp_timer_start_periodic(ldr_timer, LDR_SEND_INTERVAL_MS * 1000); // Convert to microseconds
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "LDR timer started successfully");
        } else {
            ESP_LOGE(TAG, "Failed to start LDR timer: %s", esp_err_to_name(err));
        }
    } else {
        ESP_LOGE(TAG, "Failed to create LDR timer: %s", esp_err_to_name(err));
    }

    // Start LCD test timer
    esp_timer_create_args_t lcd_test_timer_args = {
        .callback = lcd_test_timer_callback,
        .name = "lcd_test_timer"
    };
    
    err = esp_timer_create(&lcd_test_timer_args, &lcd_test_timer);
    if (err == ESP_OK) {
        err = esp_timer_start_periodic(lcd_test_timer, 5000000); // Convert to microseconds (5 seconds)
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "LCD test timer started successfully");
        } else {
            ESP_LOGE(TAG, "Failed to start LCD test timer: %s", esp_err_to_name(err));
        }
    } else {
        ESP_LOGE(TAG, "Failed to create LCD test timer: %s", esp_err_to_name(err));
    }
}

static void example_ble_mesh_provisioning_cb(esp_ble_mesh_prov_cb_event_t event,
                                             esp_ble_mesh_prov_cb_param_t *param)
{
    switch (event) {
    case ESP_BLE_MESH_PROV_REGISTER_COMP_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_PROV_REGISTER_COMP_EVT, err_code %d", param->prov_register_comp.err_code);
        break;
    case ESP_BLE_MESH_NODE_PROV_ENABLE_COMP_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_NODE_PROV_ENABLE_COMP_EVT, err_code %d", param->node_prov_enable_comp.err_code);
        break;
    case ESP_BLE_MESH_NODE_PROV_LINK_OPEN_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_NODE_PROV_LINK_OPEN_EVT, bearer %s",
            param->node_prov_link_open.bearer == ESP_BLE_MESH_PROV_ADV ? "PB-ADV" : "PB-GATT");
        break;
    case ESP_BLE_MESH_NODE_PROV_LINK_CLOSE_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_NODE_PROV_LINK_CLOSE_EVT, bearer %s",
            param->node_prov_link_close.bearer == ESP_BLE_MESH_PROV_ADV ? "PB-ADV" : "PB-GATT");
        break;
    case ESP_BLE_MESH_NODE_PROV_COMPLETE_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_NODE_PROV_COMPLETE_EVT");
        prov_complete(param->node_prov_complete.net_idx, param->node_prov_complete.addr,
            param->node_prov_complete.flags, param->node_prov_complete.iv_index);
        break;
    case ESP_BLE_MESH_NODE_PROV_RESET_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_NODE_PROV_RESET_EVT");
        // Stop and delete timer if it exists
        if (ldr_timer) {
            esp_timer_stop(ldr_timer);
            esp_timer_delete(ldr_timer);
            ldr_timer = NULL;
        }
        if (lcd_test_timer) {
            esp_timer_stop(lcd_test_timer);
            esp_timer_delete(lcd_test_timer);
            lcd_test_timer = NULL;
        }
        is_provisioned = false;
        break;
    case ESP_BLE_MESH_NODE_SET_UNPROV_DEV_NAME_COMP_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_NODE_SET_UNPROV_DEV_NAME_COMP_EVT, err_code %d", param->node_set_unprov_dev_name_comp.err_code);
        break;
    default:
        break;
    }
}

static void example_ble_mesh_config_server_cb(esp_ble_mesh_cfg_server_cb_event_t event,
                                              esp_ble_mesh_cfg_server_cb_param_t *param)
{
    if (event == ESP_BLE_MESH_CFG_SERVER_STATE_CHANGE_EVT) {
        switch (param->ctx.recv_op) {
        case ESP_BLE_MESH_MODEL_OP_APP_KEY_ADD:
            ESP_LOGI(TAG, "ESP_BLE_MESH_MODEL_OP_APP_KEY_ADD");
            ESP_LOGI(TAG, "net_idx 0x%04x, app_idx 0x%04x",
                param->value.state_change.appkey_add.net_idx,
                param->value.state_change.appkey_add.app_idx);
            ESP_LOG_BUFFER_HEX("AppKey", param->value.state_change.appkey_add.app_key, 16);
            
            // Store app_idx for later use
            app_idx = param->value.state_change.appkey_add.app_idx;
            break;
        case ESP_BLE_MESH_MODEL_OP_MODEL_APP_BIND:
            ESP_LOGI(TAG, "ESP_BLE_MESH_MODEL_OP_MODEL_APP_BIND");
            ESP_LOGI(TAG, "elem_addr 0x%04x, app_idx 0x%04x, cid 0x%04x, mod_id 0x%04x",
                param->value.state_change.mod_app_bind.element_addr,
                param->value.state_change.mod_app_bind.app_idx,
                param->value.state_change.mod_app_bind.company_id,
                param->value.state_change.mod_app_bind.model_id);
            break;
        default:
            break;
        }
    }
}

static void example_ble_mesh_custom_model_cb(esp_ble_mesh_model_cb_event_t event,
                                             esp_ble_mesh_model_cb_param_t *param)
{
    switch (event) {
    case ESP_BLE_MESH_MODEL_OPERATION_EVT:
        ESP_LOGI(TAG, "Received message with opcode: 0x%06" PRIx32 ", length: %d", 
                 param->model_operation.opcode, param->model_operation.length);
        
        if (param->model_operation.opcode == ESP_BLE_MESH_VND_MODEL_OP_SEND) {
            uint16_t tid = *(uint16_t *)param->model_operation.msg;
            ESP_LOGI(TAG, "Recv 0x%06" PRIx32 ", tid 0x%04x", param->model_operation.opcode, tid);
            esp_err_t err = esp_ble_mesh_server_model_send_msg(&vnd_models[0],
                    param->model_operation.ctx, ESP_BLE_MESH_VND_MODEL_OP_STATUS,
                    sizeof(tid), (uint8_t *)&tid);
            if (err) {
                ESP_LOGE(TAG, "Failed to send message 0x%06x", ESP_BLE_MESH_VND_MODEL_OP_STATUS);
            }
        } else if (param->model_operation.opcode == ESP_BLE_MESH_VND_MODEL_OP_DHT_DATA) {
            ESP_LOGI(TAG, "DHT data received!");
            if (param->model_operation.length == sizeof(dht_data_t)) {
                dht_data_t *dht_data = (dht_data_t *)param->model_operation.msg;
                ESP_LOGI(TAG, "=== DHT Data Received ===");
                ESP_LOGI(TAG, "Temperature: %.2f°C", dht_data->temperature);
                ESP_LOGI(TAG, "Humidity: %.2f%%", dht_data->humidity);
                ESP_LOGI(TAG, "Timestamp: %lu ms", dht_data->timestamp);
                ESP_LOGI(TAG, "From address: 0x%04x", param->model_operation.ctx->addr);
                ESP_LOGI(TAG, "========================");
                
                // Store the received DHT data
                last_temperature = dht_data->temperature;
                last_humidity = dht_data->humidity;
                dht_data_received = true;
                
                // Update LCD display with new DHT data
                update_lcd_display();
            } else {
                ESP_LOGW(TAG, "Received DHT data with incorrect length: %d (expected %d)",
                         param->model_operation.length, sizeof(dht_data_t));
            }
        } else {
            ESP_LOGW(TAG, "Unknown opcode received: 0x%06" PRIx32, param->model_operation.opcode);
        }
        break;
    case ESP_BLE_MESH_MODEL_SEND_COMP_EVT:
        if (param->model_send_comp.err_code) {
            ESP_LOGE(TAG, "Failed to send message 0x%06" PRIx32, param->model_send_comp.opcode);
            break;
        }
        ESP_LOGI(TAG, "Send 0x%06" PRIx32, param->model_send_comp.opcode);
        break;
    default:
        break;
    }
}

static esp_err_t ble_mesh_init(void)
{
    esp_err_t err;

    esp_ble_mesh_register_prov_callback(example_ble_mesh_provisioning_cb);
    esp_ble_mesh_register_config_server_callback(example_ble_mesh_config_server_cb);
    esp_ble_mesh_register_custom_model_callback(example_ble_mesh_custom_model_cb);

    err = esp_ble_mesh_init(&provision, &composition);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize mesh stack");
        return err;
    }

    err = esp_ble_mesh_node_prov_enable((esp_ble_mesh_prov_bearer_t)(ESP_BLE_MESH_PROV_ADV | ESP_BLE_MESH_PROV_GATT));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable mesh node");
        return err;
    }

    board_led_operation(LED_G, LED_ON);

    ESP_LOGI(TAG, "BLE Mesh Node initialized");

    return ESP_OK;
}

// I2C LCD Driver Functions
static esp_err_t i2c_master_init(void)
{
    int i2c_master_port = I2C_MASTER_NUM;

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    i2c_param_config(i2c_master_port, &conf);

    return i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}

static void lcd_write_nibble(uint8_t nibble, uint8_t rs)
{
    uint8_t data = nibble | 0x08; // Enable backlight
    if (rs) data |= 0x01; // Set RS bit if needed
    
    // Write with E high
    data |= 0x04;
    i2c_master_write_to_device(I2C_MASTER_NUM, LCD_ADDR, &data, 1, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    vTaskDelay(pdMS_TO_TICKS(1));
    
    // Write with E low
    data &= ~0x04;
    i2c_master_write_to_device(I2C_MASTER_NUM, LCD_ADDR, &data, 1, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    vTaskDelay(pdMS_TO_TICKS(1));
}

static void lcd_write_cmd(uint8_t cmd)
{
    lcd_write_nibble(cmd & 0xF0, 0);
    lcd_write_nibble((cmd << 4) & 0xF0, 0);
    vTaskDelay(pdMS_TO_TICKS(2));
}

static void lcd_write_data(uint8_t data)
{
    lcd_write_nibble(data & 0xF0, 1);
    lcd_write_nibble((data << 4) & 0xF0, 1);
    vTaskDelay(pdMS_TO_TICKS(2));
}

static esp_err_t lcd_init(void)
{
    vTaskDelay(pdMS_TO_TICKS(50)); // Wait for LCD to power up
    
    // Initialize in 4-bit mode
    lcd_write_nibble(0x30, 0);
    vTaskDelay(pdMS_TO_TICKS(5));
    lcd_write_nibble(0x30, 0);
    vTaskDelay(pdMS_TO_TICKS(1));
    lcd_write_nibble(0x30, 0);
    vTaskDelay(pdMS_TO_TICKS(1));
    lcd_write_nibble(0x20, 0); // Set to 4-bit mode
    
    // Function set: 4-bit, 2 lines, 5x8 dots
    lcd_write_cmd(0x28);
    // Display control: display on, cursor off, blink off
    lcd_write_cmd(0x0C);
    // Clear display
    lcd_write_cmd(0x01);
    vTaskDelay(pdMS_TO_TICKS(2));
    // Entry mode: increment cursor, no shift
    lcd_write_cmd(0x06);
    
    ESP_LOGI(TAG, "LCD initialized successfully");
    return ESP_OK;
}

static void lcd_clear(void)
{
    lcd_write_cmd(0x01);
    vTaskDelay(pdMS_TO_TICKS(2));
}

static void lcd_set_cursor(uint8_t col, uint8_t row)
{
    uint8_t address = (row == 0) ? 0x80 : 0xC0;
    address += col;
    lcd_write_cmd(address);
}

static void lcd_print(const char* str)
{
    while (*str) {
        lcd_write_data(*str++);
    }
}

static void update_lcd_display(void)
{
    char line1[17], line2[17];
    
    if (dht_data_received) {
        snprintf(line1, sizeof(line1), "Temp: %.1fC", last_temperature);
        snprintf(line2, sizeof(line2), "Humi: %.1f%%", last_humidity);
    } else {
        snprintf(line1, sizeof(line1), "Waiting for");
        snprintf(line2, sizeof(line2), "DHT data...");
    }
    
    lcd_clear();
    lcd_set_cursor(0, 0);
    lcd_print(line1);
    lcd_set_cursor(0, 1);
    lcd_print(line2);
}

void app_main(void)
{
    esp_err_t err;

    ESP_LOGI(TAG, "Initializing...");

    err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    board_init();

    err = bluetooth_init();
    if (err) {
        ESP_LOGE(TAG, "esp32_bluetooth_init failed (err %d)", err);
        return;
    }

    // Initialize I2C and LCD
    ESP_LOGI(TAG, "Initializing I2C...");
    err = i2c_master_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C initialization failed: %s", esp_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "Initializing LCD...");
    err = lcd_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "LCD initialization failed: %s", esp_err_to_name(err));
        return;
    }

    // Display initial message
    update_lcd_display();

    ble_mesh_get_dev_uuid(dev_uuid);

    /* Initialize the Bluetooth Mesh Subsystem */
    err = ble_mesh_init();
    if (err) {
        ESP_LOGE(TAG, "Bluetooth mesh init failed (err %d)", err);
    }
}

