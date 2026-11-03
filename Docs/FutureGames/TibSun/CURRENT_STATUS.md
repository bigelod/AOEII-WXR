# Tiberian Sun — CURRENT_STATUS

**As of 2026-11-01.** Not started, and deliberately gated.

---

## State

| | |
|---|---|
| Phase | **Reconnaissance done.** No code, nothing of ours on hardware |
| Runs in WinlatorXR? | **Yes, as supplied** **[owner]** — but *with cnc-ddraw installed*. Whether it runs without it is Stage T0b |
| Gate | AoE2 **Milestone 6** — a tabletop with head tracking the owner confirms feels locked |
| Position in the family | **ranked 2nd of the four later ports** (`../README.md` section 3) — and the **first real portability test**, being the first on a different engine |
| First action when unblocked | **Stage T0b** — does `Game.exe` run with cnc-ddraw removed? Needs no code |

There is no `BUG_FIXES.md` in this folder because there is no code to have
defects in. Create one with the first commit of real source.

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

- `Game.exe`, **2.03-CnCNet-patch**, 32-bit PE32, image base `0x00400000`.
- `DDRAW.dll` → `DirectDrawCreate` only. **No Direct3D import at all.**
- **No DirectDraw interface GUID appears anywhere in the binary**, so the game
  uses `IDirectDraw` / `IDirectDrawSurface` **version 1** and nothing else.
  Tied with SWGB's expansion exe as the cleanest interception target of the
  five games.
- Mouse via `GetCursorPos`; **no DirectInput, no raw input** — the AoE2 laser
  pointer hook transfers directly.
- Carries `.p_text` / `.p_rdata` / `.p_data` / `.p_bss` patch sections.
- `ddraw.dll` present is **cnc-ddraw 1.1.4.15** (cncnet.org), `Renderer=gdi`.
  The owner has confirmed it may be removed.
- Launched indirectly through `Launcher.exe` / `tsconfig.exe` / `ts-spawn.exe`.

Full detail and the reasoning in [PROJECT_OVERVIEW.md](PROJECT_OVERVIEW.md).

---

## Open questions, for when this starts

1. **Does it run without cnc-ddraw?** Stage T0b. It runs *with* it already
   **[owner]**; everything else depends on the version without.
2. **Which injection vector?** Needs the full import table read properly. The
   vacated `ddraw.dll` slot is a legitimate candidate here in a way it is not
   for AoE2.
3. **Firestorm or base game**, or both? **[owner]**
4. **Is the per-cell terrain height field reachable?** Decides whether the
   heightmap table is possible at all.
5. **What resolutions does the CnCNet patch accept?** This is the
   supersampling and sharpness story.

---

## Note to the next session

The valuable thing in this folder is section 1 of `../README.md`: all four
games share one integration surface. If the AoE2 port is being written as a
single program rather than a shared layer plus an adapter, **say so now** —
that is the one decision that makes or breaks this port, and it is a decision
being taken in the AoE2 code today, not here.
