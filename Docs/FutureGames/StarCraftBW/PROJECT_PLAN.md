# StarCraft: Brood War — PROJECT_PLAN (high-level)

**Gated** on AoE2 Milestone 6, and best done last of the four. Read
[PROJECT_OVERVIEW.md](PROJECT_OVERVIEW.md) and [../README.md](../README.md)
first.

A sketch, deliberately. Stages 3 to 10 of
`Docs/ClaudeCode/PROJECT_PLAN.md` apply above the wrapper; only the bottom of
the stack is new, and that is where this plan spends its attention.

---

## Milestones

- [x] **S0a** `StarCraft.exe` 1.16.1 runs in a container as supplied **[owner]**
- [ ] **S0b** Runs **with `WMode.dll` removed**
- [ ] **S1** **Spike: does a `ddraw.dll` in the game directory get loaded by Storm?**
- [ ] **S2** Shared VR layer linked in, logging and build-stamped
- [ ] **S3** Window owned; sync pixel drawn; XrApi client alive
- [ ] **S4** **Flat tabletop with head tracking that feels locked**
- [ ] **S5** Laser pointer selects and commands
- [ ] **S6** HUD lifted onto floating quads
- [ ] **S7** Input mapping, owner-confirmed
- [ ] **S8** Presentation mode measured; comfort settings
- [ ] **S9** *Opportunity:* sprites from `.grp` / `.mpq` rather than screen scraping

S4 is the milestone that matters. S1 is the one that decides whether this port
is easy or a reverse-engineering project.

---

## Stage S0 — Does it run *without* `WMode.dll`?

**S0a is already done:** 1.16.1 runs in WinlatorXR as supplied **[owner]**.
But it is supplied *with* `WMode.dll` present, so what is proven is
**StarCraft + WMode**. S0b asks the narrower question.

**No code.** Move `WMode.dll` and `WMode_Fix.dll` aside and launch
`StarCraft.exe` again.

Our own layer has to own the window anyway — borderless and filling the
display is a non-negotiable, and a windowed-mode wrapper is a second opinion on
the same question. But `WMode` is there because someone needed it, so find out
what happens without it.

| Outcome | What it means |
|---|---|
| Runs fullscreen on Wine's DirectDraw | ideal |
| Runs but the mode is wrong | our window handling has to solve it; read what `WMode` does |
| Will not run | keep `WMode`, and inject somewhere else. Harder |

> ### CHECKPOINT S0b — with the owner, on the device
> Does 1.16.1 start and play with the windowed-mode wrapper removed? Keep the
> files; S0a proves they work.

---

## Spike S1 — Will Storm load our `ddraw.dll`?

**The decisive experiment of this port**, and it is cheap.

`storm.dll` resolves DirectDraw at runtime with
`LoadLibraryA("ddraw.dll")` + `GetProcAddress("DirectDrawCreate")`
**[measured]**, and `LoadLibrary` searches the application directory first. So
a `ddraw.dll` of ours next to `StarCraft.exe` should be what Storm gets.

Build the smallest possible proof: a `ddraw.dll` exporting `DirectDrawCreate`,
forwarding to the real one, writing one log line with a build stamp.

| Outcome | What it means |
|---|---|
| **Our DLL is loaded and the game runs** | the whole port is the shared layer plus an adapter. Proceed to S2 |
| Loaded but the game fails | an export or forwarding problem — solvable, and `WMode.dll`'s existence proves the pattern works for this game |
| **Never loaded** | Wine resolves `ddraw` to its builtin before the app directory. Fall back to hooking Storm's exports **by ordinal**, which needs the community's Storm ordinal map and is a materially bigger job |

> ### CHECKPOINT S1 — on the device
> Does the log file appear, with the build stamp on line 1, from a normal
> launch of `StarCraft.exe`?

---

## Stages S2–S8 — the shared layer

Fourth time through. These should be adapter work only.

| Stage | The per-game part |
|---|---|
| S2–S3 | nothing. XrApi, sync pixel and window handling are game-independent |
| S4 | wrap **`IDirectDraw` / `IDirectDrawSurface` version 1 only** **[measured]**. Then the frame upload and the palette lookup table — noting the palette is **Storm's**, so it is built from Storm's palette calls rather than the game's |
| S5 | the `GetCursorPos` hook, fourth time unchanged **[measured]**. Note the exe also calls `ClipCursor`, so expect the same contention as AoE2 |
| S6 | StarCraft's HUD: a bottom panel with the minimap at bottom-left, command card at bottom-right, resources along the top. Three regions, cleanly separated — probably the easiest slicing of the four |
| S7 | StarCraft bindings. Control groups and hotkeys matter more in this game than in any of the others, so the radial-menu question from AoE2 Stage 9 is sharpest here |
| S8 | mono versus AER, comfort settings, container resolution |

> ### CHECKPOINT S4 — on the device, the big one
> Same questions as AoE2 Checkpoint 6. The flat world means fewer ways for the
> table to look wrong, so a defect here is more likely to be the camera than
> the geometry.

---

## Stage S9 — the opportunity

**Sprites from the game's own assets.** StarCraft's unit art is `.grp` inside
`.mpq`, both formats exhaustively documented, with open-source readers
available. If the billboard phase needs to render sprites rather than scrape
them — AoE2's Route A — this is the title where that path is best supported.

Not planned. Full template treatment first, and the AoE2 project's findings on
reading unit state from memory should arrive before this starts.

**Assets are read from the player's own install at runtime. Nothing extracted
from an `.mpq` is ever committed** — `.gitignore` covers the archive
extensions, and that rule is not negotiable.

---

## Risks

Beyond the shared rules and risks in `../README.md` section 6:

| Risk | What we do |
|---|---|
| **Storm does not load our `ddraw.dll`** | Spike S1, before anything is built. Fallback is hooking Storm by ordinal, which is a real project — know that before committing |
| **Hooking Storm needs its ordinal map** | find the community's; do not rebuild it |
| **Removing `WMode.dll` breaks the game** | Stage S0b finds out, and keeps the file. S0a already proves the *with*-wrapper configuration works |
| **Players arrive with the wrong build** | **1.16.1 only.** Remastered and the 1.18+ free-to-play release both require a Battle.net account and are out of scope. State the required build in the install guide |
| **Scope creep toward Battle.net DRM** | **a line not to cross, not a risk to manage** **[owner]**. No DRM work, no authentication bypass, no online path. Offline, skirmish and LAN only |
| **Largest audience of the four** | a reason to do this port carefully and last, not first. The release bar is higher here |
