#include "log.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

namespace wxr {
namespace {

// Embedded so the owner can confirm from the device which binary is running.
// The deploy script refuses a binary older than the newest source file
// (Docs/general_wxr/TOOLS.md, "A build-and-deploy script").
const char* kBuildStamp = "built " __DATE__ " " __TIME__;

CRITICAL_SECTION g_lock;
HANDLE g_file = INVALID_HANDLE_VALUE;
bool g_ready = false;
bool g_verbose = false;
char g_dir[MAX_PATH] = {0};
DWORD g_t0 = 0;

void WriteRaw(const char* s) {
  if (g_file == INVALID_HANDLE_VALUE) return;
  DWORD written = 0;
  WriteFile(g_file, s, (DWORD)strlen(s), &written, NULL);
  FlushFileBuffers(g_file);  // a crash must not eat the last line
}

void Emit(const char* fmt, va_list ap) {
  char body[2048];
  _vsnprintf_s(body, sizeof(body), _TRUNCATE, fmt, ap);

  char line[2176];
  _snprintf_s(line, sizeof(line), _TRUNCATE, "[%8.3f] %s\r\n",
              (GetTickCount() - g_t0) / 1000.0, body);

  EnterCriticalSection(&g_lock);
  WriteRaw(line);
  LeaveCriticalSection(&g_lock);
}

}  // namespace

const char* ModuleDir() { return g_dir; }
bool Verbose() { return g_verbose; }

void LogInit(HMODULE self) {
  if (g_ready) return;
  InitializeCriticalSection(&g_lock);
  g_t0 = GetTickCount();

  // Next to our own DLL: always writable, always findable by the owner, and
  // it does not depend on where the container maps C:. BUG_FIXES #8.
  GetModuleFileNameA(self, g_dir, MAX_PATH);
  char* slash = strrchr(g_dir, '\\');
  if (slash) *slash = '\0';

  char path[MAX_PATH];
  _snprintf_s(path, sizeof(path), _TRUNCATE, "%s\\aoe2wxr.log", g_dir);
  g_file = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ, NULL,
                       CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

  char flag[MAX_PATH];
  _snprintf_s(flag, sizeof(flag), _TRUNCATE, "%s\\aoe2wxr_verbose.txt", g_dir);
  g_verbose = GetFileAttributesA(flag) != INVALID_FILE_ATTRIBUTES;

  g_ready = true;

  // Line 1 is the build stamp, unconditionally.
  char first[512];
  _snprintf_s(first, sizeof(first), _TRUNCATE,
              "AOEII-WXR d3d9 %s  pid=%lu  verbose=%d\r\n",
              kBuildStamp, GetCurrentProcessId(), g_verbose ? 1 : 0);
  WriteRaw(first);
}

void LogShutdown() {
  if (!g_ready) return;
  Log("shutdown");
  EnterCriticalSection(&g_lock);
  if (g_file != INVALID_HANDLE_VALUE) {
    CloseHandle(g_file);
    g_file = INVALID_HANDLE_VALUE;
  }
  LeaveCriticalSection(&g_lock);
  g_ready = false;
  DeleteCriticalSection(&g_lock);
}

void Log(const char* fmt, ...) {
  if (!g_ready) return;
  va_list ap;
  va_start(ap, fmt);
  Emit(fmt, ap);
  va_end(ap);
}

void LogV(const char* fmt, ...) {
  if (!g_ready || !g_verbose) return;
  va_list ap;
  va_start(ap, fmt);
  Emit(fmt, ap);
  va_end(ap);
}

}  // namespace wxr
