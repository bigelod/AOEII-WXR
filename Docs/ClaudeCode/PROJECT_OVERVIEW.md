# PROJECT_OVERVIEW — Age of Empires II to standalone VR via WinlatorXR

**What this is.** The goal, the intake answers, and the decisions taken
deliberately so they are not relitigated each session. The staged plan is in
[PROJECT_PLAN.md](PROJECT_PLAN.md); live state is in
[CURRENT_STATUS.md](CURRENT_STATUS.md); defects are in
[BUG_FIXES.md](BUG_FIXES.md).

Read `Docs/general_wxr/` first — this document does not re-derive what that
set already settles, and cites it rather than repeating it.

Claims are tagged **[owner]** (stated by the owner), **[spec]** / **[runtime]**
(from the general_wxr protocol docs), **[code]** (read from the MetaAI
skeleton in this repo), **[unverified]** (my expectation, not yet measured) and
**[measured]** (established on hardware or from a binary). Nothing here is
tagged `[measured]` yet, which is itself the honest summary of the project.

---

## 1. The goal

Age of Empires II is a **2D sprite game with no 3D renderer and no camera**.
It is not a flat game that needs a VR camera written; it is a flat game that
needs a 3D scene **invented around it**. That distinction drives every
decision below.

Three things, in increasing order of ambition and risk:

1. **Tabletop.** The game's 2D frame rendered onto a surface in a 3D room,
   viewed with 6DOF head tracking — a war table you lean over rather than a
   screen you sit in front of.
2. **Floating HUD.** The resource bar, command panel and minimap lifted off
   the table and hung in space around it, scaled to be readable.
3. **Billboards.** Units, buildings and trees extracted from the 2D frame and
   stood upright on the table, facing the viewer — Paper Mario. The terrain
   stays flat; the things standing on it do not.

**Goal 3 is the moonshot, and goals 1 and 2 are not a consolation prize for
it.** A head-tracked war table with a laser pointer and a floating HUD is a
complete, shippable VR experience for an RTS, and it is reachable without
answering any open research question. Goal 3 rests on an assumption that is
very likely false in its simplest form (section 5).

### What the player does with their hands

`FIRST_STEPS.md` names the genre answer directly: *"an RTS wants a tabletop
view and a laser pointer"*, and lists accepting no head-look as viable
precisely for this genre. So:

- **The head does not aim. It looks.** The camera is a viewpoint onto a table,
  never a cursor. This removes the entire class of comfort problem that
  first-person ports fight, and it is why this port can be comfortable by
  construction.
- **A controller ray is the mouse.** Point at the table, the cursor goes
  there; trigger clicks. See section 4, "the pointer", for why this is the
  single riskiest piece of plumbing in the project.
- **No hands in view.** A ray and a dot on the table, not modelled hands.

---

## 2. Intake — answers only the owner can give

### The game

| Question | Answer |
|---|---|
| Game, version, patch level | AoK base + The Conquerors **1.0c** — confirmed from the version resource: `AGE2_X1.EXE`, FileVersion `00.07.26.0809`, "Age of Empires II Expansion", Microsoft Corp. 2001 **[measured]**. **UserPatch is out of scope** — decision taken, section 2a |
| Executable | `age2_x1/age2_x1.exe`, 3,145,728 bytes. **Not `empires2.exe`**, which is the base-game binary **[measured]** |
| Store or distribution | A no-CD/repack build: the exe carries four non-standard appended sections (`THIS_COD…`, `THIS_DAT…`, `Inf32Dat…`, `.patch` at 0x6c000 bytes) alongside the original 2001 link timestamp **[measured]**. Consequence: **this is already a modified binary**, so published offsets for a pristine 1.0c need re-finding regardless of patch level |
| Install path (target) | `D:/VR/AoE2-WXR` — does not exist yet, will be created **[owner]** |
| Install path (for inspection) | A local path on the owner's machine, **deliberately not recorded in this repository** **[owner]** |
| Bitness | **32-bit confirmed** — PE32, machine `0x014c` (i386), image base `0x00400000`, subsystem 2 (GUI) **[measured]** |
| Renderer | **DirectDraw only, and nothing else.** The exe imports exactly one function from `DDRAW.dll` — `DirectDrawCreate` — and imports **no** `d3d*.dll` at all; the strings `D3D`, `Direct3D` and `ddraw` do not occur **[measured]**. The interface-version question is settled in section 2b |
| Engine | Genie (in-house, Ensemble Studios) |
| Source available? | No. No source release, no SDK. |
| Mod API? | None for rendering. UserPatch 1.5 adds data and AI extensibility, not a render hook. |
| Does a VR build exist? | No. Neither AoK/AoC nor the HD/DE remasters have a VR mode, so `WINLATORXR_DESC.md` Path E does not apply. |
| Existing PCVR mod to retarget? | None known. |
| Modding community? | Large and long-lived — AoC/UserPatch memory layout, the SLP sprite format, DRS archives and the palette are all documented by the scenario, AI and openage communities. **Search before disassembling** (`TOOLS.md`). |
| Native head-tracking support? | No. Path D does not apply. |

