/****************************************************************************
 Module
   WS2812.c

 Description
   Bit-banged WS2812 (NeoPixel) driver for PIC32 (cycle-accurate)

 Author
   Ege Turan
****************************************************************************/

#include "pic32Neopixel.h"
#include <xc.h>

#define MAX_LEDS 123
#define WS_PIN  LATBbits.LATB5    // RPB5 output latch
#define WS_TRIS TRISBbits.TRISB5  // RPB5 TRIS

static uint8_t leds_buffer[MAX_LEDS * 3]; // LED data (G,R,B)

/****************************************************************************
// Initialization
****************************************************************************/
void neopixel_init(void)
{
    WS_TRIS = 0;  // Output
    WS_PIN = 0;
}

/****************************************************************************
// Send single WS2812 bit using tight NOP loops
// Timing for 40MHz PBCLK (1 tick = 25ns)
// '0': T_H ~ 0.35us, T_L ~ 0.8us
// '1': T_H ~ 0.7us,  T_L ~ 0.6us
// Caution: blocking code
// Note: Hard-coded according to oscilloscope measurements. Tested and verified.
****************************************************************************/
static inline void send_bit(uint8_t bit)
{
    if (bit)
    {
        // '1' bit
        WS_PIN = 1;
        __asm__ volatile (
            "nop\n nop\n nop\n nop\n nop\n nop\n nop\n nop\n"
            "nop\n nop\n nop\n nop\n nop\n nop\n nop\n nop\n"
            "nop\n nop\n nop\n nop\n nop\n nop\n nop\n nop\n"
            "nop\n nop\n nop\n nop\n"
        ); // ~700ns high
        WS_PIN = 0;
        __asm__ volatile (
            "nop\n nop\n nop\n nop\n nop\n nop\n nop\n nop\n"
            "nop\n nop\n nop\n nop\n nop\n nop\n nop\n nop\n"
            "nop\n nop\n nop\n nop\n nop\n nop\n"
        ); // ~600ns low
    }
    else
    {
        // '0' bit
        WS_PIN = 1;
        __asm__ volatile (
            "nop\n nop\n nop\n nop\n nop\n nop\n nop\n"
            "nop\n nop\n nop\n nop\n nop\n nop\n"
        ); // ~350ns high
        WS_PIN = 0;
        __asm__ volatile (
            "nop\n nop\n nop\n nop\n nop\n nop\n nop\n"
            "nop\n nop\n nop\n nop\n nop\n nop\n nop\n nop\n"
            "nop\n nop\n nop\n nop\n nop\n nop\n nop\n nop\n"
            "nop\n nop\n nop\n nop\n nop\n nop\n nop\n nop\n"
        ); // ~800ns low
    }
}

/****************************************************************************
// Send one color byte (MSB first)
****************************************************************************/
static void send_color_byte(uint8_t color)
{
    for (int i = 7; i >= 0; i--)
    {
        send_bit((color >> i) & 0x1);
    }
}

/****************************************************************************
// Update LEDs
****************************************************************************/
void neopixel_show(void)
{
    for (int i = 0; i < MAX_LEDS; i++)
    {
        send_color_byte(leds_buffer[i*3 + 0]); // G
        send_color_byte(leds_buffer[i*3 + 1]); // R
        send_color_byte(leds_buffer[i*3 + 2]); // B
    }

    // Reset >50us
    WS_PIN = 0;
    for (volatile int i = 0; i < 2000; i++) __asm__ volatile ("nop");
}

/****************************************************************************
// Public: Set pixel color
****************************************************************************/
void neopixel_set_pixel(int i, uint8_t r, uint8_t g, uint8_t b)
{
    if (i < 0 || i >= MAX_LEDS) return;
    leds_buffer[i*3 + 0] = g;
    leds_buffer[i*3 + 1] = r;
    leds_buffer[i*3 + 2] = b;
}

/****************************************************************************
// Public: Clear all LEDs
****************************************************************************/
void neopixel_clear(void)
{
    for (int i = 0; i < MAX_LEDS*3; i++) leds_buffer[i] = 0;
}
