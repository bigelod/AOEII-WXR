# CURRENT_STATUS

**As of 2026-11-02.** Stage 2 and Spike S3 **confirmed on a Quest 3**, on the
full pinned stack. The project's first code has run on hardware.

Read [PROJECT_OVERVIEW.md](PROJECT_OVERVIEW.md) for the goal and
[PROJECT_PLAN.md](PROJECT_PLAN.md) for the stages this refers to.

---

## The one-paragraph truth

**Spike S3 is answered yes, in Age of Empires II itself.** With cnc-ddraw
7.1.0.0 on `renderer=direct3d9`, our `d3d9.dll` loaded, hooked
`IDirect3D9::CreateDevice` and then `IDirect3DDevice9::Present`, survived five
device resets, and presented **603 frames** through our hook across a 49-second
session — with the probe rectangle visible in the menu **[owner, screenshot]**.
So the architecture in section 4 of `PROJECT_OVERVIEW.md` holds end to end: the
game's DirectDraw really does arrive as a D3D9 backbuffer we can draw into, and
`Present` is the injection point. That removes the project's one unresolved
critical-path unknown about the renderer. **It ran on Windows, not on the
headset** — Wine, Box86 and the container are all still untested, and the
remaining critical-path unknown is now Spike S1, the pointer, which got
*harder* on inspection (see below). Everything below this paragraph about the
binary remains true.

---

## What the first run established

