// Fuzzes clawd.c with random host events, context, typing and time jumps,
// flagging any frame that takes over 20 ms (the firmware hung in render).
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "../firmware/claude/clawd.c"

static double ms(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec * 1e3 + t.tv_nsec / 1e6; }

int main(int argc, char **argv) {
    unsigned seed = argc > 1 ? atoi(argv[1]) : 1;
    srand(seed);
    uint8_t fb[CLAWD_FB_SIZE];
    clawd_frame_t fr;
    uint32_t now = 1, last_key = 0;
    clawd_init(now);
    for (long frame = 0; frame < 2000000; frame++) {
        now += (rand() % 1000 == 0) ? rand() % 600000 : 33;  // occasional big gap
        if (rand() % 40 == 0) clawd_event(now, rand() % 12, rand() % 5, rand() % 3, rand() % 2);
        if (rand() % 300 == 0) clawd_set_context(rand() % 101);
        uint8_t wpm = 0;
        if (rand() % 4 == 0) { last_key = now; wpm = rand() % 120; }
        clawd_input_t in = {now, now - last_key, wpm};
        double t0 = ms();
        clawd_render(&in, fb, &fr);
        double dt = ms() - t0;
        if (dt > 20) {
            printf("seed %u frame %ld: render took %.0f ms | anim=%d x=%f facing=%f target=%f\n",
                   seed, frame, dt, pl.cur, d.x, d.facing, d.target);
            return 1;
        }
    }
    printf("seed %u: 2M frames clean\n", seed);
    return 0;
}
