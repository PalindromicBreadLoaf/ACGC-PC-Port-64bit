#include <stddef.h>

#include "Famicom/famicom.h"
#include "dolphin/dvd.h"
#include "jaudio_NES/audiostruct.h"
#include "pc_audio_bank.h"
#include "pc_audio_wsys.h"
#include "pc_executable.h"
#include "m_common_data.h"
#include "m_scene.h"

_Static_assert(sizeof(DVDDriveInfo) == 32, "DVDDriveInfo layout changed");
_Static_assert(sizeof(DVDDiskID) == 32, "DVDDiskID layout changed");
_Static_assert(sizeof(ArcEntry) == 16, "ArcEntry layout changed");
_Static_assert(offsetof(ArcEntry, param2) == 14, "ArcEntry field offsets changed");
_Static_assert(sizeof(adpcmloop) == 48, "ADPCM loop layout changed");
_Static_assert(offsetof(adpcmloop, predictor_state) == 16, "ADPCM loop field offsets changed");
_Static_assert(sizeof(adpcmbook) == 8, "ADPCM book header layout changed");
_Static_assert(sizeof(pc_audio_wtstr_disk) == 8, "audio wtstr disk layout changed");
_Static_assert(sizeof(pc_audio_voicetable_disk) == 32, "audio voice disk layout changed");
_Static_assert(sizeof(pc_audio_perctable_disk) == 16, "audio percussion disk layout changed");
_Static_assert(sizeof(pc_audio_wavetable_disk) == 16, "audio wavetable disk layout changed");
_Static_assert(sizeof(pc_audio_loop_disk) == 16, "audio loop disk layout changed");
_Static_assert(sizeof(pc_audio_book_disk) == 8, "audio book disk layout changed");
_Static_assert(sizeof(pc_audio_env_disk) == 4, "audio envelope disk layout changed");
_Static_assert(sizeof(pc_audio_wsys_disk) == 24, "audio WSYS disk layout changed");
_Static_assert(sizeof(pc_audio_wsys_archive_bank_disk) == 8, "audio WINF disk layout changed");
_Static_assert(sizeof(pc_audio_wsys_control_group_disk) == 12, "audio WBCT disk layout changed");
_Static_assert(sizeof(pc_audio_wsys_scene_disk) == 24, "audio SCNE disk layout changed");
_Static_assert(sizeof(pc_audio_wsys_control_disk) == 8, "audio control disk layout changed");
_Static_assert(sizeof(pc_audio_wsys_wave_id_disk) == 56, "audio wave ID disk layout changed");
_Static_assert(sizeof(pc_audio_wsys_archive_disk) == 116, "audio archive disk layout changed");
_Static_assert(sizeof(pc_audio_wsys_wave_disk) == 40, "audio wave disk layout changed");
_Static_assert(sizeof(pc_dol_header_disk) == 0x100, "DOL header layout changed");
_Static_assert(sizeof(pc_rel_header_v1_disk) == 0x40, "REL v1 header layout changed");
_Static_assert(sizeof(pc_rel_header_v2_disk) == 0x48, "REL v2 header layout changed");
_Static_assert(sizeof(pc_rel_header_v3_disk) == 0x4c, "REL v3 header layout changed");
_Static_assert(sizeof(pc_rel_section_disk) == 8, "REL section layout changed");
_Static_assert(sizeof(SampleMedium) == 4, "audio medium enum storage changed");
_Static_assert(sizeof(AudioCacheType) == 4, "audio cache enum storage changed");
_Static_assert(sizeof(FamicomSaveDataHeader) == FAMICOM_SAVE_HEADER_SIZE, "Famicom save header layout changed");
_Static_assert(sizeof(MemcardGameHeader_t) == 32, "Famicom memory-card header layout changed");
_Static_assert(sizeof(enum filer_demo_mode) == 4, "Famicom enum storage changed");
_Static_assert(offsetof(MemcardGameHeader_t, flags0) == 28, "Famicom flags0 offset changed");
_Static_assert(offsetof(MemcardGameHeader_t, flags1) == 29, "Famicom flags1 offset changed");
_Static_assert(sizeof(Anmmem_c) == 0x138, "animal memory layout changed");
_Static_assert(_Alignof(Anmmem_c) == 8, "animal memory alignment changed");
_Static_assert(sizeof(Animal_c) == 0x988, "animal layout changed");
_Static_assert(offsetof(Animal_c, home_info) == 0x898, "animal home-info offset changed");
_Static_assert(sizeof(Save_t) == 0x242A0, "save data layout changed");
_Static_assert(sizeof(Save) == 0x26000, "sector-aligned save layout changed");
_Static_assert(offsetof(Save_t, private_data) == 0x20, "save player-data offset changed");
_Static_assert(offsetof(Save_t, land_info) == 0x9120, "save land-info offset changed");
_Static_assert(offsetof(Save_t, animals) == 0x17438, "save animal-data offset changed");
_Static_assert(offsetof(Save_t, time_delta) == 0x22528, "save time-delta offset changed");
_Static_assert(sizeof(Actor_data) == 0x10, "actor placement record layout changed");
_Static_assert(sizeof(Scene_Word_u) > 8, "PC scene words must hold native pointers");

int main(void) {
    MemcardGameHeader_t header = {0};
    Actor_data actor_data = {0};
    Scene_Word_u scene_word = mSc_DATA_PLAYER(&actor_data);

    if (scene_word.actor.data_p != &actor_data) {
        return 1;
    }

    Famicom_SetHasCommentImage(&header, TRUE);
    Famicom_SetCommentType(&header, MEMCARD_COMMENT_TYPE_COPY_ROM);
    Famicom_SetBannerType(&header, MEMCARD_BANNER_TYPE_COPY_EMBEDDED);
    Famicom_SetIconType(&header, MEMCARD_ICON_TYPE_DEFAULT);
    Famicom_SetCopyDisabled(&header, TRUE);
    Famicom_SetMoveDisabled(&header, TRUE);
    Famicom_SetBannerFormat(&header, 2);

    if (header.flags0 != 0xDB || header.flags1 != 0xC0) {
        return 1;
    }
    if (!Famicom_HasCommentImage(&header) || !Famicom_IsCopyDisabled(&header) ||
        !Famicom_IsMoveDisabled(&header)) {
        return 1;
    }
    if (Famicom_GetCommentType(&header) != MEMCARD_COMMENT_TYPE_COPY_ROM ||
        Famicom_GetBannerType(&header) != MEMCARD_BANNER_TYPE_COPY_EMBEDDED ||
        Famicom_GetIconType(&header) != MEMCARD_ICON_TYPE_DEFAULT || Famicom_GetBannerFormat(&header) != 2) {
        return 1;
    }
    return 0;
}
