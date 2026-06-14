# OrcaSlicer-bambulab — Windows 11 Build Guide

> **Session handoff (2026-06-14)**  
> Mac build succeeded; app launches. Bambu network plugin still problematic on macOS 27 beta (Lima bridge).  
> **Branch:** `cursor/filament-manager-ams-sync-phase2`  
> **Remote:** https://github.com/shawvibes/OrcaSlicer-bambulab  
> **Filament Manager:** phases 1, 2, 4+5 complete (local store, AMS sync, React tab, cloud stack).

Use this guide for tomorrow’s **Windows 11** build and test session.

> **Start here:** For a step-by-step “download everything → build → login → Filament Manager” checklist, see **[WINDOWS_SETUP_CHECKLIST.md](./WINDOWS_SETUP_CHECKLIST.md)**.

---

## 1. Windows prerequisites (install before first build)

### Required

| Prerequisite | Purpose | Install |
|---|---|---|
| **Git for Windows** | Source + `git apply` in deps | https://git-scm.com/download/win |
| **Visual Studio 2022** (17.x) | MSVC toolchain | VS Installer → workload **Desktop development with C++** |
| **Windows 10/11 SDK** | Win32 build (CI uses 10.0.26100+) | Included in VS workload; add latest SDK if missing |
| **CMake 4.x** | Build system (project min 3.13; CI uses ~4.3) | https://cmake.org/download/ or `winget install Kitware.CMake` |
| **Strawberry Perl** | Some deps build steps (CI installs this) | `choco install strawberryperl` or https://strawberryperl.com/ |
| **Python 3** | Optional gettext `--full` path (`HintsToPot.py`) | https://python.org or `winget install Python.Python.3.12` |
| **Network access** | Downloads deps, Node 22, pnpm, Bambu plugin | — |
| **~50 GB free disk** | `deps/build` + `build/` + Node cache | — |

### VS 2022 workload checklist

In **Visual Studio Installer → Modify → Workloads**:

- [x] Desktop development with C++
- [x] MSVC v143 (or latest) x64/x86 build tools
- [x] Windows 10/11 SDK (10.0.19041+; 26100+ matches CI)

Open builds from **“x64 Native Tools Command Prompt for VS 2022”** *or* any shell where `msbuild -version` works.

### PATH ordering (important)

CMake **fails configure** if Strawberry Perl’s `c\bin` appears **before** CMake in `PATH`. Either:

- Put `C:\Program Files\CMake\bin` ahead of `C:\Strawberry\perl\bin` and `C:\Strawberry\c\bin`, or  
- Temporarily remove Strawberry from `PATH` during configure.

### Optional (packaging / CI parity)

| Tool | Purpose | Install |
|---|---|---|
| **NSIS** | `cpack -G NSIS` installer | `choco install nsis -y` |
| **7-Zip** | Portable zip packaging | https://www.7-zip.org/ |
| **Ninja** | Faster builds with `-x` | `choco install ninja` |
| **WSL2** | Bambu network bridge at **runtime** | See §4 |

### Not required via package manager

- **Boost, wxWidgets, OpenCV, …** — built into `deps/build/OrcaSlicer_dep/`
- **Node.js / pnpm** — CMake auto-downloads for Filament Manager / DeviceWeb on first configure
- **gettext** — Windows build uses bundled `tools/msgfmt.exe`, `tools/xgettext.exe` via `scripts/run_gettext.bat`

---

## 2. Exact build steps (Visual Studio 2022)

### 2.1 Clone and checkout

```powershell
git clone https://github.com/shawvibes/OrcaSlicer-bambulab.git
cd OrcaSlicer-bambulab
git checkout cursor/filament-manager-ams-sync-phase2
git pull
```

### 2.2 Prepare Linux bridge inputs (required for full slicer build)

`build_release_vs.bat` **refuses to build the slicer** without:

1. `tools\pjarczak_bambu_linux_host\runtime\linux-x86_64\` — Linux host binaries + certs  
2. `tools\pjarczak_bambu_runtime\rootfs\windows-wsl2-rootfs.tar` — WSL2 rootfs

See **§4** for how to produce or copy these. Skip to §2.2a only if you intentionally want a deps-only build.

### 2.2a Build deps only (no bridge required)

```powershell
cd OrcaSlicer-bambulab
.\build_release_vs.bat deps
```

Output: `deps\build\OrcaSlicer_dep\`  
First run: **1–3 hours**. Requires Strawberry Perl (CI installs via choco).

Pack deps (optional): `.\build_release_vs.bat pack` → `deps\OrcaSlicer_dep_win64_<date>_vs2022.zip`

### 2.2b Full Release build (recommended)

After bridge artifacts are in place:

```powershell
cd OrcaSlicer-bambulab

