/* definitions.h by robin prillwitz 2020 */

#ifndef DEFINITIONS_H
#define DEFINITIONS_H
#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <esp_netif.h>
#include <esp_event.h>
#include <esp_sleep.h>
#include <esp_log.h>
#include <esp_spi_flash.h>
#include <nvs_flash.h>
#include <nvs.h>
#include <math.h>
#include <esp_sntp.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>

#include <driver/gpio.h>
#include <driver/i2c.h>
#include <driver/periph_ctrl.h>
#include <driver/timer.h>
#include <driver/ledc.h>
#include <driver/spi_master.h>

#define LEDC_HS_TIMER          LEDC_TIMER_0
#define LEDC_HS_MODE           LEDC_HIGH_SPEED_MODE
#define LEDC_HS_CH0_CHANNEL    LEDC_CHANNEL_0

#define WRITE_BIT I2C_MASTER_WRITE
#define ACK_CHECK_EN 0x01

#define ABS(a) (a < 0 ? -a : a)
#define MAX(a,b) (a > b ? a : b )
#define MIN(a,b) (a < b ? a : b )
#define _BV(bit) (1<<(bit))

extern void vTaskGetRunTimeStats( char *pcWriteBuffer );
esp_err_t write_i2c(uint8_t, uint8_t*, size_t);
void resume_update();

time_t now;
struct tm timeinfo;

uint8_t disp_buffer[16*2];

ledc_channel_config_t ledc_channel;
spi_device_handle_t spi;
TaskHandle_t updateTask;

uint8_t shutdown_timer;
bool display_off;
bool time_set;

#endif
