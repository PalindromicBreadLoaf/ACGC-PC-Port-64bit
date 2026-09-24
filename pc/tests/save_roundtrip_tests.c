#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dolphin/card.h"
#include "m_card.h"
#include "pc_save_bswap.h"

#define GCI_HEADER_SIZE sizeof(CARDDir)
#define GCI_DATA_SIZE mCD_LAND_SAVE_SIZE
#define GCI_MAIN_OFFSET OTHERS_SIZE
#define GCI_BACKUP_OFFSET (OTHERS_SIZE + sizeof(Save))
#define GOLDEN_GCI_HASH UINT64_C(0xC31E387656B5C5D4)
#define GOLDEN_DECODED_HASH UINT64_C(0x6008B1D65CA54F12)

static uint32_t fixture_state;

void OSReport(const char* fmt, ...) {
    (void)fmt;
}

static void fixture_fill(void* data, size_t size, uint32_t seed) {
    uint8_t* bytes = data;
    size_t i;

    fixture_state = seed;
    for (i = 0; i < size; i++) {
        fixture_state ^= fixture_state << 13;
        fixture_state ^= fixture_state >> 17;
        fixture_state ^= fixture_state << 5;
        bytes[i] = (uint8_t)fixture_state;
    }
}

static uint64_t fixture_hash(const void* data, size_t size) {
    const uint8_t* bytes = data;
    uint64_t hash = UINT64_C(14695981039346656037);
    size_t i;

    for (i = 0; i < size; i++) {
        hash ^= bytes[i];
        hash *= UINT64_C(1099511628211);
    }
    return hash;
}

static void put_be16(uint8_t* dst, uint16_t value) {
    dst[0] = (uint8_t)(value >> 8);
    dst[1] = (uint8_t)value;
}

static void put_be32(uint8_t* dst, uint32_t value) {
    dst[0] = (uint8_t)(value >> 24);
    dst[1] = (uint8_t)(value >> 16);
    dst[2] = (uint8_t)(value >> 8);
    dst[3] = (uint8_t)value;
}

static void set_checksum(uint8_t* data, size_t size, size_t checksum_offset) {
    uint16_t checksum;

    data[checksum_offset] = 0;
    data[checksum_offset + 1] = 0;
    checksum = pc_checksum_be(data, (u32)size, 0);
    put_be16(data + checksum_offset, checksum);
}

static int fail_hash(const char* label, uint64_t expected, uint64_t actual) {
    fprintf(stderr, "%s hash mismatch: expected %016llx, got %016llx\n", label,
            (unsigned long long)expected, (unsigned long long)actual);
    return 1;
}

static int checksum_valid(const uint8_t* data, size_t size, size_t checksum_offset) {
    uint16_t checksum = (uint16_t)(((uint16_t)data[checksum_offset] << 8) |
                                   data[checksum_offset + 1]);
    return pc_checksum_be(data, (u32)size, checksum) == checksum;
}

