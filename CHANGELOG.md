# 📦 Changelog

All notable changes to **Empire‑OBS** are documented here.
Format based on [Keep a Changelog](https://keepachangelog.com/) · versioning is [SemVer](https://semver.org/).
Each release carries a sequential **Update #** number.

![keep-a-changelog](https://img.shields.io/badge/changelog-Keep_a_Changelog-E50914?style=flat-square&labelColor=141414)
![semver](https://img.shields.io/badge/SemVer-2.0-141414?style=flat-square&labelColor=E50914)

---

## 🔜 [Unreleased]

The big remaining ideas are separate projects — see [ROADMAP.md](ROADMAP.md): AI background removal (use the [plugin](Background-Removal.md)), a mobile companion, IoT.

---

## 🟥 [v0.7.2] — Installer &nbsp;·&nbsp; `Update #013` &nbsp;·&nbsp; 2026-06-08

> *One‑click setup.*

### ✨ Added
- **Windows installer** — an NSIS `…-installer.exe` with a Start‑menu shortcut and a proper uninstaller, published alongside the existing **portable ZIP**. Built by CI on every `v*` tag.

---

## 🟥 [v0.7.1] — Cinematic Polish &nbsp;·&nbsp; `Update #012` &nbsp;·&nbsp; 2026-06-08

> *The mockup, realized.*

### ✨ Added
- **Preview badges** — a LIVE / REC · resolution · scene pill drawn right over the program preview. `a77c280`
- **Right navigation rail** — STREAMY / NAGRANIA / SCENY / USTAWIENIA quick-launch (toggle docks · open recordings · Settings). `53ade5f`
- **Transitions panel** — pick the active scene transition and its duration. `a77c280`
- **Quick 9:16 toggle** on the Command bar — flip the Vertical preview on/off in one click. `f53c2e5`
- **Source management** in the Sources card — a *+ Add source* menu plus a right-click Properties / Filters / Remove. `a77c280`
- **Scene management** in the Scenes card — a *+ New scene* button plus a right-click Rename / Duplicate / Remove. `aef016e`
- **Audio meters &amp; dB** — a live peak VU bar and a dB readout per source. `aef016e` `a77c280`
- **Output resolution** on the Command bar, beside CPU / FPS. `a77c280`

### 💅 Changed
- **Seamless cards** — the Empire docks now wear styled headers (no window chrome) over a unified panel palette, so the whole layout reads as the cinematic mockup. `f53c2e5` `aef016e`
- Source cards show an eye marker — ◉ shown / ○ hidden. `aef016e`

### 🛠️ CI
- Inherited upstream workflows (Push, Scheduled) are guarded to skip on forks — no more false red ❌ on release tags. `93d4099` `7e13942`

---

## 🟥 [v0.7.0] — Modern Interface &nbsp;·&nbsp; `Update #011` &nbsp;·&nbsp; 2026-06-08

> *A cinematic command center.*

### ✨ Added
- **Empire Modern theme** — a bold, cinematic UI overhaul, now the **default** look. `c14ea83` `b107ee7`
- **Command Center bar** — a top status strip: Empire branding, active profile / scene collection, live CPU + FPS, ON AIR / REC timer, clock, and one-click Go Live / Record / Studio. `4b3c4c6` `741844a`
- **Scenes panel** — your scenes as one-click cards. `ce6e5b2`
- **Audio panel** — per-source volume faders + mute, Empire-styled. `70dfafc`
- **Sources panel** — the current scene's sources as cards with show/hide toggles. `355c805`
- **Controls panel** — the *Kontrolki* card: Stream · Recording · Replay Buffer · Studio Mode · Settings · Exit, with live state (active outputs light up red). `a5c415f`
- **Default Empire layout** — on first run the docks arrange themselves into the cinematic layout (a top Command bar + a bottom row of Scenes · Sources · Audio · Controls), replacing the stock docks; fully reversible via **Docks → Reset UI**. `a5c415f`

### 🛡️ Hardened
- **Vertical 9:16** — null-guarded encoders and a re-sync on scene-collection change; added a cppcheck CI pass over the Empire sources. `d67ef48`

---

## 🟥 [v0.6.0] — Polish &amp; Personalize &nbsp;·&nbsp; `Update #010` &nbsp;·&nbsp; 2026‑06‑07

> *Make it yours.*

### ✨ Added
- **Accent themes** — *Empire Blue · Purple · Green* alongside Red / AMOLED / Light (Settings → Appearance); LIVE controls stay red. `3e1f9eb`
- **First‑run welcome** — a one‑time branded intro to the theme, Vertical 9:16, multistream and the performance dock. `3e1f9eb`
- **Vertical layout persistence** — the Custom 9:16 composition (reframe + added sources) is saved and restored across sessions. `f49e334`

---

## 🟥 [v0.5.3] — Vertical Composer &nbsp;·&nbsp; `Update #009` &nbsp;·&nbsp; 2026‑06‑07

> *Compose your 9:16 by hand.*

### ✨ Added
- **Custom (compose) mode** for the vertical dock — freeze the mirror and lay out the 9:16 by hand: **drag** to move, **mouse‑wheel** to scale, with a red selection outline. `64573ac`
- **Right‑click → Add source** (Image · Text · Color · Media · Browser · Camera · Display · Window) drops a source into the vertical scene and opens its properties; **Properties… / Remove** act on the selection.

### 📝 Docs
- **AI background removal** — documented integration of the mature [obs‑backgroundremoval](https://github.com/locaal-ai/obs-backgroundremoval) ONNX plugin → [Background Removal](Background-Removal.md).

---

## 🟥 [v0.5.2] — Vertical Live &nbsp;·&nbsp; `Update #008` &nbsp;·&nbsp; 2026‑06‑06

### ✨ Added
- **Vertical RTMP streaming** — a **Go Live 9:16** button streams the vertical canvas to any RTMP destination (URL + key, persisted), with a dedicated encoder (HW/x264, 6 Mb/s) + auto‑reconnect — independently of and alongside the main 16:9 stream. `bb37eaf`

---

## 🟥 [v0.5.1] — Branding &amp; Templates &nbsp;·&nbsp; `Update #007` &nbsp;·&nbsp; 2026‑06‑06

### ✨ Added
- **Empire app icon** — a red “E” window / taskbar icon replacing the stock OBS mark. `cbd276c`
- **Scene‑template auto‑install** — *Empire Gaming · Just Chatting · Podcast* collections drop into your scenes folder on first run (one‑time, marker‑guarded, never clobbers existing collections). `591144e`

---

## 🟥 [v0.5.0] — Vertical &nbsp;·&nbsp; `Update #006` &nbsp;·&nbsp; 2026‑06‑06

> *Go vertical without a second layout.*

### ✨ Added
- **Vertical 9:16 dock** — a dedicated **1080×1920** `obs_canvas` mirroring your program scene live (`View → Docks → Empire Vertical 9:16`). `09a9e9b`
- **Fill / Fit framing** toggle — crop‑to‑cover or letterbox the 16:9 program into the 9:16 frame. `db9caef`
- **Record 9:16** — one click records the vertical canvas to `empire-vertical-<timestamp>.mp4` (hardware encoder when available, main audio mix). `8384705`

### ⚙️ Technical
- Frontend‑only — **no libobs changes**. The private canvas is drawn straight via `obs_canvas_render` (a private canvas has no core‑composited texture → `obs_render_canvas_texture` showed garbage). Recording binds a dedicated encoder to the canvas video + `ffmpeg_muxer`.

---

## 🟥 [v0.4.0] — Hardening &amp; Health &nbsp;·&nbsp; `Update #005` &nbsp;·&nbsp; 2026‑06‑06

> *Faster CI, enforced quality, live stream‑health.*

### ✨ Added
- **Stream‑health banner** in the Performance dock — GOOD / WARNING / CRITICAL from `max(dropped %, missed %)`. `06dc843`
- **Hardware encoder** for dedicated per‑destination multistream — auto‑selects NVENC / QSV / AMF, x264 fallback (spares CPU). `06dc843`
- **Per‑destination bitrate** in the Multistream dock — blank shares the main encoder; a value gives that destination its own encode.
- **Performance dock** — dropped‑frames % + outgoing‑bitrate live graphs.
- **Theme variants** — `Yami_EmpireAMOLED` (pure‑black OLED) and `Yami_EmpireLight`.
- **Documentation site** (MkDocs Material) → GitHub Pages, + auto‑mirrored **Wiki**.
- **Release automation** — CPack packaging + GitHub Release on `v*` tags.
- **CI workflows** — upstream‑drift watcher · clang‑format lint · docs deploy · wiki sync.

### 🔧 Changed
- **Rebrand** — window title `Empire‑OBS` + package name `empire-obs`.
- **Faster CI** — builds now **cache** Qt6 + CEF + obs‑deps (no multi‑GB re‑download per run). `86f2f9f`
- **Enforced formatting** — the clang‑format lint is now **blocking** (pinned to clang‑format‑18). `d4ee54a`

---

## 🟥 [v0.3.0] — Multistream &nbsp;·&nbsp; `Update #004` &nbsp;·&nbsp; 2026‑06‑06

> *Stream everywhere at once.*

### ✨ Added
- **Native Multistream dock** — manage up to **3 extra RTMP destinations** in‑app (enable · URL · key · live status). `167cad35`
- **One‑click “Start all streams”** button + main‑stream status indicator. `cb5f0178`
- **Multistream Lua script** (`empire-scripts/empire-multistream.lua`) — self‑contained alternative, up to 3 destinations. `2d368a8` `cfeb6a1`

### ⚙️ Technical
- Extra destinations **share the main stream's encoders** (one encode pass → multiple uploads, no extra GPU/CPU).
- Outputs start/stop automatically via frontend `STREAMING_STARTED` / `STREAMING_STOPPING` events.
- Destinations persist to the profile config (`[EmpireMultistream]`).

---

## 🟥 [v0.2.0] — Creator Tools &nbsp;·&nbsp; `Update #003` &nbsp;·&nbsp; 2026‑06‑06

### ✨ Added
- **Scene‑collection templates** — *Gaming*, *Just Chatting*, *Podcast* ready‑made scaffolds (`templates/`). `c8cc503`
- **Smart Setup** — detected‑hardware summary (CPU physical cores + available encoders: NVENC/QSV/AMF/VideoToolbox) on the auto‑config wizard's results page. `dac3ae2`

---

## 🟥 [v0.1.1] — Polish &nbsp;·&nbsp; `Update #002` &nbsp;·&nbsp; 2026‑06‑06

### 🔧 Changed
- **Performance dock** now shows the **peak (max)** value next to the live value on every graph. `e9a6b58`

---

## 🟥 [v0.1.0] — Foundation &nbsp;·&nbsp; `Update #001` &nbsp;·&nbsp; 2026‑06‑06

> *From a stock ZIP to a building, running, branded fork.*

### ✨ Added
- **Netflix Cinematic theme** (`Yami_Empire.ovt`) — `#141414` / `#E50914`, 8–12 px radius, red LIVE state — set as the **default** theme. `8d165ea`
- **Empire Performance dock** — dependency‑free custom‑painted live graphs (CPU · FPS · render · RAM). `8d165ea`
- `empire-windows-x64` CMake preset (Visual Studio 2026 generator, Windows SDK 10.0.26100).

### 🐛 Fixed (Visual Studio 2026 toolchain)
- **STL1011** — silenced deprecated `<experimental/coroutine>` in `libobs-winrt` (MSVC 14.51 turned it into a hard error).
- **MSB3374 / MSB3073** — build runs **serial** to avoid a file‑lock race on the nested x86 virtual‑camera sub‑build.

### 🏁 Baseline
- Forked from upstream OBS `f61619ce3` (2026‑05‑27). First successful build **and** run on VS 2026.

---

<div align="center">

`v0.1.0` → `v0.3.0` &nbsp;·&nbsp; **8 commits** &nbsp;·&nbsp; **4 updates** &nbsp;·&nbsp; 1 day 🎬

</div>

[Unreleased]: https://github.com/Gh0s777tt/empire-OBS/compare/cb5f01783...HEAD
[v0.3.0]: https://github.com/Gh0s777tt/empire-OBS/compare/dac3ae2d3...cb5f01783
[v0.2.0]: https://github.com/Gh0s777tt/empire-OBS/compare/e9a6b5819...dac3ae2d3
[v0.1.1]: https://github.com/Gh0s777tt/empire-OBS/compare/8d165ea30...e9a6b5819
[v0.1.0]: https://github.com/Gh0s777tt/empire-OBS/compare/f61619ce3...8d165ea30
