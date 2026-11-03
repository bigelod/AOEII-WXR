// AOEII-WXR - d3d9.dll proxy and the Present hook.
//
// Stack (Docs/ClaudeCode/PROJECT_OVERVIEW.md section 4a):
//
//   age2_x1.exe -> cnc-ddraw -> THIS -> the real d3d9 -> DXVK -> Vulkan
//
// cnc-ddraw owns DirectDraw and hands us a finished 2D frame in the
// backbuffer; we intercept Present. Nothing here depends on cnc-ddraw's
// internals, only on "the backbuffer contains the frame at Present", which is
// true by construction.
//
// Stage 2 + Spike S3 scope only: load, log, prove we are inside the right
// device, and put one coloured rectangle on screen. No camera, no tabletop,
// no XrApi yet.

#include <windows.h>
#include <d3d9.h>
#include <stdio.h>
#include <string.h>

#include "log.h"

namespace {

HMODULE g_self = NULL;
HMODULE g_real = NULL;

// --- vtable indices. Stable across the whole life of D3D9; counted from the
// three IUnknown slots. Recorded here so nobody re-derives them, the way
// BUG_FIXES #2 records the DirectDraw ones.
const int kIDirect3D9_CreateDevice = 16;
const int kIDirect3DDevice9_Reset = 16;
const int kIDirect3DDevice9_Present = 17;

typedef IDirect3D9*(WINAPI* PFN_Direct3DCreate9)(UINT);
typedef HRESULT(WINAPI* PFN_Direct3DCreate9Ex)(UINT, IDirect3D9Ex**);

typedef HRESULT(STDMETHODCALLTYPE* PFN_CreateDevice)(IDirect3D9*, UINT,
                                                     D3DDEVTYPE, HWND, DWORD,
                                                     D3DPRESENT_PARAMETERS*,
                                                     IDirect3DDevice9**);
typedef HRESULT(STDMETHODCALLTYPE* PFN_Present)(IDirect3DDevice9*,
                                                const RECT*, const RECT*,
                                                HWND, const RGNDATA*);
typedef HRESULT(STDMETHODCALLTYPE* PFN_Reset)(IDirect3DDevice9*,
                                              D3DPRESENT_PARAMETERS*);

PFN_CreateDevice g_realCreateDevice = NULL;
PFN_Present g_realPresent = NULL;
PFN_Reset g_realReset = NULL;

bool g_probe = true;  // S3: draw a visible rectangle. See ReadFlags().
long g_presents = 0;

// ---------------------------------------------------------------- vtable

bool PatchSlot(void* iface, int index, void* replacement, void** original) {
  if (!iface) return false;
  void** vtbl = *(void***)iface;
  DWORD old = 0;
  if (!VirtualProtect(&vtbl[index], sizeof(void*), PAGE_READWRITE, &old))
    return false;
  if (original) *original = vtbl[index];
  vtbl[index] = replacement;
  VirtualProtect(&vtbl[index], sizeof(void*), old, &old);
  return true;
}

// ---------------------------------------------------------------- hooks

HRESULT STDMETHODCALLTYPE Hooked_Present(IDirect3DDevice9* self,
                                         const RECT* src, const RECT* dst,
                                         HWND wnd, const RGNDATA* dirty) {
  const long n = InterlockedIncrement(&g_presents);
  if (n == 1) wxr::Log("Present: first frame - we are in the device");

  // A heartbeat at normal level, not just verbose. On device the first log
  // we got back ended mid-session with no DLL_PROCESS_DETACH, so anything
  // written only at shutdown may never be written at all. Liveness has to be
  // visible from the body of the log. BUG_FIXES #19.
  if ((n % 600) == 0) wxr::Log("Present: %ld frames", n);
  else if ((n % 60) == 0) wxr::LogV("Present: %ld frames", n);

  // Spike S3's whole deliverable: our pixels in the window. Clear of a
  // sub-rectangle needs no shaders, no vertex buffer and no state juggling -
  // and it is the same mechanism the sync pixel will use later, so proving it
  // here proves that too.
  if (g_probe && self) {
    D3DRECT box = {16, 16, 16 + 96, 16 + 96};
    self->Clear(1, &box, D3DCLEAR_TARGET,
                D3DCOLOR_XRGB(255, 0, 255), 1.0f, 0);
  }

  if (!g_realPresent) return D3DERR_INVALIDCALL;
  return g_realPresent(self, src, dst, wnd, dirty);
}

HRESULT STDMETHODCALLTYPE Hooked_Reset(IDirect3DDevice9* self,
                                       D3DPRESENT_PARAMETERS* pp) {
  if (pp) {
    wxr::Log("Reset: %ux%u fmt=%d windowed=%d",
             pp->BackBufferWidth, pp->BackBufferHeight,
             (int)pp->BackBufferFormat, pp->Windowed ? 1 : 0);
  }
  if (!g_realReset) return D3DERR_INVALIDCALL;
  return g_realReset(self, pp);
}

HRESULT STDMETHODCALLTYPE Hooked_CreateDevice(IDirect3D9* self, UINT adapter,
                                              D3DDEVTYPE type, HWND focus,
                                              DWORD flags,
                                              D3DPRESENT_PARAMETERS* pp,
                                              IDirect3DDevice9** out) {
  if (pp) {
    wxr::Log("CreateDevice: %ux%u fmt=%d windowed=%d hwnd=%p devtype=%d",
             pp->BackBufferWidth, pp->BackBufferHeight,
             (int)pp->BackBufferFormat, pp->Windowed ? 1 : 0,
             (void*)pp->hDeviceWindow, (int)type);
  }

  HRESULT hr = g_realCreateDevice
                   ? g_realCreateDevice(self, adapter, type, focus, flags, pp,
                                        out)
                   : D3DERR_INVALIDCALL;
  if (FAILED(hr) || !out || !*out) {
    wxr::Log("CreateDevice FAILED hr=0x%08lx", (unsigned long)hr);
    return hr;
  }

  static bool patched = false;
  if (!patched) {
    patched = true;
    const bool p = PatchSlot(*out, kIDirect3DDevice9_Present,
                             (void*)&Hooked_Present, (void**)&g_realPresent);
    const bool r = PatchSlot(*out, kIDirect3DDevice9_Reset,
                             (void*)&Hooked_Reset, (void**)&g_realReset);
    wxr::Log("device hooks: Present=%d Reset=%d  probe=%d",
             p ? 1 : 0, r ? 1 : 0, g_probe ? 1 : 0);
    if (!p) wxr::Log("FATAL: could not hook Present - nothing will be drawn");
  }
  return hr;
}

// ---------------------------------------------------------------- startup

// Read the PE machine type without loading the file. On a headset there is no
// way to inspect a DLL by hand, so the log has to do it: "LoadLibrary failed"
// alone sent the owner looking at DXVK when the file was simply 64-bit.
WORD PeMachine(const char* path) {
  HANDLE h = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL,
                         OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (h == INVALID_HANDLE_VALUE) return 0;
  BYTE buf[1024];
  DWORD got = 0;
  WORD machine = 0;
  if (ReadFile(h, buf, sizeof(buf), &got, NULL) && got >= 0x40) {
    const LONG off = *(const LONG*)(buf + 0x3c);
    if (off > 0 && (DWORD)off + 6 <= got && *(const DWORD*)(buf + off) == 0x4550)
      machine = *(const WORD*)(buf + off + 4);
  }
  CloseHandle(h);
  return machine;
}

const char* MachineName(WORD m) {
  switch (m) {
    case 0x014c: return "i386 32-bit";
    case 0x8664: return "x86-64 - WRONG, this process is 32-bit";
    case 0xaa64: return "arm64 - WRONG";
    case 0:      return "unreadable / not a PE";
    default:     return "unrecognised";
  }
}

// Load the genuine d3d9 WITHOUT tripping over our own name.
//
// This is the trap that would have killed the inherited ddraw proxy
// (BUG_FIXES #1): Wine applies DLL overrides by module NAME, not path, so
// asking for "d3d9.dll" can hand back our own module and the first forwarded
// call recurses until the stack dies. d3d9 escapes it properly because the
// real implementation - DXVK's or the system's - is an ordinary renameable PE
// file. So prefer an explicitly renamed copy, and verify we did not get
// ourselves.
bool LoadReal() {
  if (g_real) return true;

  char path[MAX_PATH];
  _snprintf_s(path, sizeof(path), _TRUNCATE, "%s\\d3d9_real.dll",
              wxr::ModuleDir());
  if (GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES) {
    const WORD machine = PeMachine(path);
    g_real = LoadLibraryA(path);
    const DWORD err = g_real ? 0 : GetLastError();
    wxr::Log("real d3d9: %s -> %p  machine=0x%04x (%s)", path, (void*)g_real,
             machine, MachineName(machine));
    if (!g_real) {
      wxr::Log("LoadLibrary failed, err=%lu%s", err,
               err == 193 ? " (ERROR_BAD_EXE_FORMAT - wrong architecture. "
                            "Need the 32-bit d3d9.dll: syswow64/ in a "
                            "Wine-layout DXVK archive, x32/ upstream. "
                            "system32/ and x64/ are the 64-bit ones.)"
                          : err == 126 ? " (ERROR_MOD_NOT_FOUND - the file "
                                         "loaded but one of ITS dependencies "
                                         "did not. For DXVK that is usually a "
                                         "missing 32-bit vulkan-1.dll.)"
                                       : "");
    }
  } else {
    // Expected to fail, and tried anyway only so the log says so
    // explicitly. The loader keys its module list on the FILE NAME, not
    // the path, so a request for any "d3d9.dll" - absolute path included -
    // hands back the module already loaded under that name, which is us.
    // d3d9_real.dll is therefore mandatory, not a convenience.
    wxr::Log("no d3d9_real.dll beside us. Trying the system copy, which "
             "the loader will almost certainly answer with ourselves.");
    char sys[MAX_PATH];
    GetSystemDirectoryA(sys, MAX_PATH);
    _snprintf_s(path, sizeof(path), _TRUNCATE, "%s\\d3d9.dll", sys);
    g_real = LoadLibraryA(path);
    wxr::Log("real d3d9: %s -> %p", path, (void*)g_real);
  }

  if (g_real == g_self) {
    wxr::Log("FATAL: loaded OURSELVES as the real d3d9. Ship d3d9_real.dll "
             "beside this module. Refusing to recurse. cnc-ddraw will get "
             "NULL and fall back to another renderer, so the game should "
             "still run - without us.");
    g_real = NULL;
    return false;
  }
  if (!g_real) {
    wxr::Log("FATAL: no real d3d9 could be loaded");
    return false;
  }
  return true;
}

void* RealProc(const char* name) {
  if (!LoadReal()) return NULL;
  return (void*)GetProcAddress(g_real, name);
}

// Spike S3 insists a missing D3D9 device must fail loudly rather than
// mysteriously, so say on line 2 what the wrapper situation actually is.
void ReportWrapper() {
  const char* inis[] = {"ddraw.ini", "cnc-ddraw.ini"};
  char found[MAX_PATH] = {0};
  char renderer[64] = {0};

  for (int i = 0; i < 2; ++i) {
    char path[MAX_PATH];
    _snprintf_s(path, sizeof(path), _TRUNCATE, "%s\\%s", wxr::ModuleDir(),
                inis[i]);
    if (GetFileAttributesA(path) == INVALID_FILE_ATTRIBUTES) continue;
    strcpy_s(found, sizeof(found), inis[i]);
    GetPrivateProfileStringA("ddraw", "renderer", "", renderer,
                             sizeof(renderer), path);
    if (renderer[0]) break;
  }

  char ddraw[MAX_PATH];
  _snprintf_s(ddraw, sizeof(ddraw), _TRUNCATE, "%s\\ddraw.dll",
              wxr::ModuleDir());
  const bool haveDdraw =
      GetFileAttributesA(ddraw) != INVALID_FILE_ATTRIBUTES;

  wxr::Log("cnc-ddraw: ddraw.dll=%s ini=%s renderer=%s",
           haveDdraw ? "present" : "MISSING",
           found[0] ? found : "none",
           renderer[0] ? renderer : "unset");

  if (!haveDdraw) {
    wxr::Log("WARNING: no ddraw.dll beside the exe. Without cnc-ddraw there "
             "is no D3D9 device to hook and nothing below will run.");
  } else if (renderer[0] && _stricmp(renderer, "direct3d9") != 0) {
    wxr::Log("WARNING: cnc-ddraw renderer is '%s', not 'direct3d9'. Only the "
             "direct3d9 renderer creates a device we can hook.", renderer);
  }
}

void ReadFlags() {
  char path[MAX_PATH];
  _snprintf_s(path, sizeof(path), _TRUNCATE, "%s\\aoe2wxr_noprobe.txt",
              wxr::ModuleDir());
  if (GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES) g_probe = false;
}

}  // namespace

