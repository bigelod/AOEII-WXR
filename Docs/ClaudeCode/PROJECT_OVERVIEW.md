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
| Game, version, patch level | Age of Empires II: The Age of Kings **v2.0a**, The Conquerors Expansion **v1.0c**, **UserPatch 1.5** **[owner]** |
| Store or distribution | TBD — matters only for whether the install can be scripted |
| Install path (target) | `D:/VR/AoE2-WXR` — does not exist yet, will be created **[owner]** |
| Install path (for inspection) | TBD — owner is copying the game files to this machine **[owner]** |
| Bitness | 32-bit **[unverified — confirm with `dumpbin /headers`]** |
| Renderer | DirectDraw **[unverified — which interface version is the open question, section 5]** |
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
| Presentation mode | **Mono first** (`MODE_3D = 0`), then measure **AER** (`2`). SBS is the least likely winner here — section 3. |
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

## 3. Presentation: the baseline we must beat

**Before any code, AoE2 already runs in VR.** WinlatorXR's own non-VR mode
puts the game on a floating window in the headset and keeps its
right-controller-gyro mouse emulation. That needs no mod at all.

So the honest framing of this project is not "make AoE2 work in VR" — it is
**"beat a floating window"**. Writing that down matters, because it is the bar
every milestone is judged against, and because it is also the **shipping
fallback**: a comfort option for players who want it, and a performance option
when the tabletop costs too much.

The modes, in the order this port uses them (`XRAPI_V0_4_DESC.md` 5a):

| Stage | `MODE_VR` | `MODE_3D` | Why |
|---|---|---|---|
| Baseline / fallback | 2 | -1 | flat window, runtime keeps the mouse. Free, no code. |
| Menus and pre-game | 0 | — | stops the pose stream we are not using, returns the runtime's own mouse, and drops out of immersive presentation so a whole menu is readable at once. The Interstellar Marines port does exactly this. |
| **First real milestone** | 1 | 0 | mono tabletop, 6DOF head tracking. One render. |
| Depth, if affordable | 1 | 2 | **AER** — depth at mono cost, each eye at half rate. |
| Probably never | 1 | 1 | SBS doubles scene cost. See below. |

