#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "os_adapter.h"
#include "vdecoder_config.h"

#include "cdxalloc.h"


//*******************************************************//
//********** Functions for Heap Operation. **************//
//*******************************************************//
void* mem_alloc(u32 size)
{
	return malloc(size);
}


void  mem_free(void* p)
{
	free(p);
}


void* mem_palloc(u32 size, u32 align)
{
	return cdxalloc_alloc(size);	//* the CedarX memory manage library is responsible of managing VE's 64MB memory space reserved by cedar_dev.ko.
}


void  mem_pfree(void* p)
{
	cdxalloc_free(p);	//* the CedarX memory manage library is responsible of managing VE's 64MB memory space reserved by cedar_dev.ko.
}


//*******************************************************//
//********* Functions for Memory Operation. *************//
//*******************************************************//
void  mem_set(void* mem, u32 value, u32 size)
{
	memset(mem, value, size);
}


void  mem_cpy(void* dst, void* src, u32 size)
{
	memcpy(dst, src, size);
}


void  mem_flush_cache(u8* mem, u32 size)
{
	//* the 64MB memory space reserved by cedar_dev.ko for VE decoding is mapped to cache, so do nothing here.
}


u32 mem_get_phy_addr(u32 virtual_addr)
{
	return cdxalloc_vir2phy((void*)virtual_addr);	//* the CedarX memory manage library is responsible of managing VE's 64MB memory space reserved by cedar_dev.ko.
}

//*******************************************************//
//********* Functions for Memory Operation. *************//
//*******************************************************//

Handle semaphore_create(u32 cnt)
{
	pthread_mutex_t *mutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));

	pthread_mutex_init(mutex, NULL);

	return (Handle)(mutex);
}


s32 semaphore_delete(Handle h, u32 opt)
{
	pthread_mutex_destroy((pthread_mutex_t *)h);
	if(h)
	{
		free(h);
		h = NULL;
	}
	return 0;
}


s32 semaphore_pend(Handle h, u32 timeout)
{
	pthread_mutex_lock((pthread_mutex_t *)h);
	return 0;
}


s32 semaphore_post(Handle h)
{
	pthread_mutex_unlock((pthread_mutex_t *)h);
	return 0;
}


//*******************************************************//
//************** MISC Functions of OS. ******************//
//*******************************************************//
void sys_sleep(u32 ms)
{
	usleep(ms*1000);
}

