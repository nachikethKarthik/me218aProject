/****************************************************************************
  NeoPixel (WS2812) Driver for PIC32
  Author: Ege Turan
****************************************************************************/

#ifndef NEOPIXEL_H
#define NEOPIXEL_H

#include <stdint.h>
#include <stdbool.h>

// Public Interface
// Caution: blocking code
// Note: Hard-coded according to oscilloscope measurements. Tested and verified.
void neopixel_init(void);
void neopixel_show(void);
void neopixel_clear(void);
void neopixel_set_pixel(int i, uint8_t r, uint8_t g, uint8_t b);

#endif /* NEOPIXEL_H */
