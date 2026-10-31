# CURRENT_STATUS

**As of 2026-10-31.** Planning plus Stage A reconnaissance complete; no code
written yet.

Read [PROJECT_OVERVIEW.md](PROJECT_OVERVIEW.md) for the goal and
[PROJECT_PLAN.md](PROJECT_PLAN.md) for the stages this refers to.

---

## The one-paragraph truth

**Nothing has been built, and nothing has run on hardware** — but the game
binary has now been read, and that turned five assumptions into measurements.
The unmodified game runs cleanly in a WinlatorXR container **[owner]**,
closing Stage 1. The inherited `ddraw.dll` skeleton in
`Docs/MetaAI/Phase1Proj` is now known to be **unsalvageable as written**: it
hooks the DirectDraw 7 interfaces, and the game's executable does not contain
the DD7 GUIDs at all, so it could never have intercepted a single call. In
exchange, the project's scariest risk got much smaller — the game reads its
mouse through plain Win32 `GetCursorPos` with no DirectInput, so the laser
pointer that the whole input design rests on is a straightforward IAT hook.
One new open question came out of the reconnaissance: **UserPatch 1.5 is not
installed** on the copy supplied, though the project was specified with it.

---

## Milestone state

| # | Milestone | State |
|---|---|---|
| 0 | Feasibility assessed, intake complete | **done** |
| 1 | Unmodified game runs in a container | **done** — runs cleanly **[owner]** |
| A | Binary reconnaissance | **done** — findings below |
| 2 | Code loads, logs, build stamp | **next** |
| S1 | Spike: mouse motion from in-container | not started — *expected to pass* |
| 3 | Borderless, display-filling window | not started |
| 4 | XrApi 0.5 client | not started |
| 5 | Sync pixel | not started |
| S3 | Spike: can we present our own frame | not started — **the live unknown** |
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

**What this did not de-risk:** Spike S3, whether our own rendered frame can
reach the window at all while the game is presenting through DirectDraw. That
is now the project's largest live unknown, and no amount of static analysis
will settle it.

---

## Open decision: UserPatch 1.5

The project was specified as *v2.0a + Expansion 1.0c + UserPatch 1.5*
**[owner]**. The supplied copy has **no UserPatch on it** — no
`age2_x1.5.exe`, no `Script.Ai/` or `Script.Rm/`, no `.per` files, and the
string `UserPatch` does not appear in the binary. What is there is a no-CD
patched Conquerors 1.0c, plus two local DLL overrides already in use
(`winmx.dll`, an OGG-WinMM build playing the soundtrack from files, and a
replacement `dsound.dll`).

It matters in three places: UserPatch runs from a **different executable**, so
every memory offset differs; it is what supplies custom and widescreen
resolutions, which is the container-resolution story in
`PROJECT_OVERVIEW.md` section 6; and it changes multiplayer behaviour.

**Until it is settled the plan assumes stock 1.0c.** Nothing before Stage 11
depends on a specific offset, so this does not block the next three actions —
but it should be answered before then.

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

## Next three actions

1. **Owner:** decide the UserPatch question — target stock 1.0c, or install
   UserPatch 1.5 and let me re-run Stage A against its executable.
2. **Me:** Stage 2 — the source tree, `version.dll` injection forwarding its
   three real functions, the logger with a build stamp on line 1, and the
   build-and-deploy script **with its freshness guard**, before there is
   anything complicated to debug. Plus the DirectDraw wrapper logging which
   interface version the game actually asks for, which is one line of output
   and closes the last of the Stage A questions.
3. **Me:** Spike S1 alongside it — hook `GetCursorPos`, return a fixed point,
   see whether the cursor obeys and whether the game fights back.

Then Checkpoint 2 with the owner.

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
