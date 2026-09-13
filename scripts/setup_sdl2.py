#!/usr/bin/env python3
import os
import sys
import urllib.request
import zipfile
import shutil

SDL2_ZIP_URL = "https://github.com/libsdl-org/SDL/releases/download/release-2.30.12/SDL2-devel-2.30.12-mingw.zip"

def ensure_sdl2(target_dir):
    sdl_header = os.path.join(target_dir, "include", "SDL2", "SDL.h")
    sdl_dll = os.path.join(target_dir, "bin", "SDL2.dll")
    sdl_lib = os.path.join(target_dir, "lib", "libSDL2.a")

    if os.path.isfile(sdl_header) and os.path.isfile(sdl_dll) and os.path.isfile(sdl_lib):
        print(f"[setup_sdl2] SDL2 MinGW already present in {target_dir}")
        return True

    print(f"[setup_sdl2] SDL2 not found. Downloading from {SDL2_ZIP_URL}...")
    zip_path = os.path.join(target_dir, "sdl2_temp.zip")
    os.makedirs(target_dir, exist_ok=True)

    try:
        urllib.request.urlretrieve(SDL2_ZIP_URL, zip_path)
        print("[setup_sdl2] Download complete. Extracting x86_64-w64-mingw32...")
        
        with zipfile.ZipFile(zip_path, 'r') as z:
            prefix = None
            for name in z.namelist():
                if "x86_64-w64-mingw32" in name:
                    parts = name.split("x86_64-w64-mingw32/")
                    if len(parts) == 2 and parts[1]:
                        rel_path = parts[1]
                        dest_path = os.path.join(target_dir, rel_path)
                        if name.endswith("/"):
                            os.makedirs(dest_path, exist_ok=True)
                        else:
                            os.makedirs(os.path.dirname(dest_path), exist_ok=True)
                            with z.open(name) as src, open(dest_path, "wb") as dst:
                                shutil.copyfileobj(src, dst)
        
        if os.path.exists(zip_path):
            os.remove(zip_path)
            
        print(f"[setup_sdl2] Successfully installed SDL2 to {target_dir}")
        return True
    except Exception as ex:
        print(f"[setup_sdl2] ERROR: Failed to install SDL2: {ex}", file=sys.stderr)
        if os.path.exists(zip_path):
            try:
                os.remove(zip_path)
            except Exception:
                pass
        return False

# When run as PlatformIO SCons script
try:
    from SCons.Script import Import
    Import("env")
    
    project_dir = env.subst("$PROJECT_DIR")
    sdl2_dir = os.path.join(project_dir, "third_party", "sdl2")
    
    ensure_sdl2(sdl2_dir)
    
    inc_dir = os.path.join(sdl2_dir, "include")
    inc_sdl2_dir = os.path.join(sdl2_dir, "include", "SDL2")
    lib_dir = os.path.join(sdl2_dir, "lib")
    sim_dir = os.path.join(project_dir, "src", "simulator")
    freertos_dir = os.path.join(project_dir, "src", "simulator", "freertos")
    
    env.Append(
        CPPPATH=[
            inc_dir,
            inc_sdl2_dir,
            sim_dir,
            freertos_dir,
        ],
        LIBPATH=[lib_dir],
        LIBS=[
            "mingw32",
            "SDL2main",
            "SDL2",
            "m",
            "user32",
            "gdi32",
            "winmm",
            "imm32",
            "ole32",
            "oleaut32",
            "shell32",
            "version",
        ]
    )

    dll_src = os.path.join(sdl2_dir, "bin", "SDL2.dll")
    
    def copy_dll(target, source, env):
        build_dir = env.subst("$BUILD_DIR")
        os.makedirs(build_dir, exist_ok=True)
        dll_dst = os.path.join(build_dir, "SDL2.dll")
        if os.path.isfile(dll_src):
            shutil.copy2(dll_src, dll_dst)
            print(f"[setup_sdl2] Copied SDL2.dll to {dll_dst}")
            # Also copy next to project root if needed
            shutil.copy2(dll_src, os.path.join(project_dir, "SDL2.dll"))

    env.AddPostAction("$PROGPATH", copy_dll)

except ImportError:
    # Standalone execution
    if __name__ == "__main__":
        base_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
        target_dir = os.path.join(base_dir, "third_party", "sdl2")
        ensure_sdl2(target_dir)