### The port

| Question | Answer |
|---|---|
| Real VR port or injection port? | **Real VR port.** XrApi poses, our own 3D scene. An injection port cannot produce a tabletop, because there is nothing in the game to inject a camera into. |
| Target headsets | Meta Quest 3 **[owner]** |
| Presentation mode | **Monocular** (`MODE_VR = 1`, `MODE_3D = 0`), and that is the commitment, not a first step **[owner]**. Stereo turns billboards into cardboard cutouts — section 3. AER measured once for the tabletop-only configuration; **SBS never built**. |
| Ship a non-VR mode? | **Yes — and it exists for free before we write anything.** Section 3. |
| Container resolution | Starting point 2800x1400; square-ish modes to be measured. Section 6. |
| Framing | **Tabletop / strategy.** Not first-person, not third-person. |
| Hands in view? | No. A laser pointer. |
| Mouse *motion* needed? | **Yes — and this is the critical path.** InputAPI cannot move the mouse (`WINLATORXR_DESC.md` Path B). Section 4. |
| Multiplayer in scope? | Out of scope. Render-only changes should not desync, but nothing is being done to guarantee it. |

### Constraints

| Question | Answer |
|---|---|
| May the result be published? | Yes — as a mod. Sibling convention is MIT (see `Interstellar-Marines-WXR`). |
| May the game's source appear in the repository? | **There is no source to leak.** The repository contains only our own code. No game assets, no extracted sprites, no palette dumps — the player's own install supplies all of it at runtime. This is a stronger rule than it sounds: a committed test fixture of SLP data is redistribution. |
| Release-owned files | None yet. Declare them here before automation touches a `Release/` folder. |
| Who verifies on hardware | The owner, on a Quest 3. Every milestone ends with them, not with me. |
| Prior work to inherit | Only `Docs/MetaAI/Phase1Proj` **[owner]**. Its proxy shape is partly reusable; its architecture rests on a factual error (section 7). |

---

## 2a. Stage A findings — what the binary says

Read from the owner's local copy with a PE import-table reader and a GUID
search. All **[measured]**, and three of them change the plan.

### UserPatch is out of scope — settled

The project was initially specified as *v2.0a + Expansion 1.0c + UserPatch
1.5*, but the supplied copy has **no UserPatch on it**: no `age2_x1.5.exe`
(UserPatch installs a separate executable rather than overwriting
`age2_x1.exe`), no `Script.Ai/` or `Script.Rm/`, no `.per` files, and the
string `UserPatch` does not occur anywhere in the binary. The `AI/` folder
contains a single `AI.txt`.

**The owner has since dropped the UserPatch requirement** **[owner]**: it is
not needed, and the supplied build already runs in WinlatorXR. **The target is
the supplied stock Conquerors 1.0c**, and that is now a settled decision rather
than an assumption — nothing should be designed for a UserPatch binary, and
nothing is blocked waiting on this.

One consequence to carry forward, in section 6: without UserPatch there are no
custom or widescreen resolutions, so the game is limited to its stock modes.
That turns out not to matter much, for a reason specific to this port.

What *is* present: a no-CD patched 1.0c, plus two local DLL overrides already
in use — `winmx.dll` (an OGG-WinMM build, a renamed `winmm` wrapper that
plays the CD soundtrack from files; configured by `winmx.ini`) and a
replacement `dsound.dll`.

One piece of incidental good news: the install **already demonstrates
local-DLL redirection working for this game**, twice. Adding a third is
normal here, not novel.

### 2b. The game cannot use the DirectDraw 7 interfaces

The decisive measurement. The binary carries a contiguous GUID table holding
`CLSID_DirectDraw`, `IID_IDirectDraw`, `IID_IDirectDraw2`,
`IID_IDirectDraw4`, `IID_IDirectDrawSurface`, `IID_IDirectDrawSurface2`,
`IID_IDirectDrawSurface3`, `IID_IDirectDrawSurface4`,
`IID_IDirectDrawPalette` and `IID_IDirectDrawClipper`.

**`IID_IDirectDraw7` and `IID_IDirectDrawSurface7` are absent** — from both
`age2_x1.exe` and `empires2.exe`. A binary cannot `QueryInterface` for a GUID
it does not contain, so the game provably never uses those interfaces. It
enters through `DirectDrawCreate`, which returns `IDirectDraw` (version 1),
and the highest it can climb is **DD4 / Surface4**.

**The inherited skeleton therefore could never have worked, for a subtler
reason than "wrong index".** Its indices are right (`BUG_FIXES.md` item 2) and
its `QueryInterface(IID_IDirectDraw7)` would have *succeeded* — real
`ddraw.dll` implements DD7 whether the game wants it or not. But each
interface version has its **own vtable**, so patching DD7's `CreateSurface`
slot intercepts nothing the game does through DD1/2/4. It would have hooked
successfully and logged nothing, which is the worst kind of failure to debug
on a headset.

