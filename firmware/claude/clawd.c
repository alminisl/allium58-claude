// Claude mascot for a 128x32 OLED. See clawd.h.
//
// Rig geometry and every pose function below are ported from
// Ivel10Go/claude-mascot (src/renderer/overlay/rig/{mark,anims}.js, MIT).
// What changed on the way to 1bpp: colour, glow and opacity are dropped,
// eyes and the headband are punched out of the body in black, and anything
// thinner than a pixel is widened to one so it never flickers out of view.

#include "clawd.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#define TAU 6.2831853f
#define PI 3.1415927f

// ── Rig geometry (24x24 authoring grid) ──────────────────────────────────

#define BODY_X 3.0f
#define BODY_Y 5.0f
#define BODY_W 18.0f
#define BODY_H 12.0f
#define NUB_W 3.0f
#define NUB_H 3.1f
#define NUB_Y 10.95f
#define LEG_W 1.49f
#define LEG_H 3.0f
#define LEG_Y 17.0f
#define EYE_W 1.49f
#define EYE_H 2.85f
#define EYE_Y 8.1f
#define PIVOT_X 12.0f
#define GROUND_Y 20.0f
#define LEG_COUNT 4

static const float LEG_XS[LEG_COUNT] = {3, 6, 15, 18};
static const float EYE_XS[2]         = {6, 16.51f};

// Grid units to pixels, and where the feet land on screen.
#define SC 1.25f
#define GROUND_PX 30.0f

// Where the mascot stands while it reports on Claude, leaving the left of
// the screen for text.
#define STATION_X 102.0f
#define X_MIN 18.0f
#define X_MAX 110.0f

// ── Pose ─────────────────────────────────────────────────────────────────

typedef struct {
    float angle, dx, dy, len;
} arm_t;
typedef struct {
    float angle, dy, scaleY;
} leg_t;

typedef struct {
    float bodyDy, squashX, squashY, tilt, scale;
    arm_t arms[2];
    leg_t legs[LEG_COUNT];
    float eyeOpen, eyeDx, eyeDy, eyeSquint;
    float sweat, zzz;
    float headband, bandWave, flag, flagWave, flagLean;
    float confetti, confettiAmt, sparkle, sparkleWave;
} pose_t;

static void neutral_pose(pose_t *p) {
    memset(p, 0, sizeof(*p));
    p->squashX = p->squashY = p->scale = 1;
    for (int i = 0; i < 2; i++) p->arms[i].len = 1;
    for (int i = 0; i < LEG_COUNT; i++) p->legs[i].scaleY = 1;
    p->eyeOpen = 1;
}

// Every field is a float, so a crossfade is a straight walk over the struct.
static void lerp_pose(const pose_t *a, const pose_t *b, float k, pose_t *out) {
    const float *fa = (const float *)a, *fb = (const float *)b;
    float       *fo = (float *)out;
    for (unsigned i = 0; i < sizeof(pose_t) / sizeof(float); i++) fo[i] = fa[i] + (fb[i] - fa[i]) * k;
}

// ── Easing helpers (anims.js) ────────────────────────────────────────────

static float clampf(float v, float lo, float hi) { return v < lo ? lo : v > hi ? hi : v; }
static float sinp(float t, float period, float phase) { return sinf(t / period * TAU + phase); }
static float ease_out(float k) {
    k = 1 - clampf(k, 0, 1);
    return 1 - k * k * k;
}
static float ease_in_out(float k) {
    if (k < 0.5f) return 4 * k * k * k;
    float m = -2 * k + 2;
    return 1 - m * m * m / 2;
}
static float envelope(float t, float dur, float ramp) { return clampf(t / ramp, 0, 1) * clampf((dur - t) / ramp, 0, 1); }

// Quick rise, a beat of hang time, then an accelerating fall.
static float hop(float k) {
    float c = k - floorf(k);
    if (c < 0.42f) return sinf(c / 0.42f * (PI / 2));
    float d = (c - 0.42f) / 0.58f;
    return 1 - d * d * d;
}

// A cycle that dwells at its extremes, which is what gives a walk rhythm.
static float held(float k, float bias) {
    float s = sinf(k * TAU);
    float m = powf(fabsf(s), 1 - bias);
    return s < 0 ? -m : m;
}

static const float LEG_PHASE[LEG_COUNT] = {0, 0.055f, 0.5f, 0.555f};
static const float LEG_LEAN[LEG_COUNT]  = {-7, -8, -8, -9};

// ── Animations ───────────────────────────────────────────────────────────

enum {
    A_IDLE, A_BLINK, A_LOOKUP, A_SCAN, A_WALK, A_RUN, A_TURN, A_WAVE, A_THINK,
    A_CELEBRATE, A_ALERT, A_LIMITHIT, A_CLONE, A_STUMBLE, A_VICTORY, A_CONFETTI,
    A_HEADBAND, A_MEDITATE, A_BACKFLIP, A_DANCE, A_SPIN, A_STRETCH, A_FACEPALM,
    A_SHRUG, A_NOD, A_SALUTE, A_BOW, A_PEEK, A_ROLL, A_TYPING, A_DIGGING,
    A_SEARCHING, A_BROADCASTING, A_CURL, A_SLEEP, A_LAYDOWN, A_COUNT
};

