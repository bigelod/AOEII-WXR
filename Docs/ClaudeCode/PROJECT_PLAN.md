# PROJECT_PLAN — the staged plan

Read [PROJECT_OVERVIEW.md](PROJECT_OVERVIEW.md) first for the goal and the
decisions already taken. This file is **how we get there, in order**, with the
human checkpoints that cannot be reasoned past.

Stage numbers follow `general_wxr/FIRST_STEPS.md` so the two can be read side
by side. **Each stage leaves something testable**, so a failure is attributable
to the stage that caused it. The temptation in this project — build the whole
3D scene and then debug it — is the specific thing this ordering exists to
prevent.

**Three spikes sit outside the stage order** because they settle architecture
and are cheap. They are scheduled as early as their prerequisites allow, not
where the feature they serve lands.

---

## Milestones

The live tick-list. Status detail lives in [CURRENT_STATUS.md](CURRENT_STATUS.md).

- [x] **0** Feasibility assessed, intake complete
- [x] **1** Unmodified game runs in a container — *runs cleanly* **[owner]**
- [x] **A** Binary reconnaissance — **done**, see `PROJECT_OVERVIEW.md` 2a–2e
- [ ] **2** Our code loads and writes a log with a build stamp
- [ ] **S1** **Spike: can we move the mouse from inside the container?**
- [ ] **3** Window is borderless and fills the display
- [ ] **4** XrApi 0.5 client parses real device packets; button table built from a device log
- [ ] **5** Sync pixel drawn, last, 10x10
- [ ] **S3** **Spike: can we present our own frame at all?**
- [ ] **6** **The tabletop: 2D frame on a quad, mono, head tracking feels locked** (the big one)
- [ ] **7** Laser pointer selects and commands units
- [ ] **8** HUD lifted off the table onto floating quads
- [ ] **9** Full input mapping, owner-confirmed
- [ ] **S2** **Spike: the blit census** — does a per-sprite interception exist?
- [ ] **10** Presentation mode measured (mono vs AER); comfort settings; in-game menu
- [ ] **11** Billboards — only once S2 has named the route

Milestone 6 is the project. Everything before it is plumbing and everything
after it is layered on top, which is why nothing proceeds past its checkpoint
until the owner says head tracking feels locked.

---

## Stage A — Binary reconnaissance ✅ done

**Findings are in `PROJECT_OVERVIEW.md` sections 2a–2e** and are summarised in
`CURRENT_STATUS.md`. The headlines: 32-bit confirmed; DirectDraw only, no
Direct3D; **the DD7 interfaces are absent from the binary**, so the inherited
hooks could never have fired; the mouse is read through plain
`GetCursorPos` with no DirectInput, which largely de-risks the laser pointer;
the palette API is fully imported, so 8-bit is the main path; and **UserPatch
1.5 is not actually installed** on the inspected copy — an open decision for
the owner.

The original stage brief is kept below, because the same questions must be
re-asked if the target binary changes (which the UserPatch decision may do).

**Local, on the desktop, no device, no code.** Turns three assumptions into
measurements.

1. **Confirm 32-bit** and identify the real executable.
   `dumpbin /headers age2_x1.exe` — and establish whether UserPatch 1.5
   installs alongside or replaces it. Stop calling it `empires2.exe`; that is
   the base-game binary and this project runs the expansion.
2. **Which DirectDraw interface does it actually use?** `dumpbin /imports`
   for the `ddraw.dll` imports, then `strings` / a disassembler for the
   `IID_IDirectDraw*` and `IID_IDirectDrawSurface*` GUIDs the binary carries.
   A 1999 game very plausibly uses `IDirectDraw2` or `IDirectDraw4` and
   `IDirectDrawSurface3`, and the MetaAI skeleton hooks **only** the `7`
   interfaces **[code]**. If the game never asks for `7`, that code can never
   fire — and that single fact is worth more than the rest of this stage.
3. **Does it import `version.dll`?** Decides whether the preferred injection
   vector is available, and whether `dinput.dll` is the fallback.
4. **What does the window look like?** Does it go true-fullscreen or
   borderless-windowed, and at what resolutions will UserPatch 1.5 run?

