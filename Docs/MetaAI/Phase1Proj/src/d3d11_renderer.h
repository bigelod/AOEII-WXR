#pragma once
#include <windows.h>
#include <ddraw.h>
#include <d3d11.h>
class D3D11Renderer{public: bool Init(HWND); void Shutdown(); bool IsInitialized() const {return init;} void RenderFromLockedSurface(const DDSURFACEDESC2&); void Present(); private: bool CreateShaders(); bool CreateQuad(); bool init=false; HWND hwnd=nullptr; ID3D11Device* dev=nullptr; ID3D11DeviceContext* ctx=nullptr; IDXGISwapChain* sw=nullptr; ID3D11RenderTargetView* rtv=nullptr; ID3D11Texture2D* tex=nullptr; ID3D11ShaderResourceView* srv=nullptr; ID3D11Buffer* vb=nullptr; ID3D11Buffer* ib=nullptr; ID3D11VertexShader* vs=nullptr; ID3D11PixelShader* ps=nullptr; ID3D11InputLayout* lay=nullptr; ID3D11Buffer* cb=nullptr; ID3D11SamplerState* samp=nullptr; int tw=0,th=0;};
