# StarCraft: Brood War — PROJECT_OVERVIEW (high-level)

**Status: not started, and gated** on AoE2 reaching Milestone 6. See
[../README.md](../README.md) for the shared analysis, the wrapper question and
the hard rules — none of it is repeated here.

**Recommended as the fourth port**, despite being the title with by far the
largest audience. It is the architectural odd one out, and the second port's
job is to prove the shared layer rather than to fight a new engine.

Tags as elsewhere: **[measured]** (read from the binary, 2026-11-01),
**[unverified]**, **[owner]**.

---

## 1. What this is

StarCraft and the Brood War expansion, patch **1.16.1** — the last classic
build before Remastered. A 2D isometric RTS with a **genuinely flat world**:
no per-cell elevation, cliffs drawn as art rather than geometry.

That makes it the *simplest tabletop* of the four and the *hardest
interception*. Those two facts pull in opposite directions, which is the whole
character of this port.

Same three-step goal as AoE2: head-tracked 3D tabletop, laser pointer, HUD
lifted off the table, then sprites standing up.

> ## Hard scope: 1.16.1 only — the pre-Battle.net-DRM build
>
> **The target is classic Brood War patch 1.16.1, and nothing else** **[owner]**.
> Two other things are called "StarCraft" and both are explicitly out of scope,
> for the same reason:
>
> - **StarCraft: Remastered** — a different renderer, a different integration
>   story, and it requires a Battle.net account.
> - **The modern free-to-play "classic" release** (1.18 and later) — it looks
>   like the old game but it was rebuilt around **Battle.net account login and
>   DRM**.
>
> **We do not touch Battle.net DRM, and we do not work around it** **[owner]**.
> That is not a technical judgement about difficulty; it is a line this project
> does not cross. Any route that involves defeating, emulating or bypassing
> account authentication is off the table, and the answer to "would this work
> on 1.18?" is that we are not going to find out.
>
> This is why **1.16.1 is the right target and not merely a convenient one**: it
> is the last build that runs standalone, offline, from files the player owns,
> with no account and no authentication server. The copy inspected here is
> 1.16.1 **[measured]**, with `SFNoCD.dll` present, so it already runs that way.
>
> Everything online is out of scope too — no Battle.net, no ladder,
> no anti-cheat surface. Single-player, skirmish and LAN only. Say all of this
> plainly in any release notes, because players will ask.

---

## 2. What the binary says

| Question | Answer |
|---|---|
| Executable | `StarCraft.exe`, 1,220,608 bytes **[measured]** |
| Version | **1.16.1**, in both FileVersion and ProductVersion **[measured]** |
| Bitness | 32-bit PE32, i386, image base `0x00400000`, GUI subsystem **[measured]** |
| **Imports** | **`storm.dll` only — 135 functions, all by ordinal.** Plus the usual Win32. No graphics library at all **[measured]** |
| Graphics path | `storm.dll` loads `ddraw.dll` **dynamically** — it carries the strings `ddraw.dll` and `DirectDrawCreate` and imports `LoadLibraryA` / `GetProcAddress`, with a "DirectDraw services are not available" error message **[measured]** |
| Direct3D | **none**, in either the exe or Storm **[measured]** |
| DirectDraw interfaces | **No DirectDraw interface GUID appears in `StarCraft.exe` or `storm.dll`** — so `IDirectDraw` / `IDirectDrawSurface` **version 1** only **[measured]** |
| Palette | `CreatePalette`, `RealizePalette`, `SetPaletteEntries` in **`storm.dll`**, with `GetDIBits` / `CreateDIBitmap` / `CreateCompatibleDC` — Storm owns the 8-bit surface and has a GDI DIB path as well as DirectDraw **[measured]** |
| Mouse | `GetCursorPos`, `SetCursorPos`, `ClipCursor` in the exe; `GetCursorPos` in Storm too. **No DirectInput, no raw input** **[measured]** |
| Assets | `.mpq` archives (`StarCraft.mpq`, `BroodWar.mpq`, `StarDat.mpq`, `BrooDat.mpq`, `patch_rt.mpq`) **[measured]** |
| Wrappers present | `WMode.dll` / `WMode_Fix.dll` — a windowed-mode wrapper. Removable **[owner]** |
| No-CD | `SFNoCD.dll` present **[measured]** |

### The one real difference in the family: Storm

The other three games call DirectDraw themselves. StarCraft does not call
*anything* itself — the executable's entire import surface is `storm.dll`, by
ordinal, and Storm owns the window, the surface, the palette and the blit.

