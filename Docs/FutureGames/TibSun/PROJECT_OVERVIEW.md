# Tiberian Sun — PROJECT_OVERVIEW (high-level)

**Status: not started, and gated** on AoE2 reaching Milestone 6. See
[../README.md](../README.md) for the shared analysis, the wrapper question and
the hard rules — none of it is repeated here.

**Recommended as the second port of the family**, because it has the simplest
integration surface of the four.

Tags as elsewhere: **[measured]** (read from the binary, 2026-11-01),
**[unverified]**, **[owner]**.

---

## 1. What this is

Command & Conquer: Tiberian Sun (+ Firestorm), community-patched. A 2D
isometric RTS with **per-cell terrain elevation** and **voxel vehicles** —
which makes it a more ambitious and more rewarding tabletop than AoE2.

The goal is the same three-step shape as AoE2: a head-tracked 3D tabletop with
a laser pointer, then the HUD lifted off the table, then sprites standing up.
With two Tiberian-Sun-specific opportunities in section 4.

---

## 2. What the binary says

| Question | Answer |
|---|---|
| Executable | `Game.exe`, 3,739,648 bytes. `SUN.EXE` (81 KB) is a stub, not the game **[measured]** |
| Version | **`2.03-CnCNet-patch-`** in the version resource **[measured]** |
| Bitness | 32-bit PE32, i386, image base `0x00400000`, GUI subsystem **[measured]** |
| Graphics | `DDRAW.dll`, **one** import: `DirectDrawCreate`. No Direct3D import at all **[measured]** |
| DirectDraw interfaces | **No DirectDraw interface GUID appears anywhere in the binary** — not even DD2 or DD4 **[measured]** |
| Mouse | `GetCursorPos`, `SetCursorPos`; **no DirectInput, no raw input** **[measured]** |
| Keyboard | `GetKeyState`, `GetAsyncKeyState` **[measured]** |
| Patched | Yes — carries `.p_text`, `.p_rdata`, `.p_data`, `.p_bss` sections, which a stock 1999 binary does not **[measured]** |
| Wrapper present | `ddraw.dll` = **cnc-ddraw 1.1.4.15** (cncnet.org), `Renderer=gdi`. Removable **[owner]** |
| Launcher chain | `Launcher.exe`, `tsconfig.exe`, `ts-spawn.exe`, `cncnet5.exe` — the game is normally started indirectly |

### The headline: DirectDraw version 1, and nothing else

The absence of *every* interface GUID is stronger than AoE2's result, where
DD2 and DD4 were at least present. Tiberian Sun cannot `QueryInterface` for
any DirectDraw interface, so it uses exactly what `DirectDrawCreate` hands it:
**`IDirectDraw` and `IDirectDrawSurface`, version 1**.

That makes this the cleanest interception target of the four games. One
interface pair to wrap, no version negotiation, no `DDSURFACEDESC` versus
`DDSURFACEDESC2` branch.

### The mouse story is the same as AoE2

`GetCursorPos` with no DirectInput — so the laser pointer is the same IAT hook,
and if it is proven on AoE2 it is proven here. The game also calls
`SetCursorPos` itself, so expect the same contention.

---

## 3. Architecture

Identical to AoE2 (`Docs/ClaudeCode/PROJECT_OVERVIEW.md` section 4), with two
differences:

- **Injection vector is an open question here.** AoE2's answer was
  `version.dll`, chosen to avoid impersonating `ddraw`. Tiberian Sun's import
  table does not obviously offer the same option, and the game currently loads
  a `ddraw.dll` wrapper anyway — so taking the `ddraw.dll` slot, which is now
  vacant by the owner's decision, may be the straightforward answer. **Settle
  it by reading the import table properly at the start of the port**, not now.
- **The launcher chain matters.** `ts-spawn.exe` and `tsconfig.exe` start the
  game and may set their own DLL overrides and resolution. Our DLL must land
  next to `Game.exe`, and the launcher's configuration has to be accounted for
  rather than fought.

Everything else — D3D9 tabletop layer, XrApi 0.5, sync pixel, borderless
window, mono then AER, `MODE_VR = 0` in menus — is the shared layer.

---

## 4. The two opportunities Tiberian Sun has and AoE2 does not

Recorded as opportunities, deliberately **not** as a plan. Both need the shared
tabletop working first.

### A real heightmap table

Tiberian Sun's terrain carries per-cell elevation, and its art is drawn for
sloped ground. So instead of AoE2's flat quad, the table could be an actual
displaced mesh with the game's frame projected onto it. Leaning over a ridge
line and seeing it rise off the table is the single most compelling thing this
engine offers VR.

**What must be proven first:** that the per-cell height field can be read at
all — from the map file, from memory, or inferred. Unknown, and not worth
guessing at **[unverified]**.

### Vehicles are already 3D

Westwood rendered Tiberian Sun's vehicles from **`.vxl` voxel models** with
`.hva` animation data, rasterised to sprites at runtime. The 3D data ships
with the game and the format is documented by the community.

So the Paper Mario compromise may be unnecessary for the most interesting
objects: vehicles could be **real geometry** on the table, with infantry and
buildings as billboards. That is a better result than AoE2 can reach by
construction, and it sidesteps the question the AoE2 project is stuck on —
whether per-sprite draw calls can be intercepted — because voxel models are
assets to be read rather than screen output to be scraped.

**What must be proven first:** reading unit position, facing and type from
memory, which is the same Route A problem as AoE2 and inherits whatever that
port learns.

---

## 5. Risks specific to this title

| Risk | Note |
|---|---|
| **Removing cnc-ddraw breaks the game** | the game already runs in a container **with** cnc-ddraw **[owner]**; it is there because someone needed it. Establish that `Game.exe` runs with the wrapper **removed** before anything else (Stage T0b). If it does not, that is the first problem |
| **The CnCNet patch interacts with our hooks** | the `.p_*` sections are someone else's code in the same process. Expect it, log it, do not assume our hook is the only one |
| **The launcher chain overrides our configuration** | resolution and DLL overrides may be set by `tsconfig.exe` rather than by us |
| **Published offsets will not match** | this is a patched 2.03 build, not stock. Same rule as AoE2: every inherited address is wrong until re-found |
| **Firestorm vs base game** | two rule sets and two sets of art; confirm which the port targets **[owner]** |
