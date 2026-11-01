# Tiberian Sun — PROJECT_PLAN (high-level)

**Gated** on AoE2 Milestone 6. Read [PROJECT_OVERVIEW.md](PROJECT_OVERVIEW.md)
and [../README.md](../README.md) first.

This is deliberately a sketch. The detailed plan is
`Docs/ClaudeCode/PROJECT_PLAN.md` — Stages 3 to 10 of it apply here almost
unchanged, and duplicating them before the AoE2 port has validated them would
be copying a plan that is still a hypothesis.

---

## Milestones

- [x] **T0a** Game runs in a WinlatorXR container as supplied **[owner]**
- [ ] **T0b** Game runs **with cnc-ddraw removed**
- [ ] **T1** Injection vector chosen from the import table, and proven on device
- [ ] **T2** Shared VR layer linked in, logging and build-stamped
- [ ] **T3** Window owned; sync pixel drawn
- [ ] **T4** XrApi client alive (shared, unchanged)
- [ ] **T5** **Flat tabletop with head tracking that feels locked**
- [ ] **T6** Laser pointer selects and commands
- [ ] **T7** HUD lifted onto floating quads
- [ ] **T8** Input mapping, owner-confirmed
- [ ] **T9** Presentation mode measured; comfort settings
- [ ] **T10** *Opportunity:* heightmap table
- [ ] **T11** *Opportunity:* voxel vehicles as real geometry

T5 is the milestone that matters. T10 and T11 are what make this port worth
doing beyond proving reuse, and neither is planned until T5 is signed off.

---

## Stage T0 — Does it run *without the wrapper*?

**T0a is already done:** the game runs in WinlatorXR as supplied **[owner]**.
But it is supplied *with cnc-ddraw installed as `ddraw.dll`*, so what is
proven is that **Game.exe + cnc-ddraw** works — not that `Game.exe` works
on Wine's own DirectDraw. That distinction is the whole of T0b.

**T0b needs no code.** Move `ddraw.dll` aside, along with `aqrit.cfg`,
`cnc-ddraw.ini`, `ddraw.ini` and `ddraw.log`, and launch `Game.exe` again.

Three outcomes, and they are genuinely different projects:

| Outcome | What it means |
|---|---|
| Runs fine on Wine's own DirectDraw | ideal. The `ddraw.dll` slot is free for us |
| Runs but badly — wrong colours, no mouse, bad mode | our wrapper has to solve whatever cnc-ddraw was solving. Read its source for *why* |
| Does not run at all | the wrapper is load-bearing. Reconsider: either fix the specific problem in our own layer, or keep cnc-ddraw and inject elsewhere |

> ### CHECKPOINT T0b — with the owner, on the device
> Does `Game.exe` start and play with the wrapper removed? Keep the files;
> they are the known-good fallback, and T0a proves they work.

---

## Stage T1 — Injection vector

Read `Game.exe`'s full import table and pick a DLL whose functionality we do
not need (the AoE2 reasoning, `Docs/ClaudeCode/PROJECT_OVERVIEW.md` section 4).
If T0 freed the `ddraw.dll` slot, taking it directly is also legitimate here —
unlike AoE2, we would then *be* the DirectDraw layer rather than impersonating
one, which removes the self-recursion trap entirely.

Account for the launcher chain: our DLL goes next to `Game.exe`, and
`ts-spawn.exe` / `tsconfig.exe` may need their own handling.

> ### CHECKPOINT T1 — on the device
> A log file with a build stamp on line 1, from a `Game.exe` launched the way
> a player launches it.

---

## Stages T2–T9 — the shared layer

**These should be almost no work**, and if they are not, the AoE2 code was
written wrong. What each needs is listed in
`Docs/ClaudeCode/PROJECT_PLAN.md`; what changes here is only the adapter:

| Stage | The per-game part |
|---|---|
| T2–T4 | nothing. XrApi, sync pixel, window handling are game-independent |
| T5 | wrap **`IDirectDraw` / `IDirectDrawSurface` version 1 only** **[measured]** — simpler than AoE2's multi-version wrapper. Then the frame-to-texture upload and the palette lookup table |
| T6 | the ray-to-table-to-screen-pixel maths is shared; the `GetCursorPos` hook is the same hook **[measured]** |
| T7 | Tiberian Sun's HUD rectangles — its sidebar is on the right, unlike AoE2's top-and-bottom layout, so the slicing geometry differs and the floating placement should too |
| T8 | TS keyboard and sidebar bindings |
| T9 | container resolution: the CnCNet patch supports high and custom modes, which is better than AoE2's stock options **[unverified — confirm what it accepts]** |

> ### CHECKPOINT T5 — on the device, the big one
> Same questions as AoE2 Checkpoint 6: does the table stay still, does it swim
> on a fast turn, does leaning feel right, is the game readable.

---

## Stages T10 / T11 — the opportunities

Not planned. Both get the template treatment — goal, *can it be done at all*,
**what must be proven first**, approach, ranked fallbacks, acceptance criteria
— written and owner-reviewed before any code, exactly as
`Docs/ClaudeCode/PROJECT_PLAN.md` Stage 11 insists for AoE2's billboards.

What is already known:

- **T10, heightmap table.** Needs the per-cell elevation field. Unknown
  whether it is reachable **[unverified]**. If it is not, the flat table from
  T5 is already a complete experience.
- **T11, voxel vehicles.** Needs unit position, facing and type from memory —
  the same problem as AoE2's Route A, and it should inherit whatever that port
  established. The `.vxl` / `.hva` formats are community-documented. Read from
  the player's own install at runtime; **never commit the assets**.

---

## Risks

Beyond the shared rules and risks in `../README.md` section 6:

| Risk | What we do |
|---|---|
| **cnc-ddraw turns out to be load-bearing** | T0a only proves the game runs *with* it. Stage **T0b** finds out, before anything is built. Its source explains what it fixes |
| **The CnCNet patch's own hooks collide with ours** | log both; never assume ours is the only code patched into this process |
| **Launcher sets configuration we do not control** | treat `tsconfig.exe` as part of the install procedure, documented for players |
| **Offsets differ from published 2.03 values** | re-find everything; this is a patched binary |
| **The shared layer is not actually shared** | this is the real risk of this port, and it is a risk about the *AoE2 code*, not about Tiberian Sun. If T2–T4 take real work, stop and fix the layering before continuing |