Two consequences, one helpful and one not:

- **Helpful: a `ddraw.dll` dropped next to `StarCraft.exe` is still found.**
  Storm resolves DirectDraw with `LoadLibraryA("ddraw.dll")`, and
  `LoadLibrary` searches the application directory first. So the proxy vector
  works here — and `WMode.dll` already in the install is proof that the
  community hooks this game exactly this way.
- **Not helpful: the interception point is one level further away.** Hooking
  `storm.dll`'s exports means hooking **by ordinal**, against a library whose
  symbol names are not in the binary. The community has mapped Storm's
  ordinals thoroughly, and that mapping is the thing to find rather than
  rebuild — the same rule `TOOLS.md` gives for X11 keycodes: *copy it, do not
  reconstruct it*.

### Everything above the wrapper is still shared

The tabletop, the XrApi client, the sync pixel, the camera, the pointer maths
— none of it cares that Storm is in the way. And the mouse profile is
identical to the other three: `GetCursorPos`, no DirectInput. The AoE2 laser
pointer hook transfers directly, which is the fourth time that has been true.

---

## 3. Architecture

The shared layer, unchanged (`Docs/ClaudeCode/PROJECT_OVERVIEW.md` section 4).
What differs is only the bottom of the stack:

- **Injection: take the `ddraw.dll` slot.** Unlike AoE2, this is the natural
  answer rather than a trap — Storm loads DirectDraw dynamically and by name
  from the app directory, so we are not impersonating a system DLL in a way
  that can recurse into itself. Confirm by experiment.
- **Interception: `IDirectDraw` / `IDirectDrawSurface` version 1 only**
  **[measured]**. The simplest wrapper of the four, tied with Tiberian Sun.
- **Fallback if that fails: hook Storm's own blit by ordinal.** More work, and
  it needs the community ordinal map. Rank it second, not first.

**Remove `WMode.dll` before testing.** Two wrappers both deciding how this
game presents itself will not resolve, and windowed-mode handling is something
our own layer has to own anyway (the window must be borderless and fill the
display — a non-negotiable). Keep it aside as the fallback if the game will
not run without it.

---

## 4. What StarCraft has that the others do not

### The simplest tabletop of the four

The world is **flat**. There is no per-cell elevation to read, no heightmap to
reconstruct, and no sloped-ground art to match. The tabletop is a plain quad
and that is *correct* here rather than a compromise — where Tiberian Sun and
Red Alert 2 have an opportunity, StarCraft has one less problem.

### The best-documented sprites

StarCraft's unit art is `.grp` files inside `.mpq` archives, and both formats
are exhaustively documented by thirty years of modding, with open-source
readers available (StormLib for MPQ). If the billboard phase ever needs to
render sprites from the game's own assets rather than scraping the screen —
which is AoE2's Route A — this is the title where that path is best supported.

Read from the player's own install at runtime. **Never commit the archives or
anything extracted from them.**

### And the largest audience by far

Worth stating plainly, because it is the reason this port is tempting to do
early. It is also the reason to do it *well* rather than *first*: the title
with the most players is the worst one to use as a learning exercise.

---

## 5. Risks specific to this title

| Risk | Note |
|---|---|
| ~~Storm sits between us and everything~~ | **mostly dissolved** — cnc-ddraw sits under Storm and we hook D3D9, so Storm is no longer between us and the frame (`../README.md` section 2) |
| ~~Hooking Storm by ordinal~~ | **only on the deep fallback path**, if both cnc-ddraw and our own `ddraw.dll` fail to be loaded by Storm. Then the community's Storm ordinal map is the thing to find, not rebuild |
| **Storm's palette lives one level down** | on the primary path this does not matter, since cnc-ddraw converts for us. It matters only on the fallback |
| **Removing `WMode.dll` breaks the game** | establish early whether 1.16.1 runs in a container without it |
| **Players arrive with the wrong build** | Remastered and the 1.18+ free-to-play release are **both out of scope** and both need a Battle.net account. The install guide must state the required build plainly, or every support question will be this one |
| **Scope creep toward Battle.net** | **not a risk to mitigate, a line not to cross** **[owner]**. No DRM work, no authentication bypass, no online path. If a feature needs any of it, the feature is cut |
| **The 8-bit surface is Storm's, not the game's** | palette handling lives at a different level than in the other three; expect the lookup table to be built from Storm's palette calls |
