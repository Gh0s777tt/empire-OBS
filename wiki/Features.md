# ✨ Features

---

## 🎨 Netflix Cinematic theme

Full dark UI in the Netflix palette, set as the **default** theme.

- Surfaces `#141414` / `#0B0B0B`, accent red `#E50914`
- Rounded corners (8–12 px), modern font fallbacks
- Red **LIVE** state on stream/record buttons, cinematic black preview
- File: `frontend/data/themes/Yami_Empire.ovt` — a variant of `Yami`, so it inherits all widget styling
- Variants: **Empire AMOLED** (pure‑black OLED) · **Empire Light** · accent themes **Blue / Purple / Green** (LIVE stays red)

Switch any time: **Settings → Appearance**.

---

## 📈 Empire Performance dock

Live, custom‑painted graphs (no Qt Charts dependency):

| Metric | Source API |
|---|---|
| CPU % | `os_cpu_usage_info_query` |
| FPS | `obs_get_active_fps` |
| Render time (ms) | `obs_get_average_frame_time_ns` |
| Missed frames % | `obs_get_total_frames` / `obs_get_lagged_frames` |
| Memory (MB) | `os_get_proc_resident_size` |
| Dropped frames % | `obs_output_get_frames_dropped` / `…_total_frames` |
| Stream bitrate (kb/s) | `obs_output_get_total_bytes` over time |

Each graph shows the **current** value and the **peak (max)**. A **stream‑health banner** at the top turns
**🟢 GOOD**, **🟡 WARNING**, or **🔴 CRITICAL** based on `max(dropped %, missed %)` while you're live — an
at‑a‑glance "is my stream OK?". Enable via **View → Docks → Empire Performance**.

---

## 🎞️ Scene templates

Ready‑made scene scaffolds for common stream types (`templates/`):

- **Empire — Gaming**: Starting Soon · Gameplay · Just Chatting · BRB · Ending
- **Empire — Just Chatting**: Starting Soon · Main Cam · BRB · Ending
- **Empire — Podcast**: Intro · Podcast · Screen Share · Outro

To use one now: copy the `.json` into your `…/obs-studio/basic/scenes/` folder, then pick it from the
**Scene Collection** menu.

---

## 🧠 Smart Setup

The auto‑config wizard (**Tools → Auto‑Configuration Wizard**) now shows a **detected‑hardware summary** on its
results page:

- CPU physical cores
- Available hardware encoders (NVENC · QuickSync · AMF · VideoToolbox)

…so the recommended settings are easier to understand.

---

## 📡 Multistreaming

Stream to **many platforms at once**. See the dedicated **[Multistream](Multistream.md)** page.

---

## 📱 Vertical 9:16

Record a **1080×1920** vertical version of your scene for TikTok / Reels / Shorts — a dedicated 9:16 canvas that mirrors your program, with **Fill / Fit** framing and one‑click **Record**. See the dedicated **[Vertical 9:16](Vertical.md)** page.
