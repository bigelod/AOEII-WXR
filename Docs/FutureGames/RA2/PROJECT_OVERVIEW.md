# Red Alert 2 / Yuri's Revenge — PROJECT_OVERVIEW (high-level)

**Status: not started, and gated** on AoE2 reaching Milestone 6. See
[../README.md](../README.md) for the shared analysis, the wrapper question and
the hard rules — none of it is repeated here.

**Recommended as the third port**, after Tiberian Sun, whose engine lineage it
shares and whose adapter should be most of this one.

Tags as elsewhere: **[measured]** (read from the binary, 2026-11-01),
**[unverified]**, **[owner]**.

---

## 1. What this is

Command & Conquer: Red Alert 2 and its Yuri's Revenge expansion. The same
Westwood isometric engine family as Tiberian Sun, two years later: **per-cell
terrain elevation**, **voxel vehicles**, and a considerably busier screen.

Same three-step goal as AoE2 — head-tracked 3D tabletop, laser pointer, HUD
lifted off the table, then sprites standing up — plus the two
Westwood-specific opportunities in section 4.

---

## 2. What the binary says

| Question | Answer |
|---|---|
| Executables | `gamemd.exe` 4,808,976 bytes (**Yuri's Revenge** — "Main executable for Yuri's Revenge"), `game.exe` 4,485,392 (Red Alert 2). `ra2.exe` and `RA2MD.exe` are small launchers **[measured]** |
| Version | version resource reads **`1.05`**; strings identify stock **Westwood Studios, © 2001** **[measured]** |
| Mods | **none** — no `Ares`, `Phobos` or `Syringe` strings anywhere **[measured]** |
| Bitness | 32-bit PE32, i386, image base `0x00400000`, GUI subsystem **[measured]** |
| Graphics | `DDRAW.dll`, **one** import: `DirectDrawCreate`. No Direct3D import at all **[measured]** |
| DirectDraw interfaces | `IDirectDraw`, `2`, `4` and `IDirectDrawSurface`, `2`, `3`, `4` present; **DD7 and Surface7 absent** — the same profile as AoE2 **[measured]** |
| Mouse | `GetCursorPos`, `SetCursorPos`; **no DirectInput, no raw input** **[measured]** |
| Keyboard | `GetKeyState`, `GetAsyncKeyState` **[measured]** |
| Video | `binkw32.dll` (13 Bink functions) for the full-motion cutscenes **[measured]** |
| Copy protection | **SafeDisc remnants** — `SECDRV.SYS`, `DRVMGT.DLL`, `Blowfish.dll`, `PATCHW32.DLL` **[measured]** |
| Wrapper present | none active |

### Interception profile: identical to AoE2

`DirectDrawCreate` in, DD7 provably unreachable, DD4 the ceiling. The
`QueryInterface`-wrapping design written for AoE2 applies here unchanged — this
is the same job twice.

### The mouse story is the same again

`GetCursorPos`, no DirectInput. Third of three. The laser pointer hook proven
on AoE2 transfers directly.

### The new risk: SafeDisc

AoE2's copy was already a no-CD build and Tiberian Sun's is CnCNet-patched, so
neither had live anti-tamper. This install still carries the SafeDisc pieces.
Two things follow, both **[unverified]**:

- **SafeDisc may object to a foreign DLL in the process**, or to code patching
  the executable's own memory. If it does, that is a problem to solve before
  anything else — and the community's existing no-CD route is the obvious
  answer, since the player owns the game.
- **`SECDRV.SYS` is a kernel driver** that does not exist under Wine at all, so
  the protection may simply be inert inside the container. Quite possibly a
  non-issue. **Establish which, early** — it is a ten-minute test and it
  determines whether this port is cheap or awkward.

---

## 3. Architecture

Identical to AoE2 (`Docs/ClaudeCode/PROJECT_OVERVIEW.md` section 4) and to
Tiberian Sun. Read `gamemd.exe`'s full import table at the start of the port to
choose the injection vector; everything above the wrapper is the shared layer.

**Target Yuri's Revenge (`gamemd.exe`), not Red Alert 2 (`game.exe`).** It is
the superset, it is what players run, and supporting both is a packaging
question for much later. Confirm with the owner **[owner]**.

---

## 4. The opportunities, inherited from the Westwood engine

Both are the Tiberian Sun opportunities, and both should arrive here already
solved if TS is done first. That ordering is the main argument for doing TS
second and this third.

- **A real heightmap table.** RA2 carries per-cell terrain elevation and draws
  its art for sloped ground.
- **Voxel vehicles as real geometry.** RA2's vehicles are `.vxl` models with
  `.hva` animation, rasterised to sprites at runtime — the 3D data ships with
  the game. Infantry and buildings stay billboards.

Neither is planned here. Both get the full "what must be proven first"
treatment before any code, and both read assets from the player's own install
at runtime — **never committed**.

### One thing RA2 does better than either of the others

**The sidebar is genuinely large and genuinely busy** — build queues, a radar
map, tabs. Lifting it off the table onto floating quads is more valuable here
than in any of the other three titles, because there is more of it and because
a player watches it continuously. This is the port where goal 2 (the floating
HUD) might be the headline feature rather than a supporting one.

---

## 5. Risks specific to this title

| Risk | Note |
|---|---|
| **SafeDisc interferes with injection** | test early; it may be entirely inert under Wine. The player owns the game, so the community no-CD route is available if needed |
| **Two executables** | `game.exe` and `gamemd.exe` are separate builds with separate offsets. Pick one — Yuri's Revenge — and say so |
| **Bink cutscenes** | `binkw32.dll` presents its own video; our layer must not fight it. Simplest answer is `MODE_VR = 0` during cutscenes, exactly as for menus |
| **Busiest screen of the four** | more HUD, more units, more sprites. The performance headroom that makes a 1999 game comfortable on a Quest 3 is thinner here |
| **Resolution** | stock RA2 offers few and low modes. Without a CnCNet-style patch, the supersampling story is weaker than Tiberian Sun's **[unverified — establish what it accepts]** |
| **Published offsets will not match** | standard rule. Re-find everything |