**Deliverable:** the `[unverified]` rows of `PROJECT_OVERVIEW.md` section 2
replaced with `[measured]` ones.

> ### CHECKPOINT A — with the owner ✅ files supplied, measurements taken
> **One decision came back out of it:** UserPatch 1.5 is not installed on the
> inspected copy, though the project was specified with it. Target stock 1.0c,
> or install UserPatch and re-run this stage against its executable? It
> changes every offset, the available resolutions and multiplayer behaviour.
> Until it is answered, the plan assumes stock 1.0c.

### Stage A findings that change later stages

| Stage | What changed |
|---|---|
| 2 | the injection vector is confirmed available (`VERSION.dll` imported), but must forward three real functions |
| **S1** | **expected to succeed on route 1.** The game reads the mouse via Win32 `GetCursorPos`, no DirectInput. It also calls `SetCursorPos` and `ClipCursor` itself, so expect contention |
| 6 | the 8-bit palette path is the **main** path, not a fallback. Budget for the lookup table from the start |
| 6 / S3 | hooks must target **DD1/2/4 and Surface1/2/3/4** — never the 7 interfaces. Prefer wrapping `QueryInterface` over patching vtables, so the version in use becomes a log line |
| 9 | keyboard is read via `GetKeyState` / `GetAsyncKeyState`, so map scrolling can be fed by hooking those rather than synthesising events |

---

## Stage 2 — Get our code into the process

**The whole deliverable is a log file.** One file, one line, with
`__DATE__ __TIME__` compiled in and printed first.

1. `version.dll` (per Stage A) forwarding its handful of exports, with a
   thread of our own.
2. Logger: fixed path with a desktop fallback, mutex-correct, build stamp on
   line 1 — see `BUG_FIXES.md` items 6–8 for the three defects in the
   inherited logger, all of which are cheaper to not reintroduce than to fix.
3. **The build-and-deploy script, now, before there is anything complicated
   to debug.** Copy the five things Shogo's does (`TOOLS.md`), and copy step 3
   first: read the stamp out of the built binary and **refuse to deploy** if
   it is older than the newest source file, then SHA-256 the deployed copy and
   print `match=True`. "Am I running the binary I just built?" gets answered
   permanently, from the start.

> ### CHECKPOINT 2 — with the owner, on the device
> Does the log appear, and does line 1 show the stamp just built? This is also
> where we learn whether our injection vector works **under Box86 and Wine**,
> which is not a given and is far cheaper to find out now than after a feature
> depends on it. If `version.dll` does not load, fall back per
> `PROJECT_OVERVIEW.md` section 4 before going further.

---

## Spike S1 — Can we move the mouse from inside the container?

**Scheduled immediately after Stage 2, because it decides whether the laser
pointer — and therefore the whole design — is possible.** It needs nothing but
in-process code and a device.

InputAPI cannot move the mouse. **Stage A reordered this spike**: the game
reads the mouse through plain Win32 `GetCursorPos` and imports no DirectInput
at all **[measured]**, so what was route 3 is now route 1 and the spike is
expected to confirm rather than discover.

1. **Hook `GetCursorPos`** and return a fixed point. Does the game's cursor
   follow *our* number? An IAT hook on a documented function, with nothing in
   Wine or X11 getting a vote. Expected to work.
2. `SetCursorPos` to a known point. Does the AoE2 cursor move? Still worth
   knowing, because it is one line and because the answer generalises to
   other ports.
3. `mouse_event` / `SendInput` absolute. Same question.
4. **Also establish the contention**: the game calls `SetCursorPos` and
   `ClipCursor` itself **[measured]**. Find out whether it fights us, and
   whether `ClipCursor` needs neutralising so the pointer can reach the
   screen edges.

**Deliverable:** one of the four marked as working, written into
`PROJECT_OVERVIEW.md` section 4 as `[measured]`.

**If all of them fail** — now unlikely — the laser pointer is dead and the
project changes shape:
the tabletop still works as a *view*, with input left to the runtime's own
gyro-mouse (`MODE_3D = -1`). Worse, but not nothing — and much better found
out now than at Stage 7.

