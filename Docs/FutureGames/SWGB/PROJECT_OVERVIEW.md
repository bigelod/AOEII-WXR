# Star Wars: Galactic Battlegrounds — PROJECT_OVERVIEW (high-level)

**Status: not started, and gated** on AoE2 reaching Milestone 6. See
[../README.md](../README.md) for the shared analysis, the architecture and the
hard rules — none of it is repeated here.

**Recommended as the second port, ahead of Tiberian Sun.** It runs on the
**same engine as AoE2**, and the measurements below say so rather than the
marketing. Of the four later ports this is the one whose adapter is nearly
trivial and whose risk is nearly zero.

Tags as elsewhere: **[measured]** (read from the binary, 2026-11-02),
**[unverified]**, **[owner]**.

---

## 1. What this is

Star Wars: Galactic Battlegrounds and its Clone Campaigns expansion — a 2001
LucasArts commercial total conversion of **Age of Empires II's Genie engine**
into Star Wars. Same isometric 2D sprite game, same flat world, same HUD
shape, different art and a different setting.

Goal is identical to AoE2's: a head-tracked 3D tabletop, a laser pointer, the
HUD lifted off the table, then Paper Mario billboards. Nothing about the design
changes. **This port exists to prove the shared layer is genuinely shared**,
against the easiest possible second target, before the harder Westwood and
Blizzard ports.

---

## 2. What the binary says

| Question | Answer |
|---|---|
| Executables | `Game/battlegrounds_x1.exe` (**Clone Campaigns**, 2,813,952 bytes) and `Game/Battlegrounds.exe` (base, 2,805,760 bytes). `swgbg.exe` at the root and `Game/player.exe` are not the game **[measured]** |
| Versions | Clone Campaigns **1.1** (`Battlegrounds_x1.exe`, "Star Wars Galactic Battlegrounds: Clone Campaigns"); base **1.10** **[measured]** |
| Distribution | **GOG**, with Clone Campaigns. `goggame-*.dll` present; **no copy protection at all** — no SafeDisc, no CD check **[measured]** |
| Bitness | 32-bit PE32, i386, image base `0x00400000`, GUI subsystem — identical to AoE2 **[measured]** |
| Graphics | `DDRAW.dll`, **one** import: `DirectDrawCreate`. **No Direct3D import at all.** Identical to AoE2 **[measured]** |
| DirectDraw interfaces | **Differs between the two exes** — see 2b |
| Mouse | `GetCursorPos`, `SetCursorPos`; **no DirectInput, no raw input** **[measured]** |
| Keyboard | `GetKeyState`, `GetAsyncKeyState`, `GetKeyboardState` **[measured]** |
| Palette | full GDI set — `CreatePalette`, `RealizePalette`, `SetPaletteEntries`, `ResizePalette`, `GetPaletteEntries`, `GetSystemPaletteEntries`, `GetNearestPaletteIndex`. 8-bit palettized, as AoE2 **[measured]** |
| Wrapper present | **none** — no `ddraw.dll`, no DDrawCompat, no cnc-ddraw **[measured]** |
| Patched | The **expansion** exe is dated Oct 2014, so GOG rebuilt it; the base exe is the original Dec 2001 binary **[measured]** |

### 2a. It really is the same engine, and the import table proves it

Set AoE2's `age2_x1.exe` beside SWGB's `battlegrounds_x1.exe`:

| | AoE2 1.0c | SWGB Clone Campaigns |
|---|---|---|
| `VERSION.dll` | 3 functions | **the same 3** |
| `MSVFW32.dll` | 2: `ICInfo`, `MCIWndCreateA` | **the same 2** |
| renamed `winmm` | `WINMX.dll`, 12 functions | `WIN32.dll`, **the same 12** |
| `DDRAW.dll` | 1: `DirectDrawCreate` | **the same 1** |
| `DSOUND.dll` | 1, by ordinal | **the same** |
| `DPLAYX.dll` | 3, by ordinal | **the same 3** |
| `IMM32.dll` | 7 | **the same 7** |
| `ADVAPI32.dll` | 4 registry calls | **the same 4** |
| `ole32.dll` | 4 | **the same 4** |
| `WSOCK32.dll` | 4, by ordinal | **the same 4** |
| `GDI32.dll` | 28, full palette API | **28, the same set** |
| `USER32.dll` | 76 | 77 |
| Direct3D | none | none |

And the section layout matches: `.text`, **`THIS_COD…`**, `.rdata`, `.data`,
**`THIS_DAT…`**, **`Inf32Dat…`**, `.rsrc`. The folder layout matches too —
`AI/`, `Campaign/`, `Data/`, `History/`, `Random/`, `SaveGame/`, `Scenario/`,
`Sound/`, `Taunt/`. Even the `_x1` expansion-executable convention is the same.

**This is not a similar game. It is the same program with different art.**

> **A correction to the AoE2 docs came out of this.** `Docs/ClaudeCode/` called
> `THIS_COD…`, `THIS_DAT…` and `Inf32Dat…` non-standard sections, evidence that
> the supplied AoE2 exe was a no-CD repack. They are present in a **clean GOG
> SWGB build of both executables**, so they are **standard Genie sections**.
> Only `.pdata` and `.patch` are actually AoE2's repack additions. Corrected in
> `PROJECT_OVERVIEW.md` section 2 and `BUG_FIXES.md` item 15.

