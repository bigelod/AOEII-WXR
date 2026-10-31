#pragma once
#include <windows.h>
#include <ddraw.h>
void HookDirectDraw7(LPDIRECTDRAW7 pDD); void HookSurface7(LPDIRECTDRAWSURFACE7 pSurf); bool InstallVTableHook(void** vtable,int idx,void* nf,void** oldf);
