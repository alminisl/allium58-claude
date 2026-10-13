 /* Copyright 2020 Naoki Katahira
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 2 of the License, or
  * (at your option) any later version.
  *
  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.
  *
  * You should have received a copy of the GNU General Public License
  * along with this program.  If not, see <http://www.gnu.org/licenses/>.
  */

#include QMK_KEYBOARD_H
#include <stdio.h>

enum layer_number {
  _QWERTY = 0,
  _LOWER,
  _RAISE,
  _ADJUST,
  _MO4,
  _MO5
};

#define RAISE MO(_RAISE)
#define LOWER MO(_LOWER)

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {

/* QWERTY
 * ,-----------------------------------------.                    ,-----------------------------------------.
 * | ESC  |   1  |   2  |   3  |   4  |   5  |                    |   6  |   7  |   8  |   9  |   0  |  ~   |
 * |------+------+------+------+------+------|                    |------+------+------+------+------+------|
 * | Tab  |   Q  |   W  |   E  |   R  |   T  |                    |   Y  |   U  |   I  |   O  |   P  |  -   |
 * |------+------+------+------+------+------|                    |------+------+------+------+------+------|
 * |LCTRL |   A  |   S  |   D  |   F  |   G  |-------.    ,-------|   H  |   J  |   K  |   L  |   ;  |  '   |
 * |------+------+------+------+------+------|   [   |    |    ]  |------+------+------+------+------+------|
 * |LShift|   Z  |   X  |   C  |   V  |   B  |-------|    |-------|   N  |   M  |   ,  |   .  |   /  |RShift|
 * `-----------------------------------------/       /     \      \-----------------------------------------'
 *                   |LOWER | LGUI | Alt  | /Space  /       \Enter \  |BackSP| RGUI |RAISE |
 *                   |      |      |      |/       /         \      \ |      |      |      |
 *                   `-------------------''-------'           '------''--------------------'
 */

 [_QWERTY] = LAYOUT(
  KC_ESC,   KC_1,   KC_2,    KC_3,    KC_4,    KC_5,                     KC_6,    KC_7,    KC_8,    KC_9,    KC_0,    KC_GRV,
  KC_TAB,   KC_Q,   KC_W,    KC_E,    KC_R,    KC_T,                     KC_Y,    KC_U,    KC_I,    KC_O,    KC_P,    KC_MINS,
  KC_LCTL, KC_A,   KC_S,    KC_D,    KC_F,    KC_G,                     KC_H,    KC_J,    KC_K,    KC_L,    KC_SCLN, KC_QUOT,
  KC_LSFT,  KC_Z,   KC_X,    KC_C,    KC_V,    KC_B, KC_LBRC,  KC_RBRC,  KC_N,    KC_M,    KC_COMM, KC_DOT,  KC_SLSH,  KC_RSFT,
                      LOWER, KC_LGUI,  KC_LALT, KC_SPC,   KC_ENT,   KC_BSPC,  KC_RGUI, RAISE
),
/* LOWER
 * ,-----------------------------------------.                    ,-----------------------------------------.
 * |      |      |      |      |      |      |                    |      |      |      |      |      |      |
 * |------+------+------+------+------+------|                    |------+------+------+------+------+------|
 * |      |   !  |   @  |   #  |   $  |   %  |                    |   ^  |   &  |   *  |   (  |   )  |      |
 * |------+------+------+------+------+------|                    |------+------+------+------+------+------|
 * |      |   1  |   2  |   3  |   4  |   5  |-------.    ,-------|   6  |   7  |   8  |   9  |   0  |      |
 * |------+------+------+------+------+------|   [   |    |    ]  |------+------+------+------+------+------|
 * |      |      |      |      |      |      |-------|    |-------|   |  |   `  |   +  |   {  |   }  |      |
 * `-----------------------------------------/       /     \      \-----------------------------------------'
 *                   |LOWER | LGUI | Alt  | /Space  /       \Enter \  |BackSP| RGUI |RAISE |
 *                   |      |      |      |/       /         \      \ |      |      |      |
 *                   `-------------------''-------'           '------''--------------------'
 */
[_LOWER] = LAYOUT(
  _______, _______, _______, _______, _______, _______,                   _______, _______, _______,_______, _______, _______,
  _______, KC_EXLM, KC_AT,   KC_HASH, KC_DLR,  KC_PERC,                   KC_CIRC, KC_AMPR, KC_ASTR, KC_LPRN, KC_RPRN, _______,
  _______, KC_1,    KC_2,    KC_3,    KC_4,    KC_5,                      KC_6,    KC_7,    KC_8,    KC_9,    KC_0, _______,
  _______, _______, _______, _______, _______, _______, _______, _______, KC_PIPE, KC_GRAVE, KC_PLUS, KC_LCBR, KC_RCBR, _______,
                             _______, _______, _______, _______, _______,  _______, _______, _______
),
/* RAISE
 * ,-----------------------------------------.                    ,-----------------------------------------.
 * |      |      |      |      |      |      |                    |      |      |      |      |      |      |
 * |------+------+------+------+------+------|                    |------+------+------+------+------+------|
 * |  F1  |  F2  |  F3  |  F4  |  F5  |  F6  |                    |  F7  |  F8  |  F9  | F10  | F11  | F12  |
 * |------+------+------+------+------+------|                    |------+------+------+------+------+------|
 * |      |      |      |      |      |      |-------.    ,-------|      | Left | Down |  Up  |Right |      |
 * |------+------+------+------+------+------|   [   |    |    ]  |------+------+------+------+------+------|
 * |      |      |      |      |      |      |-------|    |-------|   +  |   =  |   [  |   ]  |   \  |      |
 * `-----------------------------------------/       /     \      \-----------------------------------------'
 *                   |LOWER | LGUI | Alt  | /Space  /       \Enter \  |BackSP| RGUI |RAISE |
 *                   |      |      |      |/       /         \      \ |      |      |      |
 *                   `-------------------''-------'           '------''--------------------'
 */

[_RAISE] = LAYOUT(
  _______, _______, _______, _______, _______, _______,                     _______, _______, _______, _______, _______, _______,
  KC_F1,   KC_F2,   KC_F3,   KC_F4,   KC_F5,   KC_F6,                       KC_F7,   KC_F8,   KC_F9,   KC_F10,  KC_F11,  KC_F12,
  _______, _______, _______, _______, _______, _______,                     XXXXXXX, KC_LEFT, KC_DOWN, KC_UP,   KC_RGHT, XXXXXXX,
  _______, _______, _______, _______, _______, _______,   _______, _______, KC_PLUS, KC_EQL,  KC_LBRC, KC_RBRC, KC_BSLS, _______,
                             _______, _______, _______,  _______, _______,  _______, _______, _______
),

/* ADJUST
 * ,-----------------------------------------.                    ,-----------------------------------------.
 * |      |      |      |      |      |      |                    |      |      |      |      |      |      |
 * |------+------+------+------+------+------|                    |------+------+------+------+------+------|
 * |      |      |      |      |      |      |                    |      |      |      |      |      |      |
 * |------+------+------+------+------+------|                    |------+------+------+------+------+------|
 * |      |      |      |      |      |      |-------.    ,-------|      |      |      |      |      |      |
 * |------+------+------+------+------+------|       |    |       |------+------+------+------+------+------|
 * |      |      |      |      |      |      |-------|    |-------|      |      |      |      |      |      |
 * `-----------------------------------------/       /     \      \-----------------------------------------'
 *                   |LOWER | LGUI | Alt  | /Space  /       \Enter \  |BackSP| RGUI |RAISE |
 *                   |      |      |      |/       /         \      \ |      |      |      |
 *                   `----------------------------'           '------''--------------------'
 */
  [_ADJUST] = LAYOUT(
  XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX,                   XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX,
  XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX,                   XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX,
  XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX,                   XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX,
  XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX,
                             _______, _______, _______, _______, _______,  _______, _______, _______
  ),
/* MO(4) 
 * ,-----------------------------------------.                    ,-----------------------------------------.
 * |      |      |      |      |      |      |                    |      |      |      |      |      |      |
 * |------+------+------+------+------+------|                    |------+------+------+------+------+------|
 * |      |      |      |      |      |      |                    |      |      |      |      |      |      |
 * |------+------+------+------+------+------|                    |------+------+------+------+------+------|
 * |      |      |      |      |      |      |-------.    ,-------|      |      |      |      |      |      |
 * |------+------+------+------+------+------|       |    |       |------+------+------+------+------+------|
 * |      |      |      |      |      |      |-------|    |-------|      |      |      |      |      |      |
 * `-----------------------------------------/       /     \      \-----------------------------------------'
 *                   |LOWER | LGUI | Alt  | /Space  /       \Enter \  |BackSP| RGUI |RAISE |
 *                   |      |      |      |/       /         \      \ |      |      |      |
 *                   `----------------------------'           '------''--------------------'
 */
  [_MO4] = LAYOUT(
  XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX,                   XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX,
  XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX,                   XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX,
  XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX,                   XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX,
  XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX,
                             _______, _______, _______, _______, _______,  _______, _______, _______
  ),
/* MO(5) 
 * ,-----------------------------------------.                    ,-----------------------------------------.
 * |      |      |      |      |      |      |                    |      |      |      |      |      |      |
 * |------+------+------+------+------+------|                    |------+------+------+------+------+------|
 * |      |      |      |      |      |      |                    |      |      |      |      |      |      |
 * |------+------+------+------+------+------|                    |------+------+------+------+------+------|
 * |      |      |      |      |      |      |-------.    ,-------|      |      |      |      |      |      |
 * |------+------+------+------+------+------|       |    |       |------+------+------+------+------+------|
 * |      |      |      |      |      |      |-------|    |-------|      |      |      |      |      |      |
 * `-----------------------------------------/       /     \      \-----------------------------------------'
 *                   |LOWER | LGUI | Alt  | /Space  /       \Enter \  |BackSP| RGUI |RAISE |
 *                   |      |      |      |/       /         \      \ |      |      |      |
 *                   `----------------------------'           '------''--------------------'
 */
  [_MO5] = LAYOUT(
  XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX,                   XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX,
  XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX,                   XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX,
  XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX,                   XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX,
  XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX,
                             _______, _______, _______, _______, _______,  _______, _______, _______
  )
};

