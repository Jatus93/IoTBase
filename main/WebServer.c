
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "lwip/api.h"

#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "driver/ledc.h"

#include "string.h"

#include "WifiProcedure.c"

static QueueHandle_t client_queue;
const static int client_queue_size = 10;

// handles websocket events
void websocket_callback(uint8_t num,WEBSOCKET_TYPE_t type,char* msg,uint64_t len) {
  const static char* TAG = "websocket_callback";

  switch(type) {
    case WEBSOCKET_CONNECT:
      ESP_LOGI(TAG,"client %i connected!",num);
      break;
    case WEBSOCKET_DISCONNECT_EXTERNAL:
      ESP_LOGI(TAG,"client %i sent a disconnect message",num);
      break;
    case WEBSOCKET_DISCONNECT_INTERNAL:
      ESP_LOGI(TAG,"client %i was disconnected",num);
      break;
    case WEBSOCKET_DISCONNECT_ERROR:
      ESP_LOGI(TAG,"client %i was disconnected due to an error",num);
      break;
    case WEBSOCKET_TEXT:
      ESP_LOGI(TAG,"%s",msg);
      cJSON * message = cJSON_Parse(msg);
      if(cJSON_GetObjectItem(message,"refresh")){
        if(cJSON_GetObjectItem(message,"refresh")->valueint == 1){
          if(scanDone){
            scanDone = false;
            startScanTask();
          }
        }
      }
      if(cJSON_GetObjectItem(message,"connect")){
        if(cJSON_GetObjectItem(message,"connect")->valueint == 1){
          wifi_connect(cJSON_GetObjectItem(message,"ssid")->valuestring,cJSON_GetObjectItem(message,"pwd")->valuestring,cJSON_GetObjectItem(message,"channel")->valueint);
          scanDone = true;
        }
      }
      if(cJSON_GetObjectItem(message,"save")){
        if(cJSON_GetObjectItem(message,"save")->valueint == 1){
          insertString("ssid",cJSON_GetObjectItem(message,"ssid")->valuestring);
          insertString("pwd",cJSON_GetObjectItem(message,"pwd")->valuestring);
          insertBoolen("cnf",true);
          ws_server_send_text_all(SCONNECTION,strlen(SCONNECTION));
        }
      }
      if(cJSON_GetObjectItem(message,"wps")){
        scanDone = true;
        if(cJSON_GetObjectItem(message,"wps")->valueint == 1){
          esp_wifi_wps_enable(&wps_cfg);
          esp_wifi_wps_start(0);
        }
      }
      break;
    case WEBSOCKET_BIN:
      ESP_LOGI(TAG,"client %i sent binary message of size %i:\n%s",num,(uint32_t)len,msg);
      break;
    case WEBSOCKET_PING:
      ESP_LOGI(TAG,"client %i pinged us with message of size %i:\n%s",num,(uint32_t)len,msg);
      break;
    case WEBSOCKET_PONG:
      ESP_LOGI(TAG,"client %i responded to the ping",num);
      break;
  }
}

