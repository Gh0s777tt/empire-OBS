# 🏗️ Building Empire‑OBS

![toolchain](https://img.shields.io/badge/toolchain-VS_2026-E50914?style=flat-square&labelColor=141414)
![cmake](https://img.shields.io/badge/CMake-4.3-141414?style=flat-square&labelColor=E50914)

Empire‑OBS builds on **Windows x64** with the **Visual Studio 2026** toolchain. The configure step auto‑downloads
Qt6, CEF and obs‑deps, so you don't manage those yourself.

---

## ✅ Prerequisites

| Tool | Version | Notes |
|---|---|---|
| **Visual Studio 2026 Build Tools** (or IDE) | v18.x, MSVC 14.51 | Workload **“Desktop development with C++”** (provides `cl.exe`) |
| **CMake** | ≥ 4.3 | `winget install Kitware.CMake` |
| **Git** | any recent | for `--recursive` submodules |
| Windows SDK | 10.0.26100 (or 22621) | bundled with the C++ workload |
| Disk | ~10 GB free | deps + build, **outside OneDrive** |

> [!WARNING]
> Do **not** build inside a OneDrive‑synced folder — it will try to sync gigabytes of build artifacts.
> Use e.g. `C:\dev\empire-OBS`.

---

## 🚀 Build steps

```powershell
# 1. Clone WITH submodules (obs-browser, obs-websocket, libdshowcapture)
git clone --recursive https://github.com/Gh0s777tt/empire-OBS.git C:\dev\empire-OBS
cd C:\dev\empire-OBS

# 2. Configure — downloads Qt6 + CEF + obs-deps (~8 min), generates VS 2026 solution (.slnx)
cmake --preset empire-windows-x64

# 3. Build — SERIAL (see gotcha below)
cmake --build --preset empire-windows-x64

# 4. Run — portable keeps config isolated from a real OBS install
.\build_x64\rundir\RelWithDebInfo\bin\64bit\obs64.exe --portable
```

The `empire-windows-x64` preset (in `CMakeUserPresets.json`) pins the **Visual Studio 18 2026** generator and the
**10.0.26100** Windows SDK.

---

## ⚠️ VS 2026 gotchas (already fixed in this fork)

These were required to build OBS on the newer MSVC and are committed in `v0.1.0`:

1. **`error STL1011` — `<experimental/coroutine>`**
   MSVC 14.51 turned the deprecated header (still pulled in by C++/WinRT) into a hard error.
   *Fix:* `libobs-winrt/CMakeLists.txt` defines `_SILENCE_EXPERIMENTAL_COROUTINE_DEPRECATION_WARNINGS`.

2. **`MSB3374 / MSB3073` — file‑lock race**
   `--parallel` makes several MSBuild workers hit the nested **x86** virtual‑camera sub‑build at once.
   *Fix:* **build serial** — do **not** pass `--parallel`. If a rebuild ever locks files, kill stray `msbuild` nodes:
   ```powershell
   Get-Process msbuild -ErrorAction SilentlyContinue | Stop-Process -Force
   ```

---

## 🧪 Verifying

- Binary: `build_x64\rundir\RelWithDebInfo\bin\64bit\obs64.exe`
- Empire theme deployed: `build_x64\rundir\RelWithDebInfo\data\obs-studio\themes\Yami_Empire.ovt`
- Logs: `build_x64\rundir\RelWithDebInfo\config\obs-studio\logs\*.txt`

---

## 🔄 Staying in sync with upstream

```powershell
git remote add upstream https://github.com/obsproject/obs-studio.git   # one time
git fetch upstream
git merge upstream/master     # resolve, keep Empire changes (mostly frontend/data/scripts)
```

Because ~90% of Empire lives in **frontend / data / scripts**, upstream merges stay clean.