static void pose_anim(uint8_t id, float t, pose_t *p) {
    switch (id) {
        case A_IDLE: {
            float breathe = sinp(t, 2800, 0);
            p->bodyDy     = breathe * -0.35f;
            p->squashY    = 1 + breathe * 0.018f;
            p->squashX    = 1 - breathe * 0.014f;
            p->tilt       = sinp(t, 6200, 0) * 0.8f;
            p->arms[0].angle = 3 + sinp(t, 3400, 0) * 2.5f;
            p->arms[1].angle = 3 + sinp(t, 3400, 0.9f) * 2.5f;
            break;
        }
        case A_BLINK: {
            float k    = t / 150;
            p->eyeOpen = k < 0.5f ? 1 - k * 2 : (k - 0.5f) * 2;
            p->bodyDy  = sinp(t, 2800, 0) * -0.35f;
            break;
        }
        case A_LOOKUP: {
            float k = envelope(t, 1700, 380);
            p->tilt = -7 * k;
            p->eyeDy = -0.8f * k;
            p->bodyDy = -0.5f * k;
            p->arms[0].angle = p->arms[1].angle = -14 * k;
            p->legs[0].scaleY = p->legs[1].scaleY = 1 - 0.12f * k;
            break;
        }
        case A_SCAN: {
            float k = sinf(t / 2400 * TAU);
            p->eyeDx = k * 0.9f;
            p->tilt = k * 3;
            p->bodyDy = sinp(t, 2800, 0) * -0.35f;
            break;
        }
        case A_WALK: {
            float cycle = t / 520, bounce = hop(cycle * 2);
            p->bodyDy  = -bounce * 0.55f;
            p->squashY = 1 + bounce * 0.035f;
            p->squashX = 1 - bounce * 0.03f;
            p->tilt    = held(cycle, 0.35f) * 2.2f;
            for (int i = 0; i < LEG_COUNT; i++) {
                float swing = held(cycle + LEG_PHASE[i], 0.35f);
                p->legs[i].angle = swing * 26 + LEG_LEAN[i] * 0.15f;
                p->legs[i].dy = -fmaxf(0, swing) * 0.5f;
            }
            p->arms[0].angle = held(cycle + 0.5f, 0.35f) * 16;
            p->arms[1].angle = held(cycle, 0.35f) * 16;
            break;
        }
        case A_RUN: {
            float cycle = t / 260, bounce = hop(cycle * 2);
            p->bodyDy  = -bounce * 1.1f;
            p->squashY = 1 + bounce * 0.07f;
            p->squashX = 1 - bounce * 0.06f;
            p->tilt    = 6 + held(cycle, 0.2f) * 3;
            for (int i = 0; i < LEG_COUNT; i++) p->legs[i].angle = held(cycle + LEG_PHASE[i], 0.2f) * 42 + LEG_LEAN[i] * 0.3f;
            p->arms[0].angle = held(cycle + 0.5f, 0.2f) * 34;
            p->arms[1].angle = held(cycle, 0.2f) * 34;
            p->eyeSquint = 0.3f;
            break;
        }
        case A_TURN: {
            float k = sinf(t / 240 * PI);
            p->squashX = 1 - k * 0.7f;
            p->squashY = 1 + k * 0.1f;
            p->tilt = k * 6;
            break;
        }
        case A_WAVE: {
            float k = envelope(t, 1500, 240);
            p->arms[1].angle = -62 * k + sinf(t / 260 * TAU) * 22 * k;
            p->arms[1].len = 1 + 0.25f * k;
            p->arms[0].angle = 8 * k;
            p->tilt = -3.5f * k;
            p->bodyDy = -0.3f * k;
            p->eyeSquint = 0.35f * k;
            break;
        }
        case A_THINK:
            p->tilt = -4 + sinp(t, 3600, 0) * 1.4f;
            p->bodyDy = sinp(t, 3200, 0) * -0.3f;
            p->eyeOpen = 0.62f;
            p->eyeDx = 0.55f + sinp(t, 3600, 0) * 0.25f;
            p->eyeDy = -0.25f;
            p->arms[1].angle = -34 + sinp(t, 3600, 0) * 4;
            p->arms[0].angle = 6;
            break;
        case A_CELEBRATE: {
            float k = clampf((1900 - t) / 550, 0, 1), air = hop(t / 620);
            p->bodyDy  = -air * 3.4f * k;
            p->squashY = 1 + air * 0.09f * k - fmaxf(0, 0.25f - air) * 0.4f * k;
            p->squashX = 1 - air * 0.07f * k + fmaxf(0, 0.25f - air) * 0.3f * k;
            p->tilt    = sinf(t / 620 * TAU) * 5 * k;
            p->arms[0].angle = p->arms[1].angle = -70 * k;
            p->eyeSquint = 0.5f * k;
            for (int i = 0; i < LEG_COUNT; i++) p->legs[i].scaleY = 1 - air * 0.3f * k;
            break;
        }
        case A_ALERT: {
            float pulse = (sinf(t / 440 * TAU) + 1) / 2;
            p->bodyDy = -hop(t / 440) * 2.6f;
            p->scale = 1 + pulse * 0.06f;
            p->eyeOpen = 1.25f;
            p->arms[0].angle = p->arms[1].angle = -55 - pulse * 18;
            break;
        }
        case A_LIMITHIT: {
            float slump = ease_out(clampf(t / 700, 0, 1));
            p->squashY = 1 - slump * 0.42f;
            p->squashX = 1 + slump * 0.3f;
            p->bodyDy = slump * 1.4f;
            p->tilt = slump * 5 + sinf(t / 1400) * 1.2f;
            p->eyeOpen = 1 - slump * 0.86f;
            p->eyeSquint = slump * 0.5f;
            p->eyeDy = slump * 0.6f;
            p->arms[0].angle = p->arms[1].angle = slump * 72;
            p->sweat = slump * 0.5f;
            for (int i = 0; i < LEG_COUNT; i++) {
                p->legs[i].angle = (i < 2 ? -1 : 1) * slump * 26;
                p->legs[i].scaleY = 1 - slump * 0.35f;
            }
            break;
        }
        case A_CLONE: {
            float split = sinf(clampf(t / 700 * 2, 0, 1) * PI);
            p->squashX = 1 + split * 0.55f;
            p->squashY = 1 - split * 0.2f;
            p->eyeOpen = 1 + split * 0.3f;
            p->arms[0].angle = p->arms[1].angle = -split * 40;
            p->arms[0].len = p->arms[1].len = 1 + split * 0.8f;
            break;
        }
        case A_STUMBLE: {
            float k = clampf((950 - t) / 950, 0, 1);
            p->tilt = sinf(t / 190 * TAU) * 16 * k;
            p->bodyDy = -fabsf(sinf(t / 320 * PI)) * 1.2f;
            p->eyeOpen = 1.3f;
            p->arms[0].angle = -40 * k;
            p->arms[1].angle = -25 * k;
            p->legs[0].angle = p->legs[1].angle = 34 * k;
            p->legs[2].angle = p->legs[3].angle = -22 * k;
            break;
        }
        case A_VICTORY: {
            float raise = ease_out(clampf(t / 700, 0, 1)), out = clampf((3400 - t) / 500, 0, 1), k = raise * out;
            p->flag = k;
            p->flagWave = sinf(t / 250 * TAU);
            p->flagLean = 8 + sinf(t / 950 * TAU) * 7;
            p->arms[1].angle = -78 * k;
            p->arms[1].len = 1 + 0.35f * k;
            p->arms[0].angle = -18 * k + sinf(t / 520 * TAU) * 11 * k;
            float bob = t > 700 ? hop((t - 700) / 780) * out : 0;
            p->bodyDy = -bob * 1.7f;
            p->squashY = 1 + bob * 0.05f;
            p->squashX = 1 - bob * 0.04f;
            p->tilt = -4 * k + sinf(t / 950 * TAU) * 2;
            p->eyeSquint = 0.4f * k;
            for (int i = 0; i < LEG_COUNT; i++) p->legs[i].scaleY = 1 - bob * 0.22f;
            break;
        }
        case A_CONFETTI: {
            float squat = fmaxf(0, 1 - t / 240);
            float air = (t < 240 || t > 1140) ? 0 : hop((t - 240) / 900);
            float burst = clampf((t - 300) / 1600, 0, 1);
            p->confetti = ease_out(burst);
            p->confettiAmt = burst > 0 ? clampf((1 - burst) * 3, 0, 1) : 0;
            p->bodyDy = -air * 3.6f;
            p->squashY = 1 - squat * 0.28f + air * 0.08f;
            p->squashX = 1 + squat * 0.22f - air * 0.06f;
            p->arms[0].angle = p->arms[1].angle = 24 * squat - 84 * air;
            p->arms[0].len = p->arms[1].len = 1 + air * 0.3f;
            p->eyeOpen = 1 + air * 0.3f;
            p->eyeSquint = 0.35f * air;
            for (int i = 0; i < LEG_COUNT; i++) p->legs[i].scaleY = 1 - air * 0.35f;
            break;
        }
        case A_HEADBAND: {
            float tie = ease_out(clampf(t / 850, 0, 1));
            float hands = sinf(clampf(t / 850, 0, 1) * PI);
            float work = tie * (1 - hands);
            p->headband = tie;
            p->bandWave = sinf(t / 620 * TAU);
            p->arms[0].angle = p->arms[1].angle = -58 * hands + 14 * work;
            p->arms[0].len = p->arms[1].len = 1 + hands * 0.3f;
            p->bodyDy = sinp(t, 1100, 0) * -0.4f * work - hands * 0.5f;
            p->tilt = 3 * work;
            p->squashY = 1 - 0.03f * work;
            p->squashX = 1 + 0.03f * work;
            p->eyeSquint = 0.4f * work;
            p->eyeDy = 0.25f * work;
            break;
        }
        case A_MEDITATE: {
            float breathe = sinp(t, 4200, 0), rise = ease_out(clampf(t / 1400, 0, 1));
            p->sparkle = rise * (0.55f + 0.45f * ((sinf(t / 2600 * TAU) + 1) / 2));
            p->sparkleWave = sinf(t / 3000 * TAU);
            p->bodyDy = -rise * 3.2f + breathe * -0.5f;
            p->squashY = 1 + breathe * 0.03f;
            p->squashX = 1 - breathe * 0.02f;
            p->eyeOpen = 0.08f;
            p->arms[0].angle = 30 + breathe * 4;
            p->arms[1].angle = 30 - breathe * 4;
            for (int i = 0; i < LEG_COUNT; i++) {
                p->legs[i].scaleY = 1 - rise * 0.55f;
                p->legs[i].angle = (i < 2 ? -1 : 1) * rise * 18;
            }
            break;
        }
        case A_BACKFLIP: {
            float squat = fmaxf(0, 1 - t / 170);
            float flight = clampf((t - 170) / 850, 0, 1);
            float air = sinf(flight * PI);
            float landing = sinf(clampf((t - 1020) / 230, 0, 1) * PI);
            p->bodyDy = -air * 7.5f;
            p->tilt = -360 * ease_in_out(flight);
            p->squashY = 1 - squat * 0.32f + air * 0.06f - landing * 0.25f;
            p->squashX = 1 + squat * 0.24f - air * 0.05f + landing * 0.2f;
            p->arms[0].angle = p->arms[1].angle = 30 * squat - 50 * air;
            p->eyeOpen = 1 + air * 0.35f;
            for (int i = 0; i < LEG_COUNT; i++) p->legs[i].scaleY = 1 - air * 0.55f - squat * 0.2f;
            break;
        }
        case A_DANCE: {
            float beat = t / 480, bounce = hop(beat * 2), sway = held(beat, 0.25f);
            p->bodyDy = -bounce * 1.3f;
            p->squashY = 1 + bounce * 0.06f;
            p->squashX = 1 - bounce * 0.05f;
            p->tilt = sway * 9;
            p->arms[0].angle = -70 + sway * 45;
            p->arms[1].angle = -70 - sway * 45;
            p->arms[0].len = p->arms[1].len = 1.2f;
            p->eyeSquint = 0.45f;
            for (int i = 0; i < LEG_COUNT; i++) {
                p->legs[i].angle = held(beat + LEG_PHASE[i], 0.25f) * 22;
                p->legs[i].scaleY = 1 - bounce * 0.15f;
            }
            break;
        }
        case A_SPIN: {
            float k = ease_in_out(clampf(t / 900, 0, 1)), theta = k * TAU;
            p->squashX = cosf(theta);
            p->squashY = 1 + fabsf(sinf(theta)) * 0.06f;
            p->bodyDy = -sinf(k * PI) * 1.2f;
            p->tilt = sinf(theta) * 5;
            p->arms[0].angle = p->arms[1].angle = -40 * sinf(k * PI);
            p->eyeSquint = 0.3f;
            break;
        }
        case A_STRETCH: {
            float k = sinf(clampf(t / 2000, 0, 1) * PI);
            p->squashY = 1 + k * 0.22f;
            p->squashX = 1 - k * 0.12f;
            p->bodyDy = -k * 1.4f;
            p->tilt = -k * 5;
            p->arms[0].angle = p->arms[1].angle = -100 * k;
            p->arms[0].len = p->arms[1].len = 1 + k * 0.6f;
            p->eyeOpen = 1 - k * 0.8f;
            p->eyeSquint = k * 0.5f;
            for (int i = 0; i < LEG_COUNT; i++) p->legs[i].scaleY = 1 + k * 0.25f;
            break;
        }
        case A_FACEPALM: {
            float k = envelope(t, 1600, 260), slap = ease_out(clampf(t / 320, 0, 1));
            p->arms[1].angle = -118 * slap * k;
            p->arms[1].len = 1 + 0.5f * slap * k;
            p->arms[1].dy = -1.2f * slap * k;
            p->arms[0].angle = 22 * k;
            p->bodyDy = 0.5f * k;
            p->squashY = 1 - k * 0.07f;
            p->squashX = 1 + k * 0.05f;
            p->eyeOpen = 1 - k * 0.85f;
            p->eyeDy = 0.3f * k;
            p->tilt = 9 * k + sinf(t / 520 * TAU) * 3 * k;
            break;
        }
        case A_SHRUG: {
            float k = sinf(clampf(t / 1300, 0, 1) * PI);
            for (int i = 0; i < 2; i++) {
                p->arms[i].angle = -34 * k;
                p->arms[i].len = 1 + k * 0.35f;
                p->arms[i].dy = -0.9f * k;
            }
            p->bodyDy = -0.7f * k;
            p->squashY = 1 - k * 0.06f;
            p->squashX = 1 + k * 0.05f;
            p->tilt = sinf(t / 1300 * TAU) * 4;
            p->eyeDy = -0.3f * k;
            p->eyeSquint = 0.25f * k;
            break;
        }
        case A_NOD: {
            float k = envelope(t, 1000, 160), beat = sinf(t / 330 * TAU);
            p->tilt = beat * 7 * k;
            p->bodyDy = fabsf(beat) * 0.4f * k;
            p->squashY = 1 - fabsf(beat) * 0.05f * k;
            p->eyeDy = beat * 0.35f * k;
            p->arms[0].angle = p->arms[1].angle = 10 * k;
            break;
        }
        case A_SALUTE: {
            float k = ease_out(clampf(t / 280, 0, 1)) * clampf((1500 - t) / 300, 0, 1);
            p->arms[1].angle = -128 * k;
            p->arms[1].len = 1 + 0.45f * k;
            p->arms[1].dy = -0.8f * k;
            p->arms[0].angle = 16 * k;
            p->tilt = -3 * k;
            p->squashY = 1 + 0.05f * k;
            p->bodyDy = -0.4f * k;
            p->eyeOpen = 1.1f;
            p->eyeSquint = 0.2f * k;
            for (int i = 0; i < LEG_COUNT; i++) p->legs[i].angle = LEG_LEAN[i] * 0.15f * k;
            break;
        }
        case A_BOW: {
            float k = sinf(clampf(t / 1600, 0, 1) * PI);
            p->tilt = 34 * k;
            p->bodyDy = 0.6f * k;
            p->squashY = 1 - k * 0.1f;
            p->arms[0].angle = 46 * k;
            p->arms[1].angle = -30 * k;
            p->arms[1].len = 1 + k * 0.4f;
            p->eyeOpen = 1 - k * 0.6f;
            p->eyeDy = 0.4f * k;
            for (int i = 0; i < LEG_COUNT; i++) p->legs[i].angle = -k * 10;
            break;
        }
        case A_PEEK: {
            float k = envelope(t, 2200, 380), lean = sinf(t / 2200 * TAU);
            p->tilt = lean * 14 * k;
            p->squashX = 1 - fabsf(lean) * 0.08f * k;
            p->eyeDx = lean * 1.3f * k;
            p->eyeOpen = 1 + 0.25f * k;
            p->bodyDy = -0.4f * k;
            p->arms[0].angle = p->arms[1].angle = 26 * k;
            for (int i = 0; i < LEG_COUNT; i++) p->legs[i].angle = lean * 8 * k;
            break;
        }
        case A_ROLL: {
            float k = clampf(t / 1400, 0, 1);
            p->tilt = ease_in_out(k) * 360;
            p->squashY = 0.86f;
            p->squashX = 1.14f;
            p->bodyDy = -sinf(k * PI) * 0.8f;
            p->eyeOpen = 0.5f;
            p->arms[0].angle = p->arms[1].angle = 60;
            for (int i = 0; i < LEG_COUNT; i++) p->legs[i].scaleY = 0.4f;
            break;
        }
        case A_TYPING:
            p->tilt = 4;
            p->bodyDy = sinp(t, 200, 0) * -0.18f;
            p->eyeOpen = 0.75f;
            p->eyeDy = 0.35f;
            p->arms[0].angle = 26 + sinf(t / 105 * TAU) * 16;
            p->arms[1].angle = 26 + sinf(t / 105 * TAU + PI) * 16;
            break;
        case A_DIGGING: {
            float k = fmodf(t / 560, 1), dig = sinf(k * TAU);
            p->tilt = 9 + dig * 6;
            p->bodyDy = fabsf(dig) * 0.5f;
            p->squashY = 1 - fabsf(dig) * 0.05f;
            p->arms[1].angle = 30 + dig * 46;
            p->arms[0].angle = 14;
            p->eyeOpen = 0.7f;
            p->eyeDy = 0.3f;
            break;
        }
        case A_SEARCHING: {
            float sweep = sinf(t / 1600 * TAU);
            int   probe = sweep > 0 ? 1 : 0;
            p->tilt = sweep * 6;
            p->eyeDx = sweep;
            p->eyeOpen = 1.1f;
            p->bodyDy = sinp(t, 1900, 0) * -0.3f;
            p->arms[probe].angle = -30;
            p->arms[probe].len = 1.5f;
            break;
        }
        case A_BROADCASTING: {
            float pulse = (sinf(t / 900 * TAU) + 1) / 2;
            p->bodyDy = sinp(t, 2200, 0) * -0.3f;
            p->arms[0].angle = p->arms[1].angle = -48;
            p->arms[0].len = p->arms[1].len = 1 + pulse * 0.5f;
            break;
        }
        case A_CURL: {
            float breathe = sinp(t, 4400, 0);
            p->squashY = 0.68f + breathe * 0.015f;
            p->squashX = 1.12f;
            p->bodyDy = 0.4f;
            p->eyeOpen = 0.2f;
            p->arms[0].angle = p->arms[1].angle = 46;
            for (int i = 0; i < LEG_COUNT; i++) p->legs[i].scaleY = 0.35f;
            break;
        }
        case A_SLEEP: {
            float breathe = sinp(t, 5000, 0);
            p->squashY = 0.62f + breathe * 0.03f;
            p->squashX = 1.16f - breathe * 0.02f;
            p->eyeOpen = 0.06f;
            p->zzz = (sinf(t / 2800 * TAU) + 1) / 2;
            p->arms[0].angle = p->arms[1].angle = 52;
            for (int i = 0; i < LEG_COUNT; i++) p->legs[i].scaleY = 0.25f;
            break;
        }
        case A_LAYDOWN:
            p->tilt = 74;
            p->bodyDy = 1.2f;
            p->eyeOpen = 0.4f;
            p->eyeDy = sinp(t, 5400, 0) * 0.2f;
            p->arms[0].angle = 20;
            p->arms[1].angle = -20;
            for (int i = 0; i < LEG_COUNT; i++) p->legs[i].angle = 12;
            break;
    }
}

