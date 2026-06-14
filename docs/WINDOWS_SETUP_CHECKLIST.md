# Windows 11 — Full Setup Checklist (OrcaSlicer + Filament Manager + Bambu Login)

> **Branch:** `cursor/filament-manager-ams-sync-phase2`  
> **Repo:** https://github.com/shawvibes/OrcaSlicer-bambulab  
> **Goal:** Build OrcaSlicer, install Bambu network plugin via WSL2 bridge, log in, and use **Filament Manager**.

---

## Why login is required (Mac and Windows)

The Filament Manager **UI intentionally gates most actions on Bambu login**:

- **Add Filament**, search, filters, tabs, AMS view → disabled when `logged_in === false`
- `logged_in` comes from `NetworkAgent::is_user_login()` → needs the **Bambu network plugin**
- The plugin is a **Linux `.so`**; on Windows it runs inside **WSL2** via `pjarczak_bambu_networking_bridge.dll`

Local spool data (`filament_inventory/spools.json`) exists in C++ without login, but the React page will look broken/disabled until login works.

**Mac:** Lima bridge + macOS 27 beta = unreliable today. **Windows + WSL2** is the supported path.

---

## Phase 0 — Before you start

- [ ] **Windows 11** updated (Settings → Windows Update)
- [ ] **~60 GB free disk** on `C:` (or drive where you clone)
- [ ] **Virtualization enabled** in BIOS/UEFI (required for WSL2)
- [ ] Stable **internet** (deps, Node 22, Bambu plugin download)
- [ ] **Bambu account** credentials ready (same as Bambu Studio / Handy app)
- [ ] Optional: **Bambu printer on LAN** for AMS sync testing

Use a **short clone path** to avoid Windows path-length issues:

```text
C:\src\OrcaSlicer-bambulab
```

---

## Phase 1 — Download & install (one-time)

Run **PowerShell as Administrator** for WSL/Chocolatey steps.

### 1.1 Core build tools

