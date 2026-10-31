#pragma once
#include <windows.h>
class Logger{public: static void Init(); static void Shutdown(); static void Log(const char* fmt,...); static void LogBltFast(int x,int y,RECT r,int w,int h); static void LogFlip(int c); static void LogSurfaceCreate(int w,int h,int bpp);};
