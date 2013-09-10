
#include "os_adapter.h"
#include "vdecoder_config.h"
#include "awprintf.h"
#include "epdk.h"

//*******************************************************//
//********** Functions for Heap Operation. **************//
//*******************************************************//
void* mem_alloc(u32 size)
{
	return CEDAR_MALLOC(size);
}


void  mem_free(void* p)
{
	CEDAR_FREE(p);
}


void* mem_palloc(u32 size, u32 align)
{
	return CEDAR_PHYMALLOC(size, align);
}


void  mem_pfree(void* p)
{
	CEDAR_PHYFREE(p);
}


//*******************************************************//
//********* Functions for Memory Operation. *************//
//*******************************************************//
void  mem_set(void* mem, u32 value, u32 size)
{
	eLIBs_memset(mem, value, size);
}


void  mem_cpy(void* dst, void* src, u32 size)
{
	eLIBs_memcpy(dst, src, size);
}


void  mem_flush_cache(u8* mem, u32 size)
{
	esMEMS_CleanFlushDCacheRegion(mem, size);
}


u32 mem_get_phy_addr(u32 virtual_addr)
{
	return (u32)VirAddr2PhyAddr(virtual_addr);
}

//*******************************************************//
//********* Functions for Memory Operation. *************//
//*******************************************************//

Handle semaphore_create(u32 cnt)
{
	return (Handle)esKRNL_SemCreate(cnt);
}


s32 semaphore_delete(Handle h, u32 opt)
{
	u8 err;

	if(opt == SEM_DEL_ALWAYS)
		opt = OS_DEL_ALWAYS;
	else
		opt = OS_DEL_NO_PEND;

	esKRNL_SemDel((__krnl_event_t*)h, opt, &err);

	if(err == 0)
		return 0;
	else
		return -1;
}


s32 semaphore_pend(Handle h, u32 timeout)
{
	u8 err;
	esKRNL_SemPend((__krnl_event_t*)h, (u16)timeout, &err);

	if(err == 0)
		return 0;
	else
		return -1;
}


s32 semaphore_post(Handle h)
{
	esKRNL_SemPost((__krnl_event_t*)h);
	return 0;
}


s32 semaphore_accept(Handle h)
{
	u32 cnt = esKRNL_SemAccept((__krnl_event_t*)h);
	if(cnt > 0)
		return 0;
	else
		return -1;
}


s32 semaphore_query(Handle h)
{
	OS_SEM_DATA sem_data;

	if(esKRNL_SemQuery((__krnl_event_t*)h, &sem_data) == 0)
	{
		return sem_data.OSCnt;
	}
	else
	{
		return 0;
	}
}


//*******************************************************//
//************** MISC Functions of OS. ******************//
//*******************************************************//

s32 sys_print(u8* func, u32 line, ...)
{
    va_list args;
    
    va_start(args, line);
    
	AwVLog(func, line, args);
	
    va_end(args);
    
    return 0;
}
	
void sys_sleep(u32 ms)
{
	if(ms < 10)
		ms = 10;

	esKRNL_TimeDly(ms/10);
}

