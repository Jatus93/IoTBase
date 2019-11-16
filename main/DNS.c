#include "WifiProcedure.c"
#include "mdns.h"
#include <sys/socket.h>
#include <netdb.h>

void start_mdns_service(){
  //initialize mDNS service
    esp_err_t err = mdns_init();
    if (err) {
        printf("MDNS Init failed: %d\n", err);
        return;
    }

    //set hostname
    mdns_hostname_set("IoTDev");
    //set default instance
    mdns_instance_name_set("IoTDev 0.1");
}
