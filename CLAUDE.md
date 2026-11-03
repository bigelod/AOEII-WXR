# AOEII-WXR

This is a WinlatorXR VR port. Read `Docs/general_wxr/CLAUDE.md` before
planning or writing any code.

Then read, in this order:

| File | What it is |
|---|---|
| [Docs/ClaudeCode/PROJECT_OVERVIEW.md](Docs/ClaudeCode/PROJECT_OVERVIEW.md) | the goal, the intake answers, and the decisions not to relitigate |
| [Docs/ClaudeCode/CURRENT_STATUS.md](Docs/ClaudeCode/CURRENT_STATUS.md) | what is true today, and the blocker |
| [Docs/ClaudeCode/PROJECT_PLAN.md](Docs/ClaudeCode/PROJECT_PLAN.md) | the staged plan, with human checkpoints |
| [Docs/ClaudeCode/BUG_FIXES.md](Docs/ClaudeCode/BUG_FIXES.md) | the defect register |
| `Docs/MetaAI/` | **history, not current truth** — see `PROJECT_OVERVIEW.md` section 7 for what in it is wrong |
| [Docs/FutureGames/README.md](Docs/FutureGames/README.md) | **later ports** (SWGB, Tiberian Sun, RA2, StarCraft BW) — gated on AoE2 Milestone 6, but read section 1 before designing anything: all five games share one integration surface, and **SWGB is the same Genie engine as AoE2**, so the AoE2 code should be a shared layer plus an adapter |

## Hard constraints

- **Age of Empires II: The Conquerors 1.0c.** Not HD, not DE, and
  **not UserPatch** — dropped deliberately, see `PROJECT_OVERVIEW.md` section
  2a. The executable is `age2_x1/age2_x1.exe`, not `empires2.exe`.
- **32-bit, Win32, Release, in-process.** No 64-bit, no second executable.
- **cnc-ddraw is a declared prerequisite**, pinned to a known-good version
  **that has a Direct3D 9 renderer at all** — the `1.1.4.x` lineage does not,
  and configuring `renderer=direct3d9` against it fails silently —
  with its **`direct3d9`** renderer (not `gdi`). It owns DirectDraw; we ship
  as `d3d9.dll` and hook `Present`. See `PROJECT_OVERVIEW.md` section 4.
  Its `nonexclusive=true` is part of the prerequisite too: it gives
  **borderless fullscreen** with a windowed device, so no display-mode switch
  happens. No other wrapper layers — no dgVoodoo, no ReShade.
- **DXVK is pinned to 1.10.3**, shipped as `d3d9_real.dll` **[owner]**. We do
  not use whatever D3D9 the container happens to provide: one pinned file
  serves desktop and device alike, so a difference between them is never the
  renderer. Verified cnc-ddraw build: **7.1.0.0**.
- **Monocular VR.** `MODE_VR = 1`, `MODE_3D = 0`. Not a performance
  compromise: stereo turns billboard sprites into visible cardboard cutouts
  (`PROJECT_OVERVIEW.md` section 3). Never build SBS.
- **XrApi 0.5.** The version file is read once at startup and an unrecognised
  value produces total silence, not an error.
- **The game's own 2D rendering is untouched.** We add a 3D presentation layer
  in front of it. The deviation from "keep the game's renderer" is argued in
  `PROJECT_OVERVIEW.md` section 4 — do not extend it further.
- **No game assets in this repository.** There is no source to leak, but a
  committed SLP or palette dump is still redistribution.
- **Nothing is done until the owner has verified it on hardware.** One change
  per deploy.
- **No DRM work, on any title.** No authentication bypass, no online path.
  See `Docs/FutureGames/README.md` section 5.