> ### CHECKPOINT S1 — on the device
> Which routes moved the cursor, and whether the game fought back. A photo of
> the cursor in the corner it was sent to is sufficient evidence.

---

## Stage 3 — Own the window

Still no VR. Strip `WS_CAPTION | WS_THICKFRAME | WS_BORDER`, size to screen
metrics, **re-apply every frame** because engines rebuild windows.

This is a prerequisite for the sync pixel, not a cosmetic step: with chrome in
the corner the runtime samples the chrome and head tracking silently degrades.

Watch for the engine fighting back. Genie is from the era of exactly the
consistency checks `TIPS.md` warns about, and AoE2 in particular is known for
opinions about its display mode. If it fights a per-frame fix and wins, prefer
letting the container size do the work.

> ### CHECKPOINT 3 — on the device
> Is the game filling the whole view with no title bar?

---

## Stage 4 — The XrApi client

Build it standalone and verify it on the **desktop with the UDP Tester**
before wiring it to anything. Do not go to the device for this stage.

1. Handshake files in `Z:\tmp\xr`, with a desktop fallback path; read `system`
   for the headset info and treat its existence as the "am I on a headset?"
   test. **Write `0.5` into `version`** — recorded in `PROJECT_OVERVIEW.md`,
   and it is read **once**, with an unrecognised value producing total silence
   rather than an error **[runtime]**.
2. Receive socket on 7872 only. 7873 is a duplicate, not a fallback **[runtime]**.
3. **Drain** every queued datagram per frame; keep the newest.
4. Parse 28 floats, frame id, bool string, HMD name. Tolerate trailing tokens
   and both bool-string lengths.
5. **Normalise every quaternion on ingest, unconditionally.**
6. Log the raw packet verbatim every few seconds behind the verbose flag.
7. Send `0 0 1 0 104.5 104.5` on 7278 every frame. Make the FOV pair
   **loadable from a file**, as HWXR does, so it can be tuned on the device
   without a rebuild. Remember they are degrees and are ignored at or below 1.
8. **Freeze on the last good pose** when packets drop. Never zero.

> ### CHECKPOINT 4 — with the owner
> **One raw packet captured on the actual Quest 3**, compared field by field
> against what the tester sends. Then: press each button in turn while the
> bool string is logged, and build the button table from **what moved**, not
> from the document. One minute of the owner's time, settled forever.

---

## Stage 5 — The sync pixel

Small, and the highest-value thing in the project. `RGB(HMD_SYNC, 0, 0)`,
10x10, top-left, **drawn last**, filtering and blending off, **echoed exactly
as received** — the runtime calibrates itself to whatever colour space our
renderer produces, and the remapping is gated on green being 0 **[runtime]**.
No gamma correction of our own.

Do this **before** the camera. If it is wrong, head tracking will feel broken
at Stage 6 and we will debug the camera for it.

While here, take the free debug channel: a green square below the sync square
indicating mode, as HWXR does. On a device with no debugger, four vertices
answer a question that otherwise needs a log pulled off the headset.

> ### CHECKPOINT 5 — nothing to see
> The square is near-black at low sync values. What is being checked is
> Checkpoint 6: whether tracking feels *locked* or *swimmy*.

---

## Spike S3 — Can we present our own frame at all?

**The experiment behind Stage 6**, and the one architectural unknown that no
amount of reading settles. Two presenters want the same window.

Three candidate answers, tried in order:

1. **Suppress the game's `Flip`, present our own D3D9 frame.** What the MetaAI
   skeleton does **[code]**. Cleanest if it works.
2. **Let the game present, composite into its surface.** We lock the primary,
   draw our 3D scene into it ourselves, let `Flip` proceed. No second
   presenter, but it means software rasterisation of the tabletop — viable for
   a quad, a dead end for billboards.
3. **Our own window.** Create a borderless window on top, present to that, let
   the game's window sit behind it. The runtime captures whatever is in front.

**Deliverable:** one of the three working on device, with a solid-colour
triangle visible in the headset. A triangle, not a tabletop — the question is
only whether our pixels arrive.

