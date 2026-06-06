# ❓ FAQ

---

### Where do I get the build / is there an installer?
Not yet — build from source (see **[Building](Building.md)**). A CI workflow that produces an `empire-OBS` installer on
every push is on the [roadmap](https://github.com/Gh0s777tt/empire-OBS/blob/empire/main/ROADMAP.md).

### How do I switch to the Netflix theme?
It's the **default**. To re‑select it: **Settings → Appearance → Empire (Netflix)**.

### The Empire docks aren't visible.
**View → Docks → Empire Performance / Empire Multistream**. They may also appear as floating windows on first run.

### Does multistreaming slow down my PC?
No extra **encoding** — all destinations share the main encoder (one encode pass). You only need enough
**upload bandwidth** for the sum of destinations. See **[Multistream](Multistream.md)**.

### Why build serial (no `--parallel`)?
On VS 2026 the parallel build races the nested x86 virtual‑camera sub‑build (`MSB3374`). Serial avoids it.
Details in **[Building](Building.md)**.

### Can I use my normal OBS at the same time?
Yes — run the dev build with `--portable` so it uses an isolated config and never touches your real OBS profiles.

### How do I keep up with upstream OBS?
Add the `upstream` remote and merge `upstream/master`. Empire changes live mostly in frontend/data/scripts, so
merges stay clean. See **[Building](Building.md) → Staying in sync**.

### Why is the repo private?
To protect the work. It's hardened with branch protection + Dependabot — see **[Security](Security.md)**.

### Where's the changelog / roadmap?
[CHANGELOG](https://github.com/Gh0s777tt/empire-OBS/blob/empire/main/CHANGELOG.md) ·
[ROADMAP](https://github.com/Gh0s777tt/empire-OBS/blob/empire/main/ROADMAP.md) — both kept in sync with releases.
