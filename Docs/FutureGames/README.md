# FutureGames — three more classic RTS titles

High-level starting plans for taking **Red Alert 2 / Yuri's Revenge**,
**StarCraft: Brood War** and **Tiberian Sun** to standalone VR the same way
AoE2 is going, on the same "3D tabletop with billboard sprites" idea.

| Folder | Game | Measured as |
|---|---|---|
| [RA2/](RA2/) | Command & Conquer: Yuri's Revenge | `gamemd.exe`, stock Westwood, no Ares/Phobos |
| [StarCraftBW/](StarCraftBW/) | StarCraft: Brood War | `StarCraft.exe` **1.16.1** |
| [TibSun/](TibSun/) | Tiberian Sun (+ Firestorm) | `Game.exe` **2.03-CnCNet-patch** |

> **All four games already run in WinlatorXR as supplied** **[owner]**, which
> closes `FIRST_STEPS.md` Stage 1 for every title in the family before any work
> starts. That is an unusually good position - most ports in this family spent
> real effort there. **One caveat, and it is not a small one:** two of these
> installs run *with a DirectDraw wrapper in place* (section 2), so "runs
> as-is" does not yet mean "runs on Wine's own DirectDraw". Each plan opens
> with that specific test.
>
> **These are gated.** Nothing here starts until AoE2 reaches **Milestone 6**
> — a tabletop with head tracking that the owner confirms feels locked
> (`Docs/ClaudeCode/PROJECT_PLAN.md`). Until then these documents exist to
> record what the binaries say and to stop the AoE2 work being designed in a
> way that cannot be reused. They are deliberately high-level: detailed plans
> written before the first port works would be detailed plans for the wrong
> architecture, which is the mistake `Docs/MetaAI/` already made once.

Read `Docs/general_wxr/` first, then `Docs/ClaudeCode/PROJECT_OVERVIEW.md` —
the goal, the presentation modes, the sync pixel and the renderer argument are
all identical here and are not repeated per game.

---

## 1. The finding that matters: all four games are the same port

Each install was inspected the same way as AoE2 — PE import table plus a
search for every DirectDraw interface GUID. All **[measured]**, 2026-11-01.

| | **AoE2** 1.0c | **RA2 / YR** | **StarCraft** 1.16.1 | **Tiberian Sun** 2.03 |
|---|---|---|---|---|
| Bitness | 32-bit PE32 | 32-bit PE32 | 32-bit PE32 | 32-bit PE32 |
| Image base | `0x00400000` | `0x00400000` | `0x00400000` | `0x00400000` |
| Graphics import | `DDRAW.dll`: `DirectDrawCreate` | `DDRAW.dll`: `DirectDrawCreate` | **none in the exe** — via `storm.dll`, which loads `ddraw.dll` *dynamically* | `DDRAW.dll`: `DirectDrawCreate` |
| Direct3D | **none** | **none** | **none** | **none** |
| DD7 interfaces | **absent** | **absent** | **absent** | **absent** |
| Highest DD interface reachable | DD4 / Surface4 | DD4 / Surface4 | **DD1 / Surface1** | **DD1 / Surface1** |
| Palette API | full GDI set | — | full set, in `storm.dll` | — |
| Mouse | `GetCursorPos` | `GetCursorPos` | `GetCursorPos` | `GetCursorPos` |
| DirectInput | **none** | **none** | **none** | **none** |
| Raw input | none | none | none | none |

Four games, four studios, three engines, 1995–2001 — and **one integration
surface**: enter at `DirectDrawCreate`, 8-bit palettized surfaces, DirectDraw
version 1-era interfaces, no Direct3D anywhere, and a mouse read through
plain Win32 `GetCursorPos`.

### What follows from that

- **The VR layer should be built once and shared.** Everything in the AoE2
  plan from Stage 3 to Stage 10 — the borderless window, the XrApi 0.5 client,
  the sync pixel, the D3D9 tabletop, the head-tracked camera, the
  ray-to-table-to-screen-pixel pointer, the HUD slicing, the AER measurement —
  is **game-independent**. Only the numbers change.
- **So AoE2 should be written as a library plus an adapter**, not as one
  program. Not a refactor to do later: the second port is the thing that pays
  for it, and the second port is now known to be nearly identical.
