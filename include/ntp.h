#ifndef SNTP_H
#define SNTP_H
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef void(*ntp_on_sync_t)(time_t time, void* state);

void ntp_init(void);
void ntp_on_sync_callback(ntp_on_sync_t callback, void* state);
bool ntp_syncing(void);
#ifdef __cplusplus
}
#endif
#endif // SNTP_H