#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "os_adapter.h"
#include "vdecoder_config.h"

//#include <cedarv_osal_linux.h>
#include <avheap.h>
#include <cedardev_api.h>

#include <stdarg.h>
#include "awprintf.h"


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
	return av_heap_alloc((u32)size);
}


void  mem_pfree(void* p)
{
	av_heap_free(p);
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
	//eLIBs_CleanFlushDCacheRegion(mem, size);

}


u32 mem_get_phy_addr(u32 virtual_addr)
{
	return av_heap_physic_addr((void*)virtual_addr);
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
s32 sys_print(u8* func, u32 line, ...)
{
/* use other print function
    va_list args;

    va_start(args, line);

    AwVLog((char*)func, line, args);

    va_end(args);
*/

    return 0;
}

void sys_sleep(u32 ms)
{
	usleep(ms*1000);
}

