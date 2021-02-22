/* main.c by robin prillwitz 2020 */

#include "definitions.h"
#include "font.h"
#include "wifi.h"
#include "led_matrix.h"
#include "ntp.h"

/* --------------------------------- Headers -------------------------------- */

esp_err_t initilize_i2c();
esp_err_t write_i2c(uint8_t, uint8_t *, size_t);

void update_state_task(void*);
void update_supervisor_task(void*);

nvs_handle_t nvs_user_open();
void nvs_read(void*);
void nvs_write(void*);

void on_wifi_ready();
void on_led_ready();

void app_main(void);

// sets brightness of the display as a function of the current time
void adjust_brightness()    {
    // d\left(x\right)=0.5\cos\left(\frac{x\cdot\pi}{12}\right)+0.5
    // 1-\left(1-d\left(x\right)\right)^{2}
    double time = (timeinfo.tm_hour + ((timeinfo.tm_min * 1.6666) / 100.0));
    double percentage = 1 - pow(1 - ( 0.5 * cos( ( time * M_PI) / 12.0 ) + 0.5 ), 3.0) ;
    uint32_t value = (uint32_t)round( (LED_OFF - LED_ON) * percentage + LED_ON );

    ESP_LOGI(TAG_l, "Fading to %lf -> %u", percentage, value );
    fade_l( value );
}

/* ----------------------------------- I2C ---------------------------------- */

esp_err_t initilize_i2c()   {
    i2c_config_t config = {
        .mode = I2C_MODE_MASTER,
        .master.clk_speed = CONFIG_CLK_SPEED,
        .sda_io_num = CONFIG_SDA,
        .scl_io_num = CONFIG_SCL,
        .sda_pullup_en = false,
        .scl_pullup_en = false
    };

    ESP_LOGI("I2C", "Initilizing Interface");

    ESP_ERROR_CHECK(i2c_param_config(I2C_NUM_0, &config));
    return i2c_driver_install(I2C_NUM_0, config.mode, 0, 0, 0); // last arguments for slave only
}

esp_err_t write_i2c(uint8_t addr, uint8_t *data_wr, size_t size)    {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write(cmd, data_wr, size, ACK_CHECK_EN);
    i2c_master_stop(cmd);

	esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, 500 / portTICK_RATE_MS);

    i2c_cmd_link_delete(cmd);
    return ret;
}

/* ------------------------------ Update Loops ------------------------------ */

void update_state_task(void*_args)  {
    uint32_t delay = 0;
    uint16_t counter = 0;
    struct timeval tv;
    while(1)    {
        // xLastWakeTime = xTaskGetTickCount();
        // ESP_LOGI("", "1");

        clear_l(disp_buffer);

        time(&now);
        localtime_r(&now, &timeinfo);
        gettimeofday(&tv, NULL);

        // if(false)   {
        if(time_set)    {
            // correct for daylight savings and cap hours between 0 to 23
            uint8_t hour = (timeinfo.tm_hour + (timeinfo.tm_isdst ? 1 : 0)) % 24;

            // draw time digit by dighit
            draw_char_l(disp_buffer, 1, 0, hour / 10);
            draw_char_l(disp_buffer, 7, 0, hour % 10);
            draw_char_l(disp_buffer, 1, 9,  timeinfo.tm_min / 10 );
            draw_char_l(disp_buffer, 7, 9, timeinfo.tm_min % 10 );
            // draw_vline_l(disp_buffer, 0, 15, 13);
            //draw_vline_l(disp_buffer, 15 - (timeinfo.tm_sec / 4), 15, 15);

            // draw binary representation of hours minutes and seconds
            // also acts as a visual heartbeat
            for(uint8_t i = 0; i < 7; i++)  {
                if(hour & (1 << i))  {
                    draw_point_l(disp_buffer, 13, 15-i);
                }
                if(timeinfo.tm_min & (1 << i))  {
                    draw_point_l(disp_buffer, 14, 15-i);
                }
                if(timeinfo.tm_sec & (1 << i))  {
                    draw_point_l(disp_buffer, 15, 15-i);
                }
            }

            display_l(disp_buffer);
            // correct delay for execution time
            // so we are updating nearly in sync with time changes
            // completely independent of this routine
            delay = (uint32_t)(1000 - tv.tv_usec / 1000.0) + 10;
        }   else    {
            // draw a fany waiting spinner animation
            uint8_t x = (uint8_t)roundf(7.5 + 7.5 * sinf(-counter / 10.0));
            uint8_t y = (uint8_t)roundf(7.5 + 7.5 * cosf(-counter / 10.0));

            draw_line_l(disp_buffer, 7, 7, x, y);
            draw_line_l(disp_buffer, 7, 8, x, y);
            draw_line_l(disp_buffer, 8, 7, x, y);
            draw_line_l(disp_buffer, 8, 8, x, y);

            // draw_rect_l(disp_buffer, 0, 0, 15, 15);

            display_l(disp_buffer);

            counter++;
            delay = 20;

        }


        // ESP_LOGI("", "%u", delay );
        vTaskDelay( delay / portTICK_PERIOD_MS);
    }
}

