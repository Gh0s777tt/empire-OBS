# 🔒 Security

![visibility](https://img.shields.io/badge/visibility-private-E50914?style=flat-square&labelColor=141414)
![protected](https://img.shields.io/badge/branch-protected-46D369?style=flat-square&labelColor=141414)

This repository is **private** and hardened against accidental or unauthorized changes.

---

## 🛡️ Protections in place

| Control | Status | Effect |
|---|:--:|---|
| Private visibility | ✅ | Not visible or forkable by non‑collaborators |
| Branch protection on `empire/main` | ✅ | **No force‑push**, **no deletion**, **linear history** required |
| Dependabot vulnerability alerts | ✅ | Notifies on vulnerable dependencies |
| Dependabot automated security fixes | ✅ | Opens fix PRs automatically |
| Secret scanning push protection | ⚠️ | Requires GitHub Advanced Security (Enterprise) — not on Pro |

> ℹ️ Forking can't be disabled on user‑owned private repos via the API, but **private visibility already prevents
> outside forks**.

---

## 🔑 Secrets &amp; tokens — rules

> [!CAUTION]
> **Never** commit or paste tokens, keys, or passwords.

- Use `gh auth login` or Git Credential Manager for GitHub auth — not tokens in URLs or files.
- Stream keys live only in OBS config (the Multistream dock stores them in the local profile config, not in git).
- If a token is ever exposed, **revoke it immediately** at <https://github.com/settings/tokens> and rotate.

---

## 🐞 Reporting a vulnerability

This is a personal fork. For issues in **upstream OBS**, report to the
[OBS Project](https://github.com/obsproject/obs-studio/security). For fork‑specific issues, open a **private**
security advisory or a clearly‑marked issue.

---

## ✅ Recommended account hygiene

- Enable **2FA** on the GitHub account.
- Use **fine‑grained** personal access tokens with the **minimum** scopes and an expiry.
- Review **Settings → Security log** periodically.
