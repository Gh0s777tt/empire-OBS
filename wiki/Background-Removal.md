# 🤖 AI Background Removal

![feature](https://img.shields.io/badge/AI-background_removal-E50914?style=flat-square&labelColor=141414)

Remove or blur your webcam background **without a green screen** using on‑device ML segmentation (ONNX).

Empire‑OBS uses the mature, open‑source **[obs‑backgroundremoval](https://github.com/locaal-ai/obs-backgroundremoval)** plugin (by locaal‑ai / Roy Shilkrot) rather than reinventing it — it ships a tuned ONNX runtime and segmentation models and works with any OBS 28+, including Empire‑OBS.

---

## 📥 Install

1. Download the latest Windows installer (or ZIP) from the plugin's **[Releases](https://github.com/locaal-ai/obs-backgroundremoval/releases)**.
2. Run the installer — it auto‑detects OBS. For a **portable** build, copy instead:
   - `obs-plugins/64bit/obs-backgroundremoval.dll`
   - `data/obs-plugins/obs-backgroundremoval/` (models + locale)
3. Restart Empire‑OBS.

---

## 🎬 Use

1. Select your **camera** source → **Filters**.
2. Add **Background Removal** under *Effect Filters*.
3. Pick a model (e.g. *MediaPipe* for speed, *RVM / PPHumanSeg* for quality) and choose **transparent**, **blur**, or a **replacement image**.

Stack it on any scene — including the **[Vertical 9:16](Vertical.md)** composer, where a cut‑out cam over a backdrop looks great in portrait.

---

## ⚙️ Why a plugin, not in‑tree?

ML background removal needs a bundled ONNX runtime (~100 MB) plus tuned models and per‑frame GPU/CPU inference — a substantial, well‑maintained project on its own. Empire‑OBS integrates the proven plugin so you inherit years of tuning and model updates for free, instead of a fragile from‑scratch reimplementation that would bloat the build.
