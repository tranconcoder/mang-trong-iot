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
#include <stdlib.h>
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_timer.h"

#include "esp_ble_mesh_defs.h"
#include "esp_ble_mesh_common_api.h"
#include "esp_ble_mesh_networking_api.h"
#include "esp_ble_mesh_provisioning_api.h"
#include "esp_ble_mesh_config_model_api.h"
#include "esp_ble_mesh_sensor_model_api.h"
#include "esp_ble_mesh_generic_model_api.h"

#include "ble_mesh_example_init.h"
#include "board.h"

#define TAG "SENSOR_NODE_2"

#define CID_ESP     0x02E6

/* Sensor Property ID */
#define SENSOR_PROPERTY_ID_0        0x0056  /* Present Indoor Ambient Temperature */
#define SENSOR_PROPERTY_ID_1        0x005B  /* Present Outdoor Ambient Temperature */

/* The characteristic of the two device properties is "Temperature 8", which is
 * used to represent a measure of temperature with a unit of 0.5 degree Celsius.
 * Minimum value: -64.0, maximum value: 63.5.
 * A value of 0xFF represents 'value is not known'.
 */
static int8_t temperature = 40;     /* Indoor temperature is 20 Degrees Celsius */
static int8_t humidity = 60;    /* Outdoor temperature is 30 Degrees Celsius */

#define SENSOR_POSITIVE_TOLERANCE   ESP_BLE_MESH_SENSOR_UNSPECIFIED_POS_TOLERANCE
#define SENSOR_NEGATIVE_TOLERANCE   ESP_BLE_MESH_SENSOR_UNSPECIFIED_NEG_TOLERANCE
#define SENSOR_SAMPLE_FUNCTION      ESP_BLE_MESH_SAMPLE_FUNC_UNSPECIFIED
#define SENSOR_MEASURE_PERIOD       ESP_BLE_MESH_SENSOR_NOT_APPL_MEASURE_PERIOD
#define SENSOR_UPDATE_INTERVAL      ESP_BLE_MESH_SENSOR_NOT_APPL_UPDATE_INTERVAL

static uint8_t dev_uuid[ESP_BLE_MESH_OCTET16_LEN] = { 0xdd, 0xdd };

