# BUG_FIXES.md — Bugs Fixed (short bullets)

- Duplicate _1, _2 files in zip from auto-backup when file exists — cleaned project dir, ensure single canonical files
- Missing VS2022 project — added sln/vcxproj/filters with TargetName=ddraw Win32 v143
- x64 vs Win32 mismatch — AoE2 1.0c is 32-bit, x64 dll fails silent, forced Win32 and documented
- No proxy.def exports — added src/proxy.def with EXPORTS and ModuleDefinitionFile in vcxproj
- VTable index assumption may differ in Wine — added logging of vtable ptr and non-crash fallback
- 8-bit palettized black screen — noted TODO palette conversion, logs bpp
- D3D11 own swapchain conflict — block original Flip after init, fallback if init fails
- D3DCompiler missing — log compile failure gracefully
- Log path invisible — C:\ maps to drive_c in Winlator, added Z:\tmp fallback and documented location
- WINEDLLOVERRIDES not set — added to README and Wine detection via ntdll wine_get_version
- Mutex deadlock logger Init->Log — fixed with initialized flag
- MinGW link stdcall fixup — added --enable-stdcall-fixup in CMake for non-MSVC
- Reddit domain blocked — worked around by analyzing uploaded video instead of browsing
