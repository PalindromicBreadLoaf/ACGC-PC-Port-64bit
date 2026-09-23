#ifndef _JAUDIO_HEAPCTRL_H
#define _JAUDIO_HEAPCTRL_H

#include "types.h"
#ifdef TARGET_PC
#include "pc_portability.h"
#endif

typedef struct jaheap_ jaheap_;
typedef struct jaheap_ jaheap;

#ifdef TARGET_PC
typedef uintptr_t jaheap_addr_t;
#else
typedef u32 jaheap_addr_t;
#endif

struct jaheap_ {
	u8 isRootHeap;             // _00, is this a 'mother' heap?
	u8 memoryType;             // _01, 0 = ARAM, 1 = DRAM
	u16 childCount;            // _02
	u32 heapId;                // _04
	jaheap_addr_t startAddress; // _08
	u32 usedSize;              // _0C
	u32 size;                  // _10
	jaheap_* firstChild;       // _14
	jaheap_* parent;           // _18
	jaheap_* nextSibling;      // _1C
	jaheap_* groupOwner;       // _20
	jaheap_* firstGroupedHeap; // _24
	jaheap_* nextGroupedHeap;  // _28
};

#ifdef __cplusplus
extern "C" {
#endif

void Jac_GetUnlockHeap(jaheap_*);
void Jac_CheckAlloc(jaheap_*);
void Jac_InitHeap(jaheap_*);
void Jac_SelfInitHeap(jaheap_*, jaheap_addr_t, u32, u32);
BOOL Jac_SelfAllocHeap(jaheap_*, jaheap_*, u32, jaheap_addr_t);
BOOL Jac_SetGroupHeap(jaheap_*, jaheap_*);
void Jac_CutdownHeap(jaheap_*);
void Jac_InitMotherHeap(jaheap_*, jaheap_addr_t, u32, u8);
BOOL Jac_AllocHeap(jaheap_*, jaheap_*, u32);
BOOL Jac_DeleteHeap(jaheap_*);
void Jac_GarbageCollection_St(jaheap_*);
void Jac_CheckFreeHeap_Total(jaheap_*);
void Jac_CheckFreeHeap_Linear(jaheap_*);
void Jac_ShowHeap(jaheap_*, u32);

#ifdef __cplusplus
}
#endif

#endif