// Duration of each one-shot in ms; 0 marks a loop.
static const uint16_t ANIM_DUR[A_COUNT] = {
    [A_BLINK] = 150,     [A_LOOKUP] = 1700,   [A_SCAN] = 2400,     [A_TURN] = 240,
    [A_WAVE] = 1500,     [A_CELEBRATE] = 1900, [A_CLONE] = 700,    [A_STUMBLE] = 950,
    [A_VICTORY] = 3400,  [A_CONFETTI] = 2400, [A_BACKFLIP] = 1250, [A_SPIN] = 900,
    [A_STRETCH] = 2000,  [A_FACEPALM] = 1600, [A_SHRUG] = 1300,    [A_NOD] = 1000,
    [A_SALUTE] = 1500,   [A_BOW] = 1600,      [A_PEEK] = 2200,     [A_ROLL] = 1400,
};

// ── Rasteriser ───────────────────────────────────────────────────────────
// 2D affine, SVG convention: x' = a*x + c*y + e, y' = b*x + d*y + f.

typedef struct {
    float a, b, c, d, e, f;
} mat_t;

static mat_t mul(mat_t m, mat_t n) {
    return (mat_t){m.a * n.a + m.c * n.b, m.b * n.a + m.d * n.b, m.a * n.c + m.c * n.d,
                   m.b * n.c + m.d * n.d, m.a * n.e + m.c * n.f + m.e, m.b * n.e + m.d * n.f + m.f};
}
static mat_t translate(float x, float y) { return (mat_t){1, 0, 0, 1, x, y}; }
static mat_t scale(float x, float y) { return (mat_t){x, 0, 0, y, 0, 0}; }
static mat_t rotate(float deg) {
    float r = deg * (PI / 180), c = cosf(r), s = sinf(r);
    return (mat_t){c, s, -s, c, 0, 0};
}
static mat_t rotate_about(float deg, float x, float y) { return mul(translate(x, y), mul(rotate(deg), translate(-x, -y))); }
static mat_t scale_about(float sx, float sy, float x, float y) { return mul(translate(x, y), mul(scale(sx, sy), translate(-x, -y))); }

