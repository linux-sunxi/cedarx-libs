
#include <unistd.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdlib.h>
#include <semaphore.h>
#include <string.h>
#include <pthread.h>

#include "libve_adapter.h"
#include <fbm.h>
#include "vbv.h"
#include "os_adapter.h"
#include "vdecoder_config.h"
#include <cedardev_api.h>		//* you can find this header file in cedarX/CedarX/include/include_platform/CHIP_F20
#include "avheap.h"

#define WALL_CLOCK_FREQ (50*1000)
static signed long long wall_clock = 0;
unsigned int avs_cnt_last = 0;

//****************************************************************************//
//************************ Instance of OS Interface **************************//
//****************************************************************************//
IOS_t IOS = 
{
    //* Heap operation.
    mem_alloc,
    mem_free,
    mem_palloc,
    mem_pfree,
    
    //* Memory operation.
    mem_set,
    mem_cpy,
    mem_flush_cache,
    mem_get_phy_addr,

    //* MISC functions.
    sys_print,
    sys_sleep,
};


//****************************************************************************//
//************************ Instance of OS Interface **************************//
//****************************************************************************//

typedef struct CEDARV_OSAL_CONTEXT{
	s32					fd;
	cedarv_env_info_t  	env_info;
}cedarv_osal_context_t;

cedarv_osal_context_t *cedarv_osal_ctx;
pthread_mutex_t cedarv_osal_mutex;

int cedarv_wait_ve_ready()
{
    int    status;

	//printf("wait ve start!\n"); //printf("getchar()!\n");getchar();
	status = ioctl(cedarv_osal_ctx->fd, IOCTL_WAIT_VE, 2);  //.tbd driver timeout set it to -1
	//printf("interrupt ret value: 0x%08x\n",status);

	return (status - 1);
}

void cedarv_request_ve_core()
{
	ioctl(cedarv_osal_ctx->fd, IOCTL_ENGINE_REQ, 0);
}

void cedarv_release_ve_core()
{
	ioctl(cedarv_osal_ctx->fd, IOCTL_ENGINE_REL, 0);
}

s32 cedardev_init(void)
{
	cedarv_osal_ctx = (cedarv_osal_context_t*)malloc(sizeof(cedarv_osal_context_t));
	if(!cedarv_osal_ctx)
		return -1;

	memset(cedarv_osal_ctx, 0, sizeof(cedarv_osal_context_t));

	cedarv_osal_ctx->fd = open("/dev/cedar_dev", O_RDWR);
	if(cedarv_osal_ctx->fd == -1)
		return -1;

	ioctl(cedarv_osal_ctx->fd, IOCTL_GET_ENV_INFO, (unsigned long)&cedarv_osal_ctx->env_info);
	cedarv_osal_ctx->env_info.address_macc = (unsigned int)mmap(NULL, 2048, PROT_READ | PROT_WRITE, MAP_SHARED, cedarv_osal_ctx->fd, (int)cedarv_osal_ctx->env_info.address_macc);

	pthread_mutex_init(&cedarv_osal_mutex, NULL);

	//* open CedarX memory manage library.
	av_heap_init(cedarv_osal_ctx->fd);

	cedarv_request_ve_core();

	return 0;
}

long long avs_counter_get_time_ms()
{
	unsigned int avs_cnt_curr;
	long long val;

	pthread_mutex_lock(&cedarv_osal_mutex);
	avs_cnt_curr = ioctl(cedarv_osal_ctx->fd, IOCTL_GETVALUE_AVS2, 0);

	if((avs_cnt_curr ^ avs_cnt_last)>>31){ //wrap around
		wall_clock += 0x00000000ffffffffLL;
	}
	avs_cnt_last = avs_cnt_curr;
	val = (wall_clock + avs_cnt_curr) / (WALL_CLOCK_FREQ / 1000);//((wall_clock + avs_cnt_curr)*1000 / (WALL_CLOCK_FREQ/1000));
	pthread_mutex_unlock(&cedarv_osal_mutex);

	return val;
}

s32 cedardev_exit(void)
{
	cedarv_release_ve_core();

	//* close CedarX memory manage library.
	av_heap_release();

	munmap((void *)cedarv_osal_ctx->env_info.address_macc, 2048);

	if(cedarv_osal_ctx->fd != -1)
		close(cedarv_osal_ctx->fd);

	free(cedarv_osal_ctx);
	cedarv_osal_ctx = NULL;

	pthread_mutex_destroy(&cedarv_osal_mutex);

    return 0;
}

static void      ve_reset_hardware(void);
static void      ve_enable_clock(u8 enable, u32 speed);
static void      ve_enable_intr(u8 enable);
static s32       ve_wait_intr(void);
static u32       ve_get_reg_base_addr(void);
static memtype_e ve_get_memtype(void);

IVEControl_t IVE = 
{
    ve_reset_hardware,
    ve_enable_clock,
    ve_enable_intr,
    ve_wait_intr,
    ve_get_reg_base_addr,
    ve_get_memtype
};


void cedarv_enable_ve_core()
{
	ioctl(cedarv_osal_ctx->fd, IOCTL_ENABLE_VE, 0);
}

void cedarv_disable_ve_core()
{
	ioctl(cedarv_osal_ctx->fd, IOCTL_DISABLE_VE, 0);
}


void cedarv_set_ve_freq(int freq) //MHz
{
	ioctl(cedarv_osal_ctx->fd, IOCTL_SET_VE_FREQ, freq);
}


static void ve_reset_hardware(void)
{
	ioctl(cedarv_osal_ctx->fd, IOCTL_RESET_VE, 0);
    return;
}

static void ve_enable_clock(u8 enable, u32 speed)
{
    if(enable)
    {
    	cedarv_enable_ve_core();
    	cedarv_set_ve_freq(speed / 1000000);
    }
    else
    {
    	cedarv_disable_ve_core();
    }
}

static void ve_enable_intr(u8 enable)
{
	/*
	 * cedar_dev.ko dose not support this operation, leave this method to be empty here.
	 */
}


static s32 ve_wait_intr(void)
{
	return cedarv_wait_ve_ready();
}


static u32 ve_get_reg_base_addr(void)
{
	return cedarv_osal_ctx->env_info.address_macc;
}


static memtype_e ve_get_memtype(void)
{
    return MEMTYPE_DDR3_32BITS; //* return the DRAM type.
}

void ve_init_clock(void)
{
    return;
}


void ve_release_clock(void)
{
    return;
}

