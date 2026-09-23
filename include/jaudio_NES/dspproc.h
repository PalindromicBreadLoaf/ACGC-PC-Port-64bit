#ifndef DSPPROC_H
#define DSPPROC_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef TARGET_PC
typedef uintptr_t dsp_cmd_addr_t;
#else
typedef u32 dsp_cmd_addr_t;
#endif

extern s32 DSPSendCommands(u32* commands, u32 count);
extern u32 DSPReleaseHalt();
extern void DSPWaitFinish();
extern void DsetupTable(u32 arg0, dsp_cmd_addr_t arg1, dsp_cmd_addr_t arg2, dsp_cmd_addr_t arg3,
                        dsp_cmd_addr_t arg4);
extern void DsyncFrame(u32 subframes, dsp_cmd_addr_t dspbuf_start, dsp_cmd_addr_t dspbuf_end);
extern void DwaitFrame();
extern void DiplSec(dsp_cmd_addr_t arg0);
extern void DagbSec(dsp_cmd_addr_t arg0);

#ifdef __cplusplus
}
#endif

#endif
