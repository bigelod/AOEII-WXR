# Red Alert 2 / Yuri's Revenge — PROJECT_PLAN (high-level)

**Gated** on AoE2 Milestone 6, and best done after Tiberian Sun. Read
[PROJECT_OVERVIEW.md](PROJECT_OVERVIEW.md) and [../README.md](../README.md)
first.

A sketch, deliberately. Stages 3 to 10 of
`Docs/ClaudeCode/PROJECT_PLAN.md` apply here almost unchanged, and duplicating
them before the AoE2 port has validated them would be copying a hypothesis.

---

## Milestones

- [x] **R0** `gamemd.exe` runs in a WinlatorXR container, unmodified **[owner]**
- [ ] **R1** **SafeDisc question settled** — inert under Wine, or in the way
- [ ] **R2** Injection vector chosen and proven on device
- [ ] **R3** Shared VR layer linked in, logging and build-stamped
- [ ] **R4** Window owned; sync pixel drawn; XrApi client alive
- [ ] **R5** **Flat tabletop with head tracking that feels locked**
- [ ] **R6** Laser pointer selects and commands
- [ ] **R7** **The sidebar lifted off the table** — the headline feature here
- [ ] **R8** Input mapping, owner-confirmed
- [ ] **R9** Presentation mode measured; comfort settings; cutscene handling
- [ ] **R10** *Opportunity:* heightmap table (inherited from Tiberian Sun)
- [ ] **R11** *Opportunity:* voxel vehicles (inherited from Tiberian Sun)

R5 is the milestone that matters. R7 is the one that makes this port
distinctive, because RA2's sidebar is the busiest HUD of the five games.

---

## Stage R0 / R1 — SafeDisc is the only question left here

**R0 is done:** the game runs in a WinlatorXR container as supplied
**[owner]**, with no DirectDraw wrapper installed. That is the cleanest
position of the Westwood/Blizzard group — nothing to remove, nothing to
replace, and Wine's own DirectDraw already demonstrated sufficient. (SWGB is
cleaner still, having no copy protection either.)

It also *partly* answers R1 for free: the game starts, so SafeDisc is not
blocking startup. What is still unknown is whether it objects to a **foreign
DLL** in the process, which is what we are about to add.

**No code needed yet.** The remaining question: does the SafeDisc machinery
(`SECDRV.SYS`, `DRVMGT.DLL`, `Blowfish.dll`, `PATCHW32.DLL`) do anything inside
Wine?

`SECDRV.SYS` is a **kernel driver**, and there is no Windows kernel in the
container, so the likely answer is that the protection is inert
**[unverified]**. The cheap test is to put any benign DLL into the process —
the Stage R2 skeleton will do — and see whether the game still starts.

| Outcome | What it means |
|---|---|
| Inert | ideal, and now the likely case given R0 passed. Proceed as for Tiberian Sun |
| Objects to a foreign DLL | the player owns the game; the community no-CD route is the answer. Document it as an install step rather than engineering around it |
| ~~Game will not run in a container~~ | **ruled out** — R0 passed **[owner]** |

> ### CHECKPOINT R1 — with the owner, on the device
> R0 is already confirmed. The remaining question: does Yuri's Revenge still
> start with a foreign DLL loaded into it? The Stage R2 skeleton answers it.

---

## Stages R2–R9 — the shared layer

**These should be nearly free by this point** — the third time through, after
AoE2 and Tiberian Sun. If they are not, the shared layer is not shared, which
is a problem in the AoE2/TS code rather than here.

| Stage | The per-game part |
|---|---|
| R2 | injection vector, from `gamemd.exe`'s full import table |
| R3–R4 | nothing. XrApi, sync pixel and window handling are game-independent |
| R5 | wrap `IDirectDraw` / `2` / `4` and `IDirectDrawSurface` / `2` / `3` / `4` — **the same profile as AoE2 [measured]**, so the AoE2 wrapper applies directly. Then the frame upload and palette lookup table |
| R6 | the `GetCursorPos` hook, third time unchanged **[measured]** |
| R7 | **the sidebar.** RA2's HUD is a tall right-hand panel, not AoE2's top-and-bottom strips — different slicing geometry and different floating placement. Worth real design effort here |
| R8 | RA2 bindings, including the sidebar tabs and build queues |
| R9 | plus **cutscene handling**: `binkw32.dll` presents video itself. Send `MODE_VR = 0` during cutscenes, exactly as for menus, and let the runtime show them as a flat panel |

> ### CHECKPOINT R5 — on the device, the big one
> Same questions as AoE2 Checkpoint 6. Note that RA2 is the busiest of the four
> screens, so "is the frame rate still acceptable" carries more weight here.

---

## Stages R10 / R11 — the opportunities

Inherited wholesale from Tiberian Sun: the heightmap table and voxel vehicles
are the same engine features, one generation on. If TS solved them, this is
adapter work. If TS did not, the reasons will be recorded there and apply here
too.

Not planned. Full template treatment first — goal, *can it be done at all*,
**what must be proven first**, ranked fallbacks, acceptance criteria — reviewed
by the owner before any code.

---

## Risks

Beyond the shared rules and risks in `../README.md` section 6:

| Risk | What we do |
|---|---|
| **SafeDisc blocks injection** | Stage R1, before anything is built. Most likely inert under Wine |
| **Performance** | busiest screen of the four. Measure on device at R5, not at R9 — if the tabletop is not affordable here, that changes the design while it still can |
| **Two executables diverge** | target `gamemd.exe` only until there is a reason not to |
| **Bink cutscenes fight our layer** | `MODE_VR = 0`, the same answer as menus |
| **Low stock resolutions weaken supersampling** | establish what the game accepts early; it is the main quality knob and it is weaker here than in Tiberian Sun |
| **The shared layer is not actually shared** | by this port that is a conclusion, not a risk. If R3–R4 cost real work after two previous ports, stop and fix the layering |