layer_state_t layer_state_set_user(layer_state_t state) {
    return update_tri_layer_state(state, _RAISE, _LOWER, _ADJUST);
}

#ifdef OLED_ENABLE

#include <string.h>
#include "clawd.h"
#include "transactions.h"
#include "via.h"
#include "hardware/watchdog.h"

// Hardware watchdog plus breadcrumbs, kept as a safety net after the main-loop
// hang (a stack overflow in the mascot render, see rules.mk). The RP2040 resets
// after 2 s without a feed; scratch[0] survives that reset and says which stage
// of the loop hung. The bridge reads it (0xC6).
enum { STAGE_CORE = 1, STAGE_HOUSE, STAGE_RPC, STAGE_RENDER, STAGE_OLED, STAGE_HID };
#define CRUMB(stage) (watchdog_hw->scratch[0] = 0xC0DE0000u | (stage))
static uint32_t last_hang;   // crumb from before a watchdog reset, else 0

// Raw HID from the host bridge (tools/agentd.py), on ids VIA and Vial don't use:
//   0xC1 event   [event, tool, sessions working, sessions waiting]
//   0xC2 right   [row 0-3, invert, 21 chars]   one line of the sessions screen
//   0xC3 status  [21 chars]                    top line of the mascot screen
//   0xC4 context [percent]                     context used, for the mascot
enum { HID_EVENT = 0xC1, HID_RIGHT, HID_STATUS, HID_CONTEXT };

