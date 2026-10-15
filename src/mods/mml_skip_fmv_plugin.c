#include "mod_plugins.h"

/* The runtime's accelerated, muted movie playback; the game still owns teardown. */
static void mml_skip_fmvs_activate(void) {
    (void)psx_mod_set_auto_skip_fmv(1);
}

PSX_MOD_CONSTRUCTOR(mml_register_skip_fmv_plugin) {
    (void)psx_mod_register_activation_plugin(
        "mml.fmv.skip", mml_skip_fmvs_activate);
}
