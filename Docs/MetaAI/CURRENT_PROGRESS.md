# CURRENT_PROGRESS.md — In-Progress

## Have Now (Phase1 skeleton)
- Proxy ddraw.dll: dllmain, proxy.cpp loads real system32 ddraw, hooks IDirectDraw7::CreateSurface and IDirectDrawSurface7 BltFast/Blt/Flip via vtable
- logger.cpp logs to C:\aoe2_phase1_log.txt with Wine detection
- d3d11_renderer tries D3D11 device+swapchain on HWND at Flip #10, renders backbuffer as quad, fallback to original Flip
- Build: CMakeLists.txt + VS2022 sln/vcxproj Win32, TargetName=ddraw, links d3d11/dxgi/d3dcompiler, proxy.def

## Partially Working
- VTable indices assumed from SDK (Blt=5, BltFast=7, Flip=11, CreateSurface=6) — needs log verification, may need IDirectDrawSurface4 hook
- 8-bit palette handling naive memcpy — logs bpp to decide fix
- D3D11 swapchain may conflict with Wine swapchain — fallback implemented
- D3DCompile may fail if d3dcompiler_47.dll missing

## Waiting On
- User deploy in WinlatorXR + provide drive_c/aoe2_phase1_log.txt to get actual formats and BltFast rate

## Next
- Fix palette RGBA conversion, confirm vtable, build Phase2 billboard DLL