static esp_ble_mesh_cfg_srv_t config_server = {
    /* 3 transmissions with 20ms interval */
    .net_transmit = ESP_BLE_MESH_TRANSMIT(2, 20),
    .relay = ESP_BLE_MESH_RELAY_ENABLED,
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

NET_BUF_SIMPLE_DEFINE_STATIC(sensor_data_0, 1);
NET_BUF_SIMPLE_DEFINE_STATIC(sensor_data_1, 1);

static esp_ble_mesh_sensor_state_t sensor_states[2] = {
    /* Mesh Model Spec:
     * Multiple instances of the Sensor states may be present within the same model,
     * provided that each instance has a unique value of the Sensor Property ID to
     * allow the instances to be differentiated. Such sensors are known as multisensors.
     * In this example, two instances of the Sensor states within the same model are
     * provided.
     */
    [0] = {
        /* Mesh Model Spec:
         * Sensor Property ID is a 2-octet value referencing a device property
         * that describes the meaning and format of data reported by a sensor.
         * 0x0000 is prohibited.
         */
        .sensor_property_id = SENSOR_PROPERTY_ID_0,
        /* Mesh Model Spec:
         * Sensor Descriptor state represents the attributes describing the sensor
         * data. This state does not change throughout the lifetime of an element.
         */
        .descriptor = {
            .positive_tolerance = SENSOR_POSITIVE_TOLERANCE,
            .negative_tolerance = SENSOR_NEGATIVE_TOLERANCE,
            .sampling_function = SENSOR_SAMPLE_FUNCTION,
            .measure_period = SENSOR_MEASURE_PERIOD,
            .update_interval = SENSOR_UPDATE_INTERVAL,
        },
        .sensor_data = {
            .format = ESP_BLE_MESH_SENSOR_DATA_FORMAT_A,
            .length = 0, /* 0 represents the length is 1 */
            .raw_value = &sensor_data_0,
        },
    },
    [1] = {
        .sensor_property_id = SENSOR_PROPERTY_ID_1,
        .descriptor = {
            .positive_tolerance = SENSOR_POSITIVE_TOLERANCE,
            .negative_tolerance = SENSOR_NEGATIVE_TOLERANCE,
            .sampling_function = SENSOR_SAMPLE_FUNCTION,
            .measure_period = SENSOR_MEASURE_PERIOD,
            .update_interval = SENSOR_UPDATE_INTERVAL,
        },
        .sensor_data = {
            .format = ESP_BLE_MESH_SENSOR_DATA_FORMAT_A,
            .length = 0, /* 0 represents the length is 1 */
            .raw_value = &sensor_data_1,
        },
    },
};

/* 20 octets is large enough to hold two Sensor Descriptor state values. */
/* 20 octets is large enough to hold two Sensor Descriptor state values. */
ESP_BLE_MESH_MODEL_PUB_DEFINE(sensor_pub, 20, ROLE_NODE);
static esp_ble_mesh_sensor_srv_t sensor_server = {
    .rsp_ctrl = {
        .get_auto_rsp = ESP_BLE_MESH_SERVER_RSP_BY_APP,
        .set_auto_rsp = ESP_BLE_MESH_SERVER_RSP_BY_APP,
    },
    .state_count = ARRAY_SIZE(sensor_states),
    .states = sensor_states,
};

ESP_BLE_MESH_MODEL_PUB_DEFINE(sensor_setup_pub, 20, ROLE_NODE);
static esp_ble_mesh_sensor_setup_srv_t sensor_setup_server = {
    .rsp_ctrl = {
        .get_auto_rsp = ESP_BLE_MESH_SERVER_RSP_BY_APP,
        .set_auto_rsp = ESP_BLE_MESH_SERVER_RSP_BY_APP,
    },
    .state_count = ARRAY_SIZE(sensor_states),
    .states = sensor_states,
};

/* Generic OnOff Client for receiving published messages */
static esp_ble_mesh_client_t onoff_client;

static esp_ble_mesh_model_t root_models[] = {
    ESP_BLE_MESH_MODEL_CFG_SRV(&config_server),
    ESP_BLE_MESH_MODEL_GEN_ONOFF_CLI(NULL, &onoff_client),
    ESP_BLE_MESH_MODEL_SENSOR_SRV(&sensor_pub, &sensor_server),
    ESP_BLE_MESH_MODEL_SENSOR_SETUP_SRV(&sensor_setup_pub, &sensor_setup_server),
};

static esp_ble_mesh_elem_t elements[] = {
    ESP_BLE_MESH_ELEMENT(0, root_models, ESP_BLE_MESH_MODEL_NONE),
};

static esp_ble_mesh_comp_t composition = {
    .cid = CID_ESP,
    .element_count = ARRAY_SIZE(elements),
    .elements = elements,
};

static esp_ble_mesh_prov_t provision = {
    .uuid = dev_uuid,
};

static void prov_complete(uint16_t net_idx, uint16_t addr, uint8_t flags, uint32_t iv_index)
{
    ESP_LOGI(TAG, "net_idx 0x%03x, addr 0x%04x", net_idx, addr);
    ESP_LOGI(TAG, "flags 0x%02x, iv_index 0x%08" PRIx32, flags, iv_index);
    board_led_operation(LED_G, LED_OFF);

    /* Initialize the indoor and outdoor temperatures for each sensor.  */
    net_buf_simple_add_u8(&sensor_data_0, temperature);
    net_buf_simple_add_u8(&sensor_data_1, humidity);
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
            printf("🔑 APP KEY ADDED!\n");
            ESP_LOGI(TAG, "🔑 ESP_BLE_MESH_MODEL_OP_APP_KEY_ADD");
            ESP_LOGI(TAG, "net_idx 0x%04x, app_idx 0x%04x",
                param->value.state_change.appkey_add.net_idx,
                param->value.state_change.appkey_add.app_idx);
            ESP_LOG_BUFFER_HEX("AppKey", param->value.state_change.appkey_add.app_key, 16);
            break;
        case ESP_BLE_MESH_MODEL_OP_MODEL_APP_BIND:
            printf("🔗 MODEL APP BIND COMPLETED!\n");
            ESP_LOGI(TAG, "🔗 ESP_BLE_MESH_MODEL_OP_MODEL_APP_BIND");
            ESP_LOGI(TAG, "elem_addr 0x%04x, app_idx 0x%04x, cid 0x%04x, mod_id 0x%04x",
                param->value.state_change.mod_app_bind.element_addr,
                param->value.state_change.mod_app_bind.app_idx,
                param->value.state_change.mod_app_bind.company_id,
                param->value.state_change.mod_app_bind.model_id);
            break;
        case ESP_BLE_MESH_MODEL_OP_MODEL_SUB_ADD:
            printf("📡 SUBSCRIPTION ADDED! Group: 0x%04x\n", param->value.state_change.mod_sub_add.sub_addr);
            printf("🎯 Node 2 is now subscribed and ready to receive data!\n");
            printf("📋 Model ID: 0x%04x (Should be 0x1100 for Sensor Server)\n", param->value.state_change.mod_sub_add.model_id);
            printf("🏢 Company ID: 0x%04x\n", param->value.state_change.mod_sub_add.company_id);
            ESP_LOGI(TAG, "📡 ESP_BLE_MESH_MODEL_OP_MODEL_SUB_ADD - SUBSCRIPTION CONFIGURED!");
            ESP_LOGI(TAG, "elem_addr 0x%04x, sub_addr 0x%04x, cid 0x%04x, mod_id 0x%04x",
                param->value.state_change.mod_sub_add.element_addr,
                param->value.state_change.mod_sub_add.sub_addr,
                param->value.state_change.mod_sub_add.company_id,
                param->value.state_change.mod_sub_add.model_id);
            break;
        default:
            break;
        }
    }
}

