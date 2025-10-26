#pragma once
#include <stddef.h>
#include <stdint.h>

/* Blinks/Turns off at an interval of `interblink` (common.h) depending on the bits of `bits`. Value
 * is based arount linux input events: 0 does nothing, 1 blinks, and 2 blinks with an
 * `interblink/longpress_speedup` interval. Value can also be 3, in which case a 0-bit is a short
 * blink and a 1-bit a long blink */
void binary_blink(uint8_t bits, int value, int led_fd);

/* Turns the light on if value = 1 and off if value = 0 */
void solid_blink(uint8_t value, int led_fd);

/* Blinks with the binary/morse sequence of a string of size <= len, from an array `msg` of size
 * `nbmsg` characters separated by null-terminators*/
void msg_blink(const char msg[restrict 1], size_t len, size_t nbmsg, bool morse, int led_fd);
