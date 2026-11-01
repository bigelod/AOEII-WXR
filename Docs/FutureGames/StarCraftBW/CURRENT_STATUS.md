# StarCraft: Brood War — CURRENT_STATUS

**As of 2026-11-01.** Not started, and deliberately gated.

---

## State

| | |
|---|---|
| Phase | **Reconnaissance done.** No code, nothing of ours on hardware |
| Runs in WinlatorXR? | **Yes, as supplied** **[owner]** — but *with `WMode.dll` present*. Whether it runs without it is Stage S0b |
| Gate | AoE2 **Milestone 6**, and preferably after Tiberian Sun and Red Alert 2 |
| Position in the family | **recommended fourth and last** (`../README.md` section 3) — the odd one out architecturally, and the title with the highest release bar |
| First action when unblocked | **Stage S0b** then **Spike S1** — does it run without `WMode.dll`, and will Storm load a `ddraw.dll` of ours? Neither needs much code |

No `BUG_FIXES.md` in this folder: there is no code to have defects in. Create
one with the first commit of real source.

---

## Hard scope — 1.16.1 only, no Battle.net

**[owner]** The target is **classic Brood War patch 1.16.1**. Both other things
called "StarCraft" are out of scope, for the same reason:

- **Remastered** — different renderer, and a Battle.net account.
- **The modern free-to-play "classic" release** (1.18+) — rebuilt around
  Battle.net account login and DRM.

**We do not touch Battle.net DRM and we do not work around it.** No
authentication bypass, no DRM work, no online path — not because it is hard,
but because it is a line this project does not cross. Offline, skirmish and LAN
only.

1.16.1 is therefore the *right* target rather than a convenient one: it is the
last build that runs standalone and offline from files the player owns, with no
account and no authentication server. The inspected copy is 1.16.1
**[measured]** and already runs that way.

---

## When this migrates

This folder becomes its own repository when the port starts **[owner]**.
It must leave with three things beside it: `Docs/general_wxr/` copied
unchanged, this folder promoted to `Docs/ClaudeCode/`, and
**`Docs/AoE2Notes/`** — the finished AoE2 port's findings, which for this
family are the implementation manual rather than background reading. See
`../README.md` section 5. The shared *code* is not copied in: the AoE2
repository is public, so it is read from there — but record the commit any
adapted code came from.

---

## What has been established

From the owner's local copy, 2026-11-01, by PE import table and GUID search.
The path is **deliberately not recorded in this repository**.

- `StarCraft.exe`, **1.16.1** in both FileVersion and ProductVersion. 32-bit
  PE32, image base `0x00400000`.
- **The executable imports `storm.dll` and nothing else graphical** — 135
  functions, all by ordinal. Storm owns the window, the surface, the palette
  and the blit.
- **Storm loads `ddraw.dll` dynamically**: it carries the strings `ddraw.dll`
  and `DirectDrawCreate`, imports `LoadLibraryA` / `GetProcAddress`, and has a
  "DirectDraw services are not available" error path. Since `LoadLibrary`
  searches the application directory first, **a `ddraw.dll` of ours should be
  what Storm gets** — and `WMode.dll` in the install is evidence the community
  hooks this game exactly that way. Spike S1 confirms it.
- **No Direct3D** in either the exe or Storm.
- **No DirectDraw interface GUID in either file**, so `IDirectDraw` /
  `IDirectDrawSurface` **version 1** only — tied with Tiberian Sun as the
  simplest wrapper of the four.
- Palette API (`CreatePalette`, `RealizePalette`, `SetPaletteEntries`) plus
  `GetDIBits` / `CreateDIBitmap` lives in **`storm.dll`**, not the game — so
  the palette lookup table is built from Storm's calls.
- Mouse via `GetCursorPos`; `ClipCursor` and `SetCursorPos` also present;
  **no DirectInput, no raw input**. The AoE2 laser pointer hook transfers
  directly — the fourth game in a row for which that is true.
- Assets are `.mpq` archives. `WMode.dll` / `WMode_Fix.dll` (windowed-mode
  wrapper) and `SFNoCD.dll` are present; the wrappers are removable **[owner]**.

Full detail and the reasoning in [PROJECT_OVERVIEW.md](PROJECT_OVERVIEW.md).

---

## Open questions, for when this starts

1. ~~**Will Storm load our `ddraw.dll`?**~~ **Largely dissolved** by the
   revised architecture (`../README.md` section 2): cnc-ddraw takes the
   `ddraw.dll` slot and we hook **D3D9** instead, so we never need Storm to
   load anything of ours and never hook it by ordinal. What remains is the
   narrower question of whether **cnc-ddraw** is loaded by Storm's dynamic
   `LoadLibraryA("ddraw.dll")` — very likely, since that is how `WMode.dll`
   already works here. Spike S1 now checks that.
2. **Does 1.16.1 run with `WMode.dll` removed?** Stage S0b. It runs *with* it
   already **[owner]**.
3. **How are control groups and hotkeys bound on a controller?** This game
   lives on them more than the other three, so the radial-menu question from
   AoE2 Stage 9 is sharpest here.

---

## Why this one is last, and worth it anyway

**Last** — though the margin narrowed sharply. The reason was that the
executable imports nothing but Storm, by ordinal, putting the interception
point one level further away than in the other three games. **The revised
architecture mostly removes that** (`../README.md` section 2): cnc-ddraw sits
under Storm and we hook D3D9, so Storm stops being in our way. What keeps
this port last is no longer technical — it is that StarCraft has by far the
largest audience of the four, which raises the release bar and makes it the
worst candidate for a learning exercise.

**Worth it anyway** because of two things no other title in the family offers:

- **The simplest tabletop of the four.** The world is genuinely flat — no
  per-cell elevation, cliffs drawn as art. Where Tiberian Sun and Red Alert 2
  have a heightmap *opportunity*, StarCraft has one less *problem*, and a plain
  quad is the correct answer rather than a compromise.
- **The best-documented sprites.** `.grp` inside `.mpq`, both formats
  exhaustively documented with open-source readers available. If the billboard
  phase ever needs to render from the game's own assets rather than scrape the
  screen — AoE2's Route A — this is the title where that path is best
  supported. Read at runtime from the player's own install; **never commit an
  archive or anything extracted from one**.