**Why AER over SBS here, specifically.** AER's weakness is per-eye freshness,
which bites hardest on fast motion in depth — and a war table the player leans
over has almost none of that. Its strength is costing one render. A tabletop
is the near-ideal case for AER, so the general advice in `TIPS.md` ("AER is
frequently the better bargain") is unusually clear-cut here. Measure it anyway.

---

## 4. Architecture

### Injection vector — and a correction to the inherited plan

**Recommendation: inject with `version.dll`, then hook DirectDraw from
inside.** Not a `ddraw.dll` proxy.

The MetaAI skeleton ships a `ddraw.dll` proxy, and `TOOLS.md` does list
`ddraw.dll` as the AoE2-era choice. But impersonating `ddraw` buys us nothing
we actually need, and costs two real problems:

1. **Self-recursion under Wine. [unverified, and the likeliest immediate
   crash.]** The skeleton sets `WINEDLLOVERRIDES=ddraw=n,b` and then calls
   `LoadLibraryA("C:\\windows\\system32\\ddraw.dll")` to find the real one
   **[code]**. Wine applies DLL overrides **by module name, not by path**, so
   that call can resolve back to *our own already-loaded module*. Then
   `GetProcAddress(handle, "DirectDrawCreate")` returns our own export and the
   first call recurses until the stack dies. Wine's builtin ddraw is a
   `.dll.so`, so there is no PE file to copy aside and rename either — the
   usual escape hatch is closed.
2. **Export surface.** Real `ddraw.dll` exports far more than the ten names in
   `proxy.def` **[code]** — `AcquireDDThreadLock`, `ReleaseDDThreadLock`,
   `SetAppCompatData`, `DDInternalLock`, `GetOLEThunkData` and others. A
   missing export is a load failure for anything that imports it.

Neither problem exists if we inject through a DLL whose *functionality* we do
not need. `version.dll` is loaded very early by almost everything and almost
never actually used (`TOOLS.md`), so forwarding it is trivial and nothing
depends on our forwarding being complete. Once loaded, we hook the genuine
`ddraw.dll` with MinHook — which is what we wanted all along, and which works
identically whichever DirectDraw interface version the game uses.

| | `ddraw.dll` proxy | `version.dll` + MinHook |
|---|---|---|
| Must forward exports correctly | all of them | effectively none |
| Risk of loading ourselves | **real** | none |
| Reaches DirectDraw | directly | via one hook |
| Needs `WINEDLLOVERRIDES` | `ddraw=n,b` | `version=n,b` |

**Fallbacks, ranked:** (1) `version.dll`; (2) `dinput.dll` / `dinput8.dll` if
AoE2 imports them and `version.dll` turns out not to load under Box86/Wine;
(3) the `ddraw.dll` proxy with a complete, generated export table and the real
module resolved by something other than name. Prove the vector on device at
Checkpoint 2 before building anything on it — `TOOLS.md` warns that hooking
under Box86 and Wine is one more thing that can differ from the desktop.

### The pointer — the riskiest plumbing in the project

A laser pointer needs the mouse to **move**, and InputAPI explicitly cannot do
that: clicks and scroll only. Being in-process is what saves us, and there are
four routes. Ranked by expected reliability, not by simplicity:

1. **Hook the game's own mouse read.** We are already inside the process. Hook
   `GetCursorPos` (and the DirectInput mouse path, if the binary uses one) and
   return the position *we* computed from the controller ray. This bypasses
   the OS, Wine and the window manager entirely, which is exactly why it is
   ranked first — nothing else in the stack gets a vote.
2. **`SetCursorPos` in-container.** Simplest, one line.
   `WINLATORXR_DESC.md` flags Wine-side mouse motion as unverified and
   *"worth testing early — it is the cheapest thing that would unlock
   head-look for an unmoddable game"*. A half-hour experiment settles it, and
   it is the first experiment in the plan.
3. **`SendInput` / `mouse_event` absolute.** `XeRxKEYs` ships both as output
   modules, so the shape is proven to compile and run; whether a fullscreen
   DirectDraw game under Wine honours them is the open part.
4. **InputAPI for clicks only** (`M,TFFFF`), paired with one of the above for
   motion. The hybrid `WINLATORXR_DESC.md` describes.

**Map scrolling takes a different route and is much safer.** AoE2 scrolls on
arrow keys and on minimap clicks, so edge-scroll is not needed: a thumbstick
becomes arrow keys, either in-process (`keybd_event`) or through InputAPI's
`K,<keycode>` — *X11 keycodes, not Windows VKs*, and `XeRxKEYs`'
`XKeycode.cs` is to be copied wholesale rather than rebuilt (`TOOLS.md`).
Grabbing and dragging the table itself is a nicer gesture, layered on top of
the same arrow keys.

### Renderer — a deliberate, named deviation

`general_wxr/CLAUDE.md` lists **"keep the game's own renderer"** as a standing
choice not to relitigate. This port needs a new renderer anyway, and the
reason has to be stated precisely, because "we need a modern API" is the exact
rationalisation that rule exists to stop.

The rule's own justification is: *"the things you actually need — a camera
transform you can write, a 2D blit for the sync pixel, a window you can resize
— exist in every API back to DirectDraw."* That list is correct and complete
**for a game that already renders a 3D world**. AoE2 does not. DirectDraw is a
blitter: no projection matrix, no view transform, no depth buffer, no camera
to write. There is no existing code path that can draw a tilted quad, let
alone a billboard.

So the deviation is not "modernise the renderer". It is **"add the only 3D
renderer this project will have"**, and the rule's cost-benefit does not apply
because there is no known-good 3D path being traded away. What we *do* keep is
the game's own rendering of its 2D frame — untouched, still DirectDraw, still
the path the stack is tuned for. We add a presentation layer in front of it.

| Decision | Choice | Why |
|---|---|---|
| Bitness | **32-bit, Win32** | the game is 32-bit; no OpenXR runtime to load; stays in-process, which `TIPS.md` says is worth more than any benefit of 64-bit |
| Game's 2D rendering | **untouched** | known-good under Box86/Wine; it is our texture source |
| New 3D layer | **D3D9** | see below |
| Wrappers (dgVoodoo, ReShade) | **none** | another layer inside two translation layers. WinlatorXR can auto-inject ReShade itself if a player wants to experiment (`TOOLS.md`) — that costs us nothing and is not our problem |
| Process model | **in-process** | no IPC, no second binary to ship, start and reap. `TIPS.md` records the Shogo port becoming dramatically simpler when its client moved in-process |
| XrApi version | **0.5** | human-readable, the UDP Tester speaks it, poses arrive without declaring spaces, and it is 0.4 plus the InputAPI listener at no cost. Written down here because `version` is read **once** and an unrecognised value yields total silence **[runtime]** |
| Supersampling | **container resolution** | the runtime filters on downsample; no code |
| Settings surface | **in-game overlay**, drawn into our own 3D layer | editing an INI on a headset is painful. We have a renderer, so we can draw a menu |

**D3D9 rather than the skeleton's D3D11.** Both reach the screen; D3D9 fits
the stack better. The June 2010 DirectX SDK is marked **[proven]** in
`TOOLS.md` and gives us `d3dx9` for text and sprite batching; DXVK's D3D9 path
is mature and is what Winlator containers are routinely configured for; the
API is small, era-appropriate, and per-eye SBS is two viewports. D3D11 brings
nothing this port needs and one more thing to be wrong about on device.
Record the choice; revisit only if a measurement says to.

**The unresolved part of this, honestly:** two presenters on one window. The
game's DirectDraw `Flip` and our D3D9 `Present` both target the same HWND, and
the skeleton's answer — suppress the game's `Flip` and present our own
**[code]** — may or may not survive Wine. The alternative is to let the game
keep presenting and composite *into* its surface instead. That is an
experiment, not a decision, and it is scheduled as one in the plan.

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
  the heaviest route and the most likely to work. UserPatch 1.5 is a
  *different binary* from stock 1.0c, so published 1.0c offsets will not
  match — expect to re-find them, with Cheat Engine before a disassembler
  (`TOOLS.md` rates it *"underrated for this work"*).
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
  in order: a higher container resolution (the bite shrinks proportionally);
  whether UserPatch 1.5 or a widescreen patch can reposition or shrink the
  HUD; or accepting it, since the covered strip is the bottom edge of the view
  and a lean brings it back.

---

## 6. Container resolution

`WINLATORXR_DESC.md`: container resolution *is* the render resolution, and the
runtime filters on downsample, so rendering above panel resolution is free
anti-aliasing. For this port it is also **the sharpness of the table**, which
is most of the perceived quality.

- Start at **2800x1400**, the usual starting point.
- Then measure square-ish modes — panels are nearer square than 16:9, and
  `WINLATORXR_DESC.md` notes forcing something like 1400x1400 can be a win.
  The sibling Interstellar Marines port ships 1400x1400.
- UserPatch 1.5 supports custom resolutions, which is one reason this patch
  level is a good target **[unverified — confirm what it will accept]**.
- Tell players the number. It is a real graphics setting living outside the
  mod, and probably the best quality knob we have.

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
`empires2.exe` **[code]**. This project is AoK v2.0a + Conquerors v1.0c +
**UserPatch 1.5** **[owner]**, which runs from the expansion executable
(`age2_x1/age2_x1.exe` or the UserPatch-installed variant), not
`empires2.exe`. Any address, offset or exe name inherited from those docs is
suspect.

**What is worth keeping.** The DLL-proxy *shape* — forward exports, start our
own thread, hook a vtable — is sound, and the vtable indices it guessed are in
fact correct (`BUG_FIXES.md`, item 2). The logger, the build-stamp discipline
and the fallback-on-failure instinct are all the right reflexes.

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
