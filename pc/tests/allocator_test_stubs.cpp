#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "dolphin/os.h"
#include "JSystem/JUtility/JUTConsole.h"

extern "C" void OSInitMutex(OSMutex* mutex) {
    std::memset(mutex, 0, sizeof(*mutex));
}

extern "C" void OSLockMutex(OSMutex*) {
}

extern "C" void OSUnlockMutex(OSMutex*) {
}

extern "C" void OSPanic(const char* file, int line, const char* message, ...) {
    std::fprintf(stderr, "%s:%d: %s\n", file, line, message);
    std::abort();
}

extern "C" void JUTReportConsole(const char*) {
}

extern "C" void JUTReportConsole_f(const char*, ...) {
}

extern "C" void JUTWarningConsole(const char*) {
}

extern "C" void JUTWarningConsole_f(const char*, ...) {
}