**Design consequence: wrap, do not vtable-patch.** Return our own wrapper from
`DirectDrawCreate` and wrap `QueryInterface`, so every interface the game asks
for is observed and wrapped as it is handed over. Which version it actually
uses then becomes a log line on the first run instead of a guess, and we stop
caring about the answer.

> **This whole subsection now describes the *fallback* path.** Under the
> revised architecture (section 4) cnc-ddraw handles DirectDraw and we never
> touch it. The measurement still matters: it is what makes the fallback
> specified rather than speculative, and it is the reason the inherited
> skeleton is dead either way.

The method indices are stable across versions, which helps: `Blt` = 5,
`BltFast` = 7, `Flip` = 11 on every `IDirectDrawSurface*`, and
`CreateSurface` = 6 on every `IDirectDraw*` — later versions append methods
rather than reordering them. The one real difference is the surface
description struct: `DDSURFACEDESC` for versions 1–3, `DDSURFACEDESC2` for 4
and 7.

### 2c. The laser pointer is far less risky than feared

The biggest de-risking of the session. From `USER32.dll` the exe imports
`GetCursorPos`, `SetCursorPos`, `ClipCursor`, `GetKeyState`,
`GetAsyncKeyState`, `GetKeyboardState`, `ClientToScreen` and `ScreenToClient`
— and it imports **no DirectInput at all** (`DINPUT`/`dinput8` do not appear
in the import table).

So the game reads the mouse through plain Win32 `GetCursorPos`. Route 1 of
Spike S1 — hook the game's own mouse read and return the point we computed
from the controller ray — is a straightforward IAT hook on a documented
function, with nothing in Wine, X11 or the window manager getting a vote. The
spike still runs, but its expected outcome has moved from "hope one of four
works" to "confirm the obvious one does".

Two cautions that come with it. The game calls **`SetCursorPos` and
`ClipCursor` itself**, so it has opinions about where the cursor is and will
contend with ours — which is another argument for owning `GetCursorPos`
rather than pushing the real cursor around. And keyboard state is read
through `GetKeyState` / `GetAsyncKeyState`, so map scrolling can be fed by
the same technique instead of synthesising events.

### 2d. The 8-bit palette path is the default, not an edge case

From `GDI32.dll` the exe imports `CreatePalette`, `SelectPalette`,
`RealizePalette`, `ResizePalette`, `GetPaletteEntries`, `SetPaletteEntries`,
`GetSystemPaletteEntries` and `GetNearestPaletteIndex` — the full palette API,
which a game only needs in a palettized display mode.

`BUG_FIXES.md` item 3 is therefore not a defensive note about an unlikely
format; it is the **main path**, and the inherited code's `w * 4` copy would
produce garbage on the very first frame.

> **Solved upstream as of the revised architecture (section 4):** cnc-ddraw
> converts the palettized surface to a D3D9 texture for us, so we never see an
> 8-bit surface. This measurement is why that is worth so much — it removes
> work that was guaranteed to be needed, not work that might have been.
> On the fallback path the obligation returns: read the palette, build a
> 256-entry lookup table, rebuild it when `SetPaletteEntries` is called.

### 2e. The injection vector is available — with one correction

`VERSION.dll` **is** imported, so the preferred vector exists **[measured]**.
But correct one thing claimed in section 4: the game imports three real
functions from it — `GetFileVersionInfoA`, `GetFileVersionInfoSizeA`,
`VerQueryValueA` — so our proxy **must forward those three properly**. It is
still three functions against `ddraw.dll`'s dozen-plus, so the argument holds;
it just is not quite "nothing depends on our forwarding".

The `dinput8.dll` fallback is **not available** — there is no DirectInput
import to proxy.

> **Superseded by section 4.** The mod now ships as **`d3d9.dll`**, which is
> both a listed vector in `TOOLS.md` and a module we genuinely are rather than
> impersonate. `version.dll` drops to the fallback list, alongside
> `MSVFW32.dll` and the `winmm`-style redirection this install already proves
> works.

---

## 3. Presentation: monocular, and the baseline we must beat

### The decision: monocular VR

**[owner]** This port is **monocular** — `MODE_VR = 1`, `MODE_3D = 0`. One
render, one image to both eyes, full 6DOF head tracking. The sibling
Interstellar Marines port ships the same way.

**Monocular is not non-VR, and the distinction matters.** We still send
`MODE_VR = 1`, still own the camera, still draw the sync pixel, still get full
6DOF head tracking, and the frame is still presented immersively. What we do
*not* do is render twice from two eye positions. "Mono" describes the number
of views, not the amount of VR.

### Why — and the reasoning is better than "stereo is hard"

The owner's instinct is that lining the billboard sprites up for stereo would
be difficult. The conclusion is right; the mechanism is worth stating
precisely, because it changes what we build rather than just what we skip.

There is no *alignment* problem as such. A billboard is ordinary 3D geometry —
a vertical quad at a world position — and stereo would just be rendering that
same geometry from two camera positions. Nothing needs lining up by hand.

The real problem is a **cue conflict**, and it is worse than an alignment bug
because no amount of care fixes it:

- AoE2's sprites are **pre-rendered art of 3D objects, drawn for one fixed
  isometric viewing angle**.
