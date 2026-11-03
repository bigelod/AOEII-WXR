# DEPLOY — what to copy, and what to look for

**Stage 2 + Spike S3.** This build does three things and nothing else: loads
inside the game, writes a log that proves which binary ran, and paints one
magenta square on screen to prove our pixels reach the window. There is no VR,
no tracking, no tabletop and no pointer in it yet.

One change per deploy (`CLAUDE.md`, Hard constraints). This is the first.

---

## 1. Build

```
build\build.bat
```

Output: `build\out\d3d9.dll` — Win32, Release, machine `0x014c`. The script
refuses to continue if `cl.exe` is not targeting x86, because a 64-bit DLL
will not load into the game and fails **silently**.

## 2. The four files that go in `age2_x1\`

A DLL loaded by name comes from the directory holding the **executable being
launched** — not the working directory, and not where the data lives. So The
Conquerors (`age2_x1\age2_x1.exe`, the target) is served from `age2_x1\`,
while Age of Kings (`empires2.exe`) is served from the parent. **Measured both
ways [owner]:** the log appears beside whichever copy won.

Deploying to both costs nothing and makes the log self-diagnosing.

| File | Where it comes from | Why |
|---|---|---|
| `ddraw.dll` | cnc-ddraw **6.x or newer**, pinned | owns DirectDraw, converts the game's 2D output to D3D9 |
| `ddraw.ini` | same release, edited | must say `renderer=direct3d9` |
| `d3d9.dll` | **ours**, `build\out\` | the proxy; hooks `Present` |
| `d3d9_real.dll` | **DXVK 1.10.3**, the **32-bit** `d3d9.dll`, renamed | the actual D3D9 implementation. Pinned — see 4a, and read it: picking the wrong folder is the single most repeated mistake in this project |

**Do not identify DXVK by its version resource.** It impersonates a Windows
build — `FileVersion` reads `10.0.17763.1 (WinBuild.160101.0800)`, so even an
x64 DXVK looks like a system DLL at a glance **[measured]**. What identifies
it is `ProductName = DXVK` plus a bare `v1.10.3` string inside the image; what
identifies its architecture is the PE header and nothing else. `deploy.ps1`
checks all three.

### `ddraw.ini`

Two settings are part of the declared prerequisite, not preferences:

```ini
[ddraw]
renderer=direct3d9   ; the only renderer that creates a device we can hook
nonexclusive=true    ; no fullscreen-exclusive device
```

`nonexclusive=true` is what makes the game **borderless fullscreen** rather
than exclusive fullscreen: it fills the display while the D3D9 device stays
windowed (`CreateDevice: ... windowed=1` in our own log), so no display-mode
switch happens. A mode switch is what breaks window capture and the sync
pixel. cnc-ddraw already sets it globally and again in its `[age2_x1]`
profile — **leave it alone, and check it first if the capture ever misbehaves**.

Any other renderer (`gdi`, `opengl`, `auto`) creates no D3D9 device, so there
is nothing for us to hook and the log will say so.

**It must be a build that HAS that renderer.** The older cnc-ddraw lineage —
`1.1.4.x`, "DirectDraw replacement for Tiberian Sun and Red Alert 2",
copyright 2010-2018 — ships **GDI and OpenGL only**. Its binary contains no
`direct3d9` string and never references `d3d9.dll` **[measured on 1.1.4.15]**.
Setting `renderer=direct3d9` against it is not an error: the value is simply
unrecognised, it falls back silently, and the symptom is **no log file at
all**, because our DLL is never loaded. Confirm the build before blaming
anything else:

```
(Get-Item ddraw.dll).VersionInfo.FileVersion
```

A `1.1.4.x` answer means the wrong lineage. Use a current release from
[FunkyFr3sh/cnc-ddraw](https://github.com/FunkyFr3sh/cnc-ddraw/releases),
whose README lists GDI / OpenGL / **Direct3D 9**, and **record the version**.

### Why `d3d9_real.dll` is mandatory, not a convenience

We are *called* `d3d9.dll`. The Windows and Wine loaders both key their
module list on the **file name**, not the path — so once we are loaded as
`d3d9.dll`, any request for `d3d9.dll`, absolute path included, hands back
**us**. Forwarding a call would then recurse until the stack dies. That is
`BUG_FIXES.md` #1, and the only clean escape is a real implementation under a
different file name.

Take the **32-bit** `d3d9.dll` from [DXVK
1.10.3](https://github.com/doitsujin/dxvk/releases/tag/v1.10.3), rename it to
`d3d9_real.dll`, and put it beside the others. The version is **pinned**, not
merely recorded: see 4a.

**Which folder is the 32-bit one depends on how the archive was packaged, and
both namings are misleading:**

| Layout | 32-bit lives in | 64-bit lives in |
|---|---|---|
| upstream release `.tar.gz` | `x32/` | `x64/` |
| Wine / Proton style | **`syswow64/`** | **`system32/`** |

The Wine layout inherits Windows' own backwards naming, where `System32`
holds 64-bit files. **Do not trust the folder name — check the file.** The
32-bit `d3d9.dll` of 1.10.3 is 3,305,486 bytes; the 64-bit one is 3,002,382.
The version resource is identical in both, so size or the PE machine type is
the only honest signal **[measured]**. This cost three deploys.

Microsoft's own `d3d9.dll` works for a desktop smoke test and has been used for
one **[measured]**, but it is not an option on device — under Wine it needs
kernel interfaces that are not there. Since the pin is one file for both
targets, use DXVK on the desktop too and keep the Microsoft copy for
emergencies only.

If `d3d9_real.dll` is missing, nothing crashes: `Direct3DCreate9` returns
NULL, cnc-ddraw falls back to another renderer, and the game runs **without
us**. The log says exactly that, in capitals.

### Installing cnc-ddraw through WinlatorXR instead

WinlatorXR can supply cnc-ddraw at container level. That is fine and probably
easier — but then **we cannot verify it**, because our checks only look in our
own folder. The log's `cnc-ddraw:` line becomes uninformative, and
`renderer=direct3d9` has to be set wherever WinlatorXR keeps that config
rather than in a local `ddraw.ini`. The decisive evidence is unchanged either
way: a `Direct3DCreate9` line means a D3D9 renderer is live, and its absence
means it is not.

## 3. Deploy

```
build\deploy.ps1 -GameDir "<...>\age2_x1"
```

It refuses to copy a binary older than the newest file in `src\`, prints
`match=True` only when the SHA-256 at the destination equals what was built,
and then lists which of the four files are present. Add `-Verbose_` to also
drop the verbose flag file.

## 4. Run, then read `age2_x1\aoe2wxr.log`

### What a good log looks like

```
AOEII-WXR d3d9 built <date> <time>  pid=NNNN  verbose=0
[   0.0NN] cnc-ddraw: ddraw.dll=present ini=ddraw.ini renderer=direct3d9
[   0.0NN] waiting for Direct3DCreate9
[   0.NNN] real d3d9: ...\d3d9_real.dll -> 0xNNNNNNNN
[   0.NNN] Direct3DCreate9(sdk=32) -> 0xNNNNNNNN
[   0.NNN] IDirect3D9::CreateDevice hook=1
[   0.NNN] CreateDevice: 1024x768 fmt=NN windowed=1 hwnd=0xNNNN devtype=1
[   0.NNN] device hooks: Present=1 Reset=1  probe=1
[   1.NNN] Present: first frame - we are in the device
```

…and a **magenta square**, 96×96, near the top-left corner of the game
window, over the top of whatever the game is drawing.

### What each failure means

| Log says | Meaning |
|---|---|
| **no log file at all** | our DLL never loaded. The likeliest cause is not our DLL at all: **nothing ever asked for `d3d9.dll`**, because the installed cnc-ddraw has no Direct3D 9 renderer. Check that first (see below), then wrong folder, 64-bit, or a missing dependency |
| `ddraw.dll=MISSING` | no cnc-ddraw **in `age2_x1\`**. If you enabled it through WinlatorXR's own setting instead, it may live in the container's `system32`, where we cannot see it — in that case this line is a **false alarm**: ignore it and judge by whether `Direct3DCreate9` ever arrives |
| `renderer` is not `direct3d9` | fix `ddraw.ini`; there will be no device |
| stops after `waiting for Direct3DCreate9` | we loaded, but nothing ever asked for D3D9. This is the Spike S3 question answered **no**, and it is the one result that changes the architecture |
| `FATAL: loaded OURSELVES` | `d3d9_real.dll` is absent |
| `CreateDevice` logged, no `Present` | the device exists but frames go out another way — look for a swapchain `Present` |
| `Present: first frame` but no square | our code runs and our pixels do not survive. cnc-ddraw is probably presenting from a different surface than the one we clear |

The last two rows are the interesting failures. Either would be worth the
deploy on its own.

## 4a. `d3d9_real.dll` — one pinned file, both targets

**DXVK 1.10.3, the 32-bit `d3d9.dll`, renamed to `d3d9_real.dll`** **[owner]**.
The same file on the Windows desktop and on device. Not the container's copy,
not Microsoft's — pinning it means a difference between desktop and headset is
never the renderer. 1.10.3 also asks only for Vulkan 1.1, where DXVK 2.x wants
1.3.

(Microsoft's `C:\Windows\SysWOW64\d3d9.dll` also works for a quick desktop
check, and `System32` is the 64-bit one — the names are backwards. But it is
not the pin, so do not leave it in place.)

**The file is still needed on device, even though WinlatorXR supplies DXVK.**
Supplying it is not the issue; *reaching* it is. WinlatorXR installs DXVK as
the container's `d3d9.dll`, and we take that name in the application
directory, which wins the search. When we then ask the loader for the real
one, it answers from its module list — keyed on **base name** — and hands back
the module already loaded under `d3d9.dll`: us. Absolute paths do not help.
A renamed copy is the only thing the loader treats as a distinct module.

**There is a way to drop this requirement, and it is not worth taking yet.**
If we were injected under some other name and hooked DXVK's exported
`Direct3DCreate9` in place, the container's own `d3d9.dll` would load normally
and no renamed copy would be needed. That trades one file for a new injection
vector and a trampoline hook — more moving parts, no new capability. Revisit
only if the renamed copy causes trouble on device.

## 5. To turn the square off

Put an empty file named `aoe2wxr_noprobe.txt` in `age2_x1\`. No rebuild.

---

## What this does not test

Nothing in here touches XrApi, so WinlatorXR is not required for step 4 —
**run it on the desktop first**. If the square appears in a plain Windows
window, the whole D3D9 chain is confirmed before the headset is involved at
all, and any difference on the Quest afterwards is Wine's or the container's,
not ours. That is the cheapest possible split of two unknowns.