#define CLAWD_FRAME_MS 33
#define HOST_STALE_MS 15000
#define STATUS_COLS 13   // the mascot reports from the right of the screen

// The right half owns the second screen but not the USB link, so the master
// mirrors this struct across in chunks over a split RPC.
typedef struct {
    char    lines[4][21];
    uint8_t invert;   // bit per row
} right_screen_t;

#define CHUNK 28
typedef struct {
    uint8_t index;
    uint8_t data[CHUNK];
} chunk_t;
#define CHUNK_COUNT ((sizeof(right_screen_t) + CHUNK - 1) / CHUNK)

static right_screen_t right, right_sent;
static char           status_line[22];
static uint32_t       host_seen, clawd_last_frame, sync_timer;
static uint8_t        clawd_fb[CLAWD_FB_SIZE], sync_next;

static void set_line(uint8_t row, bool invert, const char *text, uint8_t len) {
    if (row > 3) return;
    memset(right.lines[row], ' ', 21);
    memcpy(right.lines[row], text, len > 21 ? 21 : len);
    if (invert)
        right.invert |= 1 << row;
    else
        right.invert &= ~(1 << row);
}

static void right_rpc(uint8_t in_len, const void *in, uint8_t out_len, void *out) {
    const chunk_t *c   = in;
    size_t         off = (size_t)c->index * CHUNK;
    if (off >= sizeof(right)) return;
    size_t n = sizeof(right) - off < CHUNK ? sizeof(right) - off : CHUNK;
    memcpy((uint8_t *)&right + off, c->data, n);
}