- A billboard shows **the same flat image to both eyes**.
- So in stereo, the disparity between the eyes says *"this is a flat card
  standing at distance X"*, while the picture itself says *"this is a knight"*.
  The brain believes the disparity. Every unit reads as a **cardboard
  cutout** — which is precisely what it is, and stereo is the cue that reveals
  it.
- **In mono there is no disparity cue at all**, so nothing contradicts the
  flatness, and the illusion survives. This is why Paper Mario works.

So monocular is not a compromise forced by performance. **It is the mode in
which the art style actually reads**, and stereo would actively damage the
central visual idea of the project. Worth writing down plainly, because "add
stereo later" looks like an obvious improvement and is not one.

Two secondary benefits, both real: one render instead of two, which is the
whole performance story; and the sync square's blue channel stays 0 with no
AER eye flag to manage, and no need for the packet's IPD at all.

### The one nuance: stereo would suit the *table*, just not the sprites

A tilted textured quad genuinely is a flat surface at a known distance, so
stereo represents it honestly — depth cues and picture agree. Stereo helps the
tabletop and hurts the billboards.

That matters only because the tabletop ships **first**: Stages 6 to 10 produce
a complete, playable war table with no billboards in it at all. So:

- **Keep the mode selection in one function** and leave `AER` reachable, as
  `XRAPI_V0_4_DESC.md` 5a advises and as HWXR learned the hard way at three
  separate sites.
- **Measure AER once during the tabletop phase**, at Stage 10, before
  billboards exist. If it is comfortable and affordable it is a legitimate
  option *for that configuration*.
- **Default to mono, and switch to mono unconditionally once billboards are
  on.** Never build SBS: it doubles scene cost for a result we have just
  argued is worse.

### The baseline we must beat

**Before any code, AoE2 already runs in VR.** WinlatorXR's own non-VR mode puts
the game on a floating window in the headset and keeps its
right-controller-gyro mouse emulation. That needs no mod at all.

So the honest framing is not "make AoE2 work in VR" but **"beat a floating
window"**. That is the bar every milestone is judged against, and it is also
the **shipping fallback**: a comfort option for players who want it, and a
performance option if the tabletop costs too much.

The modes, in the order this port uses them (`XRAPI_V0_4_DESC.md` 5a):

| Stage | `MODE_VR` | `MODE_3D` | Why |
|---|---|---|---|
| Baseline / fallback | 2 | -1 | flat window, runtime keeps the mouse. Free, no code |
| Menus and pre-game | 0 | — | stops the pose stream we are not using, returns the runtime's own mouse, and drops out of immersive presentation so a whole menu is readable at once. The Interstellar Marines port does exactly this |
| **The port** | 1 | 0 | **monocular tabletop, 6DOF head tracking.** One render |
| Tabletop-only option, measured at Stage 10 | 1 | 2 | AER — depth at mono cost. Legitimate *before* billboards exist; not after |
| Never | 1 | 1 | SBS doubles scene cost for a worse result |

Two cautions that come with `MODE_VR = 0` for menus: **keep sending the
outbound packet every frame** or the stream cannot be turned back on, and
**expect a gap** when returning to `1`, so hold the last good pose across it
rather than treating the silence as a dropout **[runtime]**.

---

## 4. Architecture

**Revised 2026-11-01** on the owner's observation that WinlatorXR can drop
cnc-ddraw in at the top level **[owner]**. That changes the bottom of the stack
substantially and for the better; the previous DirectDraw-interception design
is kept as the ranked fallback in section 4c.

### 4a. The stack

```
age2_x1.exe                       emits DirectDraw calls. Untouched.
  └── cnc-ddraw                   DirectDraw -> D3D9. WinlatorXR drops it in.
      └── OUR d3d9.dll proxy      <-- we live here
          └── DXVK d3d9           -> Vulkan -> the container's display
                                     and the window WinlatorXR captures
```

**We never touch DirectDraw.** The game draws its 2D frame however it likes;
cnc-ddraw turns that into D3D9; we intercept D3D9.

**The hook is `Present`, and the technique is deliberately dumb.** At `Present`
time the backbuffer holds the game's finished 2D frame, whoever produced it. So:

1. copy the backbuffer into a texture (`StretchRect` to an offscreen target),
2. clear, and bind a depth buffer of our own,
3. draw our 3D scene — the tabletop quad using that texture, the HUD quads,
   later the billboards,
4. draw the sync pixel last,
5. call the real `Present`.

Nothing in that depends on understanding cnc-ddraw's internals, which matters
because its internals are not an API. It only depends on "the backbuffer
contains the frame at `Present`", which is true by construction.

### 4b. What this buys, and it is a lot

Four of the project's named risks disappear, not shrink:

