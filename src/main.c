#include "../include/blinksequences.h"
#include "../include/common.h"
#include <bits/getopt_core.h>
#include <err.h>
#include <fcntl.h>
#include <limits.h>
#include <linux/input-event-codes.h>
#include <linux/input.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

enum { BINARY, SOLID, MSG, CMD } blink_mode = SOLID;

static constexpr char led_path_default[] = "/sys/class/leds/input0::capslock/brightness";
static constexpr char input_path_default[] = "/dev/input/event0";

uint32_t interblink = 200;
uint32_t intermsg = 6000;
uint32_t longpress_speedup = 2;

/* Our `fd`s are gobal because there is no reasonable way to close them otherwise, as the program
 only exits on error or when receiving a signal.

 Even though signals involve some form of concurency, marking these as atomic just doesn't make
 sense and isn't easily done. */
static int caps_fd = -1;
static int input_fd = -1;

static void print_help(void)
{
    printf(
        "Capslock Morse\n"
        "\tManipulate a system LED to follow an input device or a set message\n"
        "Usage:\n"
        "\t-b : Blink to the binary sequence matching the scancodes emitted by the input device\n"
        "\t-s : Shine the LED as long as the input device is active\n"
        "\t-m \"STRING1|STRING2\" : Like -b, but matching one of the messages separated by pipes\n"
        "\t-c \"CMD\" : Like -b, but matches the output of a command that is called each time\n"
        "\t-M : If paired with -m|-f, use the morse code instead of the binary sequence\n"
        "\t-L : Change the LED device file (must be using the /sys/class/led/*/brightness format)\n"
        "\t-I : Change the polled input device (must be using the /dev/input/event* format)\n"
        "\t-d n : Set delay between individual blinks (default: 200ms)\n"
        "\t-D n : Set delay between message/command calls (default: 6000ms)\n");
}

/* We shouldn't mark this as noreturn because that's not *actually* guaranteed, we just know it to
 * be true.
  N.B. exit() in an atextit() function is undefined. */
static void clear_light_exit(void)
{
    if (caps_fd >= 0)
    {
        write(caps_fd, &bit0, sizeof(bit0));
    }
    // We are allowed to close even if fd == -1
    close(caps_fd);
    close(input_fd);
}

[[noreturn]]
static void clear_light_sig(int _)
{
    (void)_;
    exit(0); // Will call clear_light_exit
}

[[noreturn]]
int main(int argc, char *argv[])
{
    atexit(clear_light_exit);
    signal(SIGTERM, clear_light_sig);
    signal(SIGINT, clear_light_sig);
    signal(SIGKILL, clear_light_sig);

    srand((uint32_t)time(nullptr));

    const char *input_path = input_path_default;
    const char *led_path = led_path_default;

    // String used to print messages if the mode is MSG, messages are separated by a |
    char *msg_string = nullptr;
    size_t msg_size = 0;
    size_t msg_count = 1;
    bool morse = false;

    int opt = '\0';
    while ((opt = getopt(argc, argv, "hL:I:MAbsm:c:d:D:")) != -1)
    {
        switch (opt)
        {
        case 'h':
            print_help();
            exit(EXIT_SUCCESS);
        case 'L':
            led_path = optarg;
            break;
        case 'I':
            input_path = optarg;
            break;
        case 'M':
            morse = true;
            break;
        case 'b':
            blink_mode = BINARY;
            break;
        case 's':
            blink_mode = SOLID;
            break;
        case 'm':
            blink_mode = MSG;
            msg_string = optarg;
            break;
        case 'c':
            blink_mode = CMD;
            msg_string = optarg;
            break;
        case 'd':
            interblink = (uint32_t)strtoull(optarg, nullptr, 10);
            break;
        case 'D':
            intermsg = (uint32_t)strtoull(optarg, nullptr, 10);
            break;
        case ':':
        default:
            exit(EXIT_FAILURE);
            break;
        }
    }

    // If we are in message mode, we turn the single string into a contiguous list of strings.
    // we then note said string's length (since we have no other way of knowing we're at the end) to
    // pass it all to the blink function later.
    if (blink_mode == MSG && msg_string != nullptr)
    {
        msg_size = strlen(msg_string) + 1;
        for (size_t i = 0; i < msg_size; ++i)
        {
            if (msg_string[i] == '|')
            {
                msg_string[i] = '\0';
                msg_count += 1;
            }
        }
    }

    // Will be closed in the exit function.
    caps_fd = open(led_path, O_WRONLY);
    input_fd = open(input_path, O_RDONLY);
    if (caps_fd < 0 || input_fd < 0)
    {
        errx(EXIT_FAILURE, "Failed to open files: caps = %d & input = %d", caps_fd, input_fd);
    }

    while (true)
    {
        struct input_event ev;
        if (read(input_fd, &ev, sizeof(struct input_event)) != sizeof(struct input_event))
        {
            // This program is meant to be ran as a daemon and its resource usage is near-nil,
            // so this is the least annoying option in case input exceptionally cannot be read
            // at any point.
            fprintf(stderr, "Failed to read input event\n");
            usleep(1000 * 1000);
        }
        if (blink_mode != MSG && blink_mode != CMD && ev.type != EV_KEY)
        {
            continue;
        }

        switch (blink_mode)
        {
        case BINARY:
            binary_blink((uint8_t)ev.code, ev.value, caps_fd);
            break;
        case SOLID:
            solid_blink((uint8_t)ev.value, caps_fd);
            break;
        case MSG:
            msg_blink(msg_string, msg_size, msg_count, morse, caps_fd);
            break;
        case CMD:
            FILE *f = popen(msg_string, "r");
            char *line = NULL;
            size_t size = 0;
            while (getline(&line, &size, f) != -1)
            {
                msg_blink(line, size, 1, morse, caps_fd);
            }
            free(line);
            pclose(f);
            break;
        default:
            break;
        }
    }
}