# One-shot: deps + slicer + gettext + install + bridge copy
.\build_release_vs.bat

# Or step-by-step (matches CI):
.\build_release_vs.bat deps
.\build_release_vs.bat slicer
```

**Output directory:** `build\OrcaSlicer\`  
**Run:** `build\OrcaSlicer\orca-slicer.exe` (or `OrcaSlicer.exe` depending on install target)

### 2.3 Build script reference

| Script | Bridge preflight | Bridge copy | Notes |
|---|---|---|---|
| `build_release_vs.bat` | Yes | Yes | **Use this** — auto-detects VS 2019/2022/2026 |
| `build_release_vs2022.bat` | No | No | Legacy/simple; **no Bambu bridge bundling** |

Common flags (both scripts):

```powershell
.\build_release_vs.bat debug          # Debug build → build-dbg\
.\build_release_vs.bat debuginfo      # RelWithDebInfo → build-dbginfo\
.\build_release_vs.bat slicer         # Slicer only (deps must exist)
.\build_release_vs.bat -x             # Ninja Multi-Config (build_release_vs.bat only)
```

Environment (optional):

```powershell
$env:CMAKE_POLICY_VERSION_MINIMUM = "3.5"   # set automatically by scripts
$env:ORCA_UPDATER_SIG_KEY = "..."           # only if testing updater signing
$env:PJARCZAK_WSL_ROOTFS_TAR = "D:\path\windows-wsl2-rootfs.tar"
```

### 2.4 Manual CMake (alternative)

```powershell
# Deps
cd deps
mkdir build; cd build
cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release --target deps -- -m

# Slicer
cd ..\..
mkdir build; cd build
cmake .. -G "Visual Studio 17 2022" -A x64 -DORCA_TOOLS=ON -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release --target ALL_BUILD -- -m
cd ..
.\scripts\run_gettext.bat
cd build
cmake --build . --target install --config Release
```

Then run bridge copy manually (see `build_release_vs.bat` label `:copy_linux_bridge_runtime`).

### 2.5 Filament Manager unit tests (optional)

Windows bat scripts do not expose `-T`. Configure manually:

```powershell
cd build
cmake .. -G "Visual Studio 17 2022" -A x64 -DORCA_TOOLS=ON -DBUILD_TESTS=ON
cmake --build . --config Release --target ALL_BUILD -- -m
ctest --config Release --output-on-failure
```

Tests live under `tests/fila_manager/`.

### 2.6 First-build Filament Manager note

First slicer configure/build downloads **Node 22 + pnpm** and compiles the React bundle:

- Source: `src/slic3r/GUI/DeviceWeb/device_page/`
- Output: `resources/web/device_page/dist/`
- Cache: `node-cache/` at repo root

Allow network; add ~5–10 min on first build.

---

## 3. Mac → Windows porting — known issues & fixes already applied

Issues hit on macOS that **will not recur** on Windows if branch is up to date:

| Issue | Fix on branch |
|---|---|
| `wxMediaCtrl2.h` — `constexpr wxMediaState` out of range | Removed unused `MEDIASTATE_BUFFERING` constant |
| `GUI_App.cpp` — incomplete `DeviceWebPage` | Added `#include DeviceWebPage.hpp` |
| `DevFilaColorType` missing in Orca | `from_ams_color_type(int)` uses `DevAmsTray::ctype` |
| `get_preset_alias` / `display_name` missing | Uses `preset.alias` + local display-name derivation |
| `GetFilaInfo` private | Uses `GetFilaInfoMap()` → `GetColorCode()` |
| `set_custom_preset_alias` protected | Removed; read-only alias/display derivation |
| Missing `tab_filament_active.svg` | Added under `resources/images/` |

### Windows-specific pitfalls to watch for

1. **`build_release_vs2022.bat` vs `build_release_vs.bat`** — only `build_release_vs.bat` bundles the WSL bridge; using the wrong script yields a build with no network runtime files.