Budget for this failing in an interesting way. It is also the stage where
DXVK's D3D9 path inside the container gets its first real test.

> ### CHECKPOINT S3 — on the device
> Is there a coloured triangle over the game? A photo answers it.

---

## Stage 6 — The tabletop (the first real milestone)

**Mono only.** `MODE_VR = 1`, `MODE_3D = 0`. No stereo, no pointer, no HUD
work. Just the game's frame on a surface in a room, and a head that moves.

1. **The frame as a texture.** Capture the game's finished 2D output and
   upload it. **Handle the pixel format properly** — the inherited code
   assumes 32bpp and will produce garbage or read out of bounds at 8 or 16
   (`BUG_FIXES.md` item 3). If the surface is palettized, read the DirectDraw
   palette, build a 256-entry lookup table, rebuild it when the palette
   changes, and convert on upload.
2. **The table.** One quad, tilted, at a chosen height and size in metres.
   Put the scale in **one constant** — `XRAPI_V0_4_DESC.md` section 6 on why a
   magic multiplier scattered through the code cannot be changed later.
3. **The camera, orientation first.** Recentre inverse, rotate the canonical
   axes, flip for handedness, metres to our own units. Get looking around
   right before adding position — fewer variables.
4. **Then position.** The head moves within the room; the table does not move.
   This is the whole comfort story: the player's own motion is the only motion,
   so there is nothing to induce vection.
5. **Recentre on a button**, immediately. It will be used constantly, and for
   a seated tabletop it is the single most-used control.

Pitfalls in the order they usually bite: an axis flipped, so leaning the wrong
way; pitch and roll swapped; metres added to table units, putting the camera
in the next room; and the table's tilt and the head's pitch fighting because
both were tuned at once.

> ### CHECKPOINT 6 — on the device, and this is the big one
> - Does the table stay still when the head is still?
> - Does it swim or lag on a quick turn? (suspect Stage 5, not the camera)
> - Does leaning move the view the right way, and the right *amount*?
> - Is the horizon level, and does the table sit where a table should?
> - **Is the game readable?** A war table you have to squint at is a failure
>   even with perfect tracking. This is where container resolution gets its
>   first real judgement.
>
> **Do not proceed until this is right.** A camera defect will otherwise be
> misattributed to whatever is built next.

---

## Stage 7 — The laser pointer

Only now, and only along the route S1 proved.

1. **Ray from the dominant controller**, intersected with the table quad.
   Convert the hit point to table UV, then to game screen pixels. Measure this
   in the **table's** frame, not the hand's — `TIPS.md` on why a value
   expressed in a rotating frame can never converge.
2. **Feed the position** by the S1 route. Clamp to the game's screen rect.
3. **Trigger is click.** Decide held versus edge-triggered deliberately — a VR
   trigger is a *held state*, and a game waiting for a down-edge waits forever.
   Drag-select needs press, move, release to all survive the route.
4. **Draw the ray and a dot on the table.** Visible feedback is most of what
   makes pointing feel accurate.
5. **Thumbstick scrolls the map** via arrow keys.

**Handedness: swap once, at ingest**, then never test the flag again for
anything spatial.

> ### CHECKPOINT 7 — on the device
> Can the owner select a villager, order it to build, and queue units from the
> command panel? Ask specifically whether pointing feels *accurate* or
> *approximate* — the latter usually means the table's frame, not the ray.

---

## Stage 8 — The HUD, lifted

Slice the captured frame: world region to the table, HUD regions to their own
quads hung in space. No sprite work needed — see `PROJECT_OVERVIEW.md`
section 5.

1. Measure the HUD rects at the chosen container resolution. **Derive them
   from the measured width against a reference width**, with a strength
   control, rather than hardcoding pixels — the regions move with resolution.
2. Hang the resource bar high and the command panel low, within reach and
   outside the table's silhouette.
3. The minimap deserves its own quad and its own placement. It is the one HUD
   element a player looks at deliberately rather than glances at.
4. **The pointer must now hit the HUD quads too**, mapping back to the HUD's
   original screen pixels. The ray tests the table *and* each HUD quad; the
   nearest hit wins.
