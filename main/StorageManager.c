#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"

#include "esp_log.h"

static char* currentNS = NULL;
static nvs_handle_t my_handle;
int err;
static const char *STORAGE_TAG = "STORAGE PROCEDURE";
void storage_init(){
    err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        /* NVS partition was truncated
         * and needs to be erased */
        ESP_ERROR_CHECK(nvs_flash_erase());

        /* Retry nvs_flash_init */
        ESP_ERROR_CHECK(nvs_flash_init());
    }
    ESP_ERROR_CHECK(err);
}

void setNameSpace(char* namespace){
    storage_init();
    currentNS = namespace;
    err = nvs_open(currentNS,NVS_READWRITE,&my_handle);
    ESP_ERROR_CHECK_WITHOUT_ABORT(err);
    nvs_close(my_handle);
}

size_t getString(char* key, char* value, size_t maxLen){
    size_t l = 0;

    err = nvs_open(currentNS,NVS_READONLY,&my_handle);
    ESP_ERROR_CHECK_WITHOUT_ABORT(err);
    
    err = nvs_get_str(my_handle,key,NULL,&l);
    ESP_ERROR_CHECK_WITHOUT_ABORT(err);

    if(l>maxLen)
        return ESP_ERR_INVALID_SIZE;

    err = nvs_get_str(my_handle,key,value,&l);
    ESP_ERROR_CHECK_WITHOUT_ABORT(err);
    nvs_close(my_handle);

    ESP_LOGI(STORAGE_TAG,"retrivec: %s",value);

    return l;
}

int getNumber(char* key){
    int16_t toReturn = 0;
    err = nvs_open(currentNS,NVS_READONLY,&my_handle);
    ESP_ERROR_CHECK_WITHOUT_ABORT(err);

    err = nvs_get_i16(my_handle, key , &toReturn);
    ESP_ERROR_CHECK_WITHOUT_ABORT(err);

    err = nvs_commit(my_handle);
    nvs_close(my_handle);

    return toReturn;
}

bool getBoolean(char* key){
    int8_t toReturn = 0;
    err = nvs_open(currentNS,NVS_READWRITE,&my_handle);
    ESP_ERROR_CHECK_WITHOUT_ABORT(err);

    err = nvs_get_i8(my_handle, key , &toReturn);
    ESP_ERROR_CHECK_WITHOUT_ABORT(err);

    err = nvs_commit(my_handle);
    nvs_close(my_handle);

    return (bool)toReturn;
}

void insertString(char* key, char * value){
    err = nvs_open(currentNS,NVS_READWRITE,&my_handle);
    ESP_ERROR_CHECK_WITHOUT_ABORT(err);

    err = nvs_set_str(my_handle, key , value);
    ESP_ERROR_CHECK_WITHOUT_ABORT(err);
    
    err = nvs_set_str(my_handle, key , value);
    ESP_ERROR_CHECK_WITHOUT_ABORT(err);

    err = nvs_commit(my_handle);
    nvs_close(my_handle);
}
void insertNumber(char* key, int value){
    err = nvs_open(currentNS,NVS_READWRITE,&my_handle);
    ESP_ERROR_CHECK_WITHOUT_ABORT(err);
    err = nvs_set_i16(my_handle, key , value);
    ESP_ERROR_CHECK_WITHOUT_ABORT(err);
    err = nvs_commit(my_handle);
    nvs_close(my_handle);
}

void insertBoolean(char* key, bool value){
    err = nvs_open(currentNS,NVS_READWRITE,&my_handle);
    ESP_ERROR_CHECK_WITHOUT_ABORT(err);
    err = nvs_set_i8(my_handle, key , (int8_t)value);
    ESP_ERROR_CHECK_WITHOUT_ABORT(err);
    err = nvs_commit(my_handle);
    nvs_close(my_handle);
}



void resetAll(){
    err = nvs_open(currentNS,NVS_READWRITE,&my_handle);
    ESP_ERROR_CHECK_WITHOUT_ABORT(err);
    
    err = nvs_erase_all(my_handle);
    ESP_ERROR_CHECK_WITHOUT_ABORT(err);
    
    err = nvs_commit(my_handle);
    nvs_close(my_handle);
}