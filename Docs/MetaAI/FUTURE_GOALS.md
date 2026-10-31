# FUTURE_GOALS.md — Not Yet Started

## Phase 2 Core
- [ ] Read scroll/camera from memory (search 0x0059xxxx for m_scrollX/Y in 1.0c)
- [ ] Classify sprites: terrain vs unit vs building vs UI vs cursor by size/frequency
- [ ] Terrain mesh builder: stitch tiles into XZ mesh with optional heightmap
- [ ] Cylindrical billboard shader: vertical quads facing camera, up=(0,1,0)
- [ ] Depth sorting by world Y + Z

## Phase 3 VR Polish
- [ ] VR tabletop anchor grabbable/resizable with OpenXR hand rays
- [ ] Hand laser selection, pinch to move camera
- [ ] Directional light + blob shadows
- [ ] Settings INI C:\aoe2_3d_config.ini

## Compatibility
- [ ] HD Edition d3d9.dll proxy
- [ ] UserPatch 1.5 / WololoKingdoms
- [ ] Multiplayer render-only no desync

## Performance
- [ ] Instanced sprite batching, atlas packing, palette conversion cache, DXVK friendly formats

## QoL
- [ ] Auto-detect Wine vs Windows log path, ImGui overlay toggle 2D/3D
