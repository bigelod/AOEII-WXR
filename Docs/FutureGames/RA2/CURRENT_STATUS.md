# Red Alert 2 / Yuri's Revenge — CURRENT_STATUS

**As of 2026-11-01.** Not started, and deliberately gated.

---

## State

| | |
|---|---|
| Phase | **Reconnaissance done.** No code, nothing of ours on hardware |
| Runs in WinlatorXR? | **Yes, as supplied, with no DirectDraw wrapper installed** **[owner]**. Clean on the wrapper question, but it still carries **SafeDisc remnants**, so SWGB is the cleaner start overall |
| Gate | AoE2 **Milestone 6**, and preferably Tiberian Sun first |
| Position in the family | **ranked 3rd of the four later ports** (`../README.md` section 3) |
| First action when unblocked | **Stage R1** — settle whether SafeDisc objects to a foreign DLL. R0 is already done |

No `BUG_FIXES.md` in this folder: there is no code to have defects in. Create
one with the first commit of real source.

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

- `gamemd.exe` — "Main executable for Yuri's Revenge", stock Westwood
  Studios © 2001, version resource `1.05`. 32-bit PE32, image base
  `0x00400000`. `game.exe` is the Red Alert 2 build alongside it.
- **Unmodded** — no `Ares`, `Phobos` or `Syringe` strings anywhere.
- `DDRAW.dll` → `DirectDrawCreate` only. **No Direct3D import at all.**
- DirectDraw interface GUIDs: DD1/2/4 and Surface1/2/3/4 present, **DD7 and
  Surface7 absent**. **The same profile as AoE2**, so the AoE2
  `QueryInterface`-wrapping design applies here unchanged.
- Mouse via `GetCursorPos`; **no DirectInput, no raw input** — the AoE2 laser
  pointer hook transfers directly. Third of three games with this profile.
- `binkw32.dll` imported for the full-motion cutscenes.
- **SafeDisc remnants present**: `SECDRV.SYS`, `DRVMGT.DLL`, `Blowfish.dll`,
  `PATCHW32.DLL`. The only one of the four installs with live copy protection,
  and the one genuinely new risk in this port.
- No DirectDraw wrapper currently installed.

Full detail and the reasoning in [PROJECT_OVERVIEW.md](PROJECT_OVERVIEW.md).

---

## Open questions, for when this starts

1. **Does SafeDisc object to a foreign DLL?** `SECDRV.SYS` is a kernel driver
   and there is no Windows kernel in the container, so it is probably inert —
   and the game **already starts in a container** **[owner]**, which rules out
   the worst case. What remains is whether it tolerates our DLL.
2. **Yuri's Revenge only, or Red Alert 2 as well?** **[owner]** The plan
   assumes `gamemd.exe` alone.
3. **What resolutions does stock RA2 accept?** Weaker here than Tiberian Sun's
   CnCNet patch, and it is the main quality knob.
4. **Can the per-cell terrain height be read?** Decides the heightmap table.
5. **Is the frame rate affordable?** Busiest screen of the five games.

---

## Why this one is worth doing anyway

Two things, recorded so they are not lost:

- **The sidebar.** RA2's HUD is the largest and busiest of the five titles, and
  a player watches it continuously. Lifting it off the table and hanging it in
  space is more valuable here than anywhere else — this is the port where the
  floating HUD could be the headline feature rather than a supporting one.
- **Voxel vehicles.** Westwood's vehicles are `.vxl` models rasterised to
  sprites at runtime, so the 3D data ships with the game. Real geometry on the
  table, rather than billboards, is reachable in a way it simply is not for
  AoE2 — and if Tiberian Sun is ported first, that work arrives already done.