| Risk in the previous design | Status now |
|---|---|
| **8-bit palette conversion** — `BUG_FIXES.md` item 3, measured as the *main* path (section 2d), would have produced garbage on frame one | **gone.** cnc-ddraw converts to a D3D9 texture for us |
| **Two presenters on one window** — Spike S3, which this document called the largest live unknown | **gone.** One D3D9 device, and we are inside it |
| **DirectDraw interception** — the DD1-vs-DD4 question (section 2b), `QueryInterface` wrapping, per-game surface quirks | **gone.** We never see DirectDraw |
| **Per-game DirectDraw bugs** across four titles | **inherited as fixed.** cnc-ddraw ships profiles for all four, including `[age2_x1]` and `[EMPIRES2]` |

And for the wider family (`Docs/FutureGames/`), it removes the single largest
risk in the StarCraft port: Storm owning the renderer and loading `ddraw.dll`
dynamically, with a fallback of hooking Storm's exports *by ordinal*.
cnc-ddraw sits underneath Storm as happily as anywhere else, so that work
never happens.

**The layering objection does not apply here.** `TIPS.md` warns against wrapper
layers, and it is right to — but it is warning about *stacking*, and this is
not a stack. `ddraw -> cnc-ddraw -> D3D9 -> DXVK -> Vulkan` is exactly as deep
as `ddraw -> us -> D3D9 -> DXVK -> Vulkan`. There is one translation step
either way; the only question is who writes it, and the answer "someone who
already did, for these four games specifically" is a good one.

**The injection vector becomes obvious, and the trap that killed the inherited
plan does not exist here.** Our mod ships as **`d3d9.dll`** next to the game.
That is a listed vector in `TOOLS.md`, and unlike the `ddraw.dll` proxy it
replaces, it is not impersonating a module whose functionality we do not want
— we genuinely are a D3D9 wrapper.

It also sidesteps `BUG_FIXES.md` item 1, the self-recursion trap. That problem
existed because Wine's builtin `ddraw` is a `.dll.so` with no PE file to copy
aside and rename, so loading "the real ddraw" by name could resolve back to
our own module. **DXVK's `d3d9.dll` is an ordinary renameable PE file.** Ship
it alongside as `d3d9_dxvk.dll`, load that explicitly by its own name, and
name-based DLL override cannot bite. Confirm on device, but the escape hatch
exists here where it did not before.

### 4c. The cost, and the fallback

**We take a hard dependency on cnc-ddraw** **[owner]** — a declared
prerequisite with a pinned known-good version, documented as an install step,
exactly as the sibling Interstellar Marines port requires BepInEx 5. Its
internals are not a stable API, so:

- **Pin the version and state it in the README.** Record which build was
  verified. A player on a different one is the first thing to check when a
  report makes no sense.
- **Depend only on `Present` semantics**, never on how cnc-ddraw draws. The
  design above is deliberately ignorant of its internals for this reason.
- **Require the `direct3d9` renderer**, not `gdi`. The owner confirms D3D9
  works in a container **[owner]**; the Tiberian Sun install's `Renderer=gdi`
  is a per-install preference, not a limitation. If no D3D9 device exists
  there is nothing to hook, so **log the renderer in use on line 2 of our log**
  and fail loudly rather than mysteriously.

Two smaller costs, both accepted: pulling the backbuffer into a texture is a
full-frame GPU copy every frame, and cnc-ddraw's device will not have created
a depth buffer, so we create and attach our own.

**Fallback, if the dependency proves untenable:** write the DirectDraw layer
ourselves, as this document previously planned. The measurements in sections
2b and 2d were taken for that design and remain valid for it — target
DD1/2/4 and Surface1/2/3/4, never the 7 interfaces, wrap `QueryInterface`
rather than patching vtables blind, and convert the palette ourselves. That
route is fully specified and costs the four risks above back. It is a real
fallback, not a theoretical one.

### Decisions taken deliberately

| Decision | Choice | Why |
|---|---|---|
| Bitness | **32-bit, Win32** | the game is 32-bit; no OpenXR runtime to load; stays in-process, which `TIPS.md` says is worth more than any benefit of 64-bit |
| Game's 2D rendering | **untouched** | known-good under Box86/Wine; it is our texture source |
| ddraw -> D3D9 translation | **cnc-ddraw**, pinned, declared prerequisite | four risks removed; already fixed for all four games; same layer depth as writing it ourselves |
| Our hook point | **D3D9 `Present`** | the one thing that is true regardless of cnc-ddraw's internals |
| Our module name | **`d3d9.dll`** | a real D3D9 wrapper rather than an impersonation, and DXVK's own `d3d9.dll` is a renameable PE, so the recursion trap is escapable |
| 3D layer | **D3D9** | the June 2010 SDK is **[proven]** in `TOOLS.md`; DXVK's D3D9 path is what containers are tuned for; small, era-appropriate API |
| Wrappers beyond cnc-ddraw | **none** | no dgVoodoo, no ReShade. WinlatorXR can auto-inject ReShade itself if a player wants to experiment (`TOOLS.md`) — not our problem |
| Process model | **in-process** | no IPC, no second binary. `TIPS.md` records the Shogo port becoming dramatically simpler when its client moved in-process |
| XrApi version | **0.5** | human-readable, the UDP Tester speaks it, poses arrive without declaring spaces, and it is 0.4 plus the InputAPI listener at no cost. Recorded because `version` is read **once** and an unrecognised value yields total silence **[runtime]** |
| Presentation | **monocular, and that is the commitment** | section 3. Not a performance compromise — stereo actively damages billboards |
| Supersampling | **container resolution** | the runtime filters on downsample; no code |
| Settings surface | **in-game overlay**, drawn into our own 3D layer | editing an INI on a headset is painful. We have a renderer, so we can draw a menu |