// ---------------------------------------------------------------- exports

extern "C" IDirect3D9* WINAPI Direct3DCreate9(UINT sdk) {
  static PFN_Direct3DCreate9 real = NULL;
  if (!real) real = (PFN_Direct3DCreate9)RealProc("Direct3DCreate9");
  if (!real) return NULL;

  IDirect3D9* d3d = real(sdk);
  wxr::Log("Direct3DCreate9(sdk=%u) -> %p", sdk, (void*)d3d);
  if (!d3d) return NULL;

  static bool patched = false;
  if (!patched) {
    patched = true;
    const bool ok = PatchSlot(d3d, kIDirect3D9_CreateDevice,
                              (void*)&Hooked_CreateDevice,
                              (void**)&g_realCreateDevice);
    wxr::Log("IDirect3D9::CreateDevice hook=%d", ok ? 1 : 0);
    if (!ok)
      wxr::Log("FATAL: could not hook CreateDevice - we will never see the "
               "device");
  }
  return d3d;
}

extern "C" HRESULT WINAPI Direct3DCreate9Ex(UINT sdk, IDirect3D9Ex** out) {
  static PFN_Direct3DCreate9Ex real = NULL;
  if (!real) real = (PFN_Direct3DCreate9Ex)RealProc("Direct3DCreate9Ex");
  if (!real) return D3DERR_NOTAVAILABLE;

  const HRESULT hr = real(sdk, out);
  wxr::Log("Direct3DCreate9Ex(sdk=%u) -> hr=0x%08lx obj=%p  [not hooked]",
           sdk, (unsigned long)hr, out ? (void*)*out : NULL);
  // Not hooked deliberately: cnc-ddraw's direct3d9 renderer uses the
  // non-Ex entry point. If this line ever appears in a log, that assumption
  // is wrong and CreateDeviceEx needs hooking too.
  return hr;
}