static uint8_t *g_fb;
static uint32_t g_now;
static uint8_t  g_ctx;   // context window used, percent

void clawd_set_context(uint8_t pct) { g_ctx = pct > 100 ? 100 : pct; }

static void px(int x, int y, bool on) {
    if (x < 0 || x >= CLAWD_W || y < 0 || y >= CLAWD_H) return;
    uint8_t *b = &g_fb[(y >> 3) * CLAWD_W + x];
    if (on)
        *b |= 1 << (y & 7);
    else
        *b &= ~(1 << (y & 7));
}

// Fills rect (x,y,w,h) in local units under `m`, testing each pixel centre
// against the inverse transform.
static void fill_rect(mat_t m, float x, float y, float w, float h, bool on) {
    float det = m.a * m.d - m.b * m.c;
    if (fabsf(det) < 1e-4f) return;

    // Anything under a pixel wide is widened to one, so thin parts (a closed
    // eye, the flag pole) stay visible instead of strobing.
    float sx = sqrtf(m.a * m.a + m.b * m.b), sy = sqrtf(m.c * m.c + m.d * m.d);
    if (w * sx < 1) { x -= (1 / sx - w) / 2; w = 1 / sx; }
    if (h * sy < 1) { y -= (1 / sy - h) / 2; h = 1 / sy; }

    float cx[4] = {x, x + w, x, x + w}, cy[4] = {y, y, y + h, y + h};
    float x0 = 1e9f, x1 = -1e9f, y0 = 1e9f, y1 = -1e9f;
    for (int i = 0; i < 4; i++) {
        float X = m.a * cx[i] + m.c * cy[i] + m.e, Y = m.b * cx[i] + m.d * cy[i] + m.f;
        x0 = fminf(x0, X); x1 = fmaxf(x1, X); y0 = fminf(y0, Y); y1 = fmaxf(y1, Y);
    }
    int ix0 = (int)fmaxf(0, floorf(x0)), ix1 = (int)fminf(CLAWD_W - 1, ceilf(x1));
    int iy0 = (int)fmaxf(0, floorf(y0)), iy1 = (int)fminf(CLAWD_H - 1, ceilf(y1));
    if (ix0 > ix1 || iy0 > iy1) return;

    float ia = m.d / det, ib = -m.b / det, ic = -m.c / det, id = m.a / det;
    float ie = (m.c * m.f - m.d * m.e) / det, iff = (m.b * m.e - m.a * m.f) / det;
    for (int py = iy0; py <= iy1; py++) {
        float Y = py + 0.5f, X = ix0 + 0.5f;
        float u = ia * X + ic * Y + ie, v = ib * X + id * Y + iff;
        for (int pxl = ix0; pxl <= ix1; pxl++, u += ia, v += ib) {
            if (u >= x && u < x + w && v >= y && v < y + h) px(pxl, py, on);
        }
    }
}

