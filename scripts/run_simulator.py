#!/usr/bin/env python3
import os
import sys
import subprocess
import shutil

# Ensure UTF-8 encoding
os.environ["PYTHONIOENCODING"] = "utf-8"
if sys.platform == "win32":
    try:
        subprocess.run(["chcp", "65001"], shell=True, check=False, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    except Exception:
        pass

def main():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    src_dir = os.path.dirname(script_dir)
    os.chdir(src_dir)

    print("==========================================================")
    print(" BookPoint 2.0.0 — Native SDL2 Simulator Launcher")
    print(" Working directory:", src_dir)
    print("==========================================================")

    # 1. Ensure SDL2 dependencies
    import setup_sdl2
    sdl2_dir = os.path.join(src_dir, "third_party", "sdl2")
    if not setup_sdl2.ensure_sdl2(sdl2_dir):
        print("ERROR: Failed to prepare SDL2 library.", file=sys.stderr)
        return 1

    # 2. Build simulator environment if needed
    build_dir = os.path.join(src_dir, ".pio", "build", "simulator")
    prog_exe = os.path.join(build_dir, "program.exe")
    if sys.platform != "win32":
        prog_exe = os.path.join(build_dir, "program")

    skip_build = "--skip-build" in sys.argv
    extra_args = [a for a in sys.argv[1:] if a != "--skip-build"]

    if not skip_build or not os.path.isfile(prog_exe):
        print("\n[Simulator] Compiling simulator target [env:simulator]...")
        cmd = ["pio", "run", "-e", "simulator"]
        res = subprocess.run(cmd, cwd=src_dir)
        if res.returncode != 0:
            print("ERROR: Compilation failed with exit code", res.returncode, file=sys.stderr)
            return res.returncode

    if not os.path.isfile(prog_exe):
        print("ERROR: Simulator binary not found at", prog_exe, file=sys.stderr)
        return 1

    # 3. Ensure SDL2.dll is adjacent to program.exe and in current dir
    if sys.platform == "win32":
        dll_src = os.path.join(sdl2_dir, "bin", "SDL2.dll")
        dll_dst = os.path.join(build_dir, "SDL2.dll")
        if os.path.isfile(dll_src):
            shutil.copy2(dll_src, dll_dst)
            shutil.copy2(dll_src, os.path.join(src_dir, "SDL2.dll"))

    # 4. Launch simulator
    print("\n[Simulator] Launching BookPoint 2.0.0 Simulator...")
    print("[Simulator] Window: 480x800 E-Ink (Nearest-Neighbor)")
    print("[Simulator] Controls: Left-click=Tap/Drag, Escape/H=Home, Space/Enter=Select, Up/Down=Page, P=Power")
    print("----------------------------------------------------------\n")
    
    cmd = [prog_exe] + extra_args
    proc = subprocess.run(cmd, cwd=src_dir)
    return proc.returncode

if __name__ == "__main__":
    sys.exit(main())