### The pointer — still the riskiest plumbing, and unchanged by this

Everything in section 2c stands: the game reads the mouse through plain Win32
`GetCursorPos` with no DirectInput **[measured]**, so the laser pointer is an
IAT hook on a documented function. That is independent of how we get frames,
and it is now the *only* remaining item on the critical path that has not been
de-risked by either measurement or this architecture change.

**The design is either-handed** **[owner]**: a ray from whichever controller
the player raises, with the trigger driving the in-game mouse. Better than a
dominant-hand-only pointer for a table, at the cost of an arbitration rule —
there is one OS cursor and two possible rays. The rules, and why each exists,
are in `PROJECT_PLAN.md` Stage 7; the load-bearing one is that **a trigger
press locks cursor ownership until release**, because AoE2's selection is a
drag and a stolen cursor mid-drag ends the box somewhere the player never
pointed.

A useful consequence: **the pointer never asks which hand is dominant.** Keep
the ingest handedness swap, and let it decide only the non-pointer bindings.

Routes for feeding the position, ranked, unchanged:

1. **Hook `GetCursorPos`** and return the point computed from the controller
   ray. Section 2c confirms this is the route. Expected to work.
2. **`SetCursorPos`** — one line, and `WINLATORXR_DESC.md` flags Wine-side
   mouse motion as unverified and *"worth testing early"*.
3. **`SendInput` / `mouse_event` absolute** — `XeRxKEYs` ships both, so the
   shape compiles and runs; whether a fullscreen game under Wine honours them
   is the open part.
4. **InputAPI for clicks only** (`M,TFFFF`), paired with one of the above for
   motion — the hybrid `WINLATORXR_DESC.md` describes.

**Map scrolling takes a safer route.** AoE2 scrolls on arrow keys and minimap
clicks, so edge-scroll is unnecessary: either thumbstick becomes held arrow
keys **[owner]**.

**It must be in-process `keybd_event`, not InputAPI.** InputAPI cannot hold a
key down " + EM + " `WINLATORXR_DESC.md` states keys in a message are pressed
*together* with no separate down/up event, and each message carries a 50 ms
hold between press and release, so a held arrow would need re-sending every
50 ms and would stutter. InputAPI remains the fallback for *discrete* keys
only, where `XeRxKEYs`' `XKeycode.cs` X11 mapping is to be copied wholesale
rather than rebuilt (`TOOLS.md`). Details and the stuck-key discipline in
`PROJECT_PLAN.md` Stage 7.

Note that **grip is bound to right-click** **[owner]**, so the reach-out-and-grab
gesture this document once floated for panning is not available " + EM + " the
thumbsticks cover panning instead.

**Handedness: swap once, at ingest**, then never test the flag again for
anything spatial.

### Renderer — the deviation, restated more cheaply

`general_wxr/CLAUDE.md` lists **"keep the game's own renderer"** as a standing
choice. This port adds a 3D renderer anyway, and the reason has to be precise,
because "we need a modern API" is the exact rationalisation that rule exists
to stop.

The rule's justification is: *"the things you actually need — a camera
transform you can write, a 2D blit for the sync pixel, a window you can resize
— exist in every API back to DirectDraw."* That is correct and complete **for
a game that already renders a 3D world**. AoE2 does not. Stage A settles it
beyond argument: the executable imports **one** DirectDraw function and **no
Direct3D whatsoever**, and the strings `D3D` and `Direct3D` do not occur in it
**[measured]**. There is no projection matrix, no view transform, no depth
buffer and no camera anywhere in this game to keep.

So the deviation is not "modernise the renderer" but **"add the only 3D
renderer this project will have"**, and the rule's cost-benefit does not apply
because no known-good 3D path is being traded away. What we *do* keep is the
game's own rendering of its 2D frame, untouched.

And under the revised architecture the deviation is **cheaper than it was**:
we are no longer replacing the game's presentation path, only adding a stage
to one that cnc-ddraw has already modernised.

---

## 5. What goal 3 rests on, and why it probably fails as stated

The MetaAI plan's Phase 2 premise is: *"AoE2 classic draws via
IDirectDrawSurface7::BltFast and Flip. Intercepting those gives us every
sprite dest x/y + srcRect."* **[code]**

**I expect this to be false [unverified].** AoE2's sprites are SLP graphics
needing per-pixel player-colour remapping, shadow blending, and the outline
that shows a unit standing behind a building. None of that is expressible as a
DirectDraw blit on a palettized surface. The Genie engine almost certainly
composites sprites **in software** into a locked surface and then blits or
flips the finished frame — in which case there are no per-sprite blits to
intercept, and hooking `BltFast` yields one call per frame, not one per unit.

