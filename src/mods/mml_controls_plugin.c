#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "cpu_state.h"
#include "mod_plugins.h"

/* Mega Man's object. +0x56 is his logical heading in 4096ths of a turn (the
 * camera follows it) and +0x116 an input offset. Movement is
 * `yaw -= off; MoveObject(); yaw += off` (0x80041E90) and the model matrix is
 * RotMatrix(yaw - off) (0x800325D4), so writing only `off` steers the walk and
 * the model without disturbing the heading the camera tracks. The game
 * quantises off to {0, 0x400, 0x800, 0xC00} from the d-pad; we replace it with
 * the exact left-stick angle. The right stick adds to yaw (a camera orbit the
 * camera follows) the way L1/R1 do, and drives R2 + d-pad "free look". */
#define PLAYER_OBJ   0x800B51B0u
#define OBJ_YAW      (PLAYER_OBJ + 0x56u)
#define OBJ_OFF      (PLAYER_OBJ + 0x116u)
#define INPUT_FLAGS  0x800B526Du   /* bit 6: input disabled (cutscene/event) */
#define MOVE_WRAPPER 0x80041E90u
#define MENU_STATE   0x80098864u   /* nonzero on the title screen and in the pause menu */

#define DEADZONE     0.30f
#define LOOK_ENGAGE  0.55f
#define TURN_PER_FRAME 22.0f       /* yaw units/frame at full deflection (original max ~13) */
#define STICK_CURVE  1.6f          /* >1: finer control near the centre */

#define PAD_SELECT   (1u << 0)
#define PAD_UP       (1u << 4)
#define PAD_RIGHT    (1u << 5)
#define PAD_DOWN     (1u << 6)
#define PAD_LEFT     (1u << 7)
#define PAD_R2       (1u << 9)

static int g_gameplay;           /* player model is drawn and input is enabled */
static int g_stick_on;           /* left stick deflected at the last pad sample */
static int g_stick_off;          /* wanted input offset in 4096ths of a turn */
static int g_overriding;         /* we wrote off and must clear it on release */
static float g_turn;             /* right stick X, -1..1 after dead zone */
static unsigned g_samples, g_samples_seen, g_quiet_vblanks;

static float dead(float v) {
    const float a = fabsf(v);
    if (a < DEADZONE) return 0.0f;
    return copysignf(powf((a - DEADZONE) / (1.0f - DEADZONE), STICK_CURVE), v);
}

/* Buttons are active low. Outside gameplay the stock pad is left alone so
 * menus and dialogue keep the normal stick-as-d-pad behaviour. */
static int mml_pad_transform(const PSXModPadFrame *f, PSXModPadOutput *out) {
    g_samples++;
    const float sx = ((int)f->lx - 128) / 128.0f, sy = (128 - (int)f->ly) / 128.0f;
    const float rx = ((int)f->rx - 128) / 128.0f, ry = (128 - (int)f->ry) / 128.0f;
    g_stick_on = sqrtf(sx * sx + sy * sy) > DEADZONE;
    g_turn = dead(rx);
    if (!g_gameplay) {
        g_stick_on = 0; g_turn = 0.0f;
        return 0;
    }
    int changed = 0;
    if (g_stick_on) {
        /* Left = +0x400, right = -0x400, down = 0x800, up = 0. */
        g_stick_off = (int)lroundf(atan2f(-sx, sy) * (4096.0f / 6.2831853f)) & 0xFFF;
        out->buttons |= (PAD_UP | PAD_RIGHT | PAD_DOWN | PAD_LEFT);
        out->buttons &= ~PAD_UP;                        /* hold forward only */
        out->buttons |= PAD_R2;     /* R2 (look stance) would freeze walking: release it while moving */
        changed = 1;
    } else if (fabsf(ry) > LOOK_ENGAGE && fabsf(rx) < fabsf(ry)) {
        /* Standing still: right stick up/down is the game's free look. */
        out->buttons |= (PAD_UP | PAD_DOWN);
        out->buttons &= ~(PAD_R2 | (ry > 0 ? PAD_UP : PAD_DOWN));
        changed = 1;
    }
    return changed;
}

static void on_move(struct CPUState *cpu, uint32_t address) {
    (void)address;
    if (cpu->gpr[4] != PLAYER_OBJ || !g_gameplay) return;
    if (g_stick_on) { psx_mod_write_half(OBJ_OFF, (uint16_t)g_stick_off); g_overriding = 1; }
}

/* Per VBlank: derive "in gameplay" (not the title/pause menu, input not
 * disabled by a cutscene), drop stale stick state if the pad is no longer
 * sampled, and apply the right stick's camera turn to the heading the camera
 * follows. The add commutes with the move wrapper's temporary yaw -= off. */
static void tick(void) {
    g_gameplay = psx_mod_read_word(MENU_STATE) == 0 && !(psx_mod_read_byte(INPUT_FLAGS) & 0x40);
    /* The pad is read at the game's rate (30 Hz) so allow a few quiet VBlanks. */
    g_quiet_vblanks = (g_samples == g_samples_seen) ? g_quiet_vblanks + 1 : 0;
    g_samples_seen = g_samples;
    if (g_quiet_vblanks > 6) { g_stick_on = 0; g_turn = 0.0f; }
    const float turn = g_turn * TURN_PER_FRAME;
    if (g_gameplay && turn != 0.0f) {
        const int yaw = (int16_t)psx_mod_read_half(OBJ_YAW) & 0xFFF;
        psx_mod_write_half(OBJ_YAW, (uint16_t)((yaw + (int)lroundf(turn)) & 0xFFF));
    }
    if (!g_stick_on && g_overriding && g_gameplay) {
        psx_mod_write_half(OBJ_OFF, 0);
        g_overriding = 0;
    }
}

static void free_move_activate(void) {
    static const PSXModPadTransform t = {
        sizeof(PSXModPadTransform),
        PSX_MOD_PAD_TYPE_BIT(PSX_MOD_PAD_DIGITAL),
        PSX_MOD_PAD_DIGITAL,
        mml_pad_transform
    };
    (void)psx_mod_set_pad_transform(0, &t);
}

PSX_MOD_CONSTRUCTOR(mml_register_controls_plugin) {
    (void)psx_mod_register_activation_plugin("mml.controls.free-move", free_move_activate);
    (void)psx_mod_register_vblank_plugin("mml.controls.free-move", tick);
    (void)psx_mod_register_function_entry_plugin("mml.controls.free-move", MOVE_WRAPPER, on_move);
    }
