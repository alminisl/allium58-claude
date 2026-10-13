// Desktop simulator: runs the real clawd.c through a scripted timeline and
// writes raw 512-byte frames (plus 4x22 chars of text) to stdout.
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../firmware/claude/clawd.c"

typedef struct { uint32_t at; uint8_t ev, tool, working, waiting; } ev_t;
typedef struct { uint32_t from, to; uint8_t wpm; } typing_t;

int main(int argc, char **argv) {
    // Scenario: idle -> user types prompt -> Claude works through tools ->
    // permission -> done -> user walks away and the mascot gets bored.
    const ev_t evs[] = {
        {4000, CLAWD_EV_SESSION_START, 0, 0, 0},
        {9000, CLAWD_EV_PROMPT, 0, 1, 0},
        {11500, CLAWD_EV_TOOL, CLAWD_TOOL_READ, 1, 0},
        {14500, CLAWD_EV_TOOL, CLAWD_TOOL_EDIT, 1, 0},
        {17500, CLAWD_EV_TOOL, CLAWD_TOOL_BASH, 1, 0},
        {20000, CLAWD_EV_TOOL_FAILED, 0, 1, 0},
        {22500, CLAWD_EV_PERMISSION, 0, 1, 1},
        {26000, CLAWD_EV_TOOL, CLAWD_TOOL_WEB, 1, 0},
        {29000, CLAWD_EV_STOP, 0, 0, 0},
    };
    const typing_t typ[] = {{6000, 8800, 55}};
    uint32_t end = argc > 1 ? (uint32_t)atoi(argv[1]) : 45000;
    uint32_t idle_offset = argc > 2 ? (uint32_t)atoi(argv[2]) : 0;  // pretend the user left long ago

    uint8_t fb[CLAWD_FB_SIZE];
    clawd_frame_t fr;
    clawd_init(1);
    if (argc > 4) clawd_set_context((uint8_t)atoi(argv[4]));
    size_t ei = 0;
    uint32_t last_key = 0;
    for (uint32_t t = 1; t < end; t += 50) {
        while (argc < 4 && ei < sizeof(evs) / sizeof(evs[0]) && evs[ei].at <= t) {
            clawd_event(t, evs[ei].ev, evs[ei].tool, evs[ei].working, evs[ei].waiting);
            ei++;
        }
        uint8_t wpm = 0;
        for (size_t i = 0; i < sizeof(typ) / sizeof(typ[0]); i++)
            if (t >= typ[i].from && t <= typ[i].to) { wpm = typ[i].wpm; last_key = t; }
        clawd_input_t in = {t, t - last_key + idle_offset, wpm};
        clawd_render(&in, fb, &fr);
        char text[4][22];
        memset(text, ' ', sizeof(text));
        for (int i = 0; i < fr.count; i++) {
            size_t n = strlen(fr.lines[i].str);
            memcpy(&text[fr.lines[i].row][fr.lines[i].col], fr.lines[i].str, n);
        }
        static int last = -1; if (getenv("TRACE") && pl.cur != last) { fprintf(stderr, "%5.1fs anim=%d x=%.0f\n", t / 1000.0, pl.cur, d.x); last = pl.cur; }
        fwrite(fb, 1, sizeof(fb), stdout);
        fwrite(text, 1, sizeof(text), stdout);
    }
    return 0;
}
