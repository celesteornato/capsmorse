#include <err.h>
#include <fcntl.h>
#include <linux/input.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

static constexpr char caps_path_default[] = "/sys/class/leds/input0::capslock/brightness";
static constexpr char input_path_default[] = "/dev/input/event0";

static constexpr uint32_t wait_milli = 40;
static constexpr uint32_t longpress_speedup = 2;

static constexpr char bit0 = '0';

/* Our `fd`s are gobal because there is no reasonable way to close them otherwise, as the program
 only exits on error or when receiving a signal.

 Even though signals involve some form of concurency, marking these as atomic just doesn't make
 sense and isn't easily done. */
static int caps_fd = -1;
static int input_fd = -1;

/* Main attraction, blinks with the given bit sequence*/
static void lightup_sequence(uint8_t bits, uint32_t speed)
{
    uint32_t usleep_amount = (wait_milli * 1000) / speed;

    // Iterate through each bit (Notice the size-safety!)
    uint8_t divisor = 1 << ((sizeof(bits) * 8) - 1);
    while (divisor > 0)
    {
        char bit = ((bits / divisor) % 2) + '0';
        divisor >>= 1;

        write(caps_fd, &bit, sizeof(bit));
        usleep(usleep_amount);
        write(caps_fd, &bit0, sizeof(bit0));
    }
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

    // User can provide a alternative paths instead of event0 and capslock::brightness
    const char *input_path = (argc < 2) ? input_path_default : argv[1];
    const char *caps_path = (argc < 3) ? caps_path_default : argv[2];

    // Will be closed in the exit function.
    caps_fd = open(caps_path, O_WRONLY);
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
            // This program is meant to be ran as a daemon and its resource usage is near-nil, so
            // this is the least annoying option in case input exceptionally cannot be read at any
            // point.
            fprintf(stderr, "Failed to read input event\n");
            usleep(1000 * 1000);
        }

        // Value = 0 <=> key is up
        // N.B. EV_KEY also includes some mouse/touchpad motions, so we can still pretty much plug
        // in any input device !
        if (ev.type != EV_KEY || ev.value == 0)
        {
            continue;
        }

        // Value > 1 means that the key is in a long-held state, in which case we apply the speedup.
        lightup_sequence((uint8_t)ev.code, ev.value == 1 ? 1 : longpress_speedup);
    }
}