This is exactly what the `PROJECT_PLAN.md` template means by *"what must be
proven first"*, and the experiment is cheap: log every `Blt` / `BltFast` /
`Lock` with its rectangle across a few seconds of real gameplay, and count.
Hundreds of small blits per frame means the MetaAI route lives. One big one
per frame means it is dead, and no amount of work on it helps.

**So the billboard routes are ranked, and the blit route is not first:**

- **Route A — read game state.** Get unit and building lists from the game's
  own memory (position, type, graphic, frame, player colour), then render
  sprites ourselves from the player's own SLP/DRS assets. This gives **true
  world coordinates**, which screen-scraping never can, and world coordinates
  are what depth sorting, lighting and a believable table actually need. It is
  the heaviest route and the most likely to work. The supplied exe is a no-CD
  repack with four appended sections, so published stock-1.0c offsets will
  not match — expect to re-find them, with Cheat Engine before a
  disassembler (`TOOLS.md` rates it *"underrated for this work"*).
- **Route B — hook the game's sprite draw.** Find the one software function
  that draws an SLP frame and read its arguments: destination, sprite, frame,
  player. A middle road that yields per-sprite data without reverse
  engineering the whole game state, and plausibly the cheapest route to a
  first standing villager.
- **Route C — blit scraping.** The MetaAI route. Only if the measurement says
  the blits exist.
- **Route D — hybrid.** Billboards for units, the 2D frame for terrain. The
  honest problem: a unit stood up as a billboard is *still drawn flat in the
  captured frame underneath it*, and erasing it means knowing which pixels
  were it. Route A or B gives us that rectangle; Route C does not.

### Goal 2 is much easier than it looks, and that is worth knowing early

The floating HUD needs **no sprite work at all**. AoE2's HUD occupies fixed
screen regions — resource bar along the top, command panel and minimap along
the bottom. Once we have the frame as a texture we can **slice it**: the world
region goes on the tabletop quad, the HUD regions go on separate quads hung in
space. Goal 2 therefore lands in the tabletop phase, not the billboard phase.

Two consequences, one good and one not:

- **Good, and a real synergy.** `WINLATORXR_DESC.md` warns that 2D art does
  not scale with container resolution, so a HUD shrinks as the container
  grows — normally a problem needing a scale setting. Here it is an
  *advantage*: a bigger container means more world pixels and a smaller HUD
  footprint, and because the HUD quads are scaled independently in 3D,
  readability is decoupled from pixel size entirely. The usual trade
  disappears.
- **Not good.** AoE2's HUD overlays the world; the world is not rendered
  *around* it. Slicing the HUD away therefore leaves the strip it covered
  showing HUD pixels, so the table has a bite out of it. Mitigations to test,
  in order: a higher game resolution (the bite shrinks proportionally, and
  stock 1.0c offers a few modes to choose from); cropping the table to the
  world region and accepting a smaller table; or simply accepting it, since
  the covered strip is the bottom edge of the view and a lean brings it back.
  Note that **repositioning the HUD is not available to us** — that would
  have been a UserPatch or widescreen-patch feature, and UserPatch is out of
  scope (section 2a).

---

## 6. Container resolution — two resolutions, not one

`WINLATORXR_DESC.md` treats container resolution as *the* render resolution:
it propagates through `GetSystemMetrics` into whatever mode the game picks,
and the runtime filters on downsample, so rendering above panel resolution is
free anti-aliasing.

**For this port that chain is broken, and deliberately so.** Because we own a
3D presentation layer, there are now **two independent resolutions**:

| | What sets it | What it controls |
|---|---|---|
| **Our render resolution** | the container size | the sharpness of the whole 3D scene — table edges, HUD quads, text we draw, and the supersampling the runtime gets for free |
| **The table's texture resolution** | the mode AoE2 itself runs in | how sharp the *game image on the table* is |

The second is capped by what stock 1.0c offers, since UserPatch is out of scope
(section 2a) — so the game's own mode is limited to its built-in options, up to
around 1280x1024 **[unverified — confirm what it accepts]**.

**This matters much less than it would for a conventional port, and the reason
is worth understanding.** Without our own renderer, a game capped at 1024x768
inside a 2800x1400 container would leave a small game window on a large
desktop — which breaks the display-filling requirement and puts the sync pixel
somewhere the runtime never samples. With our own renderer, the game's frame is
just a texture: we present at full container resolution regardless of what mode
the game chose, and the window fills the display because **we** make it.

So the loss from dropping UserPatch is bounded and specific: the war table's
image is as sharp as AoE2's own best mode and no sharper. Everything else in
the view scales freely.

Practically:

- Start the container at **2800x1400**, the usual starting point.
- Then measure square-ish modes — panels are nearer square than 16:9, and
  `WINLATORXR_DESC.md` notes forcing something like 1400x1400 can be a win.
  The sibling Interstellar Marines port ships 1400x1400.
- **Set the game to its highest stock mode** and leave it there. It is the
  table's texture, and there is no reason to run it lower.
