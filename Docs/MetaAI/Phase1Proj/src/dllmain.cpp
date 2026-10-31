#include <windows.h>
#include "logger.h"
#include "d3d11_renderer.h"
D3D11Renderer g_renderer;
BOOL APIENTRY DllMain(HMODULE h,DWORD r,LPVOID res){ switch(r){ case DLL_PROCESS_ATTACH: DisableThreadLibraryCalls(h); Logger::Init(); Logger::Log("ddraw proxy attached %p pid=%d\n",h,GetCurrentProcessId()); break; case DLL_PROCESS_DETACH: Logger::Log("detached\n"); Logger::Shutdown(); g_renderer.Shutdown(); break; } return TRUE; }