/* Function to parse and print received sensor data */
static void example_ble_mesh_parse_and_print_sensor_data(const uint8_t *data, uint16_t length)
{
    uint16_t offset = 0;
    static uint32_t message_count = 0;
    
    message_count++;
    
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║                          SENSOR NODE 2 - SUBSCRIPTION DATA                       ║\n");
    printf("╠══════════════════════════════════════════════════════════════════════════════════╣\n");
    printf("║ Message #%lu | Data Length: %u bytes                                        ║\n", message_count, length);
    printf("╚══════════════════════════════════════════════════════════════════════════════════╝\n");
    
    ESP_LOGI(TAG, "📨 RECEIVED SUBSCRIPTION DATA #%lu (Length: %u bytes)", message_count, length);
    ESP_LOG_BUFFER_HEX("Raw Data", data, length);

    while (offset < length) {
        uint8_t fmt = ESP_BLE_MESH_GET_SENSOR_DATA_FORMAT(data + offset);
        uint8_t data_len = ESP_BLE_MESH_GET_SENSOR_DATA_LENGTH(data + offset, fmt);
        uint16_t prop_id = ESP_BLE_MESH_GET_SENSOR_DATA_PROPERTY_ID(data + offset, fmt);
        uint8_t mpid_len = (fmt == ESP_BLE_MESH_SENSOR_DATA_FORMAT_A ?
                            ESP_BLE_MESH_SENSOR_DATA_FORMAT_A_MPID_LEN : ESP_BLE_MESH_SENSOR_DATA_FORMAT_B_MPID_LEN);

        printf("┌─────────────────────────────────────────────────────────────────────────────────┐\n");
        printf("│ SENSOR READING                                                                  │\n");
        printf("├─────────────────────────────────────────────────────────────────────────────────┤\n");
        printf("│ Property ID: 0x%04x | Format: %s | Data Length: %u                        │\n",
               prop_id, fmt == ESP_BLE_MESH_SENSOR_DATA_FORMAT_A ? "A" : "B", data_len);

        ESP_LOGI(TAG, "🔍 Parsing sensor data - Property ID: 0x%04x, Format: %s, Data Length: %u",
                 prop_id, fmt == ESP_BLE_MESH_SENSOR_DATA_FORMAT_A ? "A" : "B", data_len);

        if (data_len != ESP_BLE_MESH_SENSOR_DATA_ZERO_LEN) {
            // Get the raw sensor data value
            uint8_t raw_value = *(data + offset + mpid_len);
            
            // Display the sensor reading based on property ID
            if (prop_id == SENSOR_PROPERTY_ID_0) { // Temperature
                float temp = raw_value * 0.5f; // Convert to Celsius (Temperature 8 format)
                printf("│ 🌡️  TEMPERATURE: %.1f °C (Raw: %d)                                      │\n", temp, raw_value);
                ESP_LOGI(TAG, "🌡️ Temperature received: %.1f °C (Raw value: %d)", temp, raw_value);
            } else if (prop_id == SENSOR_PROPERTY_ID_1) { // Humidity
                printf("│ 💧 HUMIDITY: %d%% (Raw: %d)                                             │\n", raw_value, raw_value);
                ESP_LOGI(TAG, "💧 Humidity received: %d%% (Raw value: %d)", raw_value, raw_value);
            } else {
                printf("│ ❓ UNKNOWN SENSOR: Property 0x%04x, Value: %d                          │\n", prop_id, raw_value);
                ESP_LOGI(TAG, "❓ Unknown sensor data - Property ID: 0x%04x, Raw Value: %d", prop_id, raw_value);
                ESP_LOG_BUFFER_HEX("Unknown Sensor Raw Data", data + offset + mpid_len, data_len + 1);
            }
            
            offset += mpid_len + data_len + 1;
        } else {
            // Zero-length data has no raw value field
            printf("│ ⚠️  ZERO-LENGTH DATA for Property ID: 0x%04x                              │\n", prop_id);
            ESP_LOGI(TAG, "⚠️ Zero-length data received for Property ID: 0x%04x", prop_id);
            offset += mpid_len;
        }
        printf("└─────────────────────────────────────────────────────────────────────────────────┘\n");
    }
    
    printf("\n");
    printf("✅ SUBSCRIPTION DATA PROCESSING COMPLETE - Message #%lu\n", message_count);
    printf("═══════════════════════════════════════════════════════════════════════════════════\n\n");
    
    ESP_LOGI(TAG, "✅ Finished processing subscription data message #%lu", message_count);
}

