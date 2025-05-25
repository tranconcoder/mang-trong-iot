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
#include "driver/gpio.h"

#include "esp_ble_mesh_defs.h"
#include "esp_ble_mesh_common_api.h"
#include "esp_ble_mesh_networking_api.h"
#include "esp_ble_mesh_provisioning_api.h"
#include "esp_ble_mesh_config_model_api.h"
#include "esp_ble_mesh_local_data_operation_api.h"

#include "board.h"
#include "ble_mesh_example_init.h"

#define TAG "DHT_NODE"

#define CID_ESP     0x02E5

#define ESP_BLE_MESH_VND_MODEL_ID_CLIENT    0x0000
#define ESP_BLE_MESH_VND_MODEL_ID_SERVER    0x0001

#define ESP_BLE_MESH_VND_MODEL_OP_SEND      ESP_BLE_MESH_MODEL_OP_3(0x00, CID_ESP)
#define ESP_BLE_MESH_VND_MODEL_OP_STATUS    ESP_BLE_MESH_MODEL_OP_3(0x01, CID_ESP)
#define ESP_BLE_MESH_VND_MODEL_OP_DHT_DATA  ESP_BLE_MESH_MODEL_OP_3(0x02, CID_ESP)
#define ESP_BLE_MESH_VND_MODEL_OP_LED_CTRL  ESP_BLE_MESH_MODEL_OP_3(0x04, CID_ESP)

#define DHT_SEND_INTERVAL_MS    5000  // Send DHT data every 5 seconds
#define TARGET_NODE_ADDR        0x0001  // Address of the target node (client/provisioner)
#define LDR_NODE_ADDR           0x0003  // Address of the LDR node (vendor_server_2)
#define MSG_SEND_TTL            3
#define MSG_TIMEOUT             0

// Built-in LED pin (GPIO2 for most ESP32 boards)
#define BUILTIN_LED_GPIO        2

// DHT sensor data structure
typedef struct {
    float temperature;
    float humidity;
    uint32_t timestamp;
} dht_data_t;

// LED control data structure
typedef struct {
    uint8_t led_state;  // 0 = OFF, 1 = ON
    uint32_t timestamp;
} led_ctrl_data_t;

// Store structure for vendor message handling
static struct example_info_store {
    uint16_t vnd_tid;       /* TID contained in the vendor message */
} store = {
    .vnd_tid = 0,
};

// Provisioning key structure
static struct esp_ble_mesh_key {
    uint16_t net_idx;
    uint16_t app_idx;
    uint8_t  app_key[ESP_BLE_MESH_OCTET16_LEN];
} prov_key;

static uint8_t dev_uuid[ESP_BLE_MESH_OCTET16_LEN] = { 0x32, 0x10 };
static esp_timer_handle_t dht_timer;
static bool is_provisioned = false;
static uint16_t net_idx = 0;
static uint16_t app_idx = 0;

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
    ESP_BLE_MESH_MODEL_OP(ESP_BLE_MESH_VND_MODEL_OP_LED_CTRL, sizeof(led_ctrl_data_t)),
    ESP_BLE_MESH_MODEL_OP_END,
};

static esp_ble_mesh_model_t vnd_models[] = {
    ESP_BLE_MESH_VENDOR_MODEL(CID_ESP, ESP_BLE_MESH_VND_MODEL_ID_SERVER,
    vnd_op, NULL, NULL),
};

// Forward declarations
static void send_dht_data(void);
static void dht_timer_callback(void* arg);
static void init_builtin_led(void);
static void set_builtin_led(uint8_t state);
void example_ble_mesh_send_vendor_message(bool resend);

// DHT simulation functions
static dht_data_t simulate_dht_reading(void)
{
    dht_data_t data;
    
    // Simulate temperature between 20-35°C
    data.temperature = 20.0f + (esp_random() % 1500) / 100.0f;
    
    // Simulate humidity between 30-80%
    data.humidity = 30.0f + (esp_random() % 5000) / 100.0f;
    
    data.timestamp = (uint32_t)(esp_timer_get_time() / 1000); // Convert to milliseconds
    
    return data;
}

