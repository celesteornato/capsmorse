#include "../include/blinksequences.h"
#include "../include/common.h"
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

constexpr uint8_t atomorse[] = {
    0b101,    0b11000,  0b1010,   0b1100,   0b10,     0b10010,  0b1110,   0b10000,  0b100,
    0b10111,  0b1101,   0b10100,  0b111,    0b110,    0b1111,   0b10110,  0b11101,  0b1010,
    0b1000,   0b11,     0b1001,   0b10001,  0b1011,   0b11001,  0b11011,  0b11100,  0b101111,
    0b100111, 0b100011, 0b100001, 0b100000, 0b110000, 0b111000, 0b111100, 0b111110, 0b111111,
};

void binary_blink(uint8_t bits, int32_t value, int caps_fd)
{
    if (value == 0)
    {
        return;
    }
    uint32_t speed = (value == 2) ? longpress_speedup : 1;
    uint32_t usleep_inter = (interblink * 1000) / speed;

    // Iterate through each bit (Notice the size-safety!)
    uint8_t divisor = 1 << ((sizeof(bits) * CHAR_BIT) - 1);
    bool found_msb = false;
    while (divisor > 0)
    {
        char bit = ((bits / divisor) % 2) + '0';
        divisor >>= 1;

        // We skip every leading 0 in the case we aren't in value=3 mode
        // Value=3 special case: we skip leading 0s AND the msb
        if (!found_msb && (bit == '0' || (value == 3 && bit == '1')))
        {
            found_msb = bit == '1';
            continue;
        }
        found_msb = true;

        if (value == 3)
        {
            write(caps_fd, &bit1, sizeof(bit1));
            usleep(usleep_inter * (bit == '1' ? 3 : 1));
            write(caps_fd, &bit0, sizeof(bit0));
            usleep(usleep_inter);
            continue;
        }
        write(caps_fd, &bit, sizeof(bit));
        usleep(usleep_inter);
        write(caps_fd, &bit0, sizeof(bit0));
        usleep(usleep_inter);
    }
}

void solid_blink(uint8_t value, int caps_fd)
{
    if (value == 0)
    {
        write(caps_fd, &bit0, sizeof(bit0));
        return;
    }
    write(caps_fd, &bit1, sizeof(bit1));
}

void msg_blink(const char msg[restrict 1], size_t len, size_t nbmsg, bool morse, int caps_fd)
{
    (void)morse;
    size_t msg_idx = (size_t)rand() % nbmsg;

    // We stop and pick to_print at the msg_idx'th message
    const char *to_print = &msg[0];
    for (size_t i = 0; i < len && msg_idx > 0; ++i)
    {
        if (msg[i] == '\0')
        {
            msg_idx -= 1;
        }
        if (msg_idx == 0)
        {
            to_print = &msg[i + 1];
        }
    }

    for (size_t i = 0; to_print[i] != '\0'; ++i)
    {
        if (morse)
        {
            char tmp = to_print[i];
            if (tmp == ' ')
            {
                usleep(interblink * 5 * 1000);
                continue;
            }
            uint8_t c = (tmp >= 'a' && tmp <= 'z')   ? atomorse[tmp - 'a']
                        : (tmp >= 'A' && tmp <= 'Z') ? atomorse[tmp - 'A']
                        : (tmp >= '0' && tmp <= '9') ? atomorse[tmp - '0' + ('z' - 'a' + 1)]
                                                     : '\0';

            // Now, we just print the binary representation of the character
            binary_blink(c, 3, caps_fd);
        }
        else
        {
            binary_blink((uint8_t)to_print[i], 1, caps_fd);
        }
        usleep(interblink * 3 * 1000);
    }
    usleep(intermsg * 1000);
}