### 2b. The two executables differ on interfaces, which is worth a second look

| Executable | DirectDraw interface GUIDs present |
|---|---|
| `Battlegrounds.exe` (base, 2001) | `IDirectDraw`, `2`, `4`; `IDirectDrawSurface`, `2`, `3`, `4`; palette; clipper. **DD7 and Surface7 absent** — exactly AoE2's profile |
| `battlegrounds_x1.exe` (Clone Campaigns, GOG 2014 build) | **none at all** — like Tiberian Sun |

So the base game can reach DD4 and the expansion provably cannot reach
anything above **DD1 / Surface1**, which would make it the simplest
interception target of the family.

**This result is surprising enough to re-check at runtime.** Two builds of one
engine should not differ here, and the likeliest explanation is that GOG's 2014
relink dropped an unreferenced GUID table. Static absence does prove the
binary cannot name those IIDs — but on the primary architecture it does not
matter either way, because **cnc-ddraw owns DirectDraw and we never touch it**
(`../README.md` section 2). It matters only on the fallback path, and there the
answer is the usual one: wrap `QueryInterface` and log what the game asks for,
rather than deciding in advance.

### 2c. Mouse, palette and input: fifth in a row

`GetCursorPos` with no DirectInput and no raw input. Full GDI palette API. The
AoE2 pointer hook and everything above the wrapper transfers without
modification — this is now true of **all five** games measured.

---

## 3. Architecture

**Identical to AoE2, with nothing to decide.** cnc-ddraw owns DirectDraw, we
ship as `d3d9.dll` and hook `Present`, monocular, XrApi 0.5 (see
`Docs/ClaudeCode/PROJECT_OVERVIEW.md` section 4 and the root `CLAUDE.md`).

Two things make this the cleanest starting position of the four later ports:

- **No wrapper to remove.** Unlike Tiberian Sun (cnc-ddraw installed) and
  StarCraft (`WMode.dll`), there is nothing in the `ddraw.dll` slot, so adding
  cnc-ddraw is an addition rather than a replacement, with no "does it still
  run without it?" stage.
- **No copy protection.** A GOG build, so unlike Red Alert 2 there is no
  SafeDisc question to settle before a foreign DLL can be loaded.

**Target Clone Campaigns (`battlegrounds_x1.exe`)**, not the base game — it is
the superset and what players run. Confirm with the owner **[owner]**.

---

## 4. Why this is the right second port

The second port's job is to **prove the shared layer is actually shared**
(`../README.md` section 1). It should therefore be the target least likely to
be derailed by something game-specific, and SWGB wins that on every axis:

- **The engine is the same**, measured, not assumed. The adapter reduces to
  the HUD rectangles and the sprite set.
- **Nothing to remove, nothing to defeat** — no wrapper, no DRM.
- **It runs as-is in WinlatorXR** **[owner]**, as all of them do.
- **The reverse-engineering transfers**, and this is the big one. The hardest
  unanswered question in the whole family is AoE2's billboard Route A: reading
  unit and building state from memory. SWGB is the *same engine*, so those
  structures have the **same shape** — different addresses in a different
  build, but the same fields in the same order. No Westwood or Blizzard title
  offers anything like that. If AoE2's Route A work succeeds, most of its
  value lands here and nowhere else.

The cost of being wrong about the shared layer is paid four times
(`Docs/ClaudeCode/PROJECT_OVERVIEW.md` section 7a). Discovering it against the
easiest target is the cheapest possible way to find out.

---

## 5. What differs, and it is mostly art

- **Different HUD rectangles.** Same shape of HUD — resource bar, command
  panel, minimap — but not the same pixels, so the Stage 8 slicing geometry
  must be re-measured. That is the adapter's main job.
- **Different sprites**, which matters only in the billboard phase. SWGB's
  equivalents of SLP/DRS need confirming **[unverified]**, though the engine
  lineage makes the formats very likely the same.
- **The world is flat**, as AoE2's is. No heightmap opportunity here, unlike
  Tiberian Sun and Red Alert 2 — the plain tilted quad is the right answer.
- **Air units matter more.** Star Wars has flying units in a way AoE2 largely
  does not, and a billboard for something airborne has no ground cell to stand
  on. Worth noting now because it bears on the billboard phase's anchor logic
  and on the pointer's ground-plane mapping **[unverified]**.

---

## 6. Risks specific to this title

| Risk | Note |
|---|---|
| **Too similar to be a real test** | the genuine risk of picking it second. If SWGB works, that proves the layer is portable across *one engine*, not across the family. Treat Tiberian Sun as the real portability test and SWGB as the cheap regression test |
| **GOG's 2014 relink differs from the 2001 original** | the expansion exe was rebuilt, which is the likeliest explanation for 2b. Any offsets found for a 2001 build need re-finding |
| **Two executables** | base and Clone Campaigns are separate builds with separate offsets. Pick one |
| **Air units have no ground cell** | a billboard-phase and pointer question, not a tabletop one. Flagged, not solved |
| **Published AoE2 offsets will not simply work** | same engine, different build. The *structures* transfer; the *addresses* do not |
