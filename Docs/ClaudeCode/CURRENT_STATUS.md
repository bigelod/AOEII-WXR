# CURRENT_STATUS

**As of 2026-10-31.** Written at the end of the planning session, before any
code in this repository.

Read [PROJECT_OVERVIEW.md](PROJECT_OVERVIEW.md) for the goal and
[PROJECT_PLAN.md](PROJECT_PLAN.md) for the stages this refers to.

---

## The one-paragraph truth

**Nothing has been built yet, and nothing has run on hardware.** The
repository contains one commit, a README, a licence, and documentation. The
unmodified game runs cleanly in a WinlatorXR container **[owner]**, which
closes Stage 1 and is a genuinely good starting position. Inherited from a
previous Meta.AI session is a `ddraw.dll` proxy skeleton in
`Docs/MetaAI/Phase1Proj` — roughly 90 lines of real code that has **never been
compiled, deployed or run**, and whose architecture rests on a factual error
about how WinlatorXR gets an image to the headset. Its *shape* is reusable;
its plan is not. The next action is reading the game binary, not writing code.

---

## Milestone state

| # | Milestone | State |
|---|---|---|
| 0 | Feasibility assessed, intake complete | **done** — this session |
| 1 | Unmodified game runs in a container | **done** — runs cleanly **[owner]** |
| A | Binary reconnaissance | **blocked** — waiting on the game files |
| 2 | Code loads, logs, build stamp | not started |
| S1 | Spike: mouse motion from in-container | not started |
| 3 | Borderless, display-filling window | not started |
| 4 | XrApi 0.5 client | not started |
| 5 | Sync pixel | not started |
| S3 | Spike: can we present our own frame | not started |
| 6 | **The tabletop, head tracking locked** | not started |
| 7 | Laser pointer | not started |
| 8 | Floating HUD | not started |
| 9 | Input mapping | not started |
| S2 | Spike: the blit census | not started |
| 10 | Depth, comfort, settings | not started |
| 11 | Billboards | **gated on S2** |

---

## The blocker

**The game files are not on a readable path on this machine.** The owner is
copying them **[owner]**.

It gates Stage A, and Stage A gates the first line of real code, because three
things that any hooking code must assume are currently assumptions:

1. Which executable this patch level actually runs (not `empires2.exe`).
2. **Which DirectDraw interface version the game uses.** The inherited
   skeleton hooks only the `7` interfaces **[code]**. If AoE2 asks for
   `IDirectDraw2` or `IDirectDraw4` — plausible for a 1999 title — that code
   can never fire, and every hour spent on it is wasted.
3. Whether `version.dll` is imported, which decides the injection vector.

All three are read out of the binary in minutes. None is reliably guessable.
This is the cheapest blocker the project will ever have.

**Needed from the owner:** the path once the files are there. Nothing else.

---

## What exists

### In this repository

```
Docs/general_wxr/     the shared WinlatorXR porting framework (read, not ours)
Docs/MetaAI/          the previous session's plans and skeleton (history)
Docs/ClaudeCode/      these four documents
README.md, LICENSE    one commit
```

`Docs/` and `.vscode/` are **untracked**. Nothing in `Docs/ClaudeCode/` is
committed yet.

Missing, and due at Stage 2: a root `CLAUDE.md` pointing at
`Docs/general_wxr/CLAUDE.md` (the framework asks every port to add that one
line), a source tree, a build script, a `.gitignore` entry for the game files.

### The inherited skeleton — an honest assessment

`Docs/MetaAI/Phase1Proj` is a `ddraw.dll` proxy: `proxy.cpp` forwards ten
exports and QueryInterfaces to `IDirectDraw7`; `hooks.cpp` patches three
surface vtable slots and `CreateSurface`; `d3d11_renderer.cpp` builds a D3D11
device and swapchain on the game's HWND and draws the locked primary surface
as a textured quad; `logger.cpp` writes to `C:\aoe2_phase1_log.txt`.

**Never built, never run** — the MetaAI notes say so themselves, and list
"user deploy in WinlatorXR" as what they were waiting on.

| Keep | Replace |
|---|---|
| the proxy shape — forward exports, hook a vtable, start our own thread | the **injection vector**: `version.dll` + MinHook, for the self-recursion reason in `PROJECT_OVERVIEW.md` section 4 |
| the vtable indices, which are in fact correct (`BUG_FIXES.md` item 2) | the **DD7-only assumption** — pending Stage A |
| logging every blit as the first diagnostic | the **conclusion** drawn from it (`PROJECT_OVERVIEW.md` section 5) |
| fallback-on-failure as a reflex | the **renderer**: D3D9, not D3D11 |
| the build-stamp discipline, as an intention | the logger, which has three defects and no stamp (`BUG_FIXES.md` 6–8) |

There is no XrApi client, no sync pixel, no window handling and no head
tracking anywhere in it. Those are the non-negotiables, and the skeleton
predates knowing about them.

---

## What this session established

Not code — but several things that were previously assumptions, and two that
were wrong:

- **WinlatorXR captures the window.** The inherited plan believed emitted
  D3D11 reaches the headset through DXVK and OpenXR. It does not; there is no
  OpenXR in a mod's path at all. Corrected in `PROJECT_OVERVIEW.md` section 7.
  This is the most consequential finding of the session, because it is what
  adds the sync pixel, the borderless window and the entire XrApi client to a
  plan that had none of them.
- **The billboard premise is probably false**, and a cheap census settles it.
  Three alternative routes are ranked, and Stages 6–10 are built not to depend
  on the answer.
- **The floating HUD is nearly free** — slice the frame, no sprite work. It
  moves from the hard phase to the easy one.
- **Mouse motion is the critical path**, not rendering. InputAPI cannot move
  the mouse, and a laser pointer is the whole input design.
- **AoE2 already runs in VR today**, on WinlatorXR's own non-VR mode, with no
  mod. That is the bar to beat and the shipping fallback.

---

## Next three actions

1. **Owner:** put the game files on a readable path and tell me where.
2. **Me:** Stage A — read bitness, executable, DirectDraw interface version
   and imports out of the binary. Replace the `[unverified]` rows in
   `PROJECT_OVERVIEW.md` section 2 with `[measured]` ones.
3. **Me:** Stage 2 — the source tree, the `version.dll` injection, the logger
   with a build stamp on line 1, and the build-and-deploy script **with its
   freshness guard**, before there is anything complicated to debug.

Then Checkpoint 2 with the owner, and Spike S1 immediately after.

---

## Standing notes for the next session

- `Docs/MetaAI/` is **history, not current truth**. Read it for context; do
  not inherit its architecture. The corrections are in `PROJECT_OVERVIEW.md`
  section 7.
- Nothing in this project is tagged `[measured]` yet. Treat every number and
  every interface name as an assumption until Stage A says otherwise.
- The deviation from "keep the game's renderer" is **deliberate, named and
  argued** in `PROJECT_OVERVIEW.md` section 4. Do not relitigate it, and do not
  extend it — the game's own 2D rendering stays untouched.
- Three spikes (S1, S2, S3) exist because three things are genuinely unknown.
  Running them is not procrastination; building a feature on top of an unrun
  spike is.
