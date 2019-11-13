#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "cJSON.h"
#include "cJSON_Utils.h"

#include "mqtt_client.h"

#define SERVER "mqtts://taverna.pettinato.eu:8883"

#define CONFIG_EMITTER_CHANNEL_KEY "test"

static const char *TAG_MQTT = "MQTT";

static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    // your_context_t *context = event->context;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG_MQTT, "MQTT_EVENT_CONNECTED");
            msg_id = esp_mqtt_client_subscribe(client, CONFIG_EMITTER_CHANNEL_KEY"/topic/", 0);
            ESP_LOGI(TAG_MQTT, "sent subscribe successful, msg_id=%d", msg_id);

            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG_MQTT, "MQTT_EVENT_DISCONNECTED");
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG_MQTT, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            msg_id = esp_mqtt_client_publish(client, CONFIG_EMITTER_CHANNEL_KEY"/topic/", "data", 0, 0, 0);
            ESP_LOGI(TAG_MQTT, "sent publish successful, msg_id=%d", msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG_MQTT, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG_MQTT, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG_MQTT, "MQTT_EVENT_DATA");
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG_MQTT, "MQTT_EVENT_ERROR");
            break;
        default:
            break;
    }
    return ESP_OK;
}

static esp_mqtt_client_config_t cfg = {
    .uri = SERVER,
    .event_handle = mqtt_event_handler

};

static void mqtt_init(){
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&cfg);
    esp_mqtt_client_start(client);
    esp_mqtt_client_subscribe(client,"IoT/Key",1);
    char * data = "prova";
    esp_mqtt_client_publish(client,"IoT/Key",data,strlen(data),1,1);
}