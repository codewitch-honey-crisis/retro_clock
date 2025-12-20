#include "ntp.h"
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "esp_netif_sntp.h"
#include "lwip/ip_addr.h"
#include "esp_sntp.h"

static ntp_on_sync_t ntp_callback = NULL;
static void* ntp_callback_state = NULL;
static void ntp_cb(struct timeval *tv) {
    // set the sync to one hour
    if(tv!=NULL && tv->tv_sec>0) {
        time_t result = (time_t)tv->tv_sec;
        if(ntp_callback!=NULL) {
            ntp_callback(result,ntp_callback_state);
        }
        
        esp_sntp_set_sync_interval((60*60)*1000);
    }
}
void ntp_init(void) {
    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
    config.sync_cb=ntp_cb;
    ESP_ERROR_CHECK(esp_netif_sntp_init(&config));
    // temporarily set it for every 10 seconds
    esp_sntp_set_sync_interval(10*1000);
}
void ntp_on_sync_callback(ntp_on_sync_t callback, void* state) {
    ntp_callback = callback;
    ntp_callback_state = state;
}
bool ntp_syncing(void) {
    return SNTP_SYNC_STATUS_COMPLETED!=sntp_get_sync_status();
}
bool ntp_sync(void) {
    return esp_sntp_restart();
}