- **The laser pointer generalises.** The single riskiest piece of the AoE2
  design — hooking `GetCursorPos` and returning a point derived from a
  controller ray — is the same hook in all four, because not one of them uses
  DirectInput or raw input. Prove it once.
- **Target DirectDraw version 1 throughout** — *on the fallback path only.*
  None of the four can use the 7 interfaces and two use nothing above version
  1, so a wrapper against `IDirectDraw` / `IDirectDrawSurface` with a
  `QueryInterface` wrapper above it covers every case. On the primary path
  (section 2) cnc-ddraw does this and we never touch DirectDraw at all.
- **All four are monocular.** The billboard cue-conflict argument
  (`Docs/ClaudeCode/PROJECT_OVERVIEW.md` section 3) is about the art style,
  not the hardware, so it applies to every title here. Never build SBS.

The plausible per-game cost, once AoE2 works, is the adapter: which rectangles
are HUD, where the world is, what the camera scroll bindings are, how to find
unit state. **Weeks rather than months** — but only if the AoE2 code is
written with that in mind.

---

## 2. The DirectDraw wrapper question

Two of these installs already have a DirectDraw wrapper in place, and **the
owner has confirmed they may be removed** **[owner]** — so they are a starting
condition to clear, not a constraint to design around.

| Install | What is there |
|---|---|
| **Tiberian Sun** | `ddraw.dll` = **cnc-ddraw 1.1.4.15** (cncnet.org), configured `Renderer=gdi`; plus leftover `aqrit.cfg` and a `cnc-ddraw.ini` carrying per-game profiles |
| **StarCraft** | `WMode.dll` / `WMode_Fix.dll` — a windowed-mode wrapper |
| **RA2**, **AoE2** | none active - and both already run in WinlatorXR, so for these two the wrapper question is closed before it is asked **[owner]** |

`general_wxr/TIPS.md` warns against wrapper layers, and removing dgVoodoo was
progress in the Shogo port. But the warning is about **stacking** layers, and
that is not quite the choice here — our own mod *is* a DirectDraw wrapper.
Whichever route is taken there is exactly one wrapper in place. So the real
question is narrower and more interesting:

**Write our own DirectDraw layer, or fork cnc-ddraw and add VR to it?**

| | Write our own | Fork cnc-ddraw |
|---|---|---|
| Starting point | the AoE2 code, which we will have written anyway | a mature, open-source wrapper with D3D9, OpenGL and GDI backends, upscaling, windowed handling and a decade of per-game bug fixes |
| Already supports these games | we are proving it | **yes, explicitly** — its bundled profile list includes `[C&C95]`, `[ra95]`, `[EMPIRES2]` and **`[age2_x1]`**, so all four titles are already in its compatibility matrix |
| Code we control | all of it | a fork to keep rebasing |
| Fit to the stack | built for Box86/Wine from day one | built for desktop Windows; its backends are another thing to validate inside the container |
| Licence | ours | must be checked before any release |

**Recommendation, revised 2026-11-01: neither. Let cnc-ddraw do ddraw→D3D9 and
hook D3D9 instead** **[owner]**.

WinlatorXR can drop cnc-ddraw in at the top level, which makes a third option
available that is better than both columns above: we do not write a
DirectDraw layer *and* we do not fork one. We ship as **`d3d9.dll`** and hook
**`Present`** — at which point the backbuffer holds the game's finished 2D
frame, whoever produced it. Full reasoning in
`Docs/ClaudeCode/PROJECT_OVERVIEW.md` section 4.

For this family specifically it removes:

- **the 8-bit palette conversion**, in all four games;
- **all DirectDraw interception** — the DD1-vs-DD4 question in section 1
  becomes a fallback-path detail;
- **the two-presenters problem**, which was the largest unknown in the AoE2
  plan;
- **and the single largest risk in the StarCraft port** — Storm owning the
  renderer and loading `ddraw.dll` dynamically, with a fallback of hooking
  Storm's exports *by ordinal*. cnc-ddraw sits underneath Storm as happily as
  anywhere else, so that work never happens.

**The layering objection does not apply.** `ddraw → cnc-ddraw → D3D9 → DXVK`
is exactly as deep as `ddraw → us → D3D9 → DXVK`. `TIPS.md` warns against
*stacking* wrappers; there is one translation step either way, and the only
question is who wrote it.

