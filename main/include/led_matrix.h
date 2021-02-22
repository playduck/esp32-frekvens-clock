/* led_matrix.h by robin prillwitz 2020 */

#ifndef LED_MATRIX_H
#define LED_MATRIX_H
#pragma once

#include "definitions.h"

#ifndef _swap_int8_t
#define _swap_int8_t(a, b)  { \
    a = a ^ b; \
    b = b ^ a; \
    a = a ^ b; \
}
#endif

// #define LED_OFF (8191 - 2) /* without color film */
#define LED_OFF (8191 - 20)
#define LED_ON  (8191 - 6000)
// #define LED_ON  (8191 - 8191)
#define LED_FADE 2500

#define TAG_l "SPI_LED"
#define BUFFER_LEN (16 * 2 * sizeof(uint8_t))

#define PIN_NUM_MISO -1
#define PIN_NUM_MOSI 19
#define PIN_NUM_CLK 18
#define PIN_NUM_LAT 5
#define PIN_NUM_OE  17

extern const uint8_t SYMBOLS[];

void display_l(uint8_t[]);
void spi_pre_transfer_callback();
void spi_post_transfer_callback();
void initilize_spi();
void initilize_ledc();

void initilize_l( void (*cb)() );
void fade_in_l();
void fade_out_l();
void fade_l(uint32_t);
void invert_l(uint8_t[]);
void draw_point_l(uint8_t[], uint8_t, uint8_t);

void draw_hline_l(uint8_t[], uint8_t, uint8_t, uint8_t);
void draw_vline_l(uint8_t[], uint8_t, uint8_t, uint8_t);
void draw_line_l(uint8_t[], uint8_t, uint8_t, uint8_t, uint8_t);

void draw_rect_l(uint8_t[], uint8_t, uint8_t, uint8_t, uint8_t);

void draw_char_l(uint8_t[], uint8_t, uint8_t, uint8_t);

void clear_l(uint8_t[]);
void clear_left_l(uint8_t[]);
void clear_right_l(uint8_t[]);

#endif