static void apply_point(mat_t m, float x, float y, int *ox, int *oy) {
    *ox = (int)floorf(m.a * x + m.c * y + m.e);
    *oy = (int)floorf(m.b * x + m.d * y + m.f);
}

static const struct {
    float x, y, r;
} SPARKS[4] = {{-11, -2, 1.5f}, {10.5f, -4, 1.15f}, {-7, -8.5f, 1.0f}, {7.5f, -9.5f, 1.35f}};

static void draw_rig(const pose_t *p, float cx, float facing) {
    mat_t screen = mul(translate(cx, GROUND_PX), mul(scale(facing * SC, SC), translate(-PIVOT_X, -GROUND_Y)));
    mat_t root   = mul(screen, translate(0, p->bodyDy));
    mat_t rig    = mul(root, mul(rotate_about(p->tilt, PIVOT_X, GROUND_Y),
                                 scale_about(p->scale * p->squashX, p->scale * p->squashY, PIVOT_X, GROUND_Y)));

    // Contact shadow: a dotted ground line that shrinks as the body lifts.
    float lift = fmaxf(0, -p->bodyDy) / 4;
    float rx   = (BODY_W / 2) * (1 - lift * 0.35f) * fabsf(p->squashX) * SC;
    for (int x = (int)(cx - rx); x <= (int)(cx + rx); x++)
        if (x & 1) px(x, (int)GROUND_PX + 1, true);

    for (int i = 0; i < LEG_COUNT; i++) {
        const leg_t *l    = &p->legs[i];
        float        hipX = LEG_XS[i] + LEG_W / 2;
        mat_t        m    = mul(rig, mul(translate(0, l->dy), mul(rotate_about(l->angle, hipX, LEG_Y), scale_about(1, l->scaleY, hipX, LEG_Y))));
        fill_rect(m, LEG_XS[i], LEG_Y, LEG_W, LEG_H, true);
    }

    for (int i = 0; i < 2; i++) {
        const arm_t *a    = &p->arms[i];
        float        side = i == 0 ? -1 : 1;
        float        shx = i == 0 ? BODY_X : BODY_X + BODY_W, shy = NUB_Y + NUB_H / 2;
        mat_t        m = mul(rig, mul(translate(a->dx, a->dy), mul(rotate_about(a->angle * side, shx, shy), scale_about(a->len, 1, shx, shy))));
        fill_rect(m, i == 0 ? BODY_X - NUB_W : BODY_X + BODY_W, NUB_Y, NUB_W, NUB_H, true);
    }

    fill_rect(rig, BODY_X, BODY_Y, BODY_W, BODY_H, true);

    if (p->headband > 0.004f) {
        // Slides down onto the head as it is tied on. On a 1bpp screen the
        // band is a stripe punched out of the body; the tails flutter white.
        mat_t mb = mul(rig, translate(0, (1 - p->headband) * -4.5f));
        fill_rect(mb, BODY_X - 0.6f, 6.5f, BODY_W + 1.2f, 1.15f, false);
        float pvx = BODY_X - 0.6f, pvy = 6.5f + 1.15f / 2;
        for (int i = 0; i < 2; i++) {
            float angle = 10 + i * 16 + p->bandWave * (9 + i * 7);
            fill_rect(mul(mb, rotate_about(angle, pvx, pvy)), pvx - 3.6f, 6.5f, 3.6f, 0.72f, true);
        }
    }

    for (int i = 0; i < 2; i++) {
        float h = EYE_H * fmaxf(p->eyeOpen, 0.06f), w = EYE_W * (1 - p->eyeSquint * 0.45f);
        fill_rect(rig, EYE_XS[i] + p->eyeDx + (EYE_W - w) / 2, EYE_Y + p->eyeDy + (EYE_H - h) / 2, w, h, false);
    }

    if (p->sweat > 0.1f) {
        // Drops run down beside the head; a second one joins when it's bad.
        float ph = (g_now % 900) / 900.0f;
        fill_rect(rig, 21.6f, 4.5f + ph * 5, 1.1f, 1.6f, true);
        if (p->sweat > 0.6f) {
            float ph2 = ((g_now + 450) % 900) / 900.0f;
            fill_rect(rig, 1.3f, 4.5f + ph2 * 5, 1.1f, 1.6f, true);
        }
    }

    if (p->zzz > 0.15f) {
        // Two little z's drifting up off the head.
        static const uint8_t Z[4] = {0xF, 0x4, 0x2, 0xF};
        int zx, zy;
        apply_point(rig, 22, 5 - p->zzz * 2.5f, &zx, &zy);
        for (int r = 0; r < 4; r++)
            for (int c = 0; c < 4; c++)
                if (Z[r] >> (3 - c) & 1) px(zx + c, zy + r - 4, true);
        static const uint8_t z[3] = {0x7, 0x2, 0x7};
        if (p->zzz > 0.6f)
            for (int r = 0; r < 3; r++)
                for (int c = 0; c < 3; c++)
                    if (z[r] >> (2 - c) & 1) px(zx + 6 + c, zy - 9 + r, true);
    }

    if (p->flag > 0.004f) {
        // The pole swings up from behind the right shoulder.
        float ax = BODY_X + BODY_W + NUB_W * 0.4f, ay = NUB_Y + NUB_H / 2;
        float lift_deg = -100 * (1 - p->flag) + p->flagLean, s = 0.5f + p->flag * 0.5f;
        mat_t mf = mul(rig, mul(translate(ax, ay), mul(rotate(lift_deg), scale(s, s))));
        fill_rect(mf, 0.4f, -13.5f, 0.55f, 13.5f, true);
        for (int r = 0; r < 4; r++) {
            float dx = p->flagWave * sinf((r + 1) * 1.15f) * 0.95f;
            fill_rect(mf, 0.95f + dx, -13.5f + r * 1.05f, 4.6f * (1 - r * 0.07f), 1.1f, true);
        }
    }

    if (p->sparkle > 0.004f) {
        for (int i = 0; i < 4; i++) {
            float beat = 0.65f + 0.35f * sinf((p->sparkleWave + i * 0.5f) * PI);
            int   r    = (int)(SPARKS[i].r * p->sparkle * beat * SC + 0.5f);
            int   sx, sy;
            apply_point(root, PIVOT_X + SPARKS[i].x, 11 + SPARKS[i].y, &sx, &sy);
            for (int k = -r; k <= r; k++) {
                px(sx + k, sy, true);
                px(sx, sy + k, true);
            }
        }
    }

    if (p->confettiAmt > 0.2f) {
        for (int i = 0; i < 16; i++) {
            float fr = sinf((i + 1) * 12.9898f) * 43758.5453f, r1 = fr - floorf(fr);
            float f2 = sinf((i + 1) * 78.233f) * 43758.5453f, r2 = f2 - floorf(f2);
            float f3 = sinf((i + 1) * 39.425f) * 24634.6345f, r3 = f3 - floorf(f3);
            float k = p->confetti;
            float x = PIVOT_X + (r1 - 0.5f) * 30 * k;
            float y = BODY_Y + 3 + (-10 - r2 * 16) * k + 26 * k * k;
            int   ox, oy;
            apply_point(screen, x, y, &ox, &oy);
            bool flat = cosf((r3 - 0.5f) * 1080 * k * (PI / 180)) > 0;
            px(ox, oy, true);
            px(ox + flat, oy + !flat, true);
        }
    }
}

