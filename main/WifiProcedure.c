#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_wps.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "cJSON.h"
#include "cJSON_Utils.h"

#include "lwip/err.h"
#include "lwip/sys.h"


#include "websocket_server.h"

#include "StorageManager.c"
/* The examples use WiFi configuration that you can set via 'make menuconfig'.

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
#define SSID_AP         "IoTDev"
#define PWD_AP          "abcdhgfe"
#define MAX_STA_CONN    20
#define MAXAPS          10

static int cTry = 10;

static esp_wps_config_t wps_cfg = WPS_CONFIG_INIT_DEFAULT(WPS_TYPE_PBC);

static const char *TAG = "WIFI PROCEDURES";

static wifi_config_t globalCongfig = {
        .ap = {
            .ssid = SSID_AP,
            .ssid_len = strlen(SSID_AP),
            .max_connection = MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
            .password = PWD_AP
        },
    };
static uint16_t aps = MAXAPS;
static wifi_ap_record_t records[MAXAPS];
static char * SCONNECTION = "{\"ok\":true}";
static char * FCONNECTION = "{\"ok\":flase}";

TaskHandle_t hand;
static bool scanDone = false;

static char* getAuthModeName(wifi_auth_mode_t auth_mode) {

	char *names[] = {"OPEN", "WEP", "WPA PSK", "WPA2 PSK", "WPA WPA2 PSK", "MAX"};
	return names[auth_mode];
}

void char_to_uint(uint8_t* dest,char* src,int d_size,size_t s_size){
    for(int i=0; i<d_size;i++){
            if(i<=s_size)
                dest[i] = src[i];
            else
                dest[i] ='\0';
    }
    ESP_LOGI(TAG,"Received PWD: %s \n",src);
    ESP_LOGI(TAG,"Saved SSID: %s \n",(char*)dest);
}

void wifi_connect(char *ssid, char* pwd, int channel){
    if(strlen(ssid) != 0 && strlen(pwd)!=0){
        char_to_uint(globalCongfig.sta.ssid,ssid,32,strlen(ssid));
        char_to_uint(globalCongfig.sta.password,pwd,64,strlen(ssid));
        globalCongfig.sta.bssid_set = false;
        globalCongfig.sta.channel = channel;
        ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_set_config(ESP_IF_WIFI_STA, &globalCongfig));
    }
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_start());
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_connect());
}

void wifi_init_softap()
{
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &globalCongfig));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s",SSID_AP);
    ESP_LOGI(TAG, "wifi_init_softap password. PWDA:%s",PWD_AP);
}

void wifi_scan(){
    wifi_scan_config_t scan_config = {
	.ssid = 0,
	.bssid = 0,
	.channel = 0,
        .show_hidden = true
    };
    ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_config, true));
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&aps,records));

}

void scanTask(){
    sleep(10);
    if(records != NULL)
    while (!scanDone){
        wifi_scan();
        sleep(10);
    }
    vTaskDelete(hand);
    ESP_ERROR_CHECK(esp_wifi_scan_stop());
}

static void startScanTask(){
    ESP_LOGI(TAG,"STARTED SCAN TASK");
    xTaskCreate(&scanTask,"scanTask",10000,NULL,9,&hand);
}


static void wifi_event_handler(void* arg, esp_event_base_t event_base,int32_t event_id, void* event_data){
    switch (event_id){
    case WIFI_EVENT_AP_STACONNECTED :{
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" join, AID=%d", MAC2STR(event->mac), event->aid);
        break;
    }
    case WIFI_EVENT_AP_STADISCONNECTED:{
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d", MAC2STR(event->mac), event->aid);
        break;
    }
    case WIFI_EVENT_SCAN_DONE:{
        cJSON* sNetwork = NULL;
        sNetwork = cJSON_CreateArray();
        ESP_LOGI(TAG, "WIFI_EVENT_SCAN_DONE \n");
        if(records != NULL){
            for(int i = 0; i < aps; i++){
                cJSON* lNetwork = cJSON_CreateObject();
                cJSON_AddStringToObject(lNetwork,"ssid",(char *)records[i].ssid);
                cJSON_AddNumberToObject(lNetwork,"channel",(int)records[i].primary);
                cJSON_AddNumberToObject(lNetwork,"signal",(int)records[i].rssi);
                cJSON_AddStringToObject(lNetwork,"security",getAuthModeName(records[i].authmode));
                cJSON_AddNumberToObject(lNetwork,"nSec",records[i].authmode);
                cJSON_AddItemToArray(sNetwork,lNetwork);
            }
            char * nSer = cJSON_Print(sNetwork);
            int len = strlen(nSer);
            ws_server_send_text_all(nSer,len);
            ESP_LOGI(TAG," %s ",nSer);
            cJSON_Delete(sNetwork);
        }
        break;
    }
    case WIFI_EVENT_STA_CONNECTED:
        ESP_LOGI(TAG," %s ","WIFI_EVENT_STA_CONNECTED");
        //ws_server_send_text_all(SCONNECTION,strlen(SCONNECTION));
        break;
    case WIFI_EVENT_STA_DISCONNECTED:
        cTry--;
        ESP_LOGI(TAG," %s ","WIFI_EVENT_STA_DISCONNECTED");
        if(cTry == 0){
            scanDone = false;
            wifi_init_softap();
            startScanTask();
            //ws_server_send_text_all(FCONNECTION,strlen(FCONNECTION));
        }else
        {
            ESP_ERROR_CHECK(esp_wifi_connect());
        }
        break;
    case WIFI_EVENT_STA_WPS_ER_SUCCESS:
        ESP_LOGI(TAG, "WIFI_EVENT_STA_WPS_ER_SUCCESS");
        /* esp_wifi_wps_start() only gets ssid & password, so call esp_wifi_connect() here. */
        ESP_ERROR_CHECK(esp_wifi_wps_disable());
        insertBoolen("cnf",true);
        ESP_ERROR_CHECK(esp_wifi_connect());
        break;
    case WIFI_EVENT_STA_WPS_ER_FAILED:
        ESP_LOGI(TAG, "WIFI_EVENT_STA_WPS_ER_FAILED");
        ESP_ERROR_CHECK(esp_wifi_wps_disable());
        scanDone = false;
        startScanTask();
        break;
    case WIFI_EVENT_STA_WPS_ER_TIMEOUT:
        ESP_LOGI(TAG, "WIFI_EVENT_STA_WPS_ER_TIMEOUT");
        ESP_ERROR_CHECK(esp_wifi_wps_disable());
        scanDone = false;
        startScanTask();
        break;
    default:
        break;
    }

}

void connectToStored(){
    char ssid[32];
    char pwd[64];
    int ssid_size = getString("ssid",ssid,32);
    int pwd_size = getString("pwd",pwd,64);
    ESP_LOGI(TAG, "loaded SSID:%s",ssid);
    ESP_LOGI(TAG, "loaded PWD:%s",pwd);
    char_to_uint(globalCongfig.sta.ssid,ssid,32,strlen(ssid));
    char_to_uint(globalCongfig.sta.password,pwd,64,strlen(ssid));
    globalCongfig.sta.bssid_set = false;
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &globalCongfig));
    ESP_ERROR_CHECK(esp_wifi_start());
    if(ssid_size == 0){
        ESP_LOGI(TAG, "Courrupted save resetting");
        resetAll();
        esp_restart();
    }
}

void wifi_start()
{
    ESP_LOGI(TAG, "ESP_WIFI_MODE_AP");
    setNameSpace("wifi");
    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    bool cnf = getBoolen("cnf");
    if(cnf){
        scanDone = true;
        esp_wifi_set_mode(WIFI_MODE_STA);
        esp_wifi_start();
        esp_wifi_connect();
    }
    else{
        wifi_init_softap();
        startScanTask();
    }
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
}
