#include "mod_plugins.h"

/* Presents above the guest's 30 Hz game cadence; game logic, timers and audio
 * keep their stock rate. Needs the OpenGL renderer. */
static void set_rate(uint32_t fps, uint32_t blend) {
    (void)psx_mod_set_frame_interpolation_source(PSX_MOD_FRAME_SOURCE_FLIP);
    (void)psx_mod_set_frame_interpolation_blend(blend);
    (void)psx_mod_set_frame_interpolation(fps);
}

#define RATE(name, fps, blend) \
    static void name(void) { set_rate((fps), (blend)); }
RATE(r60_blend,  60,  PSX_MOD_FRAME_INTERPOLATION_LINEAR)
RATE(r60_hold,   60,  PSX_MOD_FRAME_INTERPOLATION_HOLD)
RATE(r120_blend, 120, PSX_MOD_FRAME_INTERPOLATION_LINEAR)
RATE(r120_hold,  120, PSX_MOD_FRAME_INTERPOLATION_HOLD)
RATE(rdisp_blend, 0,  PSX_MOD_FRAME_INTERPOLATION_LINEAR)
RATE(rdisp_hold,  0,  PSX_MOD_FRAME_INTERPOLATION_HOLD)

PSX_MOD_CONSTRUCTOR(mml_register_frame_rate_plugins) {
    (void)psx_mod_register_activation_plugin("mml.framerate.60.blend", r60_blend);
    (void)psx_mod_register_activation_plugin("mml.framerate.60.hold", r60_hold);
    (void)psx_mod_register_activation_plugin("mml.framerate.120.blend", r120_blend);
    (void)psx_mod_register_activation_plugin("mml.framerate.120.hold", r120_hold);
    (void)psx_mod_register_activation_plugin("mml.framerate.display.blend", rdisp_blend);
    (void)psx_mod_register_activation_plugin("mml.framerate.display.hold", rdisp_hold);
}
