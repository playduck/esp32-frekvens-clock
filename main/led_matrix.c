#include "led_matrix.h"

void display_l(uint8_t data[]) {
    esp_err_t ret;
    spi_transaction_t t;

    memset(&t, 0, sizeof(t));

    t.length = (BUFFER_LEN)*8;
    t.tx_buffer = data;
    t.user = (void*)1;
    ret=spi_device_polling_transmit(spi, &t);
    assert(ret==ESP_OK);
}

void spi_pre_transfer_callback()    {
}

void spi_post_transfer_callback()   {
    gpio_set_level(PIN_NUM_LAT, 1);
    gpio_set_level(PIN_NUM_LAT, 0);
}

void initilize_spi() {
	ESP_LOGD(TAG_l, "initilize_spi_task");

    spi_bus_config_t buscfg={
        .miso_io_num = PIN_NUM_MISO,
        .mosi_io_num = PIN_NUM_MOSI,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 16 * 2 * sizeof(uint8_t) + 1,
    };

    spi_device_interface_config_t devcfg={
        #ifdef OVERCLOCK
            .clock_speed_hz = 26*1000*1000,           //Clock out at 26 MHz
        #else
            // .clock_speed_hz = 10*1000*1000,           //Clock out at 10 MHz
        #endif
        .clock_speed_hz=1000*1000,
        .command_bits=0,
        .address_bits=0,
        .dummy_bits=0,
        .duty_cycle_pos=128,        //50% duty cycle
        .mode = 0,
        .spics_io_num = -1,
        .queue_size = 1,
        .pre_cb=spi_pre_transfer_callback,
        .post_cb=spi_post_transfer_callback,
    };

	ESP_LOGI(TAG_l, "Initializing bus.");
	ESP_ERROR_CHECK(spi_bus_initialize(HSPI_HOST, &buscfg, 2));

	ESP_LOGI(TAG_l, "Adding device bus.");
	ESP_ERROR_CHECK(spi_bus_add_device(HSPI_HOST, &devcfg, &spi));

    ESP_LOGI(TAG_l, "SPI Initilized");
}

void initilize_ledc()   {
    ledc_timer_config_t ledc_timer = {
        .duty_resolution = LEDC_TIMER_13_BIT, // resolution of PWM duty
        .freq_hz = 5000,                      // frequency of PWM signal
        .speed_mode = LEDC_HS_MODE,
        .timer_num = LEDC_HS_TIMER,
        .clk_cfg = LEDC_AUTO_CLK,              // Auto select the source clock
    };
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel={
            .channel    = LEDC_HS_CH0_CHANNEL,
            .duty       = 8191,
            .gpio_num   = PIN_NUM_OE,
            .speed_mode = LEDC_HS_MODE,
            .hpoint     = 0,
            .timer_sel  = LEDC_HS_TIMER
    };

    ledc_channel_config(&ledc_channel);
    ledc_fade_func_install(0);

    // vTaskDelay(100 / portTICK_PERIOD_MS);
}

void initilize_l( void (*cb)() )  {
    ESP_LOGI(TAG_l, "Initilizing LEDs");
    ESP_ERROR_CHECK(gpio_set_direction(PIN_NUM_MOSI, GPIO_MODE_OUTPUT));
    ESP_ERROR_CHECK(gpio_set_direction(PIN_NUM_CLK, GPIO_MODE_OUTPUT));
    ESP_ERROR_CHECK(gpio_set_direction(PIN_NUM_LAT, GPIO_MODE_OUTPUT));

    ESP_LOGI(TAG_l, "Starting SPI driver");
    initilize_spi();

    ESP_LOGI(TAG_l, "Starting LEDC driver");
    initilize_ledc();
    (*cb)();
}

void fade_in_l()    {
    ledc_set_fade_with_time(ledc_channel.speed_mode,
            ledc_channel.channel, LED_ON, LED_FADE);
    ledc_fade_start(ledc_channel.speed_mode,
            ledc_channel.channel, LEDC_FADE_NO_WAIT);
}