static void example_ble_mesh_sensor_server_cb(esp_ble_mesh_sensor_server_cb_event_t event,
                                              esp_ble_mesh_sensor_server_cb_param_t *param)
{
    ESP_LOGI(TAG, "🔔 Sensor server callback - event %d, src 0x%04x, dst 0x%04x, model_id 0x%04x",
        event, param->ctx.addr, param->ctx.recv_dst, param->model->model_id);

    switch (event) {
    case ESP_BLE_MESH_SENSOR_SERVER_RECV_GET_MSG_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_SENSOR_SERVER_RECV_GET_MSG_EVT");
        // Handle sensor get requests
        if (param->ctx.recv_op == ESP_BLE_MESH_MODEL_OP_SENSOR_STATUS) {
            printf("\n🎯 SENSOR SERVER RECEIVED STATUS MESSAGE!\n");
            printf("📡 Source: 0x%04x → Destination: 0x%04x\n", param->ctx.addr, param->ctx.recv_dst);
            ESP_LOGI(TAG, "🎯 Received SENSOR_STATUS in GET event!");
        }
        break;
    case ESP_BLE_MESH_SENSOR_SERVER_RECV_SET_MSG_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_SENSOR_SERVER_RECV_SET_MSG_EVT");
        // Handle sensor set requests
        if (param->ctx.recv_op == ESP_BLE_MESH_MODEL_OP_SENSOR_STATUS) {
            printf("\n🎯 SENSOR SERVER RECEIVED STATUS MESSAGE!\n");
            printf("📡 Source: 0x%04x → Destination: 0x%04x\n", param->ctx.addr, param->ctx.recv_dst);
            ESP_LOGI(TAG, "🎯 Received SENSOR_STATUS in SET event!");
        }
        break;
    default:
        ESP_LOGI(TAG, "🔄 Sensor Server event %d - checking for SENSOR_STATUS", event);
        // Check if this is a published SENSOR_STATUS message
        if (param->ctx.recv_op == ESP_BLE_MESH_MODEL_OP_SENSOR_STATUS) {
            printf("\n🎯 SENSOR SERVER RECEIVED PUBLISHED DATA!\n");
            printf("📡 Source: 0x%04x → Destination: 0x%04x\n", param->ctx.addr, param->ctx.recv_dst);
            ESP_LOGI(TAG, "🎯 Received published SENSOR_STATUS message!");
        }
        break;
    }
}