// Forwarded verbatim. Debug/profiling helpers that some layers call; none of
// them matter to us, but a missing export is a load failure.
extern "C" int WINAPI D3DPERF_BeginEvent(D3DCOLOR c, LPCWSTR n) {
  typedef int(WINAPI * F)(D3DCOLOR, LPCWSTR);
  static F f = (F)RealProc("D3DPERF_BeginEvent");
  return f ? f(c, n) : 0;
}
extern "C" int WINAPI D3DPERF_EndEvent(void) {
  typedef int(WINAPI * F)(void);
  static F f = (F)RealProc("D3DPERF_EndEvent");
  return f ? f() : 0;
}
extern "C" void WINAPI D3DPERF_SetMarker(D3DCOLOR c, LPCWSTR n) {
  typedef void(WINAPI * F)(D3DCOLOR, LPCWSTR);
  static F f = (F)RealProc("D3DPERF_SetMarker");
  if (f) f(c, n);
}
extern "C" void WINAPI D3DPERF_SetRegion(D3DCOLOR c, LPCWSTR n) {
  typedef void(WINAPI * F)(D3DCOLOR, LPCWSTR);
  static F f = (F)RealProc("D3DPERF_SetRegion");
  if (f) f(c, n);
}
extern "C" BOOL WINAPI D3DPERF_QueryRepeatFrame(void) {
  typedef BOOL(WINAPI * F)(void);
  static F f = (F)RealProc("D3DPERF_QueryRepeatFrame");
  return f ? f() : FALSE;
}
extern "C" void WINAPI D3DPERF_SetOptions(DWORD o) {
  typedef void(WINAPI * F)(DWORD);
  static F f = (F)RealProc("D3DPERF_SetOptions");
  if (f) f(o);
}
extern "C" DWORD WINAPI D3DPERF_GetStatus(void) {
  typedef DWORD(WINAPI * F)(void);
  static F f = (F)RealProc("D3DPERF_GetStatus");
  return f ? f() : 0;
}
extern "C" void WINAPI DebugSetMute(void) {
  typedef void(WINAPI * F)(void);
  static F f = (F)RealProc("DebugSetMute");
  if (f) f();
}

// ---------------------------------------------------------------- DllMain

BOOL APIENTRY DllMain(HMODULE self, DWORD reason, LPVOID) {
  switch (reason) {
    case DLL_PROCESS_ATTACH:
      DisableThreadLibraryCalls(self);
      g_self = self;
      wxr::LogInit(self);
      ReadFlags();
      ReportWrapper();
      wxr::Log("waiting for Direct3DCreate9");
      break;
    case DLL_PROCESS_DETACH:
      // Not guaranteed to run. Observed absent on Quest 3, where the
      // container appears to be killed outright. Never defer anything that
      // matters to here - treat it as a courtesy, not a shutdown path.
      wxr::Log("Present frames total: %ld", g_presents);
      wxr::LogShutdown();
      break;
  }
  return TRUE;
}