int main(void) {
    const size_t block_start = ALIGN_NEXT(sizeof(MemcardHeader_c) + 32, 32);
    const size_t mail_offset = block_start;
    const size_t original_offset = ALIGN_NEXT(mail_offset + mCD_KEEP_MAIL_SIZE, 32);
    const size_t diary_offset = ALIGN_NEXT(original_offset + mCD_KEEP_ORIGINAL_SIZE, 32);
    const size_t gci_size = GCI_HEADER_SIZE + GCI_DATA_SIZE;
    uint8_t* golden = malloc(gci_size);
    uint8_t* roundtrip = malloc(gci_size);
    Save* host_main = malloc(sizeof(*host_main));
    Save* host_backup = malloc(sizeof(*host_backup));
    mCD_keep_mail_c* host_mail = malloc(sizeof(*host_mail));
    mCD_keep_original_c* host_original = malloc(sizeof(*host_original));
    mCD_keep_diary_c* host_diary = malloc(sizeof(*host_diary));
    uint8_t* data;
    uint8_t* decoded_data;
    uint64_t golden_hash;
    uint64_t decoded_hash;
    int result = 1;

    if (golden == NULL || roundtrip == NULL || host_main == NULL || host_backup == NULL ||
        host_mail == NULL || host_original == NULL || host_diary == NULL) {
        goto done;
    }
    if (diary_offset + mCD_KEEP_DIARY_SIZE > OTHERS_SIZE ||
        GCI_BACKUP_OFFSET + sizeof(Save) != GCI_DATA_SIZE) {
        goto done;
    }

    fixture_fill(golden, gci_size, UINT32_C(0x47434931));
    memcpy(golden, "GAFE01", 6);
    memset(golden + 8, 0, CARD_FILENAME_MAX);
    memcpy(golden + 8, "LP64_SYNTHETIC_GOLDEN", 21);
    put_be32(golden + offsetof(CARDDir, time), UINT32_C(0x315A4C32));
    put_be32(golden + offsetof(CARDDir, iconAddr), UINT32_C(0xFFFFFFFF));
    put_be16(golden + offsetof(CARDDir, length), (uint16_t)(GCI_DATA_SIZE / mCD_MEMCARD_SECTORSIZE));

    fixture_fill(host_main, sizeof(*host_main), UINT32_C(0x53415631));
    fixture_fill(host_backup, sizeof(*host_backup), UINT32_C(0x53415632));
    fixture_fill(host_mail, sizeof(*host_mail), UINT32_C(0x4D41494C));
    fixture_fill(host_original, sizeof(*host_original), UINT32_C(0x4F524947));
    fixture_fill(host_diary, sizeof(*host_diary), UINT32_C(0x44494152));

    data = golden + GCI_HEADER_SIZE;
    memcpy(data + mail_offset, host_mail, sizeof(*host_mail));
    pc_save_bswap_keep_mail((mCD_keep_mail_c*)(data + mail_offset), PC_BSWAP_TO_BE);
    set_checksum(data + mail_offset, mCD_KEEP_MAIL_SIZE, offsetof(mCD_keep_mail_c, checksum));

    memcpy(data + original_offset, host_original, sizeof(*host_original));
    pc_save_bswap_keep_original((mCD_keep_original_c*)(data + original_offset), PC_BSWAP_TO_BE);
    set_checksum(data + original_offset, mCD_KEEP_ORIGINAL_SIZE, offsetof(mCD_keep_original_c, checksum));

    memcpy(data + diary_offset, host_diary, sizeof(*host_diary));
    pc_save_bswap_keep_diary((mCD_keep_diary_c*)(data + diary_offset), PC_BSWAP_TO_BE);
    set_checksum(data + diary_offset, mCD_KEEP_DIARY_SIZE, offsetof(mCD_keep_diary_c, checksum));

    memcpy(data + GCI_MAIN_OFFSET, host_main, sizeof(*host_main));
    pc_save_bswap((Save_t*)(data + GCI_MAIN_OFFSET), PC_BSWAP_TO_BE);
    set_checksum(data + GCI_MAIN_OFFSET, sizeof(Save_t), offsetof(Save_t, save_check.checksum));

    memcpy(data + GCI_BACKUP_OFFSET, host_backup, sizeof(*host_backup));
    pc_save_bswap((Save_t*)(data + GCI_BACKUP_OFFSET), PC_BSWAP_TO_BE);
    set_checksum(data + GCI_BACKUP_OFFSET, sizeof(Save_t), offsetof(Save_t, save_check.checksum));

    if (!checksum_valid(data + mail_offset, mCD_KEEP_MAIL_SIZE, offsetof(mCD_keep_mail_c, checksum)) ||
        !checksum_valid(data + original_offset, mCD_KEEP_ORIGINAL_SIZE,
                        offsetof(mCD_keep_original_c, checksum)) ||
        !checksum_valid(data + diary_offset, mCD_KEEP_DIARY_SIZE, offsetof(mCD_keep_diary_c, checksum)) ||
        !checksum_valid(data + GCI_MAIN_OFFSET, sizeof(Save_t), offsetof(Save_t, save_check.checksum)) ||
        !checksum_valid(data + GCI_BACKUP_OFFSET, sizeof(Save_t), offsetof(Save_t, save_check.checksum))) {
        fprintf(stderr, "GCI checksum validation failed\n");
        goto done;
    }
    if (pc_save_bswap_verify_roundtrip_mail(data + mail_offset, mCD_KEEP_MAIL_SIZE) != 0 ||
        pc_save_bswap_verify_roundtrip_original(data + original_offset, mCD_KEEP_ORIGINAL_SIZE) != 0 ||
        pc_save_bswap_verify_roundtrip_diary(data + diary_offset, mCD_KEEP_DIARY_SIZE) != 0 ||
        pc_save_bswap_verify_roundtrip(data + GCI_MAIN_OFFSET, sizeof(Save_t)) != 0 ||
        pc_save_bswap_verify_roundtrip(data + GCI_BACKUP_OFFSET, sizeof(Save_t)) != 0) {
        fprintf(stderr, "section round-trip verification failed\n");
        goto done;
    }

    golden_hash = fixture_hash(golden, gci_size);
    if (golden_hash != GOLDEN_GCI_HASH) {
        result = fail_hash("golden GCI", GOLDEN_GCI_HASH, golden_hash);
        goto done;
    }

    memcpy(roundtrip, golden, gci_size);
    decoded_data = roundtrip + GCI_HEADER_SIZE;
    pc_save_bswap_keep_mail((mCD_keep_mail_c*)(decoded_data + mail_offset), PC_BSWAP_FROM_BE);
    pc_save_bswap_keep_original((mCD_keep_original_c*)(decoded_data + original_offset), PC_BSWAP_FROM_BE);
    pc_save_bswap_keep_diary((mCD_keep_diary_c*)(decoded_data + diary_offset), PC_BSWAP_FROM_BE);
    pc_save_bswap((Save_t*)(decoded_data + GCI_MAIN_OFFSET), PC_BSWAP_FROM_BE);
    pc_save_bswap((Save_t*)(decoded_data + GCI_BACKUP_OFFSET), PC_BSWAP_FROM_BE);

    decoded_hash = fixture_hash(roundtrip, gci_size);
    if (decoded_hash != GOLDEN_DECODED_HASH) {
        result = fail_hash("decoded GCI", GOLDEN_DECODED_HASH, decoded_hash);
        goto done;
    }

    pc_save_bswap_keep_mail((mCD_keep_mail_c*)(decoded_data + mail_offset), PC_BSWAP_TO_BE);
    pc_save_bswap_keep_original((mCD_keep_original_c*)(decoded_data + original_offset), PC_BSWAP_TO_BE);
    pc_save_bswap_keep_diary((mCD_keep_diary_c*)(decoded_data + diary_offset), PC_BSWAP_TO_BE);
    pc_save_bswap((Save_t*)(decoded_data + GCI_MAIN_OFFSET), PC_BSWAP_TO_BE);
    pc_save_bswap((Save_t*)(decoded_data + GCI_BACKUP_OFFSET), PC_BSWAP_TO_BE);

    if (memcmp(roundtrip, golden, gci_size) != 0) {
        fprintf(stderr, "GCI round-trip changed serialized bytes\n");
        goto done;
    }
    result = 0;

done:
    free(host_diary);
    free(host_original);
    free(host_mail);
    free(host_backup);
    free(host_main);
    free(roundtrip);
    free(golden);
    return result;
}