void keyboard_post_init_user(void) {
    if (watchdog_enable_caused_reboot()) last_hang = watchdog_hw->scratch[0];
    watchdog_enable(2000, true);
    CRUMB(STAGE_CORE);
    clawd_init(timer_read32());
    transaction_register_rpc(RPC_ID_RIGHT_SCREEN, right_rpc);
    set_line(1, false, "  waiting for Claude", 20);
}

// Ships one changed chunk at a time to the right half.
void housekeeping_task_user(void) {
    watchdog_update();
    CRUMB(STAGE_HOUSE);
    CRUMB(STAGE_CORE);
    if (!is_keyboard_master() || timer_elapsed32(sync_timer) < 20) return;
    sync_timer = timer_read32();

    if (timer_elapsed32(host_seen) > HOST_STALE_MS && host_seen) {
        // The bridge went away (stopped, or Vial is holding the keyboard).
        host_seen = 0;
        for (uint8_t r = 0; r < 4; r++) set_line(r, false, "", 0);
        set_line(1, false, "  bridge offline", 16);
        status_line[0] = 0;
        clawd_set_context(0);
    }
    // shortcut: no periodic resync, so a right half that reboots on its own stays stale until a row changes; add one if that bites.
    for (uint8_t i = 0; i < CHUNK_COUNT; i++) {
        uint8_t idx = (sync_next + i) % CHUNK_COUNT;
        size_t  off = (size_t)idx * CHUNK;
        size_t  n   = sizeof(right) - off < CHUNK ? sizeof(right) - off : CHUNK;
        if (memcmp((uint8_t *)&right + off, (uint8_t *)&right_sent + off, n) == 0) continue;
        chunk_t c = {.index = idx};
        memcpy(c.data, (uint8_t *)&right + off, n);
        CRUMB(STAGE_RPC);
        if (transaction_rpc_send(RPC_ID_RIGHT_SCREEN, sizeof(c), &c)) memcpy((uint8_t *)&right_sent + off, c.data, n);
        CRUMB(STAGE_CORE);
        sync_next = idx + 1;
        break;
    }
}

