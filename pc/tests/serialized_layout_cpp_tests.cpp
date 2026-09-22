#include <cstddef>

#include "JSystem/JKernel/JKRArchive.h"

static_assert(sizeof(JKRArchive::SArcHeader) == 32);
static_assert(sizeof(JKRArchive::SArcDataInfo) == 32);
static_assert(sizeof(JKRArchive::SDIDirEntry) == 16);
static_assert(offsetof(JKRArchive::SDIFileEntry, mData) == 16);
static_assert(sizeof(JKRArchive::EMountMode) == 4);
static_assert(sizeof(JKRArchive::EMountDirection) == 4);