// ── Player: current animation, crossfaded against the outgoing one ───────

#define BLEND_MS 180

static struct {
    uint8_t  cur, prev;
    uint32_t cur_start, prev_start, blend_start;
} pl;

static bool is_loop(uint8_t id) { return ANIM_DUR[id] == 0; }

static void play(uint8_t id, uint32_t now) {
    if (id == pl.cur && is_loop(id)) return;
    pl.prev        = pl.cur;
    pl.prev_start  = pl.cur_start;
    pl.cur         = id;
    pl.cur_start   = now;
    pl.blend_start = now;
}

static bool oneshot_running(uint32_t now) { return !is_loop(pl.cur) && now - pl.cur_start < ANIM_DUR[pl.cur]; }

static void current_pose(uint32_t now, pose_t *out) {
    static pose_t a, b;   // static: the render is the deepest stack user (see rules.mk)
    neutral_pose(&b);
    float t = (float)(now - pl.cur_start);
    if (!is_loop(pl.cur)) t = fminf(t, ANIM_DUR[pl.cur]);
    pose_anim(pl.cur, t, &b);

    float k = (float)(now - pl.blend_start) / BLEND_MS;
    if (k < 1) {
        neutral_pose(&a);
        float tp = (float)(now - pl.prev_start);
        if (!is_loop(pl.prev)) tp = fminf(tp, ANIM_DUR[pl.prev]);
        pose_anim(pl.prev, tp, &a);
        lerp_pose(&a, &b, k, out);
    } else {
        *out = b;
    }
}

// ── Director: decides what the mascot does next ──────────────────────────

static uint32_t rng_state = 0x2545F491;
static uint32_t rnd(void) {
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state << 5;
    return rng_state;
}
static uint32_t rnd_range(uint32_t lo, uint32_t hi) { return lo + rnd() % (hi - lo + 1); }

enum { MODE_IDLE, MODE_WORK, MODE_ALERT, MODE_LIMIT, MODE_DONE };

#define WORK_STALE_MS (15UL * 60 * 1000)
#define ALERT_MAX_MS (10UL * 60 * 1000)
#define LIMIT_MAX_MS (30UL * 60 * 1000)
#define DONE_SHOW_MS 45000UL
#define SLEEP_AFTER_MS (12UL * 60 * 1000)
#define SCREEN_OFF_MS (30UL * 60 * 1000)

static struct {
    float    x, facing, target;   // target < 0: roam freely
    uint32_t last_frame;

