# PROJECT_PLAN.md — AoE2 Paper Mario 3D Mod for WinlatorXR

## Goal
Take classic Age of Empires 2 (1.0c and HD, 32-bit DirectDraw7) running under WinlatorXR (Wine + DXVK + Vulkan + OpenXR on Quest) and render it as true 3D isometric where map is 3D mesh and sprites (villagers, buildings, trees) stand vertically like Paper Mario — cylindrical billboards.

We do NOT want a Godot/openage remake. We want a drop-in DLL hooking original empires2.exe.

## Why This Approach
- Openage is engine rewrite — breaks campaigns/mods.
- AoE2 classic draws via IDirectDrawSurface7::BltFast and Flip.
- Intercepting those gives us every sprite dest x/y + srcRect.
- WinlatorXR chain: empires2.exe -> Wine ddraw -> our ddraw.dll -> D3D11 -> DXVK -> Vulkan -> OpenXR. D3D11 emitted shows up in VR.
- Proven pattern: DDrawCompat, dgVoodoo2, CnC-DDraw. OpenTSA VR video does same but with open-source engine rendering to texture then tabletop quad.

## Architecture
empires2.exe -> Wine ddraw -> proxy ddraw.dll (loads real system32 ddraw, hooks CreateSurface, BltFast/Blt/Flip via vtable) -> D3D11Renderer (Phase1: textured quad tabletop, Phase2: vertical billboards) -> DXVK -> Vulkan -> OpenXR

## Build
VS2022 v143, Win32/x86 only (AoE2 classic is 32-bit), link d3d11/dxgi/d3dcompiler, output ddraw.dll via proxy.def, deploy next to exe with WINEDLLOVERRIDES=ddraw=n,b

## Phases
Phase1: Hook + log every BltFast + Flip to C:\aoe2_phase1_log.txt, try D3D11 swapchain on HWND rendering backbuffer as quad. Fallback to original Flip if fails.
Phase2: Queue BltFast, read scroll/camera from memory, convert screen to world, terrain as flat XZ mesh, units/buildings as vertical Y-billboards facing camera.
Phase3: Depth sort, lighting, VR hand interaction.

## Reference
DDrawCompat, dgVoodoo2, OpenTSA VR (Reddit), WinlatorXR