/* Generic OnOff Client callback to receive published messages */
static void example_ble_mesh_generic_client_cb(esp_ble_mesh_generic_client_cb_event_t event,
                                               esp_ble_mesh_generic_client_cb_param_t *param)
{
    ESP_LOGI(TAG, "🔔 Generic client callback - event %d", event);

    if (param->error_code) {
        ESP_LOGE(TAG, "Generic client message failed (err %d)", param->error_code);
        return;
    }

    switch (event) {
    case ESP_BLE_MESH_GENERIC_CLIENT_GET_STATE_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_GENERIC_CLIENT_GET_STATE_EVT");
        break;
    case ESP_BLE_MESH_GENERIC_CLIENT_SET_STATE_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_GENERIC_CLIENT_SET_STATE_EVT");
        break;
    case ESP_BLE_MESH_GENERIC_CLIENT_PUBLISH_EVT:
        printf("\n🎯 GENERIC CLIENT RECEIVED PUBLISHED DATA!\n");
        ESP_LOGI(TAG, "🎯 ESP_BLE_MESH_GENERIC_CLIENT_PUBLISH_EVT");
        ESP_LOGI(TAG, "Generic client publish event, opcode 0x%04" PRIx32, param->params->opcode);
        break;
    case ESP_BLE_MESH_GENERIC_CLIENT_TIMEOUT_EVT:
        ESP_LOGW(TAG, "Generic client timeout event");
        break;
    default:
        ESP_LOGE(TAG, "Unknown Generic client event %d", event);
        break;
    }
}

static void example_ble_mesh_model_cb(esp_ble_mesh_model_cb_event_t event,
                                     esp_ble_mesh_model_cb_param_t *param)
{
    ESP_LOGI(TAG, "🔄 Model callback triggered - Event: %d", event);
    
    switch (event) {
    case ESP_BLE_MESH_MODEL_OPERATION_EVT:
        printf("📨 INCOMING MESSAGE: Opcode 0x%04x from 0x%04x to 0x%04x\n", 
               (uint16_t)param->model_operation.opcode, 
               param->model_operation.ctx->addr, 
               param->model_operation.ctx->recv_dst);
        ESP_LOGI(TAG, "📨 MODEL_OPERATION_EVT - Opcode: 0x%04x", (uint16_t)param->model_operation.opcode);
        
        if (param->model_operation.opcode == ESP_BLE_MESH_MODEL_OP_SENSOR_STATUS) {
            printf("\n🔔 SUBSCRIPTION ALERT: New sensor data received!\n");
            printf("📡 Source Address: 0x%04x\n", param->model_operation.ctx->addr);
            printf("📍 Destination Address: 0x%04x\n", param->model_operation.ctx->recv_dst);
            printf("🔑 App Key Index: 0x%04x\n", param->model_operation.ctx->app_idx);
            printf("🌐 Network Index: 0x%04x\n", param->model_operation.ctx->net_idx);
            printf("📊 Data Length: %u bytes\n", param->model_operation.length);
            printf("⏰ Timestamp: %lu ms\n", (unsigned long)(esp_timer_get_time() / 1000));
            
            ESP_LOGI(TAG, "🔔 SUBSCRIPTION: Received SENSOR_STATUS from 0x%04x to 0x%04x", 
                     param->model_operation.ctx->addr, param->model_operation.ctx->recv_dst);
            ESP_LOGI(TAG, "📡 Network info - NetIdx: 0x%04x, AppIdx: 0x%04x", 
                     param->model_operation.ctx->net_idx, param->model_operation.ctx->app_idx);
            
            if (param->model_operation.length > 0) {
                // Parse and print the received sensor data
                example_ble_mesh_parse_and_print_sensor_data(
                    param->model_operation.msg, param->model_operation.length);
            } else {
                printf("⚠️ WARNING: Received empty Sensor Status message\n");
                ESP_LOGI(TAG, "⚠️ Received empty Sensor Status message from 0x%04x", param->model_operation.ctx->addr);
            }
        } else {
            printf("📨 OTHER MESSAGE: Opcode 0x%04x, Length: %u bytes\n", 
                   (uint16_t)param->model_operation.opcode, param->model_operation.length);
            ESP_LOGI(TAG, "📨 Received model operation with opcode: 0x%04x from 0x%04x", 
                     (uint16_t)param->model_operation.opcode, param->model_operation.ctx->addr);
            if (param->model_operation.length > 0) {
                ESP_LOG_BUFFER_HEX("Message Data", param->model_operation.msg, param->model_operation.length);
            }
        }
        break;
    case ESP_BLE_MESH_MODEL_SEND_COMP_EVT:
        ESP_LOGI(TAG, "📤 Model send complete");
        break;
    case ESP_BLE_MESH_MODEL_PUBLISH_COMP_EVT:
        ESP_LOGI(TAG, "📢 Model publish complete");
        break;
    default:
        ESP_LOGI(TAG, "🔄 Model callback event: %d", event);
        break;
    }
}

