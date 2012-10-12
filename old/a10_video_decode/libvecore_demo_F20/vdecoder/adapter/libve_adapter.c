
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
#include "fbm.h"
#include "vbv.h"
#include "os_adapter.h"
#include "vdecoder_config.h"
#include <cedardev_api.h>		//* you can find this header file in cedarX/CedarX/include/include_platform/CHIP_F20
#include "cdxalloc.h"

//****************************************************************************//
//************************ Instance of FBM Interface *************************//
//****************************************************************************//
IFBM_t IFBM =
{
    fbm_init,
    fbm_release,
    fbm_decoder_request_frame,
    fbm_decoder_return_frame,
    fbm_decoder_share_frame
};


//****************************************************************************//
//************************ Instance of VBV Interface *************************//
//****************************************************************************//

static void flush_stream_frame(vstream_data_t* stream, Handle vbv)
{
    vbv_flush_stream_frame(stream, vbv);
    libcedarv_free_vbs_buffer_sem();
}

IVBV_t IVBV =
{
    vbv_request_stream_frame,
    vbv_return_stream_frame,
    flush_stream_frame,
    vbv_get_base_addr,
    vbv_get_buffer_size
};


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
    (SYS_PRINT)sys_print,
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
	cdxalloc_open();

	return 0;
}


s32 cedardev_exit(void)
{
	//* close CedarX memory manage library.
	cdxalloc_close();

	pthread_mutex_destroy(&cedarv_osal_mutex);

	munmap((void *)cedarv_osal_ctx->env_info.address_macc, 2048);

	if(cedarv_osal_ctx->fd != -1)
		close(cedarv_osal_ctx->fd);

	free(cedarv_osal_ctx);
	cedarv_osal_ctx = NULL;

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


static void ve_reset_hardware(void)
{
	ioctl(cedarv_osal_ctx->fd, IOCTL_RESET_VE, 0);
    return;
}

static void ve_enable_clock(u8 enable, u32 speed)
{
/* note, as the cedar_dev.ko in F20 does not support the IOCTL_ENABLE_VE and IOCTL_SET_VE_FREQ command, I leave this method to be empty here.
    if(enable)
    {
    	ioctl(cedarv_osal_ctx->fd, IOCTL_ENABLE_VE, 0);
    	ioctl(cedarv_osal_ctx->fd, IOCTL_SET_VE_FREQ, speed/1000000);
    }
    else
    {
    	ioctl(cedarv_osal_ctx->fd, IOCTL_DISABLE_VE, 0);
    }
*/
}

static void ve_enable_intr(u8 enable)
{
	/*
	 * cedar_dev.ko dose not support this operation, leave this method to be empty here.
	 */
}

static s32 ve_wait_intr(void)
{
    s32 status;
	status = ioctl(cedarv_osal_ctx->fd, IOCTL_WAIT_VE, 0);

	return (status & 0xf);
}


static u32 ve_get_reg_base_addr(void)
{
	return cedarv_osal_ctx->env_info.address_macc;
}


static memtype_e ve_get_memtype(void)
{
    return MEMTYPE_DDR2_32BITS; //* return the DRAM type.
}