2. **Strawberry Perl before CMake in PATH** — configure aborts with explicit error; reorder PATH.

3. **Long path names** — enable Windows long paths or keep clone short (e.g. `C:\src\OrcaSlicer-bambulab`).

4. **Antivirus** — real-time scan on `deps/` and `build/` slows linking dramatically; exclude clone dir if safe.

5. **Line endings** — bridge shell scripts must be LF; `install_runtime.ps1` runs `Convert-FileToLf` on bootstrap scripts.

6. **Cloud filament APIs** — Orca BambuNetwork plugin may still be older than BambuStudio 02.07.x; cloud sync code paths exist but plugin may return `INVALID_HANDLE` until plugin is upgraded (same as Mac).

7. **`MediaPlayCtrl.h`** — if build fails on `static_cast<wxMediaState>(6)` enum range, apply same pattern as `wxMediaCtrl2.h` (may not trigger on MSVC the way it did on clang).

8. **WebView2** — Windows uses native WebView2 (not WKWebView); Filament Manager tab should work if WebView2 runtime is installed (usually present on Win11).

---

## 4. Linux bridge runtime on Windows (Bambu network)

Bambu’s networking plugin is **Linux `.so` binaries**. On Windows, Orca runs them inside a **WSL2 micro-distro** via `pjarczak_bambu_networking_bridge.dll`.

### Architecture (short)

```
orca-slicer.exe
  → pjarczak_bambu_networking_bridge.dll
    → WSL2 distro "PJARCZAK-BAMBU"
      → pjarczak_bambu_linux_host (+ abi0/abi1)
        → libbambu_networking.so / libBambuSource.so
```

Mac uses **Lima** instead; Windows uses **WSL2**.

### 4.1 Files required before `build_release_vs.bat slicer`

**A. Linux host runtime** — `tools\pjarczak_bambu_linux_host\runtime\linux-x86_64\`:

```
pjarczak_bambu_linux_host
pjarczak_bambu_linux_host_abi1
pjarczak_bambu_linux_host_abi0
libbambu_networking.so      (may download at runtime)
libBambuSource.so           (may download at runtime)
ca-certificates.crt
slicer_base64.cer
linux_payload_manifest.json (optional)
```

**B. WSL rootfs tar** — one of:

```
tools\pjarczak_bambu_runtime\rootfs\windows-wsl2-rootfs.tar
tools\pjarczak_bambu_runtime\windows-wsl2-rootfs.tar
```

Or set `PJARCZAK_WSL_ROOTFS_TAR` to an absolute path.

### 4.2 Option A — Build bridge artifacts in WSL Ubuntu (Windows machine)

Install WSL2 + Ubuntu 24.04, then in **WSL**:

```bash
cd /mnt/c/path/to/OrcaSlicer-bambulab
sudo apt update
# Use repo's Linux build for deps if needed:
./build_linux.sh -drlL

cmake -S . -B build -G "Ninja Multi-Config" -DORCA_TOOLS=ON
cmake --build build --config Release --target pjarczak_bambu_linux_host
bash tools/pjarczak_bambu_linux_host/package_linux_host_runtime.sh build
bash tools/pjarczak_bambu_runtime/rootfs/build_windows_wsl_rootfs.sh \
  tools/pjarczak_bambu_runtime/rootfs/windows-wsl2-rootfs.tar
```

Back in **Windows PowerShell**, verify:

```powershell
Test-Path tools\pjarczak_bambu_linux_host\runtime\linux-x86_64\pjarczak_bambu_linux_host
Test-Path tools\pjarczak_bambu_runtime\rootfs\windows-wsl2-rootfs.tar
```

### 4.2 Option B — Copy from Mac/Linux CI artifacts

From GitHub Actions workflow `build_windows_bridge.yml`:

1. Run workflow (or download artifacts from a green run)
2. Extract `linux_host_runtime_*.tar.gz` → `tools\pjarczak_bambu_linux_host\runtime\`
3. Extract `wsl_rootfs_*.tar.gz` → `tools\pjarczak_bambu_runtime\rootfs\`

### 4.2 Option C — Build WSL rootfs on Windows with Docker

If Linux host binaries already exist:

```powershell
cd OrcaSlicer-bambulab
.\tools\pjarczak_bambu_runtime\rootfs\build_windows_wsl_rootfs.ps1
# Creates tools\pjarczak_bambu_runtime\rootfs\windows-wsl2-rootfs.tar
```

Requires Docker Desktop.

### 4.3 After building — install WSL runtime (first run)

The full build copies these into `build\OrcaSlicer\`:

- `pjarczak_bambu_networking_bridge.dll`
- `windows-wsl2-rootfs.tar`
- `install_runtime.ps1` / `install_runtime.cmd`
- `verify_runtime.ps1`
- `pjarczak_wsl_run_host.sh`
- Linux host binaries (flat in output dir)

**One-time setup (Admin PowerShell may be required first time for WSL):**

```powershell
# Enable WSL if needed (reboot after)
wsl --install --no-distribution
wsl --update

