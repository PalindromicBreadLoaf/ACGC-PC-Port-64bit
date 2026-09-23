#include <cstddef>

#include "JSystem/JKernel/JKRArchive.h"
#include "pc_rarc.h"

static_assert(sizeof(JKRArchive::SArcHeader) == 32);
static_assert(sizeof(JKRArchive::SArcDataInfo) == 32);
static_assert(sizeof(JKRArchive::SDIDirEntry) == 16);
static_assert(offsetof(JKRArchive::SDIFileEntry, mData) == 16);
static_assert(sizeof(pc_rarc_file_disk) == 20);
#if UINTPTR_MAX > UINT32_MAX
static_assert(sizeof(JKRArchive::SDIFileEntry) > sizeof(pc_rarc_file_disk));
#else
static_assert(sizeof(JKRArchive::SDIFileEntry) == sizeof(pc_rarc_file_disk));
#endif
static_assert(sizeof(JKRArchive::EMountMode) == 4);
static_assert(sizeof(JKRArchive::EMountDirection) == 4);