5. Deal with the bite the HUD leaves in the table — mitigations ranked in
   `PROJECT_OVERVIEW.md` section 5.

> ### CHECKPOINT 8 — on the device
> Is the HUD readable without leaning, and can the owner click it reliably?

---

## Stage 9 — Input mapping

Enumerate every control and map it explicitly, in a table, **confirmed by the
owner before implementing**. *VR action → game action (PC equivalent)*, one
line each, with separate sections for distinct states (in game, menu, paused,
scenario editor).

AoE2 specifics that need a deliberate decision:

- The control groups and hotkeys — an RTS lives on them, and a controller has
  far fewer buttons than a keyboard. A radial menu drawn on our own layer is
  probably the answer, and it is only possible because we have a renderer.
- What happens in menus: `MODE_VR = 0`, the runtime's own mouse back, flat
  presentation. Keep sending the outbound packet so the stream can be turned
  back on, and expect a gap when returning to `1` **[runtime]**.
- Shift-queue, Ctrl-group and Alt modifiers — all need a modifier scheme that
  does not consume a controller button each.

> ### CHECKPOINT 9 — with the owner
> Confirm the table before writing it, and again on the device after. Button
> layout is pure preference and the owner is the authority.

---

## Spike S2 — The blit census

**The gate on Stage 11, and it costs an hour.** Scheduled here rather than at
Stage 2 only because it wants real gameplay to observe; if the owner is
willing to play a few minutes with logging on earlier, run it earlier.

Behind the verbose flag, log every `Blt`, `BltFast`, `Lock` and `Flip` with
its rectangle and surface dimensions, across a few seconds of an actual game
with units moving. Then count:

| What the log shows | What it means |
|---|---|
| Hundreds of small blits per frame, each a sprite-sized rect | **Route C lives.** Classify by size and frequency, build Stage 11 on it. |
| One or a few large blits per frame, plus a long `Lock` | **Route C is dead.** The engine composites in software. Go to Route A or B. |
| No blits at all, only `Lock` / `Unlock` on the primary | Same conclusion, more strongly. |

**I expect the second or third outcome** (`PROJECT_OVERVIEW.md` section 5).
Being wrong here is good news, not a problem.

**Deliverable:** the route for Stage 11, named and written down with the log
excerpt that chose it.

> ### CHECKPOINT S2 — with the owner
> A few minutes of real gameplay with the verbose flag on, and the log file.

---

## Stage 10 — Depth, comfort, settings

1. **Measure AER against mono.** `MODE_3D = 2`, camera offset `±IPD/2` along
   its local right vector flipping each frame, eye flag in the sync square's
   **blue** channel (0 left, 255 right), red still the frame id. **Use the IPD
   from the packet** (float 25), not 0.064. Make the modes mutually exclusive
   in **one** function — HWXR had to order SBS before AER at three separate
   sites to stop a runtime toggle setting both.
2. **Table placement as a setting**: height, size, tilt. These are the
   controls a tabletop port lives on and the comfortable values are personal.
   Prefer a good default and few knobs.
3. **Ship the non-VR fallback** (`MODE_VR = 2`, `MODE_3D = -1`) as a
   player-selectable comfort and performance option.
4. **Turn off post-processing and verify it stays off** across a relaunch.
5. **Container resolution**: document the recommended size and its cost. No
   code.
6. **An in-game menu** on our own layer, not an INI. And when handing over a
   build, say plainly that a saved config on the device overrides a changed
   default.

> ### CHECKPOINT 10 — on the device
> Is there depth, and is it comfortable? Watch for swapped eyes (depth
> inverted, instantly unpleasant) and a frame rate that has fallen off a cliff.

---

## Stage 11 — Billboards

**Not planned in detail here, deliberately.** The route is unknown until S2
reports, and the four candidate routes need genuinely different plans
(`PROJECT_OVERVIEW.md` section 5). Writing a detailed plan for Route C now
would be writing a plan that is probably for the wrong route — which is what
the inherited MetaAI documents did.

What is settled regardless of route:

- **Terrain stays flat.** The table is the ground; only things standing on it
  stand up.
