/*
 * sunxi_alloc.c
 *
 * xiaoshujun@allwinnertech.com
 *
 * sunxi memory allocate
 *
 */

//#define LOG_NDEBUG 0
#define LOG_TAG "sunxi_alloc"
#include <CDX_Debug.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <sys/mman.h>
#include <pthread.h>

//#include <linux/sunxi_physmem.h>

#include "list.h"

/*
 * io cmd, must be same as  <linux/sunxi_physmem.h> 
 */
#define SUNXI_MEM_ALLOC 		1
#define SUNXI_MEM_FREE 			3 /* cannot be 2, which reserved in linux */
#define SUNXI_MEM_GET_REST_SZ 	4
#define SUNXI_MEM_FLUSH_CACHE   5
#define SUNXI_MEM_FLUSH_CACHE_ALL	6

#define DEV_NAME 			"/dev/sunxi_mem"

typedef struct BUFFER_NODE
{
	struct list_head i_list; 
	int phy;
	int vir;
	int size;
}buffer_node;

typedef struct SUNXI_ALLOC_CONTEXT
{
	int					fd;			// driver handle
	struct list_head	list;		// buffer list
	int					ref_cnt;	// reference count
}sunxi_alloc_context;

sunxi_alloc_context	*	g_alloc_context = NULL;
pthread_mutex_t			g_mutex_alloc = PTHREAD_MUTEX_INITIALIZER;

int sunxi_alloc_open()
{
	LOGV("sunxi_alloc_open");
	
	pthread_mutex_lock(&g_mutex_alloc);
	if (g_alloc_context != NULL)
	{
		LOGV("sunxi allocator has already been created");
		goto SUCCEED_OUT;
	}

	g_alloc_context = (sunxi_alloc_context*)malloc(sizeof(sunxi_alloc_context));
	if (g_alloc_context == NULL)
	{
		LOGE("create sunxi allocator failed, out of memory");
		goto ERROR_OUT;
	}
	else
	{
		LOGV("pid: %d, g_alloc_context = %p", getpid(), g_alloc_context);
	}

	memset((void*)g_alloc_context, 0, sizeof(sunxi_alloc_context));
	
	g_alloc_context->fd = open(DEV_NAME, O_RDWR, 0);
	if (g_alloc_context->fd <= 0)
	{
		LOGE("open %s failed", DEV_NAME);
		goto ERROR_OUT;
	}

	INIT_LIST_HEAD(&g_alloc_context->list);

SUCCEED_OUT:
	g_alloc_context->ref_cnt++;
	pthread_mutex_unlock(&g_mutex_alloc);
	return 0;

ERROR_OUT:
	if (g_alloc_context != NULL
		&& g_alloc_context->fd > 0)
	{
		close(g_alloc_context->fd);
		g_alloc_context->fd = 0;
	}
	
	if (g_alloc_context != NULL)
	{
		free(g_alloc_context);
		g_alloc_context = NULL;
	}
	
	pthread_mutex_unlock(&g_mutex_alloc);
	return -1;
}

int sunxi_alloc_close()
{
	struct list_head * pos, *q;
	buffer_node * tmp;

	LOGV("sunxi_alloc_close");
	
	pthread_mutex_lock(&g_mutex_alloc);
	if (--g_alloc_context->ref_cnt <= 0)
	{
		LOGV("pid: %d, release g_alloc_context = %p", getpid(), g_alloc_context);
		
		list_for_each_safe(pos, q, &g_alloc_context->list)
		{
			tmp = list_entry(pos, buffer_node, i_list);
			LOGV("sunxi_alloc_close del item phy= 0x%08x vir= 0x%08x, size= %d", tmp->phy, tmp->vir, tmp->size);
			list_del(pos);
			free(tmp);
		}
		
#if (LOG_NDEBUG == 0)
{
		int rest_size = ioctl(g_alloc_context->fd, SUNXI_MEM_GET_REST_SZ, 0);
		LOGV("before close allocator, rest size: 0x%08x", rest_size);
}
#endif

		close(g_alloc_context->fd);
		g_alloc_context->fd = 0;

		free(g_alloc_context);
		g_alloc_context = NULL;
	}
	else
	{
		LOGV("ref cnt: %d > 0, do not free", g_alloc_context->ref_cnt);
	}
	pthread_mutex_unlock(&g_mutex_alloc);
	
	return 0;
}

// return virtual address: 0 failed
int sunxi_alloc_alloc(int size)
{
	int rest_size = 0;
	int addr_phy = 0;
	int addr_vir = 0;
	buffer_node * alloc_buffer = NULL;

	pthread_mutex_lock(&g_mutex_alloc);

	if (g_alloc_context == NULL)
	{
		LOGE("sunxi_alloc do not opened, should call sunxi_alloc_open() before sunxi_alloc_alloc(size)");
		goto ALLOC_OUT;
	}
	
	if(size <= 0)
	{
		LOGE("can not alloc size 0");
		goto ALLOC_OUT;
	}
	
	rest_size = ioctl(g_alloc_context->fd, SUNXI_MEM_GET_REST_SZ, 0);
	LOGV("rest size: 0x%08x", rest_size);

	if (rest_size < size)
	{
		LOGE("not enough memory, rest: 0x%08x, request: 0x%08x", rest_size, size);
		goto ALLOC_OUT;
	}

	addr_phy = ioctl(g_alloc_context->fd, SUNXI_MEM_ALLOC, &size);
	if (0 == addr_phy)
	{
		LOGE("can not request physical buffer, size: %d", size);
		goto ALLOC_OUT;
	}

	addr_vir = (int)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, 
					g_alloc_context->fd, (int)addr_phy);
	if ((int)MAP_FAILED == addr_vir)
	{
		ioctl(g_alloc_context->fd, SUNXI_MEM_FREE, &addr_phy);
		addr_phy = 0;
		addr_vir = 0;		// value of MAP_FAILED is -1, should return 0
		goto ALLOC_OUT;
	}

	alloc_buffer = (buffer_node *)malloc(sizeof(buffer_node));
	if (alloc_buffer == NULL)
	{
		LOGE("malloc buffer node failed");

		// should free and unmmapphy buffer
		ioctl(g_alloc_context->fd, SUNXI_MEM_FREE, &addr_phy);
		
		if (munmap((void*)addr_vir, size) < 0)
		{
			LOGE("munmap 0x%08x, size: %d failed", addr_vir, size);
		}
		
		addr_phy = 0;
		addr_vir = 0;		// value of MAP_FAILED is -1, should return 0
		
		goto ALLOC_OUT;
	}
	alloc_buffer->phy 	= addr_phy;
	alloc_buffer->vir 	= addr_vir;
	alloc_buffer->size	= size;

	LOGV("alloc succeed, addr_phy: 0x%08x, addr_vir: 0x%08x, size: %d", addr_phy, addr_vir, size);

	list_add_tail(&alloc_buffer->i_list, &g_alloc_context->list);