# From built output dir:
cd build\OrcaSlicer
.\install_runtime.ps1 -ReplaceExisting

# Validate:
.\verify_runtime.ps1 -AllowMissingLinuxPlugin
```

This imports WSL distro **`PJARCZAK-BAMBU`** and copies plugin payload to:

- `%APPDATA%\OrcaSlicer\plugins\`
- Cache: `%APPDATA%\OrcaSlicer\ota\plugins\` (subdir from `pjarczak_plugin_cache_subdir.txt`)

Then launch `orca-slicer.exe` from `build\OrcaSlicer\`.

### 4.4 Troubleshooting Bambu network on Windows

| Symptom | Check |
|---|---|
| Build fails “Missing linux host runtime” | Complete §4.1 / §4.2 before `slicer` step |
| Build fails “Missing windows-wsl2-rootfs.tar” | Run rootfs build script or set `PJARCZAK_WSL_ROOTFS_TAR` |
| Login/cloud fails at runtime | Run `install_runtime.ps1`; confirm `wsl -l -v` shows `PJARCZAK-BAMBU` |
| Plugin download fails | Firewall; `%APPDATA%\OrcaSlicer\ota\plugins\` writable |
| Filament Manager cloud empty | Plugin version may lack filament API symbols (see §3) |

---

## 5. Filament Manager — Windows test checklist

After successful build:

- [ ] App launches; **Filament Manager** tab visible (spool icon)
- [ ] Tab navigates to `#/filament_manager` (WebView2 loads)
- [ ] Local spool CRUD → `%APPDATA%\OrcaSlicer\filament_inventory\spools.json`
- [ ] Device tab + AMS: tray data appears in Filament Manager AMS panel
- [ ] Bambu login → cloud pull (if plugin supports filament APIs)
- [ ] No crash on tab switch / printer selection

Log tags: `[FilamentVM]`, `[FilaCloudSync]`, `Filament Manager`.

---

## 6. Quick reference — tomorrow’s minimal command sequence

```powershell
# Prerequisites once
winget install Git.Git Kitware.CMake Python.Python.3.12
choco install strawberryperl -y
# + VS 2022 with C++ workload

git clone https://github.com/shawvibes/OrcaSlicer-bambulab.git
cd OrcaSlicer-bambulab
git checkout cursor/filament-manager-ams-sync-phase2

# Prepare bridge (WSL Ubuntu — see §4.2 Option A)
# ... then in PowerShell:

.\build_release_vs.bat deps
.\build_release_vs.bat slicer

cd build\OrcaSlicer
.\install_runtime.ps1 -ReplaceExisting
.\orca-slicer.exe
```

---

## 7. Related files in repo

| Path | Role |
|---|---|
| `build_release_vs.bat` | Main Windows build + bridge bundling |
| `build_release_vs2022.bat` | VS2022-only simplified script (no bridge) |
| `build_windows_bridge.yml` | CI: Linux host + WSL rootfs + Windows build |
| `tools/pjarczak_bambu_runtime/wsl/` | WSL install/verify scripts |
| `tools/pjarczak_bambu_linux_host/` | Linux host wrapper sources + runtime packaging |
| `src/slic3r/GUI/DeviceWeb/` | Filament Manager React UI + C++ bridge |
| `src/slic3r/GUI/fila_manager/` | Store, AMS sync, cloud sync |
| `resources/images/tab_filament_active.svg` | Tab icon (added for Filament Manager) |

---

*Last updated: 2026-06-14 — Mac build verified; Windows guide prepared for next session.*
