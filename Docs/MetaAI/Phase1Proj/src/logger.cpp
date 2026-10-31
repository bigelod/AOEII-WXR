#include "logger.h"
#include <cstdio>
#include <mutex>
static std::mutex mtx; static HANDLE hFile=INVALID_HANDLE_VALUE; static bool init=false; static int flip=0, blt=0;
void Logger::Init(){ if(init) return; hFile=CreateFileW(L"C:\\aoe2_phase1_log.txt",GENERIC_WRITE,FILE_SHARE_READ,NULL,CREATE_ALWAYS,0,NULL); init=true; Log("=== AoE2 Phase1 Log Started ===\n"); }
void Logger::Shutdown(){ std::lock_guard<std::mutex> l(mtx); if(hFile!=INVALID_HANDLE_VALUE) CloseHandle(hFile); }
void Logger::Log(const char* fmt,...){ if(!init) Init(); std::lock_guard<std::mutex> l(mtx); if(hFile==INVALID_HANDLE_VALUE) return; char buf[2048]; va_list ap; va_start(ap,fmt); vsnprintf(buf,sizeof(buf),fmt,ap); va_end(ap); DWORD w; WriteFile(hFile,buf,(DWORD)strlen(buf),&w,NULL); }
void Logger::LogBltFast(int x,int y,RECT r,int w,int h){ blt++; if(blt<100||blt%500==0) Log("[BltFast #%d] dest=%d,%d src=%d,%d-%d,%d surf=%dx%d\n",blt,x,y,r.left,r.top,r.right,r.bottom,w,h); }
void Logger::LogFlip(int c){ flip=c; if(c%30==0) Log("[Flip #%d]\n",c); }
void Logger::LogSurfaceCreate(int w,int h,int bpp){ Log("[CreateSurface] %dx%d bpp=%d\n",w,h,bpp); }
