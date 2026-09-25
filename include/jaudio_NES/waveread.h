#ifndef _JAUDIO_WAVEREAD_H
#define _JAUDIO_WAVEREAD_H

#include "types.h"
#ifdef TARGET_PC
#include <stddef.h>
#endif

typedef struct CtrlGroup_ CtrlGroup_;

#ifdef __cplusplus
extern "C" {
#endif

CtrlGroup_* Wave_Test(u8*);
void GetSound_Test(u32);
BOOL Wavegroup_Regist(void*, u32);
#ifdef TARGET_PC
BOOL Wavegroup_RegistSized(const void*, size_t, u32);
#endif
void Wavegroup_Init();
CtrlGroup_* WaveidToWavegroup(u32, u32);
BOOL WaveScene_Set(u32, u32);
BOOL WaveScene_Load(u32, u32);
void WaveScene_Close(u32, u32);
void WaveScene_Erase(u32, u32);

#ifdef __cplusplus
}
#endif

#endif