static esp_err_t ble_mesh_init(void)
{
    esp_err_t err;

    esp_ble_mesh_register_prov_callback(example_ble_mesh_provisioning_cb);
    esp_ble_mesh_register_config_server_callback(example_ble_mesh_config_server_cb);
    esp_ble_mesh_register_sensor_server_callback(example_ble_mesh_sensor_server_cb);
    esp_ble_mesh_register_generic_client_callback(example_ble_mesh_generic_client_cb);
    esp_ble_mesh_register_custom_model_callback(example_ble_mesh_model_cb);
    
    ESP_LOGI(TAG, "🔧 All callbacks registered successfully");
    ESP_LOGI(TAG, "🎯 Node 2 will monitor for SENSOR_STATUS (0x%04x) messages", ESP_BLE_MESH_MODEL_OP_SENSOR_STATUS);
    ESP_LOGI(TAG, "🔧 Using Sensor Server model to receive published data");

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

    /* Set device name for scanner visibility */
    err = esp_ble_mesh_set_unprovisioned_device_name("NODE 2");
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set device name");
    } else {
        ESP_LOGI(TAG, "📛 Device name set to: NODE 2");
    }

    board_led_operation(LED_G, LED_ON);

    ESP_LOGI(TAG, "BLE Mesh sensor node 2 (subscriber) initialized");

    return ESP_OK;
}

void app_main(void)
{
    esp_err_t err;

    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║                              SENSOR NODE 2 STARTING                             ║\n");
    printf("║                            (SUBSCRIPTION RECEIVER)                              ║\n");
    printf("╚══════════════════════════════════════════════════════════════════════════════════╝\n");
    printf("\n");

    ESP_LOGI(TAG, "🚀 Initializing Sensor Node 2 (Subscriber)...");

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

    ble_mesh_get_dev_uuid(dev_uuid);

    /* Initialize the Bluetooth Mesh Subsystem */
    err = ble_mesh_init();
    if (err) {
        ESP_LOGE(TAG, "Bluetooth mesh init failed (err %d)", err);
        return;
    }

    printf("\n");
    printf("✅ SENSOR NODE 2 INITIALIZATION COMPLETE!\n");
    printf("🔔 Ready to receive sensor data via BLE Mesh subscription\n");
    printf("📡 Waiting for sensor data from other nodes...\n");
    printf("═══════════════════════════════════════════════════════════════════════════════════\n\n");
    
    ESP_LOGI(TAG, "✅ Sensor Node 2 ready to receive sensor data via subscription!");
    ESP_LOGI(TAG, "📡 Device UUID: %02x%02x", dev_uuid[0], dev_uuid[1]);
    ESP_LOGI(TAG, "🔍 Monitoring for SENSOR_STATUS messages...");
}
