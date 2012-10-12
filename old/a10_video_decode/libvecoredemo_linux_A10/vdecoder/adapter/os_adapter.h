
#ifndef OS_ADAPTER_H
#define OS_ADAPTER_H

#include "libve_typedef.h"
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

    //*******************************************************//
    //********** Functions for Heap Operation. **************//
    //*******************************************************//
    void* mem_alloc(u32 size);
    void  mem_free(void* p);
    void* mem_palloc(u32 size, u32 align);
    void  mem_pfree(void* p);

    //*******************************************************//
    //********* Functions for Memory Operation. *************//
    //*******************************************************//
    void  mem_set(void* mem, u32 value, u32 size);
    void  mem_cpy(void* dst, void* src, u32 size);
    void  mem_flush_cache(u8* mem, u32 size);
    u32   mem_get_phy_addr(u32 virtual_addr);

    //*******************************************************//
    //********* Functions for Memory Operation. *************//
    //*******************************************************//
    #define SEM_DEL_NO_PEND 0
    #define SEM_DEL_ALWAYS  1

    Handle semaphore_create(u32 cnt);
    s32    semaphore_delete(Handle h, u32 opt);
    s32    semaphore_pend(Handle h, u32 timeout);
    s32    semaphore_post(Handle h);
    s32    semaphore_accept(Handle h);
    s32    semaphore_query(Handle h);

    //*******************************************************//
    //************** Misc Functions of OS. ******************//
    //*******************************************************//
	#define sys_print printf
    void  sys_sleep(u32 ms);

#ifdef __cplusplus
}
#endif

#endif

