#ifndef FAMICOM_H
#define FAMICOM_H

#include "types.h"
#include "terminal.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FAMICOM_SAVE_HEADER_SIZE 0x40
#define FAMICOM_INTERNAL_ROM_NUM 19

#define NESTAG_CMD_SIZE 3
#define NESTAG_SIZE (NESTAG_CMD_SIZE + 1)

#define NESTAG_END "END"
#define NESTAG_VEQ "VEQ"
#define NESTAG_VNE "VNE"
#define NESTAG_GID "GID"
#define NESTAG_GNM "GNM"
#define NESTAG_CPN "CPN"
#define NESTAG_OFS "OFS" /* Offset into the shared highscore data to read/write at */
#define NESTAG_HSC "HSC"
#define NESTAG_GNO "GNO"
#define NESTAG_BBR "BBR"
#define NESTAG_QDS "QDS"
#define NESTAG_SPE "SPE"
#define NESTAG_TCS "TCS"
#define NESTAG_ICS "ICS"
#define NESTAG_ESZ "ESZ"
#define NESTAG_ROM "ROM"
#define NESTAG_MOV "MOV"
#define NESTAG_NHD "NHD"
#define NESTAG_DIF "DIF"
#define NESTAG_PAT "PAT"
#define NESTAG_PAD "PAD"
#define NESTAG_FIL "FIL"
#define NESTAG_ISZ "ISZ"
#define NESTAG_IFM "IFM"
#define NESTAG_REM "REM"
#define NESTAG_APL "APL"
#define NESTAG_FGN "FGN"

typedef void* (*MALLOC_ALIGN_FUNC)(size_t size, u32 align);
typedef void (*MALLOC_FREE_FUNC)(void* ptr);
typedef int (*MALLOC_GETMEMBLOCKSIZE_FUNC)(void* ptr);
typedef int (*MALLOC_GETTOTALFREESIZE_FUNC)();

typedef struct malloc_s {
    MALLOC_ALIGN_FUNC malloc_align;
    MALLOC_FREE_FUNC free;
    MALLOC_GETMEMBLOCKSIZE_FUNC getmemblocksize;
    MALLOC_GETTOTALFREESIZE_FUNC gettotalfreesize;
} Famicom_MallocInfo;

enum filer_demo_mode {
    FILER_DEMO_MODE_NORMAL,
    FILER_DEMO_MODE_AUTO,

    FILER_DEMO_MODE_NUM
};

#define FAMICOM_SAVE_DATA_NAME_LEN 8
#define FAMICOM_MORI_NAME_LEN 16

typedef struct FamicomSaveDataHeader {
    u8 name[FAMICOM_SAVE_DATA_NAME_LEN];
    u8 _08;
    u8 _09;
    u8 headerSize;
    u8 checksum;
    u16 size;
    u8 no_save;
    u8 _temp[FAMICOM_SAVE_HEADER_SIZE - 0x000F];
} FamicomSaveDataHeader;

enum {
    MEMCARD_COMMENT_TYPE_NONE,
    MEMCARD_COMMENT_TYPE_DEFAULT,
    MEMCARD_COMMENT_TYPE_COPY_ROM,     // converts rom's comment but converts "] ROM" to "] SAVE"
    MEMCARD_COMMENT_TYPE_COPY_EMBEDDED // uses the embedded and unique 'save' comment
};

enum {
    MEMCARD_BANNER_TYPE_NONE,
    MEMCARD_BANNER_TYPE_DEFAULT,
    MEMCARD_BANNER_TYPE_COPY_ROM,     // copies the NES rom save's banner
    MEMCARD_BANNER_TYPE_COPY_EMBEDDED // uses the embedded and unique 'save' banner
};

enum {
    MEMCARD_ICON_TYPE_NONE,
    MEMCARD_ICON_TYPE_DEFAULT,
    MEMCARD_ICON_TYPE_COPY_ROM,     // copies the NES rom save's icon
    MEMCARD_ICON_TYPE_COPY_EMBEDDED // uses the embedded and unique 'save' icon
};

enum {
    FAMICOM_RESULT_OK,
    FAMICOM_RESULT_NOSPACE,
    FAMICOM_RESULT_NOENTRY,
    FAMICOM_RESULT_BROKEN,
    FAMICOM_RESULT_WRONGDEVICE,
    FAMICOM_RESULT_WRONGENCODING,
    FAMICOM_RESULT_NOCARD,
    FAMICOM_RESULT_NOFILE
};

typedef struct MemcardGameHeader_t {
    u8 _00;
    u8 _01;
    u8 mori_name[FAMICOM_MORI_NAME_LEN];
    u16 nesrom_size;
    u16 nestags_size;
    u16 icon_format;
    u16 icon_flags;
    u16 comment_img_size; /* Size of comment + banner + icon */
#ifdef TARGET_PC
    u8 flags0;
    u8 flags1;
#else
    struct {
        u8 has_comment_img : 1;
        u8 comment_type : 2;
        u8 banner_type : 2;
        u8 icon_type : 2;
        u8 no_copy_flag : 1;
    } flags0;
    struct {
        u8 no_move_flag : 1;
        u8 banner_fmt : 2;
        u8 reserved : 5;
    } flags1;
#endif
    u16 pad;
} MemcardGameHeader_t;

#define MEMCARD_GAME_FLAGS0_HAS_COMMENT_IMG 0x80
#define MEMCARD_GAME_FLAGS0_COMMENT_TYPE_MASK 0x60
#define MEMCARD_GAME_FLAGS0_COMMENT_TYPE_SHIFT 5
#define MEMCARD_GAME_FLAGS0_BANNER_TYPE_MASK 0x18
#define MEMCARD_GAME_FLAGS0_BANNER_TYPE_SHIFT 3
#define MEMCARD_GAME_FLAGS0_ICON_TYPE_MASK 0x06
#define MEMCARD_GAME_FLAGS0_ICON_TYPE_SHIFT 1
#define MEMCARD_GAME_FLAGS0_NO_COPY_FLAG 0x01
#define MEMCARD_GAME_FLAGS1_NO_MOVE_FLAG 0x80
#define MEMCARD_GAME_FLAGS1_BANNER_FMT_MASK 0x60
#define MEMCARD_GAME_FLAGS1_BANNER_FMT_SHIFT 5

static inline u8 Famicom_GetCommentType(const MemcardGameHeader_t* header) {
#ifdef TARGET_PC
    return (u8)((header->flags0 & MEMCARD_GAME_FLAGS0_COMMENT_TYPE_MASK) >>
                MEMCARD_GAME_FLAGS0_COMMENT_TYPE_SHIFT);
#else
    return header->flags0.comment_type;
#endif
}

static inline u8 Famicom_GetBannerType(const MemcardGameHeader_t* header) {
#ifdef TARGET_PC
    return (u8)((header->flags0 & MEMCARD_GAME_FLAGS0_BANNER_TYPE_MASK) >> MEMCARD_GAME_FLAGS0_BANNER_TYPE_SHIFT);
#else
    return header->flags0.banner_type;
#endif
}

static inline u8 Famicom_GetIconType(const MemcardGameHeader_t* header) {
#ifdef TARGET_PC
    return (u8)((header->flags0 & MEMCARD_GAME_FLAGS0_ICON_TYPE_MASK) >> MEMCARD_GAME_FLAGS0_ICON_TYPE_SHIFT);
#else
    return header->flags0.icon_type;
#endif
}

static inline u8 Famicom_GetBannerFormat(const MemcardGameHeader_t* header) {
#ifdef TARGET_PC
    return (u8)((header->flags1 & MEMCARD_GAME_FLAGS1_BANNER_FMT_MASK) >> MEMCARD_GAME_FLAGS1_BANNER_FMT_SHIFT);
#else
    return header->flags1.banner_fmt;
#endif
}

static inline void Famicom_SetCommentType(MemcardGameHeader_t* header, u32 value) {
#ifdef TARGET_PC
    header->flags0 = (header->flags0 & (u8)~MEMCARD_GAME_FLAGS0_COMMENT_TYPE_MASK) |
                     (u8)((value << MEMCARD_GAME_FLAGS0_COMMENT_TYPE_SHIFT) & MEMCARD_GAME_FLAGS0_COMMENT_TYPE_MASK);
#else
    header->flags0.comment_type = value;
#endif
}

static inline void Famicom_SetBannerType(MemcardGameHeader_t* header, u32 value) {
#ifdef TARGET_PC
    header->flags0 = (header->flags0 & (u8)~MEMCARD_GAME_FLAGS0_BANNER_TYPE_MASK) |
                     (u8)((value << MEMCARD_GAME_FLAGS0_BANNER_TYPE_SHIFT) & MEMCARD_GAME_FLAGS0_BANNER_TYPE_MASK);
#else
    header->flags0.banner_type = value;
#endif
}

static inline void Famicom_SetIconType(MemcardGameHeader_t* header, u32 value) {
#ifdef TARGET_PC
    header->flags0 = (header->flags0 & (u8)~MEMCARD_GAME_FLAGS0_ICON_TYPE_MASK) |
                     (u8)((value << MEMCARD_GAME_FLAGS0_ICON_TYPE_SHIFT) & MEMCARD_GAME_FLAGS0_ICON_TYPE_MASK);
#else
    header->flags0.icon_type = value;
#endif
}

static inline void Famicom_SetBannerFormat(MemcardGameHeader_t* header, u32 value) {
#ifdef TARGET_PC
    header->flags1 = (header->flags1 & (u8)~MEMCARD_GAME_FLAGS1_BANNER_FMT_MASK) |
                     (u8)((value << MEMCARD_GAME_FLAGS1_BANNER_FMT_SHIFT) & MEMCARD_GAME_FLAGS1_BANNER_FMT_MASK);
#else
    header->flags1.banner_fmt = value;
#endif
}

static inline BOOL Famicom_HasCommentImage(const MemcardGameHeader_t* header) {
#ifdef TARGET_PC
    return (header->flags0 & MEMCARD_GAME_FLAGS0_HAS_COMMENT_IMG) != 0;
#else
    return header->flags0.has_comment_img;
#endif
}

static inline BOOL Famicom_IsCopyDisabled(const MemcardGameHeader_t* header) {
#ifdef TARGET_PC
    return (header->flags0 & MEMCARD_GAME_FLAGS0_NO_COPY_FLAG) != 0;
#else
    return header->flags0.no_copy_flag;
#endif
}

static inline BOOL Famicom_IsMoveDisabled(const MemcardGameHeader_t* header) {
#ifdef TARGET_PC
    return (header->flags1 & MEMCARD_GAME_FLAGS1_NO_MOVE_FLAG) != 0;
#else
    return header->flags1.no_move_flag;
#endif
}

static inline void Famicom_SetHasCommentImage(MemcardGameHeader_t* header, BOOL enabled) {
#ifdef TARGET_PC
    header->flags0 = enabled ? (u8)(header->flags0 | MEMCARD_GAME_FLAGS0_HAS_COMMENT_IMG)
                             : (u8)(header->flags0 & (u8)~MEMCARD_GAME_FLAGS0_HAS_COMMENT_IMG);
#else
    header->flags0.has_comment_img = enabled;
#endif
}

static inline void Famicom_SetCopyDisabled(MemcardGameHeader_t* header, BOOL enabled) {
#ifdef TARGET_PC
    header->flags0 = enabled ? (u8)(header->flags0 | MEMCARD_GAME_FLAGS0_NO_COPY_FLAG)
                             : (u8)(header->flags0 & (u8)~MEMCARD_GAME_FLAGS0_NO_COPY_FLAG);
#else
    header->flags0.no_copy_flag = enabled;
#endif
}

static inline void Famicom_SetMoveDisabled(MemcardGameHeader_t* header, BOOL enabled) {
#ifdef TARGET_PC
    header->flags1 = enabled ? (u8)(header->flags1 | MEMCARD_GAME_FLAGS1_NO_MOVE_FLAG)
                             : (u8)(header->flags1 & (u8)~MEMCARD_GAME_FLAGS1_NO_MOVE_FLAG);
#else
    header->flags1.no_move_flag = enabled;
#endif
}

typedef int (*FAMICOM_GETSAVECHAN_PROC)(int* player_no, s32* slot_card_result);

extern int famicom_getErrorChan();
extern void famicom_setCallback_getSaveChan(FAMICOM_GETSAVECHAN_PROC proc);
extern int famicom_mount_archive_end_check();
extern void famicom_mount_archive();
extern int famicom_init(int rom_idx, Famicom_MallocInfo* malloc_info, int player_no);
extern int famicom_cleanup();
extern void famicom_1frame();
extern int famicom_rom_load_check();
extern int famicom_internal_data_load();
extern int famicom_internal_data_save();
extern int famicom_external_data_save();
extern int famicom_external_data_save_check();
extern int famicom_get_disksystem_titles(int* n_games, char* title_name_bufp, int namebuf_size);

extern void nesinfo_tags_set(int rom_no);
extern void nesinfo_tag_process1(u8* save_data, int mode, u32* max_ofs_p);
extern void nesinfo_tag_process2();
extern void nesinfo_tag_process3(u8* save_data);
extern void nesinfo_update_highscore(u8* save_data, int mode);
extern int nesinfo_get_highscore_num();
extern u8* nesinfo_get_moriName();
extern void nesinfo_init();
extern void highscore_setup_flags(u8* flags);

#ifdef __cplusplus
}
#endif

#endif
