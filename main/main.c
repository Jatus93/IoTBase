/*  WiFi softAP Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "WebServer.c"
//#define CONFIG_TCPIP_LWIP 1

static int status = 0;

void app_main()
{
    wifi_start();
    if(scanDone)
        status = 1;
    server_start();

}