void fade_out_l()   {
    ledc_set_fade_with_time(ledc_channel.speed_mode,
            ledc_channel.channel, LED_OFF, LED_FADE);
    ledc_fade_start(ledc_channel.speed_mode,
            ledc_channel.channel, LEDC_FADE_NO_WAIT);
}

// -\left(c-b\right)e^{-\frac{x}{30}}+c
// \frac{c-b}{100}x+b
// \frac{ce^{\frac{a}{11.35}}}{c}+b
void fade_l(uint32_t fade)  {
    ledc_set_fade_with_time(ledc_channel.speed_mode,
            ledc_channel.channel, fade, LED_FADE );
    ledc_fade_start(ledc_channel.speed_mode,
            ledc_channel.channel, LEDC_FADE_NO_WAIT);
}

void invert_l(uint8_t data[])   {
    for(uint8_t i = 0; i < 16*2; i++) {
        data[i] = ~data[i];
    }
}

void draw_point_l(uint8_t data[], uint8_t x, uint8_t y)  {
    uint8_t xidx = 31 - x - 16;
    uint8_t yidx = y;
    if(y > 7)  {   // point is in upper half
        xidx += 16;
        yidx -= 8;
    }
    data[xidx] |= 128 >> yidx;
}

void draw_hline_l(uint8_t data[], uint8_t x0, uint8_t x1, uint8_t y)    {
    for(uint8_t x = x0; x <= x1; x++)   {
        draw_point_l(data, x, y);
    }
}

void draw_vline_l(uint8_t data[], uint8_t y0, uint8_t y1, uint8_t x)    {
    for(uint8_t y = y0; y <= y1; y++)   {
        draw_point_l(data, x, y);
    }
}

void draw_line_l(uint8_t data[], uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)  {
    int8_t steep = abs(y1 - y0) > abs(x1 - x0);
    if (steep) {
        _swap_int8_t(x0, y0);
        _swap_int8_t(x1, y1);
    }

    if (x0 > x1) {
        _swap_int8_t(x0, x1);
        _swap_int8_t(y0, y1);
    }

    int8_t dx, dy;
    dx = x1 - x0;
    dy = abs(y1 - y0);

    int8_t err = dx / 2;
    int8_t ystep;

    if (y0 < y1) {
        ystep = 1;
    } else {
        ystep = -1;
    }

    for (; x0 <= x1; x0++) {
        if (steep) {
            draw_point_l(data, y0, x0);
        } else {
            draw_point_l(data, x0, y0);
        }
        err -= dy;
        if (err < 0) {
            y0 += ystep;
            err += dx;
        }
    }
}

void draw_rect_l(uint8_t data[], uint8_t x, uint8_t y, uint8_t w, uint8_t h)    {
    for(; w-x >= 0; x++) {
        draw_vline_l(data, y, h-y, x);
    }
}

void draw_char_l(uint8_t data[], uint8_t x, uint8_t y, uint8_t num)  {
    uint8_t offset = 5 * num;

    for(uint8_t i = 0; i < 5; i++) {
        for(uint8_t j = 0; j < 8; j++) {
            if( SYMBOLS[offset + i] & (128 >> j) )    {
                draw_point_l(data, x + i, y + j - 1);
            }
        }
    }
}

void clear_l(uint8_t data[])    {
    for(uint8_t i = 0; i < 16*2; i++)   {
        data[i] = 0x00;
    }
}

void clear_left_l(uint8_t data[])    {
    for(uint8_t i = 2; i < 16*2; i++)   {
        if(!(i == 16 || i == 17))  {
            data[i] = 0x00;
        }
    }
}

void clear_right_l(uint8_t data[]) {
    data[0 ] = 0x00;
    data[1 ] = 0x00;
    data[16] = 0x00;
    data[17] = 0x00;
}