static void send_dht_data(void)
{
    if (!is_provisioned) {
        ESP_LOGW(TAG, "Node not provisioned yet, skipping DHT data send");
        return;
    }
    
    dht_data_t dht_data = simulate_dht_reading();
    
    ESP_LOGI(TAG, "Sending DHT data - Temp: %.2f°C, Humidity: %.2f%%, Time: %lu ms", 
             dht_data.temperature, dht_data.humidity, dht_data.timestamp);
    ESP_LOGI(TAG, "Opcode: 0x%06x, Size: %d bytes", 
             ESP_BLE_MESH_VND_MODEL_OP_DHT_DATA, sizeof(dht_data_t));
    
    // Send to client/provisioner (TARGET_NODE_ADDR)
    esp_ble_mesh_msg_ctx_t ctx1 = {
        .net_idx = net_idx,
        .app_idx = app_idx,
        .addr = TARGET_NODE_ADDR,
        .send_ttl = 3,
        .send_rel = false,
    };
    
    esp_err_t err = esp_ble_mesh_server_model_send_msg(&vnd_models[0], &ctx1, 
                                                       ESP_BLE_MESH_VND_MODEL_OP_DHT_DATA,
                                                       sizeof(dht_data_t), (uint8_t *)&dht_data);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send DHT data to client: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "DHT data sent to client (0x%04x)", TARGET_NODE_ADDR);
    }
    
    // Send to all nodes using broadcast address
    esp_ble_mesh_msg_ctx_t ctx2 = {
        .net_idx = net_idx,
        .app_idx = app_idx,
        .addr = ESP_BLE_MESH_ADDR_ALL_NODES, // Broadcast to all nodes
        .send_ttl = 3,
        .send_rel = false,
    };
    
    err = esp_ble_mesh_server_model_send_msg(&vnd_models[0], &ctx2, 
                                             ESP_BLE_MESH_VND_MODEL_OP_DHT_DATA,
                                             sizeof(dht_data_t), (uint8_t *)&dht_data);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to broadcast DHT data: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "DHT data broadcasted to all nodes");
    }
}