void update_supervisor_task(void*_args) {
    char strftime_buf[64];
    while(1)    {
        if(time_set)    {
            time(&now);
            localtime_r(&now, &timeinfo);
            strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
            ESP_LOGI(TAG_t, "%s", strftime_buf);

            adjust_brightness();

            vTaskDelay(30000 / portTICK_PERIOD_MS);
        }

        vTaskDelay(15000 / portTICK_PERIOD_MS);

        // obtain_time();
    }
}

/* ----------------------------------- NVS ---------------------------------- */
/*
nvs_handle_t nvs_user_open()    {
    ESP_LOGD("NVS", "Opening NVS");

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &nvs_handle);

    if (err != ESP_OK)  {
        ESP_LOGE("NVS", "Error with opening NVS handle: %s", esp_err_to_name(err));
        return (nvs_handle_t)NULL;
    }   else    {
        return nvs_handle;
    }
}

void nvs_read(void*_args)   {
    uint8_t value = 0;
    nvs_handle_t nvs_handle = nvs_user_open();
    if(nvs_handle != (nvs_handle_t)NULL)  {
        ESP_LOGD("NVS", "Reading from NVS");

        esp_err_t err = nvs_get_u8(nvs_handle, "target", &value);

        switch (err) {
            case ESP_OK:
                ESP_LOGD("NVS", "Sucessfully loaded target Value");
                break;
            case ESP_ERR_NVS_NOT_FOUND:
                ESP_LOGE("NVS", "Value is not initilized");
                value = 0;
                break;
            default :
                ESP_LOGE("NVS", "Errror reading: %s",  esp_err_to_name(err));
        }
        nvs_close(nvs_handle);
        target = value;
    }

    vTaskDelete(NULL);
}

void nvs_write(void*_args)    {
    esp_err_t err;
    nvs_handle_t nvs_handle = nvs_user_open();
    if(nvs_handle != (nvs_handle_t)NULL)  {

        ESP_LOGD("NVS", "Writing to NVS");
        err = nvs_set_u8(nvs_handle, "target", target);
        if(err != ESP_OK)   {
            ESP_LOGE("NVS", "Failed writing to NVS: %s", esp_err_to_name(err));
        }   else    {
            ESP_LOGI("NVS", "Done writing to NVS");
        }

        ESP_LOGD("NVS", "Commiting changes");
        err = nvs_commit(nvs_handle);
        if(err != ESP_OK)   {
            ESP_LOGE("NVS", "Failed commiting to NVS: %s", esp_err_to_name(err));
        }   else    {
            ESP_LOGI("NVS", "Done commiting to NVS");
        }
        nvs_close(nvs_handle);
    }

    vTaskDelete(NULL);
}
*/
/* -------------------------------- Callbacks ------------------------------- */

void on_wifi_ready()    {
    ESP_LOGI(TAG_w, "Connection Online");

    initialize_sntp();

    time(&now);
    localtime_r(&now, &timeinfo);
    if (timeinfo.tm_year < (2016 - 1900)) {
        ESP_LOGI(TAG_t, "Time is not set yet. Connecting to WiFi and getting time over NTP.");
        obtain_time();
    }
    time(&now);
    adjust_brightness();

    time_set = true;
}

void on_led_ready() {
    ESP_LOGI(TAG_l, "LED Connection Ready, starting update tasks");

    xTaskCreate(update_state_task, "Update", 4096, NULL, 3 , &updateTask);
    xTaskCreate(update_supervisor_task, "Update_Shutdown", 4096, NULL, 2 , NULL);

    ledc_set_fade_with_time(ledc_channel.speed_mode,
        ledc_channel.channel, (uint32_t)(LED_ON), LED_FADE * 2);
    ledc_fade_start(ledc_channel.speed_mode,
        ledc_channel.channel, LEDC_FADE_NO_WAIT);
}

/* ---------------------------------- Main ---------------------------------- */

void app_main(void) {

    shutdown_timer = 0;
    display_off = false;
    time_set = false;

    for(uint8_t i = 0; i < 16*2; i++)    {
        disp_buffer[i] = 0xFF;
    }

    void (*callback)() = &on_led_ready;
    initilize_l(callback);

    setenv("TZ", "UTC-1", 1);
    tzset();

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    wifi_init();
    ESP_ERROR_CHECK(initilize_i2c());

}
