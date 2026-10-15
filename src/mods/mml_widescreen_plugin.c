#include "cpu_state.h"
#include "mod_plugins.h"
#include "gpu.h"

/* The parallax/sky layer manager (0x8002314C) emits 32x32 tiles per row from
 * x = -(scroll mod 32) while x < 320. game.toml widens the right bound
 * (slti_sites); the first tile of each row is moved left here so the revealed
 * band on the left is covered too. At 0x80023528 (`beq v0,zero`) a0 is the
 * first x, a2 the first map column and t0 the map width in tiles. */
#define SKY_ROW_SITE  0x80023528u
#define SKY_ROW_INSTR 0x1040001Cu

static void sky_extend_left(struct CPUState *cpu, uint32_t address) {
    (void)address;
    if (!ws_native_wide_active()) return;
    const int tiles = (gpu_ws_configured_x_reveal() + 31) / 32;
    const int width = (int)cpu->gpr[8];
    if (tiles <= 0 || width <= 0) return;
    int col = ((int)cpu->gpr[6] - tiles) % width;
    if (col < 0) col += width;
    cpu->gpr[4] -= 32u * (uint32_t)tiles;
    cpu->gpr[6] = (uint32_t)col;
}

/* Terrain cell visibility (0x80021834): a cell is kept while |x| <= 0.414*z +
 * radius in camera space, where 0.414 = tan(22.5 deg) comes from
 * `106*(z>>2) >> 6`. Scaling the pre-shift product by the wide/4:3 width ratio
 * widens that horizontal half-angle to the revealed view. The caller's guard
 * keeps cells just outside the screen from popping in. */
#define CONE_SITE  0x800218D8u   /* sra v0,v0,6 */
#define CONE_INSTR 0x00021183u
#define CONE_GUARD 96

static void widen_cone(struct CPUState *cpu, uint32_t address) {
    (void)address;
    if (!ws_native_wide_active()) return;
    const int reveal = gpu_ws_configured_x_reveal();
    if (reveal <= 0) return;
    cpu->gpr[2] = (uint32_t)(((int64_t)(int32_t)cpu->gpr[2] * (320 + 2 * (reveal + CONE_GUARD))) / 320);
}

static void mml_widescreen_16_9_activate(void) {
    psx_mod_set_native_wide_projection_correction(1);
    (void)psx_mod_set_fixed_display_aspect(16u, 9u);
}

static void mml_widescreen_21_9_activate(void) {
    psx_mod_set_native_wide_projection_correction(1);
    (void)psx_mod_set_fixed_display_aspect(21u, 9u);
}

PSX_MOD_CONSTRUCTOR(mml_register_widescreen_plugins) {
    (void)psx_mod_register_activation_plugin(
        "mml.widescreen.16-9", mml_widescreen_16_9_activate);
    (void)psx_mod_register_activation_plugin(
        "mml.widescreen.21-9", mml_widescreen_21_9_activate);
    (void)psx_mod_register_instruction_plugin(
        "mml.widescreen.16-9", SKY_ROW_SITE, SKY_ROW_INSTR, sky_extend_left);
    (void)psx_mod_register_instruction_plugin(
        "mml.widescreen.21-9", SKY_ROW_SITE, SKY_ROW_INSTR, sky_extend_left);
    (void)psx_mod_register_instruction_plugin(
        "mml.widescreen.16-9", CONE_SITE, CONE_INSTR, widen_cone);
    (void)psx_mod_register_instruction_plugin(
        "mml.widescreen.21-9", CONE_SITE, CONE_INSTR, widen_cone);
}