// serves any clients
static void http_serve(struct netconn *conn) {
  const static char* TAG = "http_server";
  const static char HTML_HEADER[] = "HTTP/1.1 200 OK\nContent-type: text/html\n\n";
  const static char ERROR_HEADER[] = "HTTP/1.1 404 Not Found\nContent-type: text/html\n\n";
  const static char JS_HEADER[] = "HTTP/1.1 200 OK\nContent-type: text/javascript\n\n";
  const static char CSS_HEADER[] = "HTTP/1.1 200 OK\nContent-type: text/css\n\n";
  //const static char PNG_HEADER[] = "HTTP/1.1 200 OK\nContent-type: image/png\n\n";
  const static char ICO_HEADER[] = "HTTP/1.1 200 OK\nContent-type: image/x-icon\n\n";
  //const static char PDF_HEADER[] = "HTTP/1.1 200 OK\nContent-type: application/pdf\n\n";
  //const static char EVENT_HEADER[] = "HTTP/1.1 200 OK\nContent-Type: text/event-stream\nCache-Control: no-cache\nretry: 3000\n\n";
  struct netbuf* inbuf;
  static char* buf;
  static uint16_t buflen;
  static err_t err;

  // default page
  extern const uint8_t index_html_start[] asm("_binary_index_html_start");
  extern const uint8_t index_html_end[] asm("_binary_index_html_end");
  const uint32_t index_html_len = index_html_end - index_html_start;

  // test.js
  extern const uint8_t myjs_js_start[] asm("_binary_myjs_js_start");
  extern const uint8_t myjs_js_end[] asm("_binary_myjs_js_end");
  const uint32_t myjs_js_len = myjs_js_end - myjs_js_start;

  extern const uint8_t entireframework_min_css_start[] asm("_binary_entireframework_min_css_start");
  extern const uint8_t entireframework_min_css_end[] asm("_binary_entireframework_min_css_end");
  const uint32_t entireframework_min_css_len = entireframework_min_css_end - entireframework_min_css_start;

  // test.css
  extern const uint8_t mycss_css_start[] asm("_binary_mycss_css_start");
  extern const uint8_t mycss_css_end[] asm("_binary_mycss_css_end");
  const uint32_t mycss_css_len = mycss_css_end - mycss_css_start;

  // favicon.ico
  extern const uint8_t favicon_ico_start[] asm("_binary_favicon_ico_start");
  extern const uint8_t favicon_ico_end[] asm("_binary_favicon_ico_end");
  const uint32_t favicon_ico_len = favicon_ico_end - favicon_ico_start;

  // error page
  extern const uint8_t error_html_start[] asm("_binary_error_html_start");
  extern const uint8_t error_html_end[] asm("_binary_error_html_end");
  const uint32_t error_html_len = error_html_end - error_html_start;

  netconn_set_recvtimeout(conn,1000); // allow a connection timeout of 1 second
  ESP_LOGI(TAG,"reading from client...");
  err = netconn_recv(conn, &inbuf);
  ESP_LOGI(TAG,"read from client");
  if(err==ERR_OK) {
    netbuf_data(inbuf, (void**)&buf, &buflen);
    if(buf) {

      // default page
      if     (strstr(buf,"GET / ")
          && !strstr(buf,"Upgrade: websocket")) {
        ESP_LOGI(TAG,"Sending /");
        netconn_write(conn, HTML_HEADER, sizeof(HTML_HEADER)-1,NETCONN_NOCOPY);
        netconn_write(conn, index_html_start,index_html_len,NETCONN_NOCOPY);
        netconn_close(conn);
        netconn_delete(conn);
        netbuf_delete(inbuf);
      }

      // default page websocket
      else if(strstr(buf,"GET / ")
           && strstr(buf,"Upgrade: websocket")) {
        ESP_LOGI(TAG,"Requesting websocket on /");
        ws_server_add_client(conn,buf,buflen,"/",websocket_callback);
        netbuf_delete(inbuf);
      }

      else if(strstr(buf,"GET /myjs.js ")) {
        ESP_LOGI(TAG,"Sending /myjs.js");
        netconn_write(conn, JS_HEADER, sizeof(JS_HEADER)-1,NETCONN_NOCOPY);
        netconn_write(conn, myjs_js_start, myjs_js_len,NETCONN_NOCOPY);
        netconn_close(conn);
        netconn_delete(conn);
        netbuf_delete(inbuf);
      }

      else if(strstr(buf,"GET /mycss.css ")) {
        ESP_LOGI(TAG,"Sending /mycss.css");
        netconn_write(conn, CSS_HEADER, sizeof(CSS_HEADER)-1,NETCONN_NOCOPY);
        netconn_write(conn, mycss_css_start, mycss_css_len,NETCONN_NOCOPY);
        netconn_close(conn);
        netconn_delete(conn);
        netbuf_delete(inbuf);
      }

      else if(strstr(buf,"GET /entireframework.min.css ")) {
        ESP_LOGI(TAG,"Sending /entireframework.min.css");
        netconn_write(conn, CSS_HEADER, sizeof(CSS_HEADER)-1,NETCONN_NOCOPY);
        netconn_write(conn, entireframework_min_css_start, entireframework_min_css_len,NETCONN_NOCOPY);
        netconn_close(conn);
        netconn_delete(conn);
        netbuf_delete(inbuf);
      }


      else if(strstr(buf,"GET /favicon.ico ")) {
        ESP_LOGI(TAG,"Sending favicon.ico");
        netconn_write(conn,ICO_HEADER,sizeof(ICO_HEADER)-1,NETCONN_NOCOPY);
        netconn_write(conn,favicon_ico_start,favicon_ico_len,NETCONN_NOCOPY);
        netconn_close(conn);
        netconn_delete(conn);
        netbuf_delete(inbuf);
      }

      else if(strstr(buf,"GET /")) {
        ESP_LOGI(TAG,"Unknown request, sending error page: %s",buf);
        netconn_write(conn, ERROR_HEADER, sizeof(ERROR_HEADER)-1,NETCONN_NOCOPY);
        netconn_write(conn, error_html_start, error_html_len,NETCONN_NOCOPY);
        netconn_close(conn);
        netconn_delete(conn);
        netbuf_delete(inbuf);
      }

      else {
        ESP_LOGI(TAG,"Unknown request");
        netconn_close(conn);
        netconn_delete(conn);
        netbuf_delete(inbuf);
      }
    }
    else {
      ESP_LOGI(TAG,"Unknown request (empty?...)");
      netconn_close(conn);
      netconn_delete(conn);
      netbuf_delete(inbuf);
    }
  }
  else { // if err==ERR_OK
    ESP_LOGI(TAG,"error on read, closing connection");
    netconn_close(conn);
    netconn_delete(conn);
    netbuf_delete(inbuf);
  }
}