| Observation | What it means |
|---|---|
| 603 frames through our `Present` | the hook is stable, not a one-shot |
| 5 × `Reset` in 49 seconds, including at startup | **the device is reset routinely.** Anything we later allocate in `D3DPOOL_DEFAULT` must be released before `Reset` and rebuilt after, or the reset fails and takes the game with it. This is a Stage 6 constraint, discovered free |
| `CreateDevice: 0x0` — backbuffer width and height both **zero** | cnc-ddraw passes 0 and lets D3D9 infer the size from the window. **We cannot learn the table's texture resolution from the present parameters**; it has to come from the swapchain or the backbuffer surface description. Section 6's two-resolution split needs that correction |
| the log landed beside whichever executable was launched — root for `empires2.exe`, `age2_x1\` for `age2_x1.exe` | **the DLL follows the EXE, not the data.** Deploy to the folder of the executable being launched; deploying to both costs nothing and the log's own location reports which one won |
| `nonexclusive=true` in the deployed profile | no fullscreen-exclusive device, which is what we want |

**Confirmed on the target.** A second run against `age2_x1.exe` — The
Conquerors, the executable named in the hard constraints — produced the same
result: device hooked, five resets survived, 50 frames through our `Present`,
probe visible **[owner]**. S3 is closed for the target on Windows. It remains
open under Wine and on device.

**Confirmed on the pinned renderer too.** A later pair of runs replaced
Microsoft's D3D9 with **DXVK 1.10.3** (32-bit) as `d3d9_real.dll`, and both
executables behaved identically: device hooked, resets survived, probe visible
**[owner]**. The stack now proven end to end is the one that will ship —

```
age2_x1.exe -> cnc-ddraw 7.1.0.0 (direct3d9) -> ours -> DXVK 1.10.3 -> Vulkan
```

— and it has since run on the **Quest 3 itself** **[owner]**, inside
WinlatorXR, under Wine and Box86, from `D:\Win\Age of Empires II\`. The
device log is identical to the desktop one line for line: same architecture
check, same `CreateDevice`, same hooks, same resets. **Wine, Box86 and the
container introduced no difference at all** — the single most load-bearing
assumption in `PROJECT_OVERVIEW.md` section 4, now measured rather than
argued.

Getting there cost three deploys to a packaging trap; see `BUG_FIXES.md`
#17-18. The device run then produced #19 immediately: its log ends
mid-session with no `DLL_PROCESS_DETACH`, so **nothing important may be
deferred to shutdown** — a rule that would have cost far more had it been
found at Stage 4 with a socket open.

---

## What exists in the tree

| Path | What it is |
|---|---|
| `src/log.h`, `src/log.cpp` | the log. Build stamp on line 1, written beside the DLL, verbose behind a flag file, flushed every line |
| `src/d3d9_proxy.cpp` | the proxy, the two hooks, the probe rectangle, and the cnc-ddraw report on line 2 |
| `src/d3d9.def` | undecorated export names. Without it every export is `_Direct3DCreate9@4` and `GetProcAddress` returns NULL |
| `build/build.bat` | compile. Refuses to run if `cl.exe` is not targeting x86 |
| `build/deploy.ps1` | copy. Refuses a binary older than the newest source file; prints `match=True` only on a SHA-256 match |
| [DEPLOY.md](DEPLOY.md) | **the four files to copy, and what each failure in the log means** |

The game needs a 32-bit DXVK `d3d9.dll` renamed to `d3d9_real.dll` beside
ours. That is not a convenience: the loader keys modules on file name, so once
we are `d3d9.dll`, any request for `d3d9.dll` returns us. See `DEPLOY.md`.

---

## Milestone state

| # | Milestone | State |
|---|---|---|
| 0 | Feasibility assessed, intake complete | **done** |
| 1 | Unmodified game runs in a container | **done** — runs cleanly **[owner]** |
| A | Binary reconnaissance | **done** — findings below |
| 2 | Code loads, logs, build stamp | **done, on hardware** **[owner]** |
| S1 | Spike: mouse motion from in-container | not started — **no longer expected to pass unaided.** cnc-ddraw 7.x locks and rescales the cursor by default; see `PROJECT_PLAN.md` |
| 3 | Borderless, display-filling window | **mostly already true** — `nonexclusive=true` gives borderless fullscreen with a windowed device **[measured]**. What remains is verifying the corner is clean and choosing the container resolution; **no window-style code is planned** |
| 4 | XrApi 0.5 client | not started |
| 5 | Sync pixel | not started |
| S3 | Spike: can we present our own frame | **yes — closed, on a Quest 3** **[owner]**. cnc-ddraw 7.1.0.0 on `direct3d9` into DXVK 1.10.3; desktop and device logs agree line for line |
| 6 | **The tabletop, head tracking locked** | not started |
| 7 | Laser pointer | not started |
| 8 | Floating HUD | not started |
| 9 | Input mapping | not started |
| S2 | Spike: the blit census | not started |
| 10 | Depth, comfort, settings | not started |
| 11 | Billboards | **gated on S2** |

---

## Stage A — what the binary says

Read from the owner's local copy with a PE import-table reader and a GUID
search. The path is **deliberately not recorded in this repository**. Full
detail in `PROJECT_OVERVIEW.md` sections 2a–2e.

| Question | Answer | Effect |
|---|---|---|
| Bitness | PE32, i386, base `0x00400000`, GUI subsystem | 32-bit confirmed; Win32 build, as planned |
| Executable | `age2_x1/age2_x1.exe`, Conquerors 1.0c, 2001 | stop referring to `empires2.exe` |
| Renderer | **one** `DDRAW.dll` import (`DirectDrawCreate`); **no** Direct3D import, no `D3D` strings | settles the renderer deviation beyond argument — there is no 3D path to keep |
| DirectDraw interfaces | DD1/2/4 and Surface1/2/3/4 present; **DD7 and Surface7 absent** | the inherited hooks could never fire. Wrap `QueryInterface`; never target 7 |
| Mouse | `GetCursorPos`, `SetCursorPos`, `ClipCursor` from USER32; **no DirectInput** | the laser pointer is an IAT hook. Biggest de-risking of the project |
| Keyboard | `GetKeyState`, `GetAsyncKeyState`, `GetKeyboardState` | map scroll can be hooked, not synthesised |
| Display format | full GDI palette API imported | **8-bit palettized is the main path**, not an edge case |
| Injection vector | `VERSION.dll` imported — three functions | the preferred vector exists; forward those three properly |
| DirectInput fallback vector | **does not exist** | replaced in the ranking by `winmm`-style redirection, which this install already proves works |

### The two surprises

**1. The skeleton's failure mode would have been a success message.** Its
`QueryInterface(IID_IDirectDraw7)` would have *succeeded* — real `ddraw.dll`
implements DD7 whether the game uses it or not — and it would have logged
"Hooked DD7 CreateSurface". But each interface version has its own vtable, so
nothing the game does through DD1/2/4 would ever have passed through the patch.
A cheerful log line followed by permanent silence is the most expensive kind of
bug to chase on a headset, and reading the GUID table cost ten minutes.

**2. The riskiest thing in the plan got much safer, and it was not the
rendering.** I had the laser pointer as the critical path, with four ranked
routes and a real chance that none worked. The game reading its mouse through
documented Win32 means route 1 is an ordinary IAT hook with nothing in Wine,
X11 or the window manager able to interfere. The spike now exists to confirm,
not to discover.

**What this did not de-risk at the time:** Spike S3, whether our own rendered
frame could reach the window while the game presented through DirectDraw.
**That question has since been removed rather than answered** — see the
architecture change below.

---

## Architecture revised: cnc-ddraw owns DirectDraw, we hook D3D9

**2026-11-01 [owner].** WinlatorXR can drop cnc-ddraw in at the top level, so
the stack is now:

```
age2_x1.exe  ->  cnc-ddraw  ->  OUR d3d9.dll  ->  DXVK d3d9  ->  Vulkan
```

We hook D3D9 **`Present`**: the backbuffer holds the finished 2D frame, so we
copy it to a texture, clear, draw the tabletop and HUD and sync pixel, then
present. Nothing depends on cnc-ddraw's internals, only on `Present`
semantics. Full reasoning in `PROJECT_OVERVIEW.md` section 4.

**Four named risks are gone, not reduced:**

| Risk | Status |
|---|---|
| 8-bit palette conversion — the *measured* main path | cnc-ddraw converts for us |
| Two presenters on one window — was the largest live unknown | one device, and we are in it |
| DirectDraw interception, DD1-vs-DD4, `QueryInterface` wrapping | we never see DirectDraw |
| Per-game DirectDraw quirks across five titles | largely inherited as fixed |

**The cost:** a hard dependency on cnc-ddraw, accepted and documented as a
pinned prerequisite **[owner]**, like the Interstellar Marines port requires
BepInEx. Plus a full-frame GPU copy per frame and a depth buffer of our own.
The previous DirectDraw design survives as a **specified fallback**
(`PROJECT_OVERVIEW.md` 4c) — the Stage A measurements were taken for it and
remain valid.

The injection vector changes with it: **`d3d9.dll`**, not `version.dll`. That
is a module we genuinely are rather than impersonate, and DXVK's own
`d3d9.dll` is a renameable PE file, so `BUG_FIXES.md` item 1's self-recursion
trap has an escape hatch that the `ddraw` route did not.

---

## Settled: monocular VR

**2026-11-01 [owner].** The port is **monocular** — `MODE_VR = 1`,
`MODE_3D = 0`. Still full 6DOF head tracking, still immersive, still the sync
pixel; just one view instead of two.

**The reason is the art style, not the frame budget.** Billboards are flat
pre-rendered images of 3D objects. In stereo, the eye disparity says *"flat
card at distance X"* while the picture says *"knight"*, and the brain
believes the disparity — every unit reads as a cardboard cutout. In mono
there is no disparity cue to contradict the flatness, so the illusion holds.
This is why Paper Mario works. `PROJECT_OVERVIEW.md` section 3.

One nuance kept open: stereo represents a flat tilted *table* honestly, and
the tabletop ships before billboards exist. So AER gets measured once at
Stage 10 for that configuration only. **SBS is never built.**

---

## Settled: UserPatch is out of scope

**[owner]** The UserPatch requirement has been dropped — it is not needed, and
the supplied build already runs in WinlatorXR. **The target is the supplied
stock Conquerors 1.0c**, `age2_x1/age2_x1.exe`.

The one consequence to carry: without UserPatch there are no custom or
widescreen resolutions, so the game runs in its stock modes. That costs less
than it appears to, because owning a 3D presentation layer **decouples our
render resolution from the game's** — the container sets the first, the game's
own mode only sets the table's texture sharpness. `PROJECT_OVERVIEW.md`
section 6.

Offsets still need re-finding for Stage 11, but for a different reason: the
supplied exe is a no-CD repack with two extra sections (`.pdata`, `.patch`),
so published
stock-1.0c addresses were never going to match anyway.

**No stage is blocked.** This was the last open question.

---

## What exists

### In this repository

```
CLAUDE.md             root pointer into general_wxr and these docs
Docs/general_wxr/     the shared WinlatorXR porting framework (read, not ours)
Docs/MetaAI/          the previous session's plans and skeleton (history)
Docs/ClaudeCode/      these four documents
README.md, LICENSE    one commit
```

All committed as of `bf08df2` ("Bringing AI up to speed"), on `main`.

`.gitignore` now also blocks game assets (`*.drs`, `*.slp`, `*.pal`, `*.per`
and the rest) on top of the `*.exe` / `*.dll` rules it already had, so the
no-redistribution constraint in `PROJECT_OVERVIEW.md` section 2 is enforced
mechanically rather than by memory.

Still missing, due at Stage 2: a source tree and a build-and-deploy script
with its freshness guard.

### The inherited skeleton — revised assessment

Harsher than before Stage A. `Docs/MetaAI/Phase1Proj` is a `ddraw.dll` proxy
that was never built or run, and its central mechanism is now known to be
aimed at interfaces the game does not use.

| Keep | Replace |
|---|---|
| the vtable indices, which are correct and in fact generalise across interface versions (`BUG_FIXES.md` item 2) | the **DD7 targeting** — provably dead (`BUG_FIXES.md` item 4) |
| the instinct to log every blit as the first diagnostic | the **conclusion** drawn from it (`PROJECT_OVERVIEW.md` section 5) |
| fallback-on-failure as a reflex | the **injection vector**: `version.dll` + MinHook (`PROJECT_OVERVIEW.md` section 4) |
| the build-stamp discipline, as an intention | the **renderer**: D3D9, not D3D11 |
| | the **approach**: wrap `QueryInterface`, do not patch vtables blind |
| | the logger — three defects, no build stamp (`BUG_FIXES.md` 6–8) |

There is no XrApi client, no sync pixel, no window handling and no head
tracking anywhere in it. Those are the framework's non-negotiables, and the
skeleton predates knowing they were needed.

---

## What the planning session established

- **WinlatorXR captures the window.** The inherited plan believed emitted
  D3D11 reaches the headset through DXVK and OpenXR. It does not, and there is
  no OpenXR in a mod's path at all. This is what adds the sync pixel, the
  borderless window and the entire XrApi client to a plan that had none of
  them.
- **The billboard premise is probably false**, and a one-hour census settles
  it. Three alternative routes are ranked; Stages 6–10 are built not to depend
  on the answer.
- **The floating HUD is nearly free** — slice the frame, no sprite work. It
  moves from the hard phase to the easy one.
- **AoE2 already runs in VR today**, on WinlatorXR's own non-VR mode, with no
  mod. That is the bar to beat and the shipping fallback.

---

## Next two actions

Nothing is waiting on the owner.

1. **Me:** Stage 2 — the source tree, the **`d3d9.dll`** proxy forwarding to
   DXVK's own (shipped alongside under its own name), the logger with a build
   stamp on line 1 and **the cnc-ddraw renderer in use on line 2**, and the
   build-and-deploy script **with its freshness guard**, before there is
   anything complicated to debug.
2. **Me:** Spikes S3 and S1 alongside it — confirm a cnc-ddraw D3D9 device
   exists and get a triangle on screen through its `Present`; and hook
   `GetCursorPos`, return a fixed point, see whether the cursor obeys and
   whether the game fights back.

Then Checkpoint 2 with the owner.

Per `PROJECT_OVERVIEW.md` section 7a, Stage 2 is also where the shared-layer
structure gets set, so the directory layout chosen there is load-bearing for
four later ports.

---

## Standing notes for the next session

- **Never commit the game files or the path to them.** The owner asked for the
  install path to stay out of the repository; it is referred to only as "the
  owner's local copy". Measurements from it are fine, the path is not.
- `Docs/MetaAI/` is **history, not current truth**. Corrections are in
  `PROJECT_OVERVIEW.md` section 7 and `BUG_FIXES.md` items 14–16.
- **Never target the DirectDraw 7 interfaces.** Measured, settled, and the
  single most expensive mistake available in this codebase.
- The deviation from "keep the game's renderer" is **deliberate, named and
  now measured** in `PROJECT_OVERVIEW.md` section 4. Do not relitigate it, and
  do not extend it — the game's own 2D rendering stays untouched.
- Three spikes (S1, S2, S3) exist because three things are genuinely unknown.
  S1 is now expected to pass and S3 is the one to worry about.
- **This port is the first of four.** RA2, StarCraft BW and Tiberian Sun were
  measured on 2026-11-01 and share AoE2's integration surface exactly —
  `DirectDrawCreate`, no Direct3D, no DD7, 8-bit palettized, `GetCursorPos`
  with no DirectInput. Write the mod as a **shared layer plus an adapter**;
  see `PROJECT_OVERVIEW.md` section 7a and `Docs/FutureGames/README.md`. The
  cost of getting this wrong is paid three times over.
