#ifndef DUMMYROM_H
#define DUMMYROM_H

#include "types.h"
#ifdef TARGET_PC
#include "pc_portability.h"
#endif
#include "jaudio_NES/audiostruct.h"
#include "libultra/libultra.h"

#define DUMMYROM_DRAM_TO_ARAM 0
#define DUMMYROM_ARAM_TO_DRAM 1

#ifdef __cplusplus
extern "C" {
#endif

extern u8* JAC_ARAM_DMA_BUFFER_TOP;
extern ALHeap aram_hp; /* placed in common bss */

extern u32 GetNeos_FileTop(void);
extern u32 GetNeosRomTop(void);
extern u32 GetNeosRom_PreLoaded(void);
extern u32 SetPreCopy_NeosRom(u8* load_addr, u32 load_size, BOOL cut_flag);
#ifdef TARGET_PC
typedef void* jaudio_mram_addr_t;
typedef aram_addr_t jaudio_aram_addr_t;
typedef uintptr_t jaudio_callback_arg_t;
#else
typedef u32 jaudio_mram_addr_t;
typedef u32 jaudio_aram_addr_t;
typedef u32 jaudio_callback_arg_t;
#endif
extern BOOL ARAMStartDMAmesg(u32 dir, jaudio_mram_addr_t dramAddr, jaudio_aram_addr_t aramAddr, u32 size, s32 unused,
                             OSMesgQueue* mq);
extern void Jac_SetAudioARAMSize(u32 size);
extern void* ARAllocFull(u32* outSize);
extern void Jac_InitARAM(u32 loadAudiorom);

#ifdef __cplusplus
}
#endif

#endif