| # | Install | Command / link | Verify |
|---|---------|----------------|--------|
| 1 | **Git for Windows** | `winget install Git.Git` | `git --version` |
| 2 | **Visual Studio 2022** | [Visual Studio](https://visualstudio.microsoft.com/vs/) → workload **Desktop development with C++** | `msbuild -version` → major **17** |
| 3 | **Windows 10/11 SDK** | VS Installer → Individual components → latest **Windows SDK** (10.0.26100+ ideal) | — |
| 4 | **CMake 4.x** | `winget install Kitware.CMake` | `cmake --version` → 3.13+ (4.x recommended) |
| 5 | **Strawberry Perl** | `choco install strawberryperl -y` | `perl --version` |
| 6 | **Python 3** | `winget install Python.Python.3.12` | `python --version` |
| 7 | **Chocolatey** (optional) | [chocolatey.org/install](https://chocolatey.org/install) | `choco -?` |

**VS 2022 workload must include:**

- MSVC v143 (or latest) **x64/x86** build tools  
- **C++ CMake tools for Windows** (helpful)  
- Windows **10/11 SDK**

### 1.2 Runtime / helper tools

| # | Install | Purpose | Command |
|---|---------|---------|---------|
| 8 | **WebView2 Runtime** | Filament Manager React UI | Usually preinstalled on Win11; if not: [WebView2](https://developer.microsoft.com/microsoft-edge/webview2/) |
| 9 | **WSL2** | Bambu network plugin host | See Phase 2 |
| 10 | **Ubuntu 24.04 on WSL** | Build bridge artifacts | `wsl --install -d Ubuntu-24.04` |
| 11 | **Ninja** (optional) | Faster builds with `-x` | `choco install ninja -y` |
| 12 | **7-Zip** (optional) | Extract CI artifacts | [7-zip.org](https://www.7-zip.org/) |

### 1.3 PATH order (critical)

CMake **fails** if Strawberry Perl’s `c\bin` is **before** CMake in `PATH`.

```powershell
# Check order (CMake path should appear BEFORE Strawberry\c\bin)
$env:Path -split ';' | Select-String -Pattern 'CMake|Strawberry'
```

Fix in **System Environment Variables** if needed:

1. `C:\Program Files\CMake\bin` → move **up**  
2. `C:\Strawberry\c\bin` → move **down** (or remove from PATH during builds)

---

## Phase 2 — Enable WSL2

In **Admin PowerShell**:

```powershell
# Enable WSL + Virtual Machine Platform (reboot if prompted)
wsl --install --no-distribution
wsl --update
```

Reboot if Windows asks you to.

Install Ubuntu (if not already):

```powershell
wsl --install -d Ubuntu-24.04
```

Create Linux user/password when prompted.

Verify:

```powershell
wsl -l -v
# Ubuntu-24.04 should show VERSION 2
```

---

## Phase 3 — Clone the repo

In **PowerShell** (normal user):

```powershell
mkdir C:\src -ErrorAction SilentlyContinue
cd C:\src
git clone https://github.com/shawvibes/OrcaSlicer-bambulab.git
cd OrcaSlicer-bambulab
git checkout cursor/filament-manager-ams-sync-phase2
git pull
```

If you have **uncommitted Mac fixes**, push from Mac first or copy patches — Windows needs the same branch state (API fixes, `tab_filament_active.svg`, etc.).

---

## Phase 4 — Build Linux bridge artifacts (in WSL)

Open **Ubuntu** terminal (`wsl` or Windows Terminal → Ubuntu).

```bash
cd /mnt/c/src/OrcaSlicer-bambulab   # adjust path if different

# One-time Linux build dependencies (~5–15 min)
sudo ./build_linux.sh -u

# Build Linux deps (long — 1–3 hr first time; can run while Windows deps build in parallel)
./build_linux.sh -drlL

# Configure + build linux host wrapper
cmake -S . -B build -G "Ninja Multi-Config" -DORCA_TOOLS=ON
cmake --build build --config Release --target pjarczak_bambu_linux_host

# Package runtime into tools/pjarczak_bambu_linux_host/runtime/linux-x86_64/
bash tools/pjarczak_bambu_linux_host/package_linux_host_runtime.sh build

# Build minimal WSL rootfs tar for Windows bridge
bash tools/pjarczak_bambu_runtime/rootfs/build_windows_wsl_rootfs.sh \
  tools/pjarczak_bambu_runtime/rootfs/windows-wsl2-rootfs.tar
```

**Requires Docker inside WSL** for the rootfs step (`docker` command available). If Docker is not in WSL:

- Install **Docker Desktop** on Windows with WSL2 integration, **or**
- From **Windows PowerShell** (with Docker Desktop running):

```powershell
cd C:\src\OrcaSlicer-bambulab
.\tools\pjarczak_bambu_runtime\rootfs\build_windows_wsl_rootfs.ps1
```

### Verify bridge files exist (Windows PowerShell)

```powershell
cd C:\src\OrcaSlicer-bambulab
Test-Path tools\pjarczak_bambu_linux_host\runtime\linux-x86_64\pjarczak_bambu_linux_host
Test-Path tools\pjarczak_bambu_linux_host\runtime\linux-x86_64\pjarczak_bambu_linux_host_abi1
Test-Path tools\pjarczak_bambu_linux_host\runtime\linux-x86_64\ca-certificates.crt
Test-Path tools\pjarczak_bambu_runtime\rootfs\windows-wsl2-rootfs.tar
```

All should return **True** before continuing.

---

## Phase 5 — Build OrcaSlicer on Windows

Use **“x64 Native Tools Command Prompt for VS 2022”** or PowerShell where `msbuild -version` works.

```powershell
cd C:\src\OrcaSlicer-bambulab

# Step 1: dependencies (~1–3 hr first time)
.\build_release_vs.bat deps

# Step 2: slicer + gettext + install + WSL bridge bundle (~30–90 min)
.\build_release_vs.bat slicer
```

**Do not use** `build_release_vs2022.bat` — it skips WSL bridge bundling.

Output:

```text
C:\src\OrcaSlicer-bambulab\build\OrcaSlicer\orca-slicer.exe
```

First slicer build also downloads **Node 22 + pnpm** and builds the Filament Manager web UI (~extra 5–15 min).

### Rebuild after code changes only

```powershell
cd C:\src\OrcaSlicer-bambulab\build
cmake --build . --config Release --target ALL_BUILD -- -m
cmake --build . --target install --config Release
# Re-run bridge copy if needed — safest: .\build_release_vs.bat slicer
```

---

## Phase 6 — Install WSL network runtime (one-time per machine)

**Admin PowerShell** may be required the first time WSL imports a distro.

```powershell
cd C:\src\OrcaSlicer-bambulab\build\OrcaSlicer

# Import PJARCZAK-BAMBU WSL distro + copy plugin payload
.\install_runtime.ps1 -ReplaceExisting

# Validate (AllowMissingLinuxPlugin = .so may download on first app launch)
.\verify_runtime.ps1 -AllowMissingLinuxPlugin
```

Expected:

- WSL distro **`PJARCZAK-BAMBU`** appears in `wsl -l -v`
- Files copied to `%APPDATA%\OrcaSlicer\plugins\`

If `install_runtime.ps1` fails:

```powershell
wsl --status
wsl --update
# Reboot, then retry
```

---

## Phase 7 — First launch & Bambu login

```powershell
cd C:\src\OrcaSlicer-bambulab\build\OrcaSlicer
.\orca-slicer.exe
```

### 7.1 Network plugin prompt

On first launch Orca may show **“network plug-in was not detected”**:

1. Click to **download/install** the plugin (or use the notification banner)
2. Confirm **Preferences → Enable network plug-in** is checked (`installed_networking`)
3. If prompted about **WSL/Linux bridge runtime**, allow Orca to install/repair (runs `install_runtime.ps1` logic)

Plugin files land in:

```text
%APPDATA%\OrcaSlicer\plugins\
  libbambu_networking.so
  libBambuSource.so
  pjarczak_bambu_networking_bridge.dll
  pjarczak_bambu_linux_host
  windows-wsl2-rootfs.tar
  install_runtime.ps1
  ...
```

### 7.2 Log in to Bambu

1. Use Orca’s **Login** / account menu (same flow as Bambu Studio)
2. Complete browser/OAuth login with your Bambu account
3. Bind/select your printer if prompted

Verify login in log:

```text
%APPDATA%\OrcaSlicer\log\debug_*.log
```

Search for `is_user_login = 1` or absence of `skipping bridge DLL load`.

### 7.3 Open Filament Manager

1. Select a **Bambu Lab printer** (BBL) in the device/printer list if required for device tabs
2. Click **Filament Manager** tab (spool icon)
3. UI should show `data-logged-in="true"` on the page root (DevTools if debugging)
4. **Add Filament** button should be enabled (green, not greyed out)

---

## Phase 8 — Filament Manager test checklist

### Without login (expect limited/broken UI)

- [ ] Tab loads but **Add Filament** disabled  
- [ ] Cloud badge shows **“Not logged in”**  
- [ ] Filters/search greyed out  

### After login + plugin working

- [ ] **Add Filament** opens dialog  
- [ ] Create spool → `%APPDATA%\OrcaSlicer\filament_inventory\spools.json` updates  
- [ ] **Cloud sync** badge active; pull/sync works  
- [ ] **Device** tab: printer connected → **AMS** tab in Filament Manager shows trays  
- [ ] AMS tray weights/colors update when printer reports MQTT state  
- [ ] Switch printers without crash  
- [ ] Log shows `[FilamentVM]` / `[FilaCloudSync]` without repeated errors  

### Known limitation (both platforms)

Cloud filament APIs may be incomplete if Orca’s BambuNetwork plugin is older than BambuStudio **02.07.x**. Local + AMS paths should work; some cloud push/pull operations may log `INVALID_HANDLE` until plugin is upgraded.

---

## Phase 9 — Troubleshooting

| Problem | What to do |
|---------|------------|
| CMake configure error about Strawberry Perl | Fix PATH order (Phase 1.3) |
| `Missing linux host runtime` on slicer build | Complete Phase 4 |
| `Missing windows-wsl2-rootfs.tar` | Run rootfs build (Phase 4) |
| `wsl --install` fails | Enable virtualization; run as Admin; reboot |
| Plugin download OK but login fails | Run `install_runtime.ps1 -ReplaceExisting`; check `wsl -l -v` |
| `verify_runtime.ps1` fails | Read stderr; ensure `%APPDATA%\OrcaSlicer\plugins` has bridge DLL + host binaries |
| Filament Manager blank WebView | Install WebView2 Runtime |
| Build extremely slow | Exclude `C:\src\OrcaSlicer-bambulab` from real-time antivirus scan |
| Cloud sync errors only | Check plugin version / log for missing filament API symbols |

**Logs:**

```text
%APPDATA%\OrcaSlicer\log\debug_*.log
```

Search: `pjarczak`, `on_init_network`, `BBLNetwork`, `[FilaCloudSync]`, `[FilamentVM]`.

---

## Quick command reference (copy-paste order)

```powershell
# === ADMIN (once) ===
wsl --install --no-distribution
wsl --update
wsl --install -d Ubuntu-24.04
# reboot if needed

winget install Git.Git Kitware.CMake Python.Python.3.12
choco install strawberryperl ninja -y
# + install VS 2022 with C++ workload manually

# === CLONE ===
cd C:\src
git clone https://github.com/shawvibes/OrcaSlicer-bambulab.git
cd OrcaSlicer-bambulab
git checkout cursor/filament-manager-ams-sync-phase2
```

```bash
# === WSL UBUNTU ===
cd /mnt/c/src/OrcaSlicer-bambulab
sudo ./build_linux.sh -u
./build_linux.sh -drlL
cmake -S . -B build -G "Ninja Multi-Config" -DORCA_TOOLS=ON
cmake --build build --config Release --target pjarczak_bambu_linux_host
bash tools/pjarczak_bambu_linux_host/package_linux_host_runtime.sh build
bash tools/pjarczak_bambu_runtime/rootfs/build_windows_wsl_rootfs.sh \
  tools/pjarczak_bambu_runtime/rootfs/windows-wsl2-rootfs.tar
```

```powershell
# === WINDOWS BUILD ===
cd C:\src\OrcaSlicer-bambulab
.\build_release_vs.bat deps
.\build_release_vs.bat slicer

cd build\OrcaSlicer
.\install_runtime.ps1 -ReplaceExisting
.\verify_runtime.ps1 -AllowMissingLinuxPlugin
.\orca-slicer.exe
# → enable network plugin → log in → Filament Manager tab
```

---

## Related docs

- `docs/WINDOWS_BUILD_GUIDE.md` — deeper build/CI reference  
- `build_windows_bridge.yml` — official CI pipeline for bridge + Windows  
- `tools/pjarczak_bambu_runtime/wsl/install_runtime.ps1` — WSL runtime installer  

---

*Last updated: 2026-06-14*