static void serve_module(struct netconn *conn) {
  const static char* TAG = "http_server";
  const static char HTML_HEADER[] = "HTTP/1.1 200 OK\nContent-type: text/html\n\n";
  const static char ERROR_HEADER[] = "HTTP/1.1 404 Not Found\nContent-type: text/html\n\n";
  const static char JS_HEADER[] = "HTTP/1.1 200 OK\nContent-type: text/javascript\n\n";
  const static char CSS_HEADER[] = "HTTP/1.1 200 OK\nContent-type: text/css\n\n";
  //const static char PNG_HEADER[] = "HTTP/1.1 200 OK\nContent-type: image/png\n\n";
  const static char ICO_HEADER[] = "HTTP/1.1 200 OK\nContent-type: image/x-icon\n\n";
  //const static char PDF_HEADER[] = "HTTP/1.1 200 OK\nContent-type: application/pdf\n\n";
  //const static char EVENT_HEADER[] = "HTTP/1.1 200 OK\nContent-Type: text/event-stream\nCache-Control: no-cache\nretry: 3000\n\n";
  struct netbuf* inbuf;
  static char* buf;
  static uint16_t buflen;
  static err_t err;

  // default page
  extern const uint8_t index_html_start[] asm("_binary_module_html_start");
  extern const uint8_t index_html_end[] asm("_binary_module_html_end");
  const uint32_t index_html_len = index_html_end - index_html_start;

  // test.js
  extern const uint8_t myjs_js_start[] asm("_binary_myjs_js_start");
  extern const uint8_t myjs_js_end[] asm("_binary_myjs_js_end");
  const uint32_t myjs_js_len = myjs_js_end - myjs_js_start;

  extern const uint8_t entireframework_min_css_start[] asm("_binary_entireframework_min_css_start");
  extern const uint8_t entireframework_min_css_end[] asm("_binary_entireframework_min_css_end");
  const uint32_t entireframework_min_css_len = entireframework_min_css_end - entireframework_min_css_start;

  // test.css
  extern const uint8_t mycss_css_start[] asm("_binary_mycss_css_start");
  extern const uint8_t mycss_css_end[] asm("_binary_mycss_css_end");
  const uint32_t mycss_css_len = mycss_css_end - mycss_css_start;

  // favicon.ico
  extern const uint8_t favicon_ico_start[] asm("_binary_favicon_ico_start");
  extern const uint8_t favicon_ico_end[] asm("_binary_favicon_ico_end");
  const uint32_t favicon_ico_len = favicon_ico_end - favicon_ico_start;

  // error page
  extern const uint8_t error_html_start[] asm("_binary_error_html_start");
  extern const uint8_t error_html_end[] asm("_binary_error_html_end");
  const uint32_t error_html_len = error_html_end - error_html_start;

  netconn_set_recvtimeout(conn,1000); // allow a connection timeout of 1 second
  ESP_LOGI(TAG,"reading from client...");
  err = netconn_recv(conn, &inbuf);
  ESP_LOGI(TAG,"read from client");
  if(err==ERR_OK) {
    netbuf_data(inbuf, (void**)&buf, &buflen);
    if(buf) {

      // default page
      if     (strstr(buf,"GET / ")
          && !strstr(buf,"Upgrade: websocket")) {
        ESP_LOGI(TAG,"Sending /");
        netconn_write(conn, HTML_HEADER, sizeof(HTML_HEADER)-1,NETCONN_NOCOPY);
        netconn_write(conn, index_html_start,index_html_len,NETCONN_NOCOPY);
        netconn_close(conn);
        netconn_delete(conn);
        netbuf_delete(inbuf);
      }

      // default page websocket
      else if(strstr(buf,"GET / ")
           && strstr(buf,"Upgrade: websocket")) {
        ESP_LOGI(TAG,"Requesting websocket on /");
        ws_server_add_client(conn,buf,buflen,"/",websocket_callback);
        netbuf_delete(inbuf);
      }

      else if(strstr(buf,"GET /myjs.js ")) {
        ESP_LOGI(TAG,"Sending /myjs.js");
        netconn_write(conn, JS_HEADER, sizeof(JS_HEADER)-1,NETCONN_NOCOPY);
        netconn_write(conn, myjs_js_start, myjs_js_len,NETCONN_NOCOPY);
        netconn_close(conn);
        netconn_delete(conn);
        netbuf_delete(inbuf);
      }

      else if(strstr(buf,"GET /mycss.css ")) {
        ESP_LOGI(TAG,"Sending /mycss.css");
        netconn_write(conn, CSS_HEADER, sizeof(CSS_HEADER)-1,NETCONN_NOCOPY);
        netconn_write(conn, mycss_css_start, mycss_css_len,NETCONN_NOCOPY);
        netconn_close(conn);
        netconn_delete(conn);
        netbuf_delete(inbuf);
      }

      else if(strstr(buf,"GET /entireframework.min.css ")) {
        ESP_LOGI(TAG,"Sending /entireframework.min.css");
        netconn_write(conn, CSS_HEADER, sizeof(CSS_HEADER)-1,NETCONN_NOCOPY);
        netconn_write(conn, entireframework_min_css_start, entireframework_min_css_len,NETCONN_NOCOPY);
        netconn_close(conn);
        netconn_delete(conn);
        netbuf_delete(inbuf);
      }


      else if(strstr(buf,"GET /favicon.ico ")) {
        ESP_LOGI(TAG,"Sending favicon.ico");
        netconn_write(conn,ICO_HEADER,sizeof(ICO_HEADER)-1,NETCONN_NOCOPY);
        netconn_write(conn,favicon_ico_start,favicon_ico_len,NETCONN_NOCOPY);
        netconn_close(conn);
        netconn_delete(conn);
        netbuf_delete(inbuf);
      }

      else if(strstr(buf,"GET /")) {
        ESP_LOGI(TAG,"Unknown request, sending error page: %s",buf);
        netconn_write(conn, ERROR_HEADER, sizeof(ERROR_HEADER)-1,NETCONN_NOCOPY);
        netconn_write(conn, error_html_start, error_html_len,NETCONN_NOCOPY);
        netconn_close(conn);
        netconn_delete(conn);
        netbuf_delete(inbuf);
      }

      else {
        ESP_LOGI(TAG,"Unknown request");
        netconn_close(conn);
        netconn_delete(conn);
        netbuf_delete(inbuf);
      }
    }
    else {
      ESP_LOGI(TAG,"Unknown request (empty?...)");
      netconn_close(conn);
      netconn_delete(conn);
      netbuf_delete(inbuf);
    }
  }
  else { // if err==ERR_OK
    ESP_LOGI(TAG,"error on read, closing connection");
    netconn_close(conn);
    netconn_delete(conn);
    netbuf_delete(inbuf);
  }
}