    // Host state.
    uint8_t  working, waiting, tool, work_anim;
    uint32_t last_event, work_start, alert_at, limit_at, done_at, done_dur;
    bool     limited, done_err;

    // Up to two one-shots queued to play once the current one finishes.
    uint8_t  queue[2], queued;

    // Ambient behaviour.
    uint32_t next_decide, move_until, last_celebrate;
    uint8_t  move_anim;
    bool     asleep;
} d;

static void enqueue(uint8_t id) {
    if (d.queued < 2) d.queue[d.queued++] = id;
}
static bool dequeue(uint8_t *id) {
    if (!d.queued) return false;
    *id = d.queue[0];
    d.queue[0] = d.queue[1];
    d.queued--;
    return true;
}

void clawd_init(uint32_t now) {
    memset(&d, 0, sizeof(d));
    d.x = 64;
    d.facing = 1;
    d.target = -1;
    d.work_anim = A_THINK;
    rng_state ^= now * 2654435761u;
    pl.cur = A_IDLE;
    pl.cur_start = pl.blend_start = now;
    enqueue(A_STRETCH);
}

static uint8_t tool_anim(uint8_t tool) {
    switch (tool) {
        case CLAWD_TOOL_EDIT: return A_TYPING;
        case CLAWD_TOOL_BASH: return A_DIGGING;
        case CLAWD_TOOL_READ: return A_SEARCHING;
        case CLAWD_TOOL_WEB: return A_BROADCASTING;
        default: return A_THINK;
    }
}

void clawd_event(uint32_t now, uint8_t event, uint8_t tool, uint8_t working, uint8_t waiting) {
    bool was_working = d.working > 0;
    d.working        = working;
    d.waiting        = waiting;
    d.last_event     = now;
    if (waiting) d.alert_at = d.alert_at ? d.alert_at : now;
    else d.alert_at = 0;

    switch (event) {
        case CLAWD_EV_SESSION_START:
            if (!working) enqueue(A_STRETCH);
            break;
        case CLAWD_EV_PROMPT:
            if (!was_working || !d.work_start) d.work_start = now;
            d.work_anim = A_HEADBAND;
            d.limited   = false;
            d.done_at   = 0;
            break;
        case CLAWD_EV_TOOL:
            d.tool      = tool;
            d.work_anim = tool_anim(tool);
            break;
        case CLAWD_EV_TOOL_FAILED:
            enqueue(A_FACEPALM);
            break;
        case CLAWD_EV_STOP:
        case CLAWD_EV_STOP_FAILED:
            if (!working) {
                d.done_at  = now;
                d.done_dur = d.work_start ? now - d.work_start : 0;
                d.done_err = event == CLAWD_EV_STOP_FAILED;
                d.work_start = 0;
                d.queued = 0;
                if (d.done_err) {
                    enqueue(A_STUMBLE);
                    enqueue(A_SHRUG);
                } else {
                    enqueue(A_VICTORY);
                    enqueue(A_WAVE);
                }
            } else {
                enqueue(A_NOD);   // one session finished, others still going
            }
            break;
        case CLAWD_EV_RATE_LIMIT:
            d.limited  = true;
            d.limit_at = now;
            break;
        case CLAWD_EV_COMPACT:
            d.work_anim = A_MEDITATE;
            break;
        case CLAWD_EV_SUBAGENT:
            enqueue(A_CLONE);
            break;
        case CLAWD_EV_SESSION_END:
            if (!working && !waiting) {
                d.done_at = 0;
                d.work_start = 0;
                enqueue(A_BOW);
            }
            break;
    }
}

static uint8_t mode(uint32_t now) {
    if (d.waiting && now - d.alert_at < ALERT_MAX_MS) return MODE_ALERT;
    if (d.limited && now - d.limit_at < LIMIT_MAX_MS) return MODE_LIMIT;
    if (d.working && now - d.last_event < WORK_STALE_MS) return MODE_WORK;
    if (d.done_at && now - d.done_at < DONE_SHOW_MS) return MODE_DONE;
    return MODE_IDLE;
}

// Ambient pools, by how long the keyboard has been left alone. Repeats are
// weights: walking and standing should dominate, tricks should be a treat.
static const uint8_t POOL_ACTIVE[] = {A_WALK, A_WALK, A_WALK, A_WALK, A_WALK, A_IDLE, A_IDLE, A_IDLE, A_IDLE,
                                      A_BLINK, A_BLINK, A_BLINK, A_LOOKUP, A_SCAN, A_WAVE, A_NOD, A_PEEK, A_SHRUG};
static const uint8_t POOL_BORED[]  = {A_WALK, A_WALK, A_WALK, A_RUN, A_IDLE, A_IDLE, A_SPIN, A_SPIN, A_DANCE, A_DANCE,
                                      A_BACKFLIP, A_ROLL, A_STRETCH, A_BOW, A_PEEK, A_CELEBRATE, A_CONFETTI,
                                      A_SHRUG, A_LOOKUP, A_SALUTE, A_BLINK};
static const uint8_t POOL_TIRED[]  = {A_IDLE, A_IDLE, A_CURL, A_CURL, A_CURL, A_LAYDOWN, A_LAYDOWN, A_STRETCH,
                                      A_BLINK, A_BLINK, A_WALK};

#define PICK(pool) pool[rnd() % sizeof(pool)]

static void start_moving(uint8_t anim, uint32_t now, uint32_t ms) {
    d.move_anim  = anim;
    d.move_until = now + ms;
    // Head away from whichever wall is closer, otherwise pick at random.
    if (d.x < X_MIN + 15) d.facing = 1;
    else if (d.x > X_MAX - 15) d.facing = -1;
    else if (rnd() & 1) d.facing = -d.facing;
    play(anim, now);
}