ALLOC_OUT:
	
	pthread_mutex_unlock(&g_mutex_alloc);

	return addr_vir;
}

void sunxi_alloc_free(void * pbuf)
{
	int flag = 0;
	int addr_vir = (int)pbuf;
	buffer_node * tmp;

	if (0 == pbuf)
	{
		LOGE("can not free NULL buffer");
		return;
	}
	
	pthread_mutex_lock(&g_mutex_alloc);
	
	if (g_alloc_context == NULL)
	{
		LOGE("sunxi_alloc do not opened, should call sunxi_alloc_open() before sunxi_alloc_alloc(size)");
		return;
	}
	
	list_for_each_entry(tmp, &g_alloc_context->list, i_list)
	{
		if (tmp->vir == addr_vir)
		{
			LOGV("sunxi_alloc_free item phy= 0x%08x vir= 0x%08x, size= %d", tmp->phy, tmp->vir, tmp->size);

			ioctl(g_alloc_context->fd, SUNXI_MEM_FREE, &tmp->phy);
			
			if (munmap(pbuf, tmp->size) < 0)
			{
				LOGE("munmap 0x%08x, size: %d failed", addr_vir, tmp->size);
			}
		
			list_del(&tmp->i_list);
			free(tmp);

#if (LOG_NDEBUG == 0)
{
			int rest_size = ioctl(g_alloc_context->fd, SUNXI_MEM_GET_REST_SZ, 0);
			LOGV("after free, rest size: 0x%08x", rest_size);
}
#endif

			flag = 1;
			break;
		}
	}

	if (0 == flag)
	{
		LOGE("sunxi_alloc_free failed, do not find virtual address: 0x%08x", addr_vir);
	}

	pthread_mutex_unlock(&g_mutex_alloc);
}

int sunxi_alloc_vir2phy(void * pbuf)
{
	int flag = 0;
	int addr_vir = (int)pbuf;
	int addr_phy = 0;
	buffer_node * tmp;
	
	if (0 == pbuf)
	{
		// LOGE("can not vir2phy NULL buffer");
		return 0;
	}
	
	pthread_mutex_lock(&g_mutex_alloc);
	
	list_for_each_entry(tmp, &g_alloc_context->list, i_list)
	{
		if (addr_vir >= tmp->vir
			&& addr_vir < tmp->vir + tmp->size)
		{
			addr_phy = tmp->phy + addr_vir - tmp->vir;
			// LOGV("sunxi_alloc_vir2phy phy= 0x%08x vir= 0x%08x", addr_phy, addr_vir);
			flag = 1;
			break;
		}
	}
	
	if (0 == flag)
	{
		LOGE("sunxi_alloc_vir2phy failed, do not find virtual address: 0x%08x", addr_vir);
	}
	
	pthread_mutex_unlock(&g_mutex_alloc);

	if(addr_phy >= 0x40000000)
		addr_phy -= 0x40000000;

	return addr_phy;
}

int sunxi_alloc_phy2vir(void * pbuf)
{
	int flag = 0;
	int addr_vir = 0;
	int addr_phy = (int)pbuf;
	buffer_node * tmp;
	
	if (0 == pbuf)
	{
		LOGE("can not phy2vir NULL buffer");
		return 0;
	}
	
	if(addr_phy < 0x40000000)
		addr_phy += 0x40000000;

	pthread_mutex_lock(&g_mutex_alloc);
	
	list_for_each_entry(tmp, &g_alloc_context->list, i_list)
	{
		if (addr_phy >= tmp->phy
			&& addr_phy < tmp->phy + tmp->size)
		{
			addr_vir = tmp->vir + addr_phy - tmp->phy;
			// LOGV("sunxi_alloc_phy2vir phy= 0x%08x vir= 0x%08x", addr_phy, addr_vir);
			flag = 1;
			break;
		}
	}
	
	if (0 == flag)
	{
		LOGE("sunxi_alloc_phy2vir failed, do not find physical address: 0x%08x", addr_phy);
	}
	
	pthread_mutex_unlock(&g_mutex_alloc);

	return addr_vir;
}

void sunxi_flush_cache(void* startAddr, int size)
{
	long arr[2];

	arr[0] = (long)startAddr;
	arr[1] = arr[0] + size - 1;

	ioctl(g_alloc_context->fd, SUNXI_MEM_FLUSH_CACHE, arr);

	return;
}

void sunxi_flush_cache_all()
{
	ioctl(g_alloc_context->fd, SUNXI_MEM_FLUSH_CACHE_ALL, 0);
}

