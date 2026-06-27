# Windows 11 + Cursor — Build OrcaSlicer-bambulab (start here)

Use this guide while setting up your Windows PC. Each phase has copy-paste commands.

**Repo path on Windows:** `C:\src\OrcaSlicer-bambulab` (keep it short)

**Branch:** `cursor/filament-manager-ams-sync-phase2`

---

## IMPORTANT: Get your Mac code onto Windows first

Your Mac has **~100 uncommitted changes** that are **not on GitHub**. A plain `git clone` will miss your latest work.

Pick **one** method:

### Method A — Copy the whole folder (easiest)

1. On Mac: zip or copy `~/3dprinter/OrcaSlicer-bambulab` to OneDrive/iCloud/USB/network share.
2. On Windows: extract to `C:\src\OrcaSlicer-bambulab`
3. Skip to **Phase 1** below.

### Method B — Clone + apply patch

1. On Mac, the file `windows-mac-sync.patch` is in the repo root (14k+ lines of your Mac changes).
2. Copy that file to Windows.
3. On Windows:

```powershell
mkdir C:\src -ErrorAction SilentlyContinue
cd C:\src
git clone https://github.com/shawvibes/OrcaSlicer-bambulab.git
cd OrcaSlicer-bambulab
git checkout cursor/filament-manager-ams-sync-phase2
git apply C:\path\to\windows-mac-sync.patch
```

If `git apply` fails on untracked files, use Method A instead.

### Method C — Push from Mac, pull on Windows

On Mac (when ready to commit):

```bash
cd ~/3dprinter/OrcaSlicer-bambulab
git add -A
git commit -m "Sync Mac work for Windows build"
git push origin cursor/filament-manager-ams-sync-phase2
```

Then on Windows: clone and checkout that branch.

---

## Phase 0 — Open in Cursor

1. Install **Cursor** from https://cursor.com
2. **File → Open Folder** → `C:\src\OrcaSlicer-bambulab`
3. When Cursor asks to install recommended extensions, accept (C++ / CMake helpful).

---

## Phase 1 — Install build tools (Admin PowerShell)

Open **PowerShell as Administrator** and run:

```powershell
# Core tools
winget install --id Git.Git -e --accept-source-agreements --accept-package-agreements
winget install --id Kitware.CMake -e --accept-source-agreements --accept-package-agreements
winget install --id Python.Python.3.12 -e --accept-source-agreements --accept-package-agreements

# Chocolatey (for Strawberry Perl) — skip if already installed
Set-ExecutionPolicy Bypass -Scope Process -Force
[System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072
iex ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))

choco install strawberryperl ninja -y

# WSL2 (reboot if prompted)
wsl --install --no-distribution
wsl --update
wsl --install -d Ubuntu-24.04
```

**Visual Studio 2022** (manual — cannot fully winget):

1. Download: https://visualstudio.microsoft.com/vs/
2. Install workload: **Desktop development with C++**
3. Include: **MSVC v143**, **Windows 10/11 SDK** (10.0.26100+ ideal), **C++ CMake tools for Windows**

**Reboot** if WSL or VS asks you to.

### Fix PATH order (critical)

CMake fails if Strawberry Perl is before CMake in PATH.

1. Start → type **Environment Variables** → Edit system environment variables
2. System variables → **Path** → Edit
3. Move `C:\Program Files\CMake\bin` **above** `C:\Strawberry\c\bin`
4. OK → restart PowerShell

### Verify

Open a **new** PowerShell (not Admin):

```powershell
cd C:\src\OrcaSlicer-bambulab
.\scripts\verify_windows_prereqs.ps1
```

Fix anything marked `[FAIL]` before continuing.

---

## Phase 2 — Bridge artifacts (Bambu network)

The slicer build needs Linux bridge files + a WSL rootfs tar.

### 2a — Linux host runtime

If you copied from Mac, you may already have these. Check:

```powershell
dir C:\src\OrcaSlicer-bambulab\tools\pjarczak_bambu_linux_host\runtime\linux-x86_64\
```

You need: `pjarczak_bambu_linux_host`, `pjarczak_bambu_linux_host_abi0`, `pjarczak_bambu_linux_host_abi1`, `ca-certificates.crt`, `slicer_base64.cer`

If missing, build in **WSL Ubuntu**:

```bash
cd /mnt/c/src/OrcaSlicer-bambulab
sudo ./build_linux.sh -u
./build_linux.sh -drlL
cmake -S . -B build -G "Ninja Multi-Config" -DORCA_TOOLS=ON
cmake --build build --config Release --target pjarczak_bambu_linux_host
bash tools/pjarczak_bambu_linux_host/package_linux_host_runtime.sh build
```

### 2b — WSL rootfs tar

Install **Docker Desktop** (https://www.docker.com/products/docker-desktop/) with WSL2 backend enabled.

Then in **PowerShell**:

```powershell
cd C:\src\OrcaSlicer-bambulab
.\tools\pjarczak_bambu_runtime\rootfs\build_windows_wsl_rootfs.ps1
```

Creates: `tools\pjarczak_bambu_runtime\rootfs\windows-wsl2-rootfs.tar` (~80 MB)

Verify everything:

```powershell
.\scripts\verify_windows_prereqs.ps1
```

All bridge checks should be `[OK]`.

---

## Phase 3 — Build OrcaSlicer

Use **"x64 Native Tools Command Prompt for VS 2022"** (Start menu → Visual Studio 2022) **or** PowerShell where `msbuild -version` returns 17.x.

```powershell
cd C:\src\OrcaSlicer-bambulab

# Step 1: dependencies (~1–3 hours first time)
.\build_release_vs.bat deps

# Step 2: slicer + install (~30–90 min)
.\build_release_vs.bat slicer
```

**Output:** `C:\src\OrcaSlicer-bambulab\build\OrcaSlicer\orca-slicer.exe`

Tips:

- Do **not** use `build_release_vs2022.bat` — it skips WSL bridge bundling.
- First build downloads Node 22 + pnpm for Filament Manager UI (~5–15 extra min).
- Exclude `C:\src\OrcaSlicer-bambulab` from real-time antivirus if builds are very slow.

### Rebuild after code changes

```powershell
cd C:\src\OrcaSlicer-bambulab
.\build_release_vs.bat slicer
```

---

## Phase 4 — Install Bambu network runtime (one-time)

```powershell
cd C:\src\OrcaSlicer-bambulab\build\OrcaSlicer
.\install_runtime.ps1 -ReplaceExisting
.\verify_runtime.ps1 -AllowMissingLinuxPlugin
```

This creates WSL distro **PJARCZAK-BAMBU** for Bambu cloud/LAN login.

---

## Phase 5 — Launch and log in

```powershell
cd C:\src\OrcaSlicer-bambulab\build\OrcaSlicer
.\orca-slicer.exe
```

1. Allow network plugin download if prompted
2. Log in with your Bambu account
3. Open **Filament Manager** tab (spool icon)

Logs: `%APPDATA%\OrcaSlicer\log\debug_*.log`

---

## Cursor tips on Windows

| Task | How |
|------|-----|
| Build from terminal | Cursor terminal → x64 Native Tools prompt, or run VS dev shell first |
| Ask Cursor for help | Reference `@docs/WINDOWS_CURSOR_QUICKSTART.md` in chat |
| Debug build errors | Paste CMake/MSBuild output; common fix is PATH order |
| Sync changes Mac ↔ Win | Git push/pull, or copy `windows-mac-sync.patch` |

---

## Troubleshooting

| Problem | Fix |
|---------|-----|
| CMake + Strawberry PATH error | Phase 1 PATH fix |
| `Missing linux host runtime` | Phase 2a |
| `Missing windows-wsl2-rootfs.tar` | Phase 2b (Docker Desktop) |
| `msbuild` not found | Open x64 Native Tools Command Prompt for VS 2022 |
| Filament Manager blank | Install WebView2 Runtime |
| Login fails | Re-run `install_runtime.ps1 -ReplaceExisting`; check `wsl -l -v` |

More detail: `docs/WINDOWS_SETUP_CHECKLIST.md`, `docs/WINDOWS_BUILD_GUIDE.md`

---

## Quick copy-paste order (after tools installed)

```powershell
# 1. Verify
cd C:\src\OrcaSlicer-bambulab
.\scripts\verify_windows_prereqs.ps1

# 2. Rootfs (if needed)
.\tools\pjarczak_bambu_runtime\rootfs\build_windows_wsl_rootfs.ps1

# 3. Build
.\build_release_vs.bat deps
.\build_release_vs.bat slicer

# 4. Runtime + run
cd build\OrcaSlicer
.\install_runtime.ps1 -ReplaceExisting
.\orca-slicer.exe
```
