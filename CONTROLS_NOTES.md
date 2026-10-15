# Controls RE notes (Mega Man Legends, SLUS-00603)

- Pad buffer (BIOS InitPAD, pad 1): 0x800C0C00 (status, id, buttons u16 active-low at +2).
  Pad 2: 0x800C0D68. InitPAD is called from 0x80011F04 (main EXE).
- Pad bit order (active low): select0 l3 1 r3 2 start3 up4 right5 down6 left7 l2 8 r2 9 l1 10 r1 11 tri12 circle13 cross14 square15.
- Framework hooks available: psx_mod_set_pad_transform, psx_mod_set_controller_source,
  psx_mod_set_local_mouse_policy, psx_mod_read_local_pad_sticks (mod_plugins.h).
- tools/dbg.py drives the debug build (--no-launcher --headless --debug-port 4370, build with -DPSX_DEBUG_TOOLS=ON).
- Unknown yet: who consumes 0x800C0C02 (movement/turn code), camera yaw address, heading address.

## Findings (analog movement)
- Pad struct (per pad, 0x168 bytes): +0 raw buffer, +0x26 held (active high), +0x28 prev, +0x2A pressed, +0x2C released, +0x30 analog bytes.
  Per-frame update: 0x800121AC. Global input copy at 0x800B52CC (0x8003C08C).
- Player object = 0x800B51B0. +0x56 yaw, +0x116 input offset, +0x1A8 rot matrix, +0x1C/+0x20/+0x24 32-bit pos.
- MoveObject 0x8002FEA4 (a0=obj, a1=local forward step, 512 when running): rotates by yaw, adds to position.
- Matrix builder 0x800325D4 uses RotMatrix(yaw - off).
- The game sets yaw = base - off with off in {0,0x400,0x800,0xC00} (up/left/down/right). base = camera-relative heading.
- Plugin src/mods/mml_controls_plugin.c: pad transform holds only "up" while stick deflected and hooks
  MoveObject + builder to write off = stick angle, yaw = base - off. Hooks listed in game.toml mod_function_entry_funcs.
- Debug: tools/dbg.py (launch, to_gameplay ~3 min of Cross presses, save/load savestate slot).