- Tell players both numbers, and which one does what.
- **Judge table readability at Checkpoint 6**, early — it is the one place
  this limit could still change the design, for instance by framing the table
  closer or cropping to the world region more aggressively.

Watch the known trap: games of this era silently restore graphics settings,
and `FIRST_STEPS.md` records it as a live problem in a sibling port. Verify
post-processing and resolution **stay** put across a relaunch.

---

## 7. Corrections to the inherited MetaAI plan

Recorded here so the error is not re-adopted by a later session reading those
docs. `Docs/MetaAI/` is history, not current truth.

**The architecture claim is wrong.** `Docs/MetaAI/PROJECT_PLAN.md` states the
chain is *"empires2.exe -> Wine ddraw -> our ddraw.dll -> D3D11 -> DXVK ->
Vulkan -> OpenXR. D3D11 emitted shows up in VR."* **[code]** It does not.
WinlatorXR **captures the game window** — it does not hook a renderer,
intercept a swap chain or read a texture **[spec]** — and **no OpenXR is
involved for a mod at all**; the integration surface is XrApi over UDP plus one
red pixel. What we draw reaches the headset because it is *in the window*, not
because it is D3D11.

Three things follow that are entirely absent from the MetaAI plan, and all
three are non-negotiables in `general_wxr/CLAUDE.md`:

- **The sync pixel.** Echo `HMD_SYNC` as red, green 0, 10x10, top-left, drawn
  last. Without it the world reprojects against the wrong head pose and
  players get motion sick — and no frame-rate work fixes it. The MetaAI plan
  never mentions it.
- **The borderless, display-filling window**, re-applied every frame. Chrome
  in the corner means the sync pixel is sampled from a title bar.
- **The XrApi client itself** — handshake files, the pose socket, the outbound
  mode packet. The MetaAI plan has no head tracking in it, which means it had
  no VR in it.

**The patch level is wrong too.** Those docs target *"1.0c and HD"* and
`empires2.exe` **[code]**. This project targets **The Conquerors 1.0c as
supplied** **[owner]** — `age2_x1/age2_x1.exe`, the expansion executable,
not the base game's. Any address, offset or exe name inherited from those
docs is suspect; see section 2a for why that holds even against published
stock-1.0c figures.

**What is worth keeping.** The DLL-proxy *shape* — forward exports, start our
own thread, hook a vtable — is sound, and the vtable indices it guessed are in
fact correct (`BUG_FIXES.md`, item 2). The logger, the build-stamp discipline
and the fallback-on-failure instinct are all the right reflexes.

---

## 7a. This port is the first of four — build it as a shared layer

Added 2026-11-01, after inspecting three more installs
(`Docs/FutureGames/README.md`).

Red Alert 2, StarCraft: Brood War and Tiberian Sun were measured the same way
as AoE2, and the result is striking: **all four games have the same
integration surface** **[measured]**. Each is 32-bit PE32 at image base
`0x00400000`, each enters graphics at `DirectDrawCreate`, **none** imports any
Direct3D, **none** can reach the DirectDraw 7 interfaces, each uses 8-bit
palettized surfaces, and **every one of them reads the mouse through plain
Win32 `GetCursorPos` with no DirectInput and no raw input**.

Four studios, three engines, 1995–2001, one interception design.

**So everything in this plan from Stage 3 to Stage 10 is game-independent** —
the borderless window, the XrApi 0.5 client, the sync pixel, the D3D9 tabletop,
the head-tracked camera, the ray-to-table-to-screen-pixel pointer, the HUD
slicing, the presentation-mode handling. Only the numbers change.

**Write it as a shared layer plus a thin per-game adapter, not as one
program.** This is not a refactor to do later. It is a decision being taken in
the AoE2 code *now*, and it is the one decision that makes the next three ports
weeks rather than months. The adapter is the small part: which rectangles are
HUD, where the world region is, the scroll bindings, and how to find unit
state.

Concretely, keep separate: (1) the XrApi client and pose maths, (2) the D3D9
presentation layer and sync pixel, (3) the DirectDraw interception and
frame-to-texture upload, (4) the pointer, and (5) the AoE2 adapter. Items 1, 2
and 4 should not contain the word "AoE2" anywhere.

One incidental finding worth knowing: **cnc-ddraw already ships a compatibility
profile for AoE2** (`[age2_x1]` and `[EMPIRES2]` sections in its bundled
config, found in the Tiberian Sun install). Its source is the best available
reference for what breaks in these games' DirectDraw usage — worth reading
before writing the wrapper, though not worth forking. See
`Docs/FutureGames/README.md` section 2 for the full argument.

---

## 8. The two failure modes to watch for in myself

From `FIRST_STEPS.md`, and both are specifically likely here:

- **Tuning numbers instead of fixing the frame they are measured in.** This
  port will be full of transforms between four spaces — game screen pixels,
  tabletop UV, world metres, head-relative. A value that only works where it
  was found means the frame is wrong.
- **Reasoning about a screenshot when a log line would answer it.** There is
  no debugger on the device. Instrument first; the MetaAI skeleton's instinct
  to log every blit was right even though its conclusion was not.
