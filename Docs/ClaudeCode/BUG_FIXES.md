# BUG_FIXES — defect register

**What this file is.** No code in this repository has been built or run yet,
so there are no fixed bugs to report. What follows is a **defect register for
the inherited skeleton** in `Docs/MetaAI/Phase1Proj`, found by reading it.

That makes this file preventative rather than historical: these are the things
**not to reintroduce** when the real source tree is written at Stage 2. Each
item is cheaper to avoid than to debug on a headset with no debugger.

Items are numbered and the numbers are referenced from the other documents in
this folder — **do not renumber**. Append new items; mark fixed ones with the
commit that fixed them.

Every finding below is **[static]** — read from the source, not observed at
runtime. Several would be caught by the first compile; the interesting ones
would not.

---

## Would not compile or load

### 1. `ddraw.dll` proxy can load itself under Wine, and recurse until the stack dies

`proxy.cpp` resolves the real DirectDraw with
`LoadLibraryA("C:\\windows\\system32\\ddraw.dll")`, while the README sets
`WINEDLLOVERRIDES=ddraw=n,b`.

**Wine applies DLL overrides by module name, not by path.** That call can
therefore resolve back to our own already-loaded module; `GetProcAddress` then
returns our own `DirectDrawCreate`, and the first call recurses until the
stack is exhausted. Wine's builtin ddraw is a `.dll.so`, so there is no PE
file to copy aside and rename — the usual escape hatch does not exist.

**Status:** avoided by design. `PROJECT_OVERVIEW.md` section 4 moves the
injection vector away from `ddraw.dll` altogether. As of 2026-11-01 the mod
ships as **`d3d9.dll`** (`PROJECT_OVERVIEW.md` section 4), which escapes this
trap properly rather than merely avoiding it: **DXVK's `d3d9.dll` is an
ordinary renameable PE file**, so the real implementation can be shipped
alongside under its own name and loaded explicitly, where Wine's builtin
`ddraw` is a `.dll.so` with no PE to rename. `version.dll` remains on the
fallback list. If a `ddraw` proxy is ever revisited, the real module must be
resolved by something other than its name.

### 2. The vtable indices are correct — recorded so nobody re-derives them

The MetaAI notes list these as *"assumed from SDK — needs log verification"*,
and they are in fact right, which is worth writing down once:

| Interface | Method | Index |
|---|---|---|
| `IDirectDrawSurface7` | `Blt` | 5 |
| `IDirectDrawSurface7` | `BltFast` | 7 |
| `IDirectDrawSurface7` | `Flip` | 11 |
| `IDirectDraw7` | `CreateSurface` | 6 |

Counting from the three `IUnknown` slots at 0–2, in declaration order. **[static]**

**And they generalise**, which matters now that Stage A has ruled the 7
interfaces out: `Blt` = 5, `BltFast` = 7 and `Flip` = 11 hold on *every*
`IDirectDrawSurface*`, and `CreateSurface` = 6 on every `IDirectDraw*`, because
later interface versions **append** methods rather than reordering them. The
one real difference is the struct: `DDSURFACEDESC` for versions 1–3,
`DDSURFACEDESC2` for 4 and 7.

**Not a bug — and it does not rescue item 4**, which is about which
*interface* the game asks for, not which slot a method occupies.

### 3. Pixel format is assumed to be 32bpp, so a palettized surface renders as garbage

`d3d11_renderer.cpp`, uploading the locked surface:

```
memcpy(dst + y * mp.RowPitch, src + y * d.lPitch, w * 4);
```

`w * 4` is the destination's bytes-per-row. The **source** may be 8 or 16
bits per pixel, in which case this reads up to four times the real row width —
garbage output, and a read past the end of the surface on the last row. The
MetaAI notes acknowledge the palette gap but the code does not guard it.

**Confirmed by Stage A: 8-bit palettized is the main path.** The exe imports
the full GDI palette API — `CreatePalette`, `SelectPalette`, `RealizePalette`,
`ResizePalette`, `GetPaletteEntries`, `SetPaletteEntries`,
`GetSystemPaletteEntries`, `GetNearestPaletteIndex` **[measured]** — which a
game only needs in a palettized display mode. This is the first frame, not an
edge case.