- **Cylindrical billboards**: vertical quads, up fixed at `(0,1,0)`, yaw
  toward the viewer. Not full camera-facing, which would make units lie back
  as the player leans over the table.
- **Depth sort by world position**, which is the reason Route A's true world
  coordinates matter.
- **A unit stood up must be erased from the flat layer**, or it appears twice.
- **Degrade to the flat tabletop** whenever a sprite cannot be classified.
  A table with some units standing and some flat is playable; a table with
  broken geometry is not.

Before any of it: write the feature up in the template's format — goal, *can
it be done at all*, **what must be proven first**, approach, ranked fallbacks,
acceptance criteria — and have the owner read it. That section exists to stop
a later session rediscovering a dead end.

---

## Verification

**Build and deploy.** `build/build-and-deploy.ps1`. Confirm the printed build
stamp, the freshness line, and SHA-256 `match=True` at every target. A stamp
older than the newest source file means the deploy is refused — **do not work
around this.**

**Confirm the right binary is running.** Line 1 of the log carries the stamp.
Check it before interpreting any device report.

**Desktop pass.** With the XrApi UDP Tester: proves the packet parsing, the
conversion maths and the render path *run*. It does **not** prove the values
are right — its quaternions are often not unit length and its hand separation
does not match a real headset.

**On-device pass** — the only real gate:

- [ ] the stage's own acceptance criteria
- [ ] head tracking still feels locked
- [ ] frame rate acceptable at the recommended container size
- [ ] the pointer still selects reliably
- [ ] correct in both right- and left-handed mode
- [ ] the non-VR fallback still works
- [ ] previously working features unaffected — name the ones the change could
      plausibly have touched, not all of them

**Nothing is committed as done until the owner has verified it on hardware.**
One change per deploy.

---

## Risks

| Risk | Likelihood | What we do about it |
|---|---|---|
| ~~The game uses a DirectDraw interface we do not hook~~ | **confirmed and closed** | Stage A: the DD7 GUIDs are absent, so the inherited hooks could never fire. Target DD1/2/4 + Surface1/2/3/4, and wrap `QueryInterface` rather than guessing |
| ~~Mouse motion is impossible from inside the container~~ | **largely closed** | Stage A: plain Win32 `GetCursorPos`, no DirectInput. S1 still runs, but to confirm rather than to discover |
| **The target binary changes** (UserPatch decision) | open | nothing is built against a specific offset yet. Settle it before Stage 11, which is the first stage that needs addresses |
| **Our pixels never reach the window** (two presenters) | medium–high | Spike S3, with three ranked answers, before Stage 6 depends on it |
| **Per-sprite interception does not exist** | **high — expected** | Spike S2 gates Stage 11; Routes A and B exist precisely for this outcome; Stages 6–10 do not depend on it at all |
| **Palettized surface mishandled** | **high — 8-bit is the measured main path** | the full GDI palette API is imported, so build the 256-entry lookup table from the start rather than discovering it on a black screen; `BUG_FIXES.md` item 3 |
| **Injection vector does not load under Box86/Wine** | medium | Checkpoint 2 proves the vector before anything is built on it; two ranked fallbacks |
| **The engine fights the borderless window** | medium | expect it (`TIPS.md`); prefer the container size doing the work over a per-frame fight |
| **Published offsets do not match this binary** | **certain** | the inspected exe is a no-CD repack with four appended sections, and the UserPatch question is still open. Treat every inherited address as wrong; re-find with Cheat Engine. Never ship a hardcoded offset without a sanity check and a graceful failure |
| **The table is unreadable at playable resolutions** | medium | judged at Checkpoint 6, early, where the answer can still change the design |
| **Tuned values do not transfer from tester to device** | medium | usually a frame-of-reference problem, not precision. If a value only works where it was found, fix the frame |
| **A saved config overrides a shipped default** | certain, eventually | say so explicitly when handing over every build |

---

## The shape of a good iteration

1. one change
2. build and deploy with the freshness guard
3. desktop test with the UDP Tester, if the change is testable there
4. device test by the owner, with logs and a screenshot
5. **read the logs before theorising** — add instrumentation if the log cannot
   answer the question
6. commit nothing until the owner has verified on hardware