**The cost is a declared dependency** **[owner]**: cnc-ddraw pinned to a
known-good version, with its **`direct3d9`** renderer rather than `gdi`,
documented as an install step — as the Interstellar Marines port requires
BepInEx. Depend only on `Present` semantics, never on its internals, and log
the renderer in use so a missing D3D9 device fails loudly.

Its source is still the best available reference for *what breaks* in these
four games' DirectDraw usage, and that knowledge remains worth more than the
code.

**Still remove the *other* wrappers before testing.** `WMode.dll` in the
StarCraft install, and the leftover `aqrit.cfg` / DDrawCompat configs in
Tiberian Sun, are second opinions on questions our layer and cnc-ddraw now
answer between them. One cnc-ddraw, configured to `direct3d9`, and nothing
else. Keep the files aside — they are the fallback if a game turns out not to
run without them.

---

## 3. Which one to do second

Ranked by how much of the AoE2 work transfers unchanged:

1. **Tiberian Sun.** The simplest integration surface of the four — DirectDraw
   version 1 only, `DirectDrawCreate`, `GetCursorPos`. Already CnCNet-patched,
   so high and custom resolutions are solved. The experience of removing
   cnc-ddraw here also answers the wrapper question for the whole family.
2. **Red Alert 2 / Yuri's Revenge.** Same interface profile as AoE2 (DD1/2/4),
   and it shares the Tiberian Sun engine lineage, so the TS adapter is most of
   it. Carries SafeDisc remnants, which is the one new risk.
3. **StarCraft: Brood War.** The odd one out architecturally — the executable
   imports nothing but `storm.dll` by ordinal, and Storm owns the rendering
   and loads `ddraw.dll` dynamically. Everything above the wrapper still
   transfers, but the injection and interception work is its own problem.

Tempting to reorder on audience size, where StarCraft wins easily. Resist it
for the *second* port and revisit for the third: the second port's job is to
prove the shared layer is actually shared, and it should be the one least
likely to be derailed by something game-specific.

---

## 4. What is genuinely different per game, and why it is exciting

The thing that makes these three *more* interesting than AoE2, not less:

- **Terrain has real elevation in Tiberian Sun and Red Alert 2.** Both engines
  carry per-cell height in the map. AoE2's tabletop is a flat quad because its
  world is flat; TS and RA2 could have a genuine **3D heightmap table** with
  sprites standing on sloped ground. That is a visibly better version of the
  same idea.
- **Vehicles in TS and RA2 are already 3D.** Westwood rendered them from
  **`.vxl` voxel models** with `.hva` animation data, rasterised to sprites at
  runtime. The 3D data ships with the game. So for these two titles the
  billboard compromise may be unnecessary for the most interesting objects —
  vehicles could be *actual geometry*, with infantry and buildings as
  billboards. Nothing in AoE2 offers this.
- **StarCraft's world is honestly flat**, so its tabletop is the simplest of
  the three, and its sprites (`.grp` in MPQ archives) are the most thoroughly
  documented by its modding community.

None of this is planned yet. It is recorded because it is the reason to do
these ports at all, and because it should not be rediscovered from scratch.

---

## 5. Migration — what travels with each port

**Each folder here becomes its own repository** **[owner]**: when a port starts,
`Docs/FutureGames/<Game>/` is lifted out into a new GitHub repo and directory
of its own, and this folder shrinks by one.

So each doc set has to be written to survive the move, and a new repo needs
three things alongside it.

### The three folders a migrated port gets

| Folder | What it is | Where it comes from |
|---|---|---|
| `Docs/general_wxr/` | the shared, game-agnostic WinlatorXR framework | copied unchanged, per its own README. Keep the folder name |
| `Docs/ClaudeCode/` | **this port's** overview, plan, status and defect register | promoted from `Docs/FutureGames/<Game>/`, renamed to match the convention the AoE2 repo uses |
| **`Docs/AoE2Notes/`** | **the finished AoE2 port's findings** | copied from the AoE2 repo at migration time |

Plus a root `CLAUDE.md` pointing at all three, in that reading order — the
pattern the AoE2 repo already uses.

### Why `AoE2Notes/` is not optional

For most projects, a sibling port's documentation would be background reading.
Here it is **the implementation manual**, and section 1 is the reason: all four
games share one integration surface, so the AoE2 port is not a precedent for
these ports — it is the **first instance** of them. Its notes carry the things
the next port cannot reconstruct:

