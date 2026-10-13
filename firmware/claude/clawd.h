// Claude mascot ("Clawd") for a 128x32 OLED.
//
// The rig and its animations are a C port of Ivel10Go/claude-mascot (MIT):
// the character is nothing but axis-aligned rectangles on a 24x24 authoring
// grid, posed by procedural functions of elapsed time. Here those rectangles
// are rasterised straight into a 1bpp SSD1306 page buffer.
//
// This file has no QMK dependency so it also builds into the desktop
// simulator under tools/; the keymap supplies time, input activity and text.

#pragma once

#include <stdbool.h>
#include <stdint.h>

#define CLAWD_W 128
#define CLAWD_H 32
#define CLAWD_FB_SIZE (CLAWD_W * CLAWD_H / 8)

// Events sent by the host over raw HID (see tools/clawd_hook.py).
enum clawd_event {
    CLAWD_EV_SYNC = 0,       // counts only, no reaction
    CLAWD_EV_SESSION_START,
    CLAWD_EV_PROMPT,
    CLAWD_EV_TOOL,           // tool category in `tool`
    CLAWD_EV_TOOL_FAILED,
    CLAWD_EV_PERMISSION,
    CLAWD_EV_STOP,
    CLAWD_EV_STOP_FAILED,
    CLAWD_EV_RATE_LIMIT,
    CLAWD_EV_COMPACT,
    CLAWD_EV_SUBAGENT,
    CLAWD_EV_SESSION_END,
};

enum clawd_tool {
    CLAWD_TOOL_OTHER = 0,
    CLAWD_TOOL_EDIT,
    CLAWD_TOOL_BASH,
    CLAWD_TOOL_READ,
    CLAWD_TOOL_WEB,
};

typedef struct {
    uint32_t now;          // ms
    uint32_t idle_ms;      // time since the user last pressed a key
    uint8_t  wpm;
} clawd_input_t;

// Text line the mascot wants shown, in 6x8 font cells (21 cols x 4 rows).
typedef struct {
    uint8_t col, row;
    char    str[22];
} clawd_text_t;

typedef struct {
    clawd_text_t lines[3];
    uint8_t      count;
    bool         display_on;
} clawd_frame_t;

void clawd_init(uint32_t now);
void clawd_event(uint32_t now, uint8_t event, uint8_t tool, uint8_t working, uint8_t waiting);
// Context window usage of the session the mascot is following, 0-100.
void clawd_set_context(uint8_t pct);
// Renders one frame into `fb` (SSD1306 page layout, 512 bytes) and fills
// `out` with the text the caller should draw on top.
void clawd_render(const clawd_input_t *in, uint8_t *fb, clawd_frame_t *out);
