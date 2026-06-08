# 🔏 Code signing &amp; the antivirus false‑positive

Empire‑OBS binaries (the `obs64.exe` and the NSIS installer) are **not code‑signed yet**, so
**Windows SmartScreen / Microsoft Defender may flag them as a threat**. This is a *false positive*:
an unsigned, low‑download‑count `.exe` from an unknown publisher is treated as suspicious by heuristics.
It is **not malware** — it is this open GPL build.

> Switching to an MSI would **not** help — an unsigned MSI is flagged the same way. The only real fix is a
> **code‑signing certificate**.

---

## ⚡ Right now — make it run (user side)

- **SmartScreen** ("Windows protected your PC"): **More info → Run anyway**.
- **Defender quarantined files** (causes *"failed to load module/theme"* after install): open
  **Windows Security → Virus &amp; threat protection → Protection history**, find the blocked item, **Restore** + **Allow on device**, then reinstall.
- Add an **exclusion** for `C:\Program Files\Empire-OBS` (Windows Security → Exclusions).
- Or just use the **portable ZIP** (extract &amp; run `bin\64bit\obs64.exe`; add an exclusion if needed).

---

## 🛠️ The real fix — enable code signing in CI

The CI is **already wired for signing** (see `.github/workflows/empire-build.yaml` → *Code sign installer*). It stays
a no‑op until you add a certificate, then it signs automatically on every `v*` release.

### 1. Get a certificate

| Option | Cost | Notes |
|---|---|---|
| **Azure Trusted Signing** | ~$10/mo | **Recommended.** Microsoft‑run; trusted by SmartScreen *immediately* (no reputation wait). Needs an Azure subscription + a one‑time identity/business check. |
| **SignPath.io (Foundation)** | **Free for OSS** | Free certificates + signing for open‑source projects; requires an application + approval. |
| **Certum Open Source** | ~€/yr | Cheap OV/“open source” cert on a cloud or USB token; identity verification required. |
| Self‑signed | free | **Does NOT help** — Windows doesn’t trust it, SmartScreen still warns. Only useful for internal testing. |

> Reputation also builds over time: as more people download a signed `.exe`, SmartScreen stops warning.

### 2. Add the secrets

Convert the certificate to a base64 `.pfx` and add two **repository secrets**
(*Settings → Secrets and variables → Actions*):

```powershell
# Encode your .pfx to base64 (one line)
[Convert]::ToBase64String([IO.File]::ReadAllBytes("empire-cert.pfx")) | Set-Clipboard
```

| Secret | Value |
|---|---|
| `WINDOWS_CERT_PFX_BASE64` | the base64 string above |
| `WINDOWS_CERT_PASSWORD` | the `.pfx` password |

That’s it — the next `v*` tag produces a **signed installer**. (Azure Trusted Signing uses a slightly different
action instead of a `.pfx`; swap the *Code sign* step for `azure/trusted-signing-action` and set the Azure secrets.)

### 3. (Full coverage) also sign `obs64.exe` + Empire DLLs

Signing only the installer stops the SmartScreen warning on the download. To also stop Defender flagging the
**installed** app, sign the binaries before packaging — add a step before *Package* that runs `signtool sign …`
on `build_x64/rundir/RelWithDebInfo/bin/64bit/obs64.exe` (and the `Empire*` plugin DLLs), then let CPack package
the signed files. Same `signtool` invocation as the installer step.

---

## 📨 Report the false‑positive to Microsoft (immediate, free)

Submit the flagged file so Microsoft whitelists it (usually within a day):

1. Go to **<https://www.microsoft.com/en-us/wdsi/filesubmission>**
2. Sign in → **Submit a file for malware analysis** → choose *"I believe this file is incorrectly detected (false positive)"*
3. Upload `empire-obs-vX.Y.Z-windows-x64-installer.exe`, set detection = the name Defender showed, submit.

Do the same at other engines via **VirusTotal** if needed.

---

*Until a certificate is in place, the README and every release note carry a "not signed yet — false positive" notice
so users aren’t alarmed.*