// Called by via.c before VIA sees the packet; returning true sends no reply,
// which matters because the bridge never reads one (see via.c).
bool raw_hid_receive_noreply_kb(uint8_t *data, uint8_t length) {
    // 0xCF "BOOT": jump to the UF2 bootloader so the host can reflash without
    // the reset button (Vial's own bootloader command is off in insecure builds).
    if (length == 32 && data[0] == 0xCF && !memcmp(&data[1], "BOOT", 4)) bootloader_jump();
    if (length < 32 || data[0] < HID_EVENT || data[0] > HID_CONTEXT) return false;
    CRUMB(STAGE_HID);
    host_seen = timer_read32();
    if (!host_seen) host_seen = 1;
    switch (data[0]) {
        case HID_EVENT:
            clawd_event(timer_read32(), data[1], data[2], data[3], data[4]);
            break;
        case HID_RIGHT:
            set_line(data[1], data[2], (const char *)&data[3], 21);
            break;
        case HID_STATUS:
            memcpy(status_line, &data[1], 21);
            status_line[21] = 0;
            break;
        case HID_CONTEXT:
            clawd_set_context(data[1]);
            break;
    }
    CRUMB(STAGE_CORE);
    return true;
}

oled_rotation_t oled_init_user(oled_rotation_t rotation) {
    if (!is_keyboard_master()) return OLED_ROTATION_180;  // the right half is mounted upside down
    return rotation;
}

static void render_right(void) {
    char buf[22];
    for (uint8_t r = 0; r < 4; r++) {
        memcpy(buf, right.lines[r], 21);
        buf[21] = 0;
        // A row the bridge marks '|' (working) gets its spinner animated here, so it needs no traffic.
        if (buf[0] == '|') buf[0] = "|/-\\"[timer_read32() / 150 % 4];
        oled_set_cursor(0, r);
        oled_write(buf, right.invert >> r & 1);
    }
}

// Left: the Claude mascot. Right: every active session. The right half's
// screen follows the left one on and off (SPLIT_OLED_ENABLE).
bool oled_task_user(void) {
    if (!is_keyboard_master()) {
        render_right();
        return false;
    }

    uint32_t now = timer_read32();
    if (now - clawd_last_frame < CLAWD_FRAME_MS) return false;
    clawd_last_frame = now;

    clawd_input_t in = {now, last_input_activity_elapsed(), get_current_wpm()};
    clawd_frame_t fr;
    CRUMB(STAGE_RENDER);
    clawd_render(&in, clawd_fb, &fr);
    CRUMB(STAGE_OLED);

    if (!fr.display_on) {
        oled_off();
        return false;
    }
    oled_on();
    oled_set_cursor(0, 0);
    oled_write_raw((const char *)clawd_fb, CLAWD_FB_SIZE);
    if (status_line[0]) {
        char top[STATUS_COLS + 1];
        strncpy(top, status_line, STATUS_COLS);
        top[STATUS_COLS] = 0;
        // Trailing spaces would wipe the mascot's head as he jumps past.
        for (int8_t i = STATUS_COLS - 1; i >= 0 && (top[i] == ' ' || !top[i]); i--) top[i] = 0;
        oled_set_cursor(0, 0);
        oled_write(top, false);
    }
    for (uint8_t i = 0; i < fr.count; i++) {
        oled_set_cursor(fr.lines[i].col, fr.lines[i].row);
        oled_write(fr.lines[i].str, false);
    }
    CRUMB(STAGE_CORE);
    return false;
}

// 0xC6: report the breadcrumb from before the last watchdog reset. Goes
// through VIA (not the no-reply path) because the bridge reads the answer.
void raw_hid_receive_kb(uint8_t *data, uint8_t length) {
    if (data[0] != 0xC6) {
        data[0] = id_unhandled;
        return;
    }
    data[1] = last_hang != 0;
    data[2] = last_hang & 0xFF;
    // Peak main-loop stack use: crt0 fills the stack with 0x55 at boot.
    extern uint8_t __process_stack_base__[], __process_stack_end__[];
    uint8_t *p = __process_stack_base__;
    while (p < __process_stack_end__ && *p == 0x55) p++;
    uint16_t used = __process_stack_end__ - p;
    uint16_t size = __process_stack_end__ - __process_stack_base__;
    data[3] = used >> 8;
    data[4] = used & 0xFF;
    data[5] = size >> 8;
    data[6] = size & 0xFF;
}

#endif // OLED_ENABLE
