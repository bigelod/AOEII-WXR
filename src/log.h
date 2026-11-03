// AOEII-WXR - logging.
//
// On a standalone headset there is no debugger and each iteration costs a
// deploy, so the log IS the debugger. Cheap, timestamped, build-stamped on
// line 1, and everything chatty behind one verbose flag that is off by
// default (Docs/general_wxr/CLAUDE.md, "How to work").
#pragma once

#include <windows.h>

namespace wxr {

// Initialise from DllMain only. Log() is a no-op before this runs - it never
// self-initialises, which is how the inherited logger raced (BUG_FIXES #7).
void LogInit(HMODULE self);
void LogShutdown();

void Log(const char* fmt, ...);

// Chatty diagnostics. Off unless the verbose flag file is present.
void LogV(const char* fmt, ...);
bool Verbose();

// Absolute path of the directory this DLL was loaded from.
const char* ModuleDir();

}  // namespace wxr
