import socket, json, sys, time

def cmd(**kw):
    s = socket.create_connection(("127.0.0.1", 4370)); s.settimeout(30)
    s.sendall((json.dumps(kw) + "\n").encode())
    buf = b""
    while b"\n" not in buf:
        d = s.recv(1 << 20)
        if not d: break
        buf += d
    s.close()
    return json.loads(buf.split(b"\n", 1)[0])

BTN = dict(select=0, l3=1, r3=2, start=3, up=4, right=5, down=6, left=7, l2=8, r2=9, l1=10, r1=11, tri=12, circle=13, cross=14, square=15)

def mask(*names):
    m = 0xFFFF
    for n in names: m &= ~(1 << BTN[n])
    return m

def press(*names, frames=6):
    cmd(cmd="set_input", buttons=mask(*names), frames=frames)
    time.sleep(frames / 60 + 0.15)
    cmd(cmd="clear_input")

def shot(path):
    return cmd(cmd="screenshot_file", path=path)

if __name__ == "__main__":
    print(cmd(cmd="ping"))

import struct
def ram(addr=0x80000000, n=0x200000):
    r = cmd(cmd="read_ram", addr=hex(addr), len=n)
    return bytes.fromhex(r["hex"] if "hex" in r else r["data"])
def words(b): return struct.unpack("<%dI" % (len(b)//4), b)
def stepn(n):
    return cmd(cmd="step", n=n)

import subprocess, os
def launch(cards="dbg_cards"):
    exe = os.path.abspath("build-debug/MegaManLegends_Recompiled.exe")
    log = open(os.path.abspath("dbg_run.log"), "w")
    env = dict(os.environ, MML_TRACE="1")
    subprocess.Popen([exe, "--no-launcher", "--headless", "--debug-port", "4370", "--memcard-dir", os.path.abspath(cards)],
                     cwd=os.path.dirname(exe), creationflags=0x08000000, stdout=log, stderr=log, env=env)
    for _ in range(60):
        try:
            cmd(cmd="ping"); return
        except Exception: time.sleep(1)
def boot_to_game():
    time.sleep(10)
    for _ in range(8):
        press("start", frames=8); time.sleep(3)
    for _ in range(5): press("down", frames=5); time.sleep(0.4)
    press("cross", frames=6); time.sleep(3)

def to_gameplay():
    """Skip prologue by spamming Cross until control is returned (~2 min)."""
    boot_to_game()
    for _ in range(80):
        press("cross", frames=6); time.sleep(1.3)
def save(slot=1): return cmd(cmd="savestate", op="save", slot=slot)
def load(slot=1): return cmd(cmd="savestate", op="load", slot=slot)

def new_game():
    """Title -> NEW GAME -> prologue; leaves the game running (HUD appears after ~110 Cross presses)."""
    time.sleep(10)
    for _ in range(8): press("start", frames=8); time.sleep(3)
    for _ in range(110): press("cross", frames=6); time.sleep(1.2)

def continue_game():
    """Fresh process -> title -> CONTINUE -> card 1 -> slot 1 (needs a copy of the user's card in dbg_cards2)."""
    time.sleep(10)
    for _ in range(4): press("start", frames=8); time.sleep(3)
    press("down", frames=5); time.sleep(1.0)
    for _ in range(5): press("cross", frames=6); time.sleep(3.5)
    time.sleep(6)
