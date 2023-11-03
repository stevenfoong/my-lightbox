#ifndef WIFI_MOD_H
#define WIFI_MOD_H

#include <stdbool.h>

//extern char *TAG;
extern bool isWifiConnected;
extern bool isNetworkConnected;

extern char *TAG;

void wifi_init_sta(void);
void wifi_init_softap(void);

#endif /* WIFI_MOD_H */