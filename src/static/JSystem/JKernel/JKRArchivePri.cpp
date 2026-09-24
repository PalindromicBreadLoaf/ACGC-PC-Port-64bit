#ifdef TARGET_PC
#include <ctype.h>
#else
#include <MSL_C/ctype.h>
#endif
#include <string.h>

#include "JSystem/JKernel/JKRArchive.h"
#include "JSystem/JKernel/JKRFileLoader.h"
#include "types.h"

#ifdef TARGET_PC
#include <limits.h>
#include "pc_rarc.h"
#endif

u32 JKRArchive::sCurrentDirID;

JKRArchive::JKRArchive() {
    mIsMounted = false;
    mMountDirection = MOUNT_DIRECTION_HEAD;
#ifdef TARGET_PC
    mMountAddress = nullptr;
#endif
}

JKRArchive::JKRArchive(s32 entryNum, JKRArchive::EMountMode mountMode) : JKRFileLoader() {
    mIsMounted = false;
    mMountMode = mountMode;
    mMountCount = 1;
    _54 = 1;
    mHeap = JKRHeap::findFromRoot(this);
    if (!mHeap) {
        mHeap = JKRHeap::sCurrentHeap;
    }
    mEntryNum = entryNum;
#ifdef TARGET_PC
    mMountAddress = nullptr;
#endif
    if (sCurrentVolume == nullptr) {
        sCurrentDirID = 0;
        sCurrentVolume = this;
    }
}

#ifdef TARGET_PC
JKRArchive::JKRArchive(void* address, JKRArchive::EMountMode mountMode) : JKRFileLoader() {
    mIsMounted = false;
    mMountMode = mountMode;
    mMountCount = 1;
    _54 = 1;
    mHeap = JKRHeap::findFromRoot(address);
    if (!mHeap) {
        mHeap = JKRHeap::sCurrentHeap;
    }
    mEntryNum = -1;
    mMountAddress = address;
    mMountDirection = MOUNT_DIRECTION_HEAD;
    if (sCurrentVolume == nullptr) {
        sCurrentDirID = 0;
        sCurrentVolume = this;
    }
}

static bool pcArchiveAddSize(size_t* total, size_t count, size_t itemSize) {
    if (itemSize != 0 && count > (SIZE_MAX - *total) / itemSize) {
        return false;
    }
    *total += count * itemSize;
    return true;
}

static bool pcArchiveAlignSize(size_t* value, size_t alignment) {
    size_t mask = alignment - 1;
    if (*value > SIZE_MAX - mask) {
        return false;
    }
    *value = (*value + mask) & ~mask;
    return true;
}

bool JKRArchive::setPcArchiveMetadata(const void* data, size_t size, bool requireFileData) {
    pc_rarc_view view;
    size_t directoryOffset = sizeof(SArcDataInfo);
    size_t fileOffset;
    size_t stringOffset;
    size_t total;

    if ((requireFileData ? pc_rarc_open(&view, data, size) : pc_rarc_open_metadata(&view, data, size)) == false ||
        !pcArchiveAddSize(&directoryOffset, view.info.node_count, sizeof(SDIDirEntry))) {
        return false;
    }
    fileOffset = directoryOffset;
    if (!pcArchiveAlignSize(&fileOffset, alignof(SDIFileEntry))) {
        return false;
    }
    stringOffset = fileOffset;
    if (!pcArchiveAddSize(&stringOffset, view.info.file_count, sizeof(SDIFileEntry))) {
        return false;
    }
    total = stringOffset;
    if (!pcArchiveAddSize(&total, view.info.string_table_length, 1) || total > UINT32_MAX) {
        return false;
    }

    SArcDataInfo* info = (SArcDataInfo*)JKRAllocFromHeap(
        mHeap, (u32)total, mMountDirection == MOUNT_DIRECTION_TAIL ? -32 : 32);
    if (info == nullptr) {
        return false;
    }
    memset(info, 0, total);
    info->num_nodes = view.info.node_count;
    info->node_offset = (u32)directoryOffset;
    info->num_file_entries = view.info.file_count;
    info->file_entry_offset = (u32)fileOffset;
    info->string_table_length = view.info.string_table_length;
    info->string_table_offset = (u32)stringOffset;
    info->nextFreeFileID = view.info.next_free_file_id;
    info->isSyncIDs = view.info.sync_file_ids;

    SDIDirEntry* directories = (SDIDirEntry*)((u8*)info + directoryOffset);
    SDIFileEntry* files = (SDIFileEntry*)((u8*)info + fileOffset);
    for (u32 i = 0; i < view.info.node_count; ++i) {
        pc_rarc_directory source;
        if (!pc_rarc_get_directory(&view, i, &source)) {
            JKRFreeToHeap(mHeap, info);
            return false;
        }
        directories[i].mType = source.type;
        directories[i].mOffset = source.name_offset;
        directories[i]._08 = source.name_hash;
        directories[i].mNum = source.file_count;
        directories[i].mFirstIdx = source.first_file_index;
    }
    for (u32 i = 0; i < view.info.file_count; ++i) {
        pc_rarc_file source;
        if (!pc_rarc_get_file(&view, i, &source)) {
            JKRFreeToHeap(mHeap, info);
            return false;
        }
        files[i].mFileID = source.file_id;
        files[i].mHash = source.name_hash;
        files[i].mFlag = source.flags_and_name_offset;
        files[i].mDataOffset = source.data_offset;
        files[i].mSize = source.data_length;
        files[i].mData = nullptr;
    }
    memcpy((u8*)info + stringOffset, pc_rarc_get_string_table(&view), view.info.string_table_length);

    mArcInfoBlock = info;
    mDirectories = directories;
    mFileEntries = files;
    mStrTable = (const char*)((u8*)info + stringOffset);
    return true;
}