**Fix at Stage 6:** read `ddpfPixelFormat` and branch. For a palettized
surface, read the DirectDraw palette, build a 256-entry lookup table, rebuild
it when the palette changes, and convert per row. Never copy `lPitch` bytes
blindly either — pitch is not width.

### 4. Only the `7` interfaces are hooked, and the game may never ask for them

`proxy.cpp` QueryInterfaces to `IID_IDirectDraw7`; `hooks.cpp` patches
`IDirectDrawSurface7`. A 1999 title may well create its surfaces through
`IDirectDraw2` or `IDirectDraw4` and draw through `IDirectDrawSurface3`, in
which case `Hooked_CreateSurface` never fires and nothing downstream of it
ever runs.

**Status: confirmed fatal, and worse than it looks. [measured]** Stage A
searched both executables for every DirectDraw interface GUID.
`IID_IDirectDraw7` and `IID_IDirectDrawSurface7` are **absent**; `IDirectDraw`,
`2`, `4` and `IDirectDrawSurface`, `2`, `3`, `4` are all present. A binary
cannot `QueryInterface` for a GUID it does not carry, so the game provably
never touches the 7 interfaces.

The subtlety: the skeleton's own `QueryInterface(IID_IDirectDraw7)` would have
**succeeded**, because real `ddraw.dll` implements DD7 regardless of what the
game wants. But each interface version has its own vtable, so patching DD7's
`CreateSurface` slot intercepts nothing the game does through DD1/2/4. The
skeleton would have logged "Hooked DD7 CreateSurface" and then never fired
again — a success message followed by silence, which is the most expensive
kind of failure to diagnose on a device with no debugger.

**Fix:** target DD1/2/4 and Surface1/2/3/4, and **wrap rather than
vtable-patch** — return our own object from `DirectDrawCreate` and wrap
`QueryInterface`, so every interface the game asks for is observed and wrapped
as it is handed over. The version in use then becomes a log line on the first
run instead of a question.

### 5. `hooks.cpp` uses `std::vector` without including `<vector>`

Caught by the first compile. Noted only because it confirms the skeleton has
never been built.

---

## Would compile, and then mislead

### 6. The logger has no build stamp, which the framework calls non-negotiable

`general_wxr/CLAUDE.md`: embed `__DATE__ __TIME__`, log it on line 1, and make
the deploy script refuse a binary older than the newest source file. The
inherited logger writes `"=== AoE2 Phase1 Log Started ==="` and no stamp.

**Why this matters more than it looks:** testing a stale build is invisible
without the check, and it wastes an entire session when it happens. Fix at
Stage 2, together with the deploy script's freshness guard — both before there
is anything complicated to debug.

### 7. `Logger::Init` is unsynchronised and `init` is not atomic

`Log()` calls `Init()` when not initialised; `Init()` sets `init = true`
*before* calling `Log()`, which is what stops the recursion the MetaAI notes
describe as a fixed deadlock. But `Init()` never takes the mutex, and `init`
is a plain `bool`. Two threads reaching `Log()` first can both run `Init()`,
both `CreateFileW` with `CREATE_ALWAYS`, and leak a handle while truncating
each other's output.

DirectDraw games are not reliably single-threaded, and we are adding a thread
of our own.

**Fix:** initialise explicitly from `DllMain` (which the skeleton already
does) and make `Log()` a no-op before that, rather than self-initialising.

### 8. Hardcoded `C:\` log path, with the documented fallback missing from the code

`BUG_FIXES.md` in `Docs/MetaAI/` claims *"added Z:\tmp fallback"*. The code
writes `C:\aoe2_phase1_log.txt` unconditionally. The claim and the code
disagree, and the code is what runs.

