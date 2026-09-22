#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "dolphin/os.h"
#include "libultra/osThread.h"

u32 __OSBusClock = 162000000;

void osCreateMesgQueue(OSMessageQueue* queue, OSMessage message, int count) {
    (void)message;
    queue->usedCount = 0;
    queue->msgCount = count;
}

int osSendMesg(OSMessageQueue* queue, OSMessage message, int flag) {
    (void)message;
    (void)flag;
    queue->usedCount = 1;
    return 0;
}

int osRecvMesg(OSMessageQueue* queue, OSMessage* message, int flag) {
    (void)message;
    (void)flag;
    queue->usedCount = 0;
    return 0;
}

OSId osGetThreadId(OSThread* thread) {
    (void)thread;
    return 1;
}

OSTime osGetTime(void) {
    return 0;
}

void OSReport(const char* format, ...) {
    (void)format;
}

void OSPanic(const char* file, int line, const char* message, ...) {
    fprintf(stderr, "%s:%d: %s\n", file, line, message);
    abort();
}