void JKRArchive::clearPcArchiveMetadata() {
    if (mArcInfoBlock != nullptr) {
        JKRFreeToHeap(mHeap, mArcInfoBlock);
    }
    mArcInfoBlock = nullptr;
    mDirectories = nullptr;
    mFileEntries = nullptr;
    mStrTable = nullptr;
}
#endif

JKRArchive::~JKRArchive() {
}

bool JKRArchive::isSameName(JKRArchive::CArcName& archiveName, u32 nameTableOffset, u16 hash) const {
    u16 arcHash = archiveName.getHash();
    if (arcHash != hash)
        return false;

    return strcmp(&mStrTable[nameTableOffset], archiveName.getString()) == 0;
}

JKRArchive::SDIDirEntry* JKRArchive::findResType(u32 type) const {
    SDIDirEntry* dirEntry = mDirectories;
    for (u32 i = 0; i < mArcInfoBlock->num_nodes; i++, dirEntry++) {
        if (dirEntry->mType == type) {
            return dirEntry;
        }
    }
    return nullptr;
}

JKRArchive::SDIDirEntry* JKRArchive::findDirectory(const char* path, u32 index) const {
    if (path == nullptr) {
        return &mDirectories[index];
    }

    CArcName arcName(&path, '/');
    SDIDirEntry* dirEntry = &mDirectories[index];
    SDIFileEntry* entry = &mFileEntries[dirEntry->mFirstIdx];

    for (int i = 0; i < dirEntry->mNum; entry++, i++) {
        if (isSameName(arcName, entry->mFlag & 0xFFFFFF, entry->mHash)) {
            if ((entry->mFlag >> 24) & 0x02) {
                return findDirectory(path, entry->mDataOffset);
            }
            break;
        }
    }

    return nullptr;
}

JKRArchive::SDIFileEntry* JKRArchive::findTypeResource(u32 type, const char* name) const {
    if (type != 0) {
        CArcName arcName;
        arcName.store(name);
        SDIDirEntry* dirEntry = findResType(type);
        if (dirEntry != nullptr) {
            SDIFileEntry* fileEntry = mFileEntries + dirEntry->mFirstIdx;
            for (int i = 0; i < dirEntry->mNum; fileEntry++, i++) {
                if (isSameName(arcName, fileEntry->mFlag & 0xFFFFFF, fileEntry->mHash)) {
                    return fileEntry;
                }
            }
        }
    }
    return nullptr;
}

JKRArchive::SDIFileEntry* JKRArchive::findFsResource(const char* path, u32 index) const {
    if (path) {
        CArcName arcName(&path, '/');
        SDIDirEntry* dirEntry = &mDirectories[index];
        SDIFileEntry* entry = &mFileEntries[dirEntry->mFirstIdx];
        for (int i = 0; i < dirEntry->mNum; entry++, i++) {
            if (isSameName(arcName, entry->mFlag & 0xFFFFFF, entry->mHash)) {
                if (((entry->mFlag >> 0x18) & 2)) {
                    return findFsResource(path, entry->mDataOffset);
                }
                if (path == 0) {
                    return entry;
                }
                return nullptr;
            }
        }
    }
    return nullptr;
}

JKRArchive::SDIFileEntry* JKRArchive::findIdxResource(u32 idx) const {
    if (idx < mArcInfoBlock->num_file_entries) {
        return mFileEntries + idx;
    }
    return nullptr;
}

JKRArchive::SDIFileEntry* JKRArchive::findNameResource(const char* name) const {
    SDIFileEntry* fileEntry = mFileEntries;

    CArcName arcName(name);
    for (int i = 0; i < mArcInfoBlock->num_file_entries; fileEntry++, i++) {
        if (isSameName(arcName, fileEntry->mFlag & 0xFFFFFF, fileEntry->mHash)) {
            return fileEntry;
        }
    }

    return nullptr;
}

JKRArchive::SDIFileEntry* JKRArchive::findPtrResource(const void* ptr) const {
    SDIFileEntry* entry = mFileEntries;
    for (u32 i = 0; i < mArcInfoBlock->num_file_entries; entry++, i++) {
        if (entry->mData == ptr) {
            return entry;
        }
    }
    return nullptr;
}

JKRArchive::SDIFileEntry* JKRArchive::findIdResource(u16 id) const {
    SDIFileEntry* entry;
    if (id != 0xFFFF) {
        entry = &mFileEntries[id];
        if (entry->mFileID == id && (entry->getFlag01())) {
            return entry;
        }

        entry = mFileEntries;
        for (int i = 0; i < mArcInfoBlock->num_file_entries; entry++, i++) {
            if (entry->mFileID == id && (entry->getFlag01())) {
                return entry;
            }
        }
    }
    return nullptr;
}

void JKRArchive::CArcName::store(const char* name) {
    mHash = 0;
    int count = 0;
    while (*name) {
        int lower = tolower(*name);
        mHash = lower + mHash * 3;
        if (count < 0x100) {
            mString[count++] = lower;
        }
        name++;
    }
    _02 = count;
    mString[count] = '\0';
}

const char* JKRArchive::CArcName::store(const char* name, char endChar) {
    mHash = 0;
    int count = 0;
    for (; *name && *name != endChar; name++) {
        int lower = tolower(*name);
        mHash = lower + mHash * 3;
        if (count < 0x100) {
            mString[count++] = lower;
        }
    }
    _02 = count;
    mString[count] = '\0';

    if (*name == 0)
        return nullptr;
    return name + 1;
}