// handles clients when they first connect. passes to a queue
static void server_task(void* pvParameters) {
  const static char* TAG = "server_task";
  struct netconn *conn, *newconn;
  static err_t err;
  client_queue = xQueueCreate(client_queue_size,sizeof(struct netconn*));

  conn = netconn_new(NETCONN_TCP);
  netconn_bind(conn,NULL,80);
  netconn_listen(conn);
  ESP_LOGI(TAG,"server listening");
  do {
    err = netconn_accept(conn, &newconn);
    ESP_LOGI(TAG,"new client");
    if(err == ERR_OK) {
      xQueueSendToBack(client_queue,&newconn,portMAX_DELAY);
      //http_serve(newconn);
    }
  } while(err == ERR_OK);
  netconn_close(conn);
  netconn_delete(conn);
  ESP_LOGE(TAG,"task ending, rebooting board");
  esp_restart();
}

// receives clients from queue, handles them
static void server_handle_task(void* pvParameters) {
  const static char* TAG = "server_handle_task";
  struct netconn* conn;
  ESP_LOGI(TAG,"task starting");
  for(;;) {
    xQueueReceive(client_queue,&conn,portMAX_DELAY);
    if(!conn) continue;
    if(scanDone)
      http_serve(conn);
    else
      serve_module(conn);    
  }
  vTaskDelete(NULL);
}

void server_start() {
  ws_server_start();
  xTaskCreate(&server_task,"server_task",3000,NULL,9,NULL);
  xTaskCreate(&server_handle_task,"server_handle_task",4000,NULL,6,NULL);
}