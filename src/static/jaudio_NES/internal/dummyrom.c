#include "jaudio_NES/dummyrom.h"

#include "dolphin/ar.h"
#include "jaudio_NES/dvdthread.h"
#include "jaudio_NES/os.h"
#include "jaudio_NES/memory.h"

ALHeap aram_hp;
u8* JAC_ARAM_DMA_BUFFER_TOP = nullptr;
static u32 AUDIO_ARAM_TOP = 0;
static u32 CARD_SECURITY_BUFFER = 0;
static u32 init_load_size = 0;
static u8* init_load_addr = nullptr;
static BOOL init_cut_flag = FALSE;
static u32 SELECTED_ARAM_SIZE = 0;

extern u32 GetNeos_FileTop(void) {
    if (init_cut_flag) {
        return 0;
    }

    return init_load_size;
}

extern u32 GetNeosRomTop(void) {
    return AUDIO_ARAM_TOP;
}

extern u32 GetNeosRom_PreLoaded(void) {
#ifdef TARGET_PC
    DVDT_DRAMtoARAM(0, init_load_addr, AUDIO_ARAM_TOP, init_load_size, nullptr, nullptr);
#else
    DVDT_DRAMtoARAM(0, (u32)init_load_addr, AUDIO_ARAM_TOP, init_load_size, nullptr, nullptr);
#endif
    return init_load_size;
}

extern u32 SetPreCopy_NeosRom(u8* load_addr, u32 load_size, BOOL cut_flag) {
    init_load_size = load_size;
    init_load_addr = load_addr;
    init_cut_flag = cut_flag;
}

extern void mesg_finishcall(jaudio_callback_arg_t mq) {
    Z_osSendMesg((OSMesgQueue*)mq, NULL, OS_MESSAGE_NOBLOCK);
}

extern BOOL ARAMStartDMAmesg(u32 dir, jaudio_mram_addr_t dramAddr, jaudio_aram_addr_t aramAddr, u32 size, s32 unused,
                             OSMesgQueue* mq) {
    aramAddr += AUDIO_ARAM_TOP;

    if (dir == DUMMYROM_ARAM_TO_DRAM) {
#ifdef TARGET_PC
        DVDT_ARAMtoDRAM((uintptr_t)mq, aramAddr, dramAddr, size, nullptr, &mesg_finishcall);
#else
        DVDT_ARAMtoDRAM((u32)mq, dramAddr, aramAddr, size, nullptr, &mesg_finishcall);
#endif
    } else {
#ifdef TARGET_PC
        DVDT_DRAMtoARAM((uintptr_t)mq, dramAddr, aramAddr, size, nullptr, &mesg_finishcall);
#else
        DVDT_DRAMtoARAM((u32)mq, dramAddr, aramAddr, size, nullptr, &mesg_finishcall);
#endif
    }

    return FALSE;
}

extern void Jac_SetAudioARAMSize(u32 size) {
    SELECTED_ARAM_SIZE = size;
}

extern void* ARAllocFull(u32* outSize) {
    u32 freeSize = aram_hp.length - (u32)(aram_hp.current - aram_hp.base);
    void* alloc = Nas_HeapAlloc(&aram_hp, freeSize - 32);
    *outSize = freeSize - 32;
    return alloc;
}

extern void Jac_InitARAM(u32 loadAudiorom) {
    u32 aram_size = AUDIO_ARAM_SIZE;
    volatile u32 audiorom_size;

    if (SELECTED_ARAM_SIZE != 0) {
        aram_size = SELECTED_ARAM_SIZE;
    }

    AUDIO_ARAM_TOP = ARGetBaseAddress();
    if (loadAudiorom) {
        audiorom_size = Jac_CheckFile("/audiorom.img");
        if (audiorom_size != 0) {
            audiorom_size = ALIGN_NEXT(audiorom_size, 32);
            (void)audiorom_size; /* leftover from some debug print? */
        }
    } else {
        audiorom_size = 0;
    }

    CARD_SECURITY_BUFFER = 0x40;
    audiorom_size += AUDIO_ARAM_TOP;
#ifdef TARGET_PC
    JAC_ARAM_DMA_BUFFER_TOP = pc_aram_get_base() + audiorom_size;
#else
    JAC_ARAM_DMA_BUFFER_TOP = (u8*)audiorom_size;
#endif
    audiorom_size += AUDIO_ARAM_HEAP_SIZE;
#ifdef TARGET_PC
    Nas_HeapInit(&aram_hp, pc_aram_get_base() + audiorom_size, aram_size - audiorom_size);
#else
    Nas_HeapInit(&aram_hp, (u8*)audiorom_size, aram_size - audiorom_size);
#endif

    /* Probably leftovers from some debug print statement */
    (void)audiorom_size;
    (void)audiorom_size;
}
