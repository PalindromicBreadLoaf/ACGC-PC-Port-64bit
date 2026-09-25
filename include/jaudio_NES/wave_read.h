#ifndef WAVEREAD_H
#define WAVEREAD_H
#include "types.h"
#ifdef TARGET_PC
#include <stddef.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

int WaveScene_Set(u32, u32);
void Wavegroup_Init();
int Wavegroup_Regist(void*, u32);
#ifdef TARGET_PC
int Wavegroup_RegistSized(const void*, size_t, u32);
#endif
#ifdef __cplusplus
}
#endif

#endif
