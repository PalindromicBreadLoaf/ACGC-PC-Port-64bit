#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "dolphin/ar.h"

#define CHECK(condition)                                                                                               \
    do {                                                                                                               \
        if (!(condition)) {                                                                                            \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, #condition);                              \
            return 1;                                                                                                  \
        }                                                                                                              \
    } while (0)

static uintptr_t callback_request;

static void transfer_done(uintptr_t request) {
    callback_request = request;
}

int main(void) {
    static const u8 source[] = { 1, 2, 3, 4, 5, 6, 7, 8 };
    u8 destination[sizeof(source)] = { 0 };
    u8 unchanged[sizeof(source)];
    ARQRequest request;
    aram_addr_t end = ARGetSize() - (aram_addr_t)sizeof(source);

    CHECK(ARInit(NULL, 0) == 0);
    CHECK(pc_aram_get_base() != NULL);
    CHECK(pc_aram_transfer(ARAM_DIR_MRAM_TO_ARAM, (void*)source, end, sizeof(source)));
    CHECK(pc_aram_transfer(ARAM_DIR_ARAM_TO_MRAM, destination, end, sizeof(destination)));
    CHECK(memcmp(source, destination, sizeof(source)) == 0);

    memcpy(unchanged, destination, sizeof(unchanged));
    CHECK(!pc_aram_transfer(ARAM_DIR_ARAM_TO_MRAM, destination, end + 1u, sizeof(destination)));
    CHECK(memcmp(unchanged, destination, sizeof(destination)) == 0);
    CHECK(!pc_aram_transfer(ARAM_DIR_MRAM_TO_ARAM, NULL, 0, 1));
    CHECK(pc_aram_transfer(ARAM_DIR_MRAM_TO_ARAM, NULL, ARGetSize(), 0));
    CHECK(!pc_aram_transfer(99, destination, 0, sizeof(destination)));

    memcpy(pc_aram_get_base(), source, sizeof(source));
    CHECK(pc_aram_transfer(ARAM_DIR_MRAM_TO_ARAM, pc_aram_get_base(), 2, sizeof(source)));
    CHECK(memcmp(pc_aram_get_base() + 2, source, sizeof(source)) == 0);

    callback_request = 0;
    memset(destination, 0, sizeof(destination));
    ARQPostRequest(&request, (ARQOwner)(uintptr_t)&destination, ARQ_TYPE_ARAM_TO_MRAM, ARQ_PRIORITY_LOW,
                   destination, 2, sizeof(destination), transfer_done);
    CHECK(callback_request == (uintptr_t)&request);
    CHECK(request.owner == (ARQOwner)(uintptr_t)&destination);
    CHECK(request.mramAddress == destination);
    CHECK(request.aramAddress == 2);
    CHECK(memcmp(destination, source, sizeof(source)) == 0);
    return 0;
}
