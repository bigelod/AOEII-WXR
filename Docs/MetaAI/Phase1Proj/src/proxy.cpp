#include <windows.h>
#include <ddraw.h>
#include "hooks.h"
#include "logger.h"
HMODULE g_hReal=NULL; typedef HRESULT (WINAPI *DDC_t)(GUID*,LPDIRECTDRAW*,IUnknown*); typedef HRESULT (WINAPI *DDCEx_t)(GUID*,LPDIRECTDRAW7*,REFIID,IUnknown*); DDC_t Real_DDC=nullptr; DDCEx_t Real_DDCEx=nullptr;
static void LoadReal(){ if(g_hReal) return; const char* ps[]={"C:\\windows\\system32\\ddraw.dll","C:\\windows\\syswow64\\ddraw.dll"}; for(auto p:ps){ g_hReal=LoadLibraryA(p); if(g_hReal) break; } if(g_hReal){ Real_DDC=(DDC_t)GetProcAddress(g_hReal,"DirectDrawCreate"); Real_DDCEx=(DDCEx_t)GetProcAddress(g_hReal,"DirectDrawCreateEx"); } }
extern "C" HRESULT WINAPI DirectDrawCreate(GUID* g,LPDIRECTDRAW* dd,IUnknown* u){ LoadReal(); Logger::Init(); Logger::Log("DirectDrawCreate\n"); if(!Real_DDC) return DDERR_GENERIC; HRESULT hr=Real_DDC(g,dd,u); if(hr==DD_OK&&dd&&*dd){ LPDIRECTDRAW7 p7=nullptr; if((*dd)->QueryInterface(IID_IDirectDraw7,(void**)&p7)==DD_OK){ HookDirectDraw7(p7); p7->Release(); } } return hr; }
extern "C" HRESULT WINAPI DirectDrawCreateEx(GUID* g,LPDIRECTDRAW7* dd,REFIID iid,IUnknown* u){ LoadReal(); Logger::Init(); Logger::Log("DirectDrawCreateEx\n"); if(!Real_DDCEx) return DDERR_GENERIC; HRESULT hr=Real_DDCEx(g,dd,iid,u); if(hr==DD_OK&&dd&&*dd) HookDirectDraw7(*dd); return hr; }
extern "C" HRESULT WINAPI DirectDrawCreateClipper(DWORD f,LPDIRECTDRAWCLIPPER* c,IUnknown* u){ LoadReal(); auto fn=(HRESULT(WINAPI*)(DWORD,LPDIRECTDRAWCLIPPER*,IUnknown*))GetProcAddress(g_hReal,"DirectDrawCreateClipper"); return fn?fn(f,c,u):DDERR_GENERIC; }
extern "C" HRESULT WINAPI DirectDrawEnumerateA(LPDDENUMCALLBACKA cb,LPVOID ctx){ LoadReal(); auto fn=(HRESULT(WINAPI*)(LPDDENUMCALLBACKA,LPVOID))GetProcAddress(g_hReal,"DirectDrawEnumerateA"); return fn?fn(cb,ctx):DDERR_GENERIC; }
extern "C" HRESULT WINAPI DirectDrawEnumerateW(LPDDENUMCALLBACKW cb,LPVOID ctx){ LoadReal(); auto fn=(HRESULT(WINAPI*)(LPDDENUMCALLBACKW,LPVOID))GetProcAddress(g_hReal,"DirectDrawEnumerateW"); return fn?fn(cb,ctx):DDERR_GENERIC; }
extern "C" HRESULT WINAPI DirectDrawEnumerateExA(LPDDENUMCALLBACKEXA cb,LPVOID ctx,DWORD fl){ LoadReal(); auto fn=(HRESULT(WINAPI*)(LPDDENUMCALLBACKEXA,LPVOID,DWORD))GetProcAddress(g_hReal,"DirectDrawEnumerateExA"); return fn?fn(cb,ctx,fl):DDERR_GENERIC; }
extern "C" HRESULT WINAPI DirectDrawEnumerateExW(LPDDENUMCALLBACKEXW cb,LPVOID ctx,DWORD fl){ LoadReal(); auto fn=(HRESULT(WINAPI*)(LPDDENUMCALLBACKEXW,LPVOID,DWORD))GetProcAddress(g_hReal,"DirectDrawEnumerateExW"); return fn?fn(cb,ctx,fl):DDERR_GENERIC; }
extern "C" HRESULT WINAPI DllGetClassObject(REFCLSID r,REFIID id,LPVOID* pp){ LoadReal(); auto fn=(HRESULT(WINAPI*)(REFCLSID,REFIID,LPVOID*))GetProcAddress(g_hReal,"DllGetClassObject"); return fn?fn(r,id,pp):DDERR_GENERIC; }
extern "C" HRESULT WINAPI DllCanUnloadNow(){ return S_FALSE; }
extern "C" HRESULT WINAPI GetSurfaceFromDC(HDC hdc,LPDIRECTDRAWSURFACE7* s){ LoadReal(); auto fn=(HRESULT(WINAPI*)(HDC,LPDIRECTDRAWSURFACE7*))GetProcAddress(g_hReal,"GetSurfaceFromDC"); return fn?fn(hdc,s):DDERR_GENERIC; }