static void ambient(const clawd_input_t *in, uint32_t now) {
    d.target    = -1;
    bool typing = in->idle_ms < 1500 && in->wpm > 0;

    if (in->idle_ms >= SLEEP_AFTER_MS) {
        if (!oneshot_running(now)) play(A_SLEEP, now);
        d.asleep     = true;
        d.move_until = 0;
        return;
    }
    if (d.asleep) {
        // Woken up by a keypress: stretch before anything else.
        d.asleep = false;
        d.queued = 0;
        enqueue(A_STRETCH);
    }

    uint8_t next;
    if (oneshot_running(now)) return;
    if (dequeue(&next)) {
        d.move_until = 0;
        play(next, now);
        d.next_decide = now + ANIM_DUR[next];
        return;
    }

    if (typing) {
        // Hammers away along with you, and cheers if you get going.
        d.move_until = 0;
        if (in->wpm >= 80 && now - d.last_celebrate > 30000) {
            d.last_celebrate = now;
            play(A_CELEBRATE, now);
        } else {
            play(A_TYPING, now);
        }
        d.next_decide = now + 800;
        return;
    }

    if (d.move_until > now) return;   // still walking; edges are handled in step()
    if (now < d.next_decide) {
        if (pl.cur == A_WALK || pl.cur == A_RUN || pl.cur == A_TYPING || pl.cur == A_TURN) play(A_IDLE, now);
        return;
    }

    uint8_t choice;
    if (in->idle_ms < 60000) choice = PICK(POOL_ACTIVE);
    else if (in->idle_ms < 4UL * 60 * 1000) choice = PICK(POOL_BORED);
    else choice = PICK(POOL_TIRED);

    switch (choice) {
        case A_WALK: start_moving(A_WALK, now, rnd_range(2000, 6000)); d.next_decide = d.move_until; break;
        case A_RUN: start_moving(A_RUN, now, rnd_range(1200, 2500)); d.next_decide = d.move_until; break;
        case A_IDLE: play(A_IDLE, now); d.next_decide = now + rnd_range(1500, 4500); break;
        case A_DANCE: play(A_DANCE, now); d.next_decide = now + rnd_range(2500, 5000); break;
        case A_CURL:
        case A_LAYDOWN: play(choice, now); d.next_decide = now + rnd_range(6000, 15000); break;
        default:
            // Bored means restless: tricks come back to back.
            play(choice, now);
            d.next_decide = now + ANIM_DUR[choice] + (in->idle_ms < 60000 ? rnd_range(500, 1500) : rnd_range(100, 500));
            break;
    }
}

static void reporting(uint8_t m, uint32_t now) {
    // Report from the station on the right, turned toward the text.
    float dist = STATION_X - d.x;
    d.target   = STATION_X;
    if (fabsf(dist) > 0.5f && !oneshot_running(now)) {
        d.facing = dist > 0 ? 1 : -1;
        play(A_RUN, now);
        d.move_anim  = A_RUN;
        d.move_until = now + 100;
        return;
    }
    d.move_until = 0;
    if (oneshot_running(now)) return;
    d.facing = -1;

    uint8_t next;
    if (dequeue(&next)) {
        play(next, now);
        return;
    }
    switch (m) {
        case MODE_ALERT: play(A_ALERT, now); break;
        case MODE_LIMIT: play(A_LIMITHIT, now); break;
        case MODE_WORK: play(d.work_anim, now); break;
        default: play(A_IDLE, now); break;
    }
}

// Moves the mascot and turns it at the walls.
static void step(uint32_t now, float dt) {
    if (d.move_until <= now || (pl.cur != A_WALK && pl.cur != A_RUN)) return;
    float speed = pl.cur == A_RUN ? 48 : 14;   // px/s
    float before = d.x;
    d.x += d.facing * speed * dt;
    if (d.target >= 0 && (before - d.target) * (d.x - d.target) <= 0) {
        d.x = d.target;
        d.move_until = 0;
        return;
    }
    if (d.x < X_MIN || d.x > X_MAX) {
        d.x      = clampf(d.x, X_MIN, X_MAX);
        d.facing = -d.facing;
        play(A_TURN, now);
        enqueue(d.move_anim);
    }
}

static void fmt_dur(char *buf, size_t n, uint32_t ms) {
    uint32_t s = ms / 1000;
    if (s >= 3600)
        snprintf(buf, n, "%luh%02lu", (unsigned long)(s / 3600), (unsigned long)(s / 60 % 60));
    else
        snprintf(buf, n, "%lu:%02lu", (unsigned long)(s / 60), (unsigned long)(s % 60));
}

static void add_line(clawd_frame_t *out, uint8_t col, uint8_t row, const char *s) {
    if (out->count >= 3) return;
    clawd_text_t *t = &out->lines[out->count++];
    t->col = col;
    t->row = row;
    snprintf(t->str, sizeof(t->str), "%s", s);
}

static const char *work_label(void) {
    switch (pl.cur) {
        case A_HEADBAND: return "on it!";
        case A_TYPING: return "editing";
        case A_DIGGING: return "running";
        case A_SEARCHING: return "reading";
        case A_BROADCASTING: return "browsing";
        case A_MEDITATE: return "compacting";
        case A_CLONE: return "delegating";
        case A_FACEPALM: return "oops!";
        case A_RUN: return "on my way";
        default: return "thinking";
    }
}

void clawd_render(const clawd_input_t *in, uint8_t *fb, clawd_frame_t *out) {
    uint32_t now = in->now;
    float    dt  = d.last_frame ? (now - d.last_frame) / 1000.0f : 0;
    d.last_frame = now;
    if (dt > 0.2f) dt = 0.2f;

    memset(out, 0, sizeof(*out));
    uint8_t m = mode(now);
    if (m == MODE_IDLE) {
        if (d.working || d.waiting) {
            // Stale: the host stopped talking mid-turn (an interrupted turn
            // sends no Stop). Forget it rather than strand the mascot.
            d.working = d.waiting = 0;
        }
        ambient(in, now);
    } else {
        d.asleep = false;
        reporting(m, now);
    }
    step(now, dt);

    out->display_on = !(m == MODE_IDLE && in->idle_ms >= SCREEN_OFF_MS);

    char buf[22], t[12];
    switch (m) {
        case MODE_WORK:
            add_line(out, 0, 1, work_label());
            fmt_dur(t, sizeof(t), d.work_start ? now - d.work_start : 0);
            if (d.working > 1)
                snprintf(buf, sizeof(buf), "%s  x%u", t, d.working);
            else
                snprintf(buf, sizeof(buf), "%s", t);
            add_line(out, 0, 2, buf);
            break;
        case MODE_ALERT:
            if ((now / 500) & 1) add_line(out, 0, 1, "NEEDS YOU!");
            add_line(out, 0, 2, "permission");
            break;
        case MODE_LIMIT:
            add_line(out, 0, 1, "limit hit");
            add_line(out, 0, 2, "take a break");
            break;
        case MODE_DONE:
            add_line(out, 0, 1, d.done_err ? "error :(" : "done!");
            if (d.done_dur) {
                fmt_dur(t, sizeof(t), d.done_dur);
                snprintf(buf, sizeof(buf), "took %s", t);
                add_line(out, 0, 2, buf);
            }
            break;
    }

    memset(fb, 0, CLAWD_FB_SIZE);
    if (!out->display_on) return;
    g_fb = fb;
    static pose_t p;
    current_pose(now, &p);

    // A filling context window visibly bloats the body from 40% on, and it
    // starts to sweat past 75%: time to /compact (port of contextFull()).
    float bloat = clampf((g_ctx - 40) / 55.0f, 0, 1);
    p.squashX *= 1 + bloat * 0.34f;
    p.squashY *= 1 + bloat * 0.12f;
    p.sweat = fmaxf(p.sweat, clampf((g_ctx - 75) / 20.0f, 0, 1));

    g_now = now;
    draw_rig(&p, d.x, d.facing);
}
