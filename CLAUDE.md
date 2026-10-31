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

## Hard constraints

- **Age of Empires II: The Age of Kings v2.0a + The Conquerors v1.0c +
  UserPatch 1.5.** Not 1.0c stock, not HD, not DE. The executable is the
  expansion's, not `empires2.exe`.
- **32-bit, Win32, Release, in-process.** No 64-bit, no second executable, no
  wrapper layers.
- **XrApi 0.5.** The version file is read once at startup and an unrecognised
  value produces total silence, not an error.
- **The game's own 2D rendering is untouched.** We add a 3D presentation layer
  in front of it. The deviation from "keep the game's renderer" is argued in
  `PROJECT_OVERVIEW.md` section 4 — do not extend it further.
- **No game assets in this repository.** There is no source to leak, but a
  committed SLP or palette dump is still redistribution.
- **Nothing is done until the owner has verified it on hardware.** One change
  per deploy.
