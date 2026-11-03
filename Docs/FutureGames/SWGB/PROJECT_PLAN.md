# Star Wars: Galactic Battlegrounds — PROJECT_PLAN (high-level)

**Gated** on AoE2 Milestone 6. Read [PROJECT_OVERVIEW.md](PROJECT_OVERVIEW.md)
and [../README.md](../README.md) first.

The shortest plan of the four, deliberately. Stages 3 to 10 of
`Docs/ClaudeCode/PROJECT_PLAN.md` apply here **unchanged** — not "almost
unchanged", which is what the other three say. SWGB runs the same engine as
AoE2, so if any of those stages needs real work here, the finding is about the
AoE2 code's layering rather than about SWGB.

---

## Milestones

- [x] **W0** Runs in WinlatorXR as supplied **[owner]**
- [ ] **W1** cnc-ddraw added, `direct3d9` renderer confirmed, our `d3d9.dll` logs
- [ ] **W2** **Flat tabletop with head tracking that feels locked**
- [ ] **W3** Laser pointer selects and commands
- [ ] **W4** HUD lifted onto floating quads — *the adapter's real work*
- [ ] **W5** Input mapping confirmed
- [ ] **W6** Comfort settings, container resolution documented
- [ ] **W7** *Later:* billboards, inheriting AoE2's route

There is no wrapper-removal stage and no DRM stage. **W1 to W3 should be close
to free**, and that is the point of running this port second
(`PROJECT_OVERVIEW.md` section 4).

---

## Stage W1 — Get in, the easy way

No wrapper to remove (nothing occupies `ddraw.dll`) and no copy protection to
settle (GOG build) **[measured]**, so this is addition rather than surgery:

1. Add cnc-ddraw, pinned to the version AoE2 was verified against, configured
   **`renderer=direct3d9`**.
2. Drop in our `d3d9.dll`, forwarding to DXVK's own under its own name.
3. Confirm the log: build stamp on line 1, cnc-ddraw renderer on line 2.

**Target `Game/battlegrounds_x1.exe`** (Clone Campaigns), not
`Battlegrounds.exe`.

> ### CHECKPOINT W1 — with the owner, on the device
> Log file present, stamp current, renderer named, game still plays.

---

## Stages W2–W3 — the shared layer, with nothing new

Link the layer and configure it. The only values that should need touching:

| | What changes |
|---|---|
| Frame source | nothing — `Present` hook, backbuffer to texture, as AoE2 |
| Palette | nothing — cnc-ddraw converts it |
| DirectDraw | nothing — we never touch it. (If the fallback path is ever needed, note `battlegrounds_x1.exe` carries **no** interface GUIDs, so DD1 only; `PROJECT_OVERVIEW.md` 2b, and re-check at runtime) |
| Camera, XrApi, sync pixel, window | nothing |
| Pointer | nothing — `GetCursorPos`, no DirectInput **[measured]** |
| Table geometry | reuse AoE2's verified height, size and tilt as the starting point; these are per-stack, not per-game |

**If any of this costs real work, stop and say so.** That is the signal the
shared layer is not shared, and it is cheaper to fix in the AoE2 repository
than to work around here " and three more ports depend on the answer.

> ### CHECKPOINT W2 — on the device
> The AoE2 Checkpoint 6 questions: does the table stay still, does it swim on a
> fast turn, does leaning feel right, is the game readable?

---

## Stage W4 — The HUD, and this is the adapter's real work

The one stage with genuine per-game content. SWGB's HUD has the same *shape*
as AoE2's — resource bar, command panel, minimap — but not the same pixels.

1. Re-measure the HUD rectangles at the chosen resolution, derived from the
   measured width against a reference width rather than hardcoded.
2. Re-place the floating quads; the proportions differ even where the layout
   does not.
3. Re-check the bite the HUD leaves in the table
   (`Docs/ClaudeCode/PROJECT_OVERVIEW.md` section 5).

**This is the whole adapter, more or less.** If that turns out to be true, the
shared-layer bet has paid off.

> ### CHECKPOINT W4 — on the device
> HUD readable without leaning, and reliably clickable.

---

## Stages W5–W6 — mapping and polish

- **Input mapping** carries over wholesale: trigger primary, grip secondary,
  thumbsticks as held arrow keys, four radial menus on X/Y/A/B, left menu as
  `ESC`. Only the *contents* of the radials change, since SWGB's hotkeys and
  unit commands are its own. Owner-confirmed, as always.
- **Container resolution** and the two-resolution split
  (`Docs/ClaudeCode/PROJECT_OVERVIEW.md` section 6) apply identically. SWGB's
  own stock display modes need establishing **[unverified]**.
- **Post-processing off, and verify it stays off** across a relaunch.

---

## Stage W7 — Billboards, later

Inherits AoE2's route entirely, and **this is where SWGB earns its place
beyond being a cheap second port**: the engine is the same, so whatever AoE2
establishes about reading unit and building state from memory describes
structures of the **same shape** here. Different build, different addresses,
same fields in the same order. No other title in the family offers that.

Two things to settle when it starts:

- **The sprite and archive formats.** Very likely the same SLP/DRS as AoE2
  given the engine, but **[unverified]** — confirm rather than assume.
- **Air units have no ground cell.** Star Wars flies in a way AoE2 does not,
  and a billboard for an airborne unit has nothing to stand on. It bears on
  both the billboard anchor and the pointer's mapping
  (`Docs/ClaudeCode/PROJECT_PLAN.md` Stage 11). Flagged, not solved.

As everywhere: assets are read from the player's own install at runtime and
**never committed**.

---

## Risks

Beyond the shared rules and risks in `../README.md` section 6:

| Risk | What we do |
|---|---|
| **It is too similar to be a real portability test** | the honest risk of going second. SWGB proves the layer travels across one *engine*; Tiberian Sun is the test of whether it travels across the *family*. Treat this as the cheap regression test and do not over-read a success |
| **GOG's 2014 relink differs from the 2001 build** | likeliest explanation for the interface-GUID difference in `PROJECT_OVERVIEW.md` 2b. Re-find any offset; re-check interfaces at runtime |
| **Two executables** | Clone Campaigns only, until there is a reason otherwise |
| **Air units** | a billboard-phase question. Recorded now so it is not a surprise then |
| **The shared layer is not actually shared** | if W1–W3 cost real work *here*, of all places, that is a conclusion rather than a risk. Fix the layering in the AoE2 repository before continuing |