static void dht_timer_callback(void* arg)
{
    send_dht_data();
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
    ESP_LOGI(TAG, "net_idx 0x%03x, addr 0x%04x", net_idx_param, addr);
    ESP_LOGI(TAG, "flags 0x%02x, iv_index 0x%08" PRIx32, flags, iv_index);
    
    net_idx = net_idx_param;
    is_provisioned = true;
    
    board_led_operation(LED_G, LED_OFF);
    
    // Start DHT data sending timer
    esp_timer_create_args_t timer_args = {
        .callback = dht_timer_callback,
        .name = "dht_timer"
    };
    
    esp_err_t err = esp_timer_create(&timer_args, &dht_timer);
    if (err == ESP_OK) {
        err = esp_timer_start_periodic(dht_timer, DHT_SEND_INTERVAL_MS * 1000); // Convert to microseconds
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "DHT timer started successfully");
        } else {
            ESP_LOGE(TAG, "Failed to start DHT timer: %s", esp_err_to_name(err));
        }
    } else {
        ESP_LOGE(TAG, "Failed to create DHT timer: %s", esp_err_to_name(err));
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
        if (dht_timer) {
            esp_timer_stop(dht_timer);
            esp_timer_delete(dht_timer);
            dht_timer = NULL;
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
    uint32_t opcode = param->model_operation.opcode;

    switch (event) {
    case ESP_BLE_MESH_MODEL_OPERATION_EVT:
        ESP_LOGI(TAG, "Recv 0x%06" PRIx32 ", tid 0x%04x", opcode, store.vnd_tid);
        if (param->model_operation.opcode == ESP_BLE_MESH_VND_MODEL_OP_SEND) {
            uint16_t tid = *(uint16_t *)param->model_operation.msg;
            ESP_LOGI(TAG, "Recv 0x%06" PRIx32 ", tid 0x%04x", param->model_operation.opcode, tid);
            store.vnd_tid = tid;
            example_ble_mesh_send_vendor_message(false);
        } else if (param->model_operation.opcode == ESP_BLE_MESH_VND_MODEL_OP_LED_CTRL) {
            ESP_LOGI(TAG, "LED control message received!");
            if (param->model_operation.length == sizeof(led_ctrl_data_t)) {
                led_ctrl_data_t *led_data = (led_ctrl_data_t *)param->model_operation.msg;
                ESP_LOGI(TAG, "=== LED Control Received ===");
                ESP_LOGI(TAG, "LED State: %s", led_data->led_state ? "ON" : "OFF");
                ESP_LOGI(TAG, "Timestamp: %lu ms", led_data->timestamp);
                ESP_LOGI(TAG, "From address: 0x%04x", param->model_operation.ctx->addr);
                ESP_LOGI(TAG, "============================");
                
                // Set the LED state
                set_builtin_led(led_data->led_state);
            } else {
                ESP_LOGW(TAG, "Received LED control data with incorrect length: %d (expected %d)",
                         param->model_operation.length, sizeof(led_ctrl_data_t));
            }
        }
        break;
    case ESP_BLE_MESH_MODEL_SEND_COMP_EVT:
        if (param->model_send_comp.err_code) {
            ESP_LOGE(TAG, "Failed to send message 0x%06" PRIx32, param->model_send_comp.opcode);
            break;
        }
        ESP_LOGI(TAG, "Send 0x%06" PRIx32, param->model_send_comp.opcode);
        break;
    case ESP_BLE_MESH_MODEL_PUBLISH_COMP_EVT:
        ESP_LOGI(TAG, "Publish completed");
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
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp32_bluetooth_init failed (err %d)", err);
        return;
    }

    // Initialize built-in LED
    init_builtin_led();

    /* Open nvs namespace for storing/restoring mesh example info */

    ble_mesh_get_dev_uuid(dev_uuid);

    /* Initialize the Bluetooth Mesh Subsystem */
    err = ble_mesh_init();
    if (err) {
        ESP_LOGE(TAG, "Bluetooth mesh init failed (err %d)", err);
    }
}

static void init_builtin_led(void)
{
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << BUILTIN_LED_GPIO),
        .pull_down_en = 0,
        .pull_up_en = 0,
    };
    gpio_config(&io_conf);
    
    // Turn off LED initially
    gpio_set_level(BUILTIN_LED_GPIO, 0);
    ESP_LOGI(TAG, "Built-in LED initialized on GPIO %d", BUILTIN_LED_GPIO);
}

static void set_builtin_led(uint8_t state)
{
    gpio_set_level(BUILTIN_LED_GPIO, state);
    ESP_LOGI(TAG, "Built-in LED %s", state ? "ON" : "OFF");
}

void example_ble_mesh_send_vendor_message(bool resend)
{
    esp_ble_mesh_msg_ctx_t ctx = {0};
    uint32_t opcode;
    esp_err_t err;

    ctx.net_idx = net_idx;
    ctx.app_idx = app_idx;
    ctx.addr = TARGET_NODE_ADDR;  // Send to client/provisioner
    ctx.send_ttl = MSG_SEND_TTL;
    opcode = ESP_BLE_MESH_VND_MODEL_OP_STATUS;

    if (resend == false) {
        store.vnd_tid++;
    }

    err = esp_ble_mesh_server_model_send_msg(&vnd_models[0], &ctx, opcode,
            sizeof(store.vnd_tid), (uint8_t *)&store.vnd_tid);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send vendor message 0x%06" PRIx32, opcode);
        return;
    }

    ESP_LOGI(TAG, "Sent vendor status message, tid: 0x%04x", store.vnd_tid);
}