- **Where the seam is** — which parts are game-independent and which belong in
  an adapter (`Docs/ClaudeCode/PROJECT_OVERVIEW.md` section 7a). The code
  itself is readable in the public repo, so what matters here is the
  *reasoning* about the boundary, which a diff never shows.
- **The cnc-ddraw arrangement**: the pinned version, the `direct3d9` requirement,
  the `Present` hook, and whatever turned out to be true about it on device.
- **Measured values that transfer**: the FOV pair that looked right, the
  container resolution and its cost, the table geometry — height, size, tilt
  — that felt comfortable. These are hours of on-device tuning each, and they
  are per-stack rather than per-game, so they should transfer.
- **The pointer rules** — either-handed arbitration, the trigger-locks-ownership
  rule, the set-position-before-button ordering, the billboard picking and
  outline behaviour.
- **What was tried and abandoned**, which is the section most likely to be
  skipped and the one that stops a later session walking into a known dead end.
- **The defect register**, because several of its entries are about this *stack*
  rather than about AoE2.

`general_wxr/TIPS.md` makes the general version of this point: document the
things that are invisible afterwards, because that is what the next port
cannot reconstruct. `AoE2Notes/` is where that lands.

### Where the shared *code* lives: in the AoE2 repository, read from there

**Settled [owner]: no shared-layer repository, no submodules.** The AoE2 port
is public, so every later port can read its code at source, at any time. Docs
are copied; code is consulted.

This document previously recommended a separate shared repository consumed as a
submodule, on the grounds that code copied into four repositories becomes four
divergent forks. **That reasoning does not fit this project's shape**, and the
owner's does:

- These ports are **sequential and solo**, not a product line under
  simultaneous maintenance. There is no live fleet to keep in step.
- The shared surface is **small and stabilises early** — the XrApi client, the
  sync pixel handling, the pose maths. By the time port two starts, the risky
  parts are the parts already device-verified, which is to say the parts least
  likely to need a retrofit.
- A submodule costs **real friction for one developer**: edit in one place,
  then commit, push and bump in four. That buys little when only one port is
  active at a time.
- Each adapter will want the layer bent slightly anyway — different HUD
  geometry, different scroll keys — so a hard shared dependency is as likely
  to constrain as to help.

**Keep one cheap discipline, which is most of what a submodule would have
given.** When a port copies or adapts code from AoE2, **record the commit it
came from** — one line in its `AoE2Notes/`:

```
Shared layer adapted from AoE2-WXR @ <commit sha>, <date>
```

Without it, "read it from the public repo" stops working the moment the repo
moves on: a later session looking at port three's renderer cannot tell whether
it predates or postdates a fix upstream. With it, the question is a `git log`
away. No tooling, no coupling.

**And this sharpens what `AoE2Notes/` is for.** Since the code is a link away,
that folder should **not** try to restate it. It carries what the code cannot
show: the measured values, the reasons behind the decisions, and the approaches
tried and abandoned — exactly what `general_wxr/TIPS.md` means by documenting
the things that are invisible afterwards. It should open with the repository
URL and the commit, then spend its length on everything that is not in there.

---

## 6. Hard rules, inherited

Unchanged from `Docs/ClaudeCode/` and `Docs/general_wxr/CLAUDE.md`:

- **32-bit, Win32, in-process.** All four binaries are PE32.
- **XrApi 0.5.** Read once at startup; an unrecognised value is silence.
- **The sync pixel is not optional.**
- **The window must be borderless and fill the display**, re-applied each frame.
- **No game assets in this repository** — no MIX, MPQ, SHP, GRP, VXL or
  palette dumps. `.gitignore` enforces it.
- **Install paths are not recorded in this repository.** The owner's copies are
  referred to only as such.
- **No DRM work, on any title.** For StarCraft specifically this rules out
  Remastered and the 1.18+ free-to-play release, both of which require a
  Battle.net account: the target is **1.16.1**, the last standalone offline
  build **[owner]**. More generally, no authentication bypass and no online
  path in any of these ports — offline, skirmish and LAN only. Where an install
  already carries copy protection (Red Alert 2's SafeDisc remnants), the
  response is the community no-CD route for a game the player owns, not our own
  circumvention work.
- **Nothing is done until the owner has verified it on hardware.**
