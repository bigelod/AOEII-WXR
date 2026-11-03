# Star Wars: Galactic Battlegrounds — CURRENT_STATUS

**As of 2026-11-02.** Not started, and deliberately gated.

---

## State

| | |
|---|---|
| Phase | **Reconnaissance done.** No code, nothing of ours on hardware |
| Runs in WinlatorXR? | **Yes, as supplied** **[owner]**, and with **no DirectDraw wrapper and no copy protection** — the cleanest starting position of any port in the family **[measured]** |
| Gate | AoE2 **Milestone 6** — a tabletop with head tracking the owner confirms feels locked |
| Position in the family | **ranked 1st of the four later ports** — i.e. the next port after AoE2, ahead of Tiberian Sun (`../README.md` section 3) |
| First action when unblocked | **Stage W1** — add cnc-ddraw on `direct3d9`, drop in our `d3d9.dll`, confirm the log. No wrapper to remove, no DRM to settle |

No `BUG_FIXES.md` in this folder: there is no code to have defects in. Create
one with the first commit of real source.

---

## When this migrates

This folder becomes its own repository when the port starts **[owner]**.
It must leave with three things beside it: `Docs/general_wxr/` copied
unchanged, this folder promoted to `Docs/ClaudeCode/`, and
**`Docs/AoE2Notes/`** — the finished AoE2 port's findings, which for this
family are the implementation manual rather than background reading. See
`../README.md` section 5. The shared *code* is not copied in: the AoE2
repository is public, so it is read from there — but record the commit any
adapted code came from.

**`AoE2Notes/` matters more here than for any other port**, because SWGB runs
the same engine: AoE2's findings are not an analogy, they are a description of
this game's internals with different addresses.

---

## What has been established

From the owner's local copy, 2026-11-02, by PE import table and GUID search.
The path is **deliberately not recorded in this repository**.

- **GOG release with Clone Campaigns.** `Game/battlegrounds_x1.exe` is Clone
  Campaigns **1.1**; `Game/Battlegrounds.exe` is the base game **1.10**. Both
  32-bit PE32, image base `0x00400000`.
- **No copy protection** — a GOG build, so nothing like Red Alert 2's SafeDisc.
- **No DirectDraw wrapper installed** — the `ddraw.dll` slot is empty, unlike
  Tiberian Sun (cnc-ddraw) and StarCraft (`WMode.dll`).
- `DDRAW.dll` → **`DirectDrawCreate` only**. **No Direct3D import at all.**
- Mouse via `GetCursorPos`; **no DirectInput, no raw input**. Full GDI palette
  API, so 8-bit palettized. The fifth game in a row with this profile.
- **It is the same engine as AoE2, and the import table proves it**: the same
  three `VERSION.dll` functions, the same two `MSVFW32.dll`, the same twelve
  from a renamed `winmm` (`WIN32.dll` here, `WINMX.dll` in AoE2), the same
  single `DDRAW` import, the same `DSOUND` / `DPLAYX` / `IMM32` / `ADVAPI32` /
  `ole32` / `WSOCK32` sets, the same 28 `GDI32` calls, the same `THIS_COD…` /
  `THIS_DAT…` / `Inf32Dat…` sections, the same `Data/`+`Scenario/`+`Taunt/`
  folder layout, and the same `_x1` expansion-executable convention.
- **The two executables differ on DirectDraw interface GUIDs**: the 2001 base
  exe carries DD1/2/4 + Surface1/2/3/4 (AoE2's profile exactly), while GOG's
  2014 expansion rebuild carries **none**. Surprising for one engine; most
  likely a relink dropping an unreferenced table. Irrelevant on the primary
  architecture, and worth re-checking at runtime if the fallback is ever used.

Full detail and the reasoning in [PROJECT_OVERVIEW.md](PROJECT_OVERVIEW.md).

---

## What this changed elsewhere

**It moved the recommended port order.** SWGB displaces Tiberian Sun as the
second port: same engine, nothing to remove, no DRM, and the adapter reduces
to the HUD rectangles. `../README.md` section 3.

**It corrected the AoE2 docs.** `Docs/ClaudeCode/` described `THIS_COD…`,
`THIS_DAT…` and `Inf32Dat…` as non-standard sections and treated them as
evidence that the supplied AoE2 exe was a no-CD repack. They are present in a
**clean GOG SWGB build of both executables**, so they are **standard Genie
sections**. Only `.pdata` and `.patch` are AoE2's actual repack additions. The
conclusion survives — the AoE2 binary *is* modified and its offsets still need
re-finding — but the evidence for it was overstated, and three documents said
so. Corrected.

---

## Open questions, for when this starts

1. **Clone Campaigns only, or the base game too?** **[owner]** The plan assumes
   `battlegrounds_x1.exe` alone.
2. **Which display modes does SWGB accept?** Sets the table's texture
   resolution, per the two-resolution split
   (`Docs/ClaudeCode/PROJECT_OVERVIEW.md` section 6).
3. **Are the sprite and archive formats the same SLP/DRS as AoE2?** Very likely
   given the engine, but unconfirmed.
4. **How are air units anchored?** Star Wars flies in a way AoE2 does not, and
   a billboard for an airborne unit has no ground cell to stand on. A
   billboard-phase question that also touches the pointer.

---

## Note to the next session

**Do not over-read a success here.** SWGB working proves the shared layer
travels across *one engine*, which is nearly the null hypothesis. Tiberian Sun
is the first real test of whether it travels across the *family*. Run SWGB
second because it is cheap and because its reverse-engineering transfers — not
because it settles the portability question.