**Fix:** write next to our own DLL, which is always writable and always
findable by the owner. Use the `Z:\` test for "am I on a headset?" only, as
the Black and White port does.

### 9. Viewport is sized to the texture, not the render target

`RenderFromLockedSurface` sets `D3D11_VIEWPORT{0, 0, tw, th, 0, 1}`, where
`tw`/`th` are the *source surface* dimensions. When the container resolution
differs from the game's surface size — which is the normal case, and is the
whole supersampling story — the viewport covers part of the window and the
sync pixel's corner is no longer where the runtime samples.

**Fix:** viewport from the backbuffer, always. And handle the backbuffer
changing size.

### 10. The primary surface is locked in full, every frame

`Hooked_Flip` calls `Lock(NULL, ..., DDLOCK_READONLY | DDLOCK_WAIT, NULL)` on
the flipping surface on every frame. A full blocking lock of a primary surface
is a performance cliff on real hardware and a likely stall under Wine, where
it may also fail outright or contend with the runtime's own window capture.

**Status: moot on the primary path.** Under the revised architecture
(`PROJECT_OVERVIEW.md` section 4) cnc-ddraw owns DirectDraw and we never lock
a surface. On the fallback path: lock a dirty rect, never the whole primary.

### 11. Two presenters on one window, with no evidence either survives

`Hooked_Flip` suppresses the game's `Flip` and returns `DD_OK` after
presenting its own D3D11 swapchain on the same HWND. This may work, may
flicker, or may deadlock — it has never been run.

**Status: closed by design, and this was the project's largest unknown.**
Under the revised architecture there is exactly one D3D9 device — cnc-ddraw's
 — and we hook its `Present` rather than competing with it. Spike S3 now
confirms the device exists rather than asking who wins a fight over the
window. See `PROJECT_OVERVIEW.md` section 4b.

### 12. One global `Real_*` pointer per hooked method, across all vtables

`HookSurface7` dedupes by vtable pointer in a function-static `std::vector`
with no synchronisation, and stores the originals in single globals. Today the
dedupe hides the problem. The moment a second interface version is hooked —
which item 4 makes likely — the second `InstallVTableHook` overwrites the
first's original pointer and calls into the wrong trampoline.

**Fix:** key the originals per vtable, or use MinHook and let it own them.

### 13. `proxy.def` exports ten names; real `ddraw.dll` exports many more

Missing: `AcquireDDThreadLock`, `ReleaseDDThreadLock`, `SetAppCompatData`,
`DDInternalLock`/`Unlock`, `DDGetAttachedSurfaceLcl`, `GetDDSurfaceLocal`,
`GetOLEThunkData`, `D3DParseUnknownCommand`, `CompleteCreateSysmemSurface`,
`DSoundHelp`, `RegisterSpecialCase` and others. Anything importing one of them
fails to load. The fixed ordinals (`@1`–`@10`) are also arbitrary and do not
match the real module's.

**Status:** moot once item 1's fix lands. AoE2 imports exactly three functions
from `VERSION.dll` — `GetFileVersionInfoA`, `GetFileVersionInfoSizeA`,
`VerQueryValueA` **[measured]** — so the replacement proxy forwards three
thunks. Note they *are* genuinely called, so forward them properly rather than
stubbing them.

---

## Not bugs, but wrong

### 14. The MetaAI architecture description is factually incorrect

*"D3D11 -> DXVK -> Vulkan -> OpenXR. D3D11 emitted shows up in VR."*
WinlatorXR captures the **game window**; it does not hook a renderer or read a
swapchain, and no OpenXR is involved in a mod's path. Corrected in full at
`PROJECT_OVERVIEW.md` section 7.

Consequence: the inherited plan has no sync pixel, no window handling and no
XrApi client — three of the framework's non-negotiables — because it believed
none were needed.

### 15. The target patch level and executable are wrong

Those docs target `empires2.exe`, which is the **base-game** binary. The game
runs from `age2_x1/age2_x1.exe` — version resource `AGE2_X1.EXE`,
"Age of Empires II Expansion", FileVersion `00.07.26.0809`, Microsoft Corp.
2001 **[measured]**.

Treat every inherited address, offset and exe name as suspect for two
reason: the inspected exe is a **no-CD repack** carrying four appended
sections (`THIS_COD…`, `THIS_DAT…`, `Inf32Dat…`, `.patch`) that a pristine
1.0c does not have, so published stock-1.0c addresses were never going to
match. (UserPatch, which would have moved them a third time, is out of scope
— `PROJECT_OVERVIEW.md` section 2a.)

### 16. One entry in the MetaAI list is not a bug fix at all

*"Reddit domain blocked — worked around by analyzing uploaded video
instead"* — a tooling limitation of that session, recorded as a code fix. Not
carried forward. Noted only because it is a reminder to keep this file for
defects in **our** code, so that it stays worth reading.
