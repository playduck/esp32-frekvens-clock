/* wifi.h by robin prillwitz 2020 */

#ifndef WIFI_H
#define WIFI_H
#pragma once

#include "definitions.h"

#define TAG_w "WIFI"

void on_wifi_ready();
void event_handler(void*, esp_event_base_t, int32_t, void*);
void wifi_init();

#endif
