/*******************************************************************************
--                                                                            --
--                    CedarX Multimedia Framework                             --
--                                                                            --
--          the Multimedia Framework for Linux/Android System                 --
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                         Softwinner Products.                               --
--                                                                            --
--                   (C) COPYRIGHT 2011 SOFTWINNER PRODUCTS                   --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
*******************************************************************************/

#include <unistd.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdlib.h>
#include <semaphore.h>
#include <string.h>
#include <pthread.h>

#include <cedarv_osal_linux.h>
#include <cedardev_api.h>
#include <cedarx_avs_counter.h>

//#define LOG_NDEBUG 0
#define LOG_TAG "cedarv_osal_linux"
#include <CDX_Debug.h>

#define NEW_VERSION 10
#define USE_SUNXI_MEM_ALLOCATOR 1 

#ifdef USE_ION_MEM_ALLOCATOR
extern int ion_alloc_open();
extern int ion_alloc_close();
extern int ion_alloc_alloc(int size);
extern void ion_alloc_free(void * pbuf);
extern int ion_alloc_vir2phy(void * pbuf);
extern int ion_alloc_phy2vir(void * pbuf);
extern void ion_flush_cache(void* startAddr, int size);
extern void ion_flush_cache_all();
#elif USE_SUNXI_MEM_ALLOCATOR
extern int sunxi_alloc_open();
extern int sunxi_alloc_close();
extern int sunxi_alloc_alloc(int size);
extern void sunxi_alloc_free(void * pbuf);
extern int sunxi_alloc_vir2phy(void * pbuf);
extern int sunxi_alloc_phy2vir(void * pbuf);
extern void sunxi_flush_cache(void* startAddr, int size);
extern void sunxi_flush_cache_all();
#else
extern int cdxalloc_open();
extern int cdxalloc_close();
extern void* cdxalloc_alloc(int size);
extern void cdxalloc_free(void *address);
extern unsigned int cdxalloc_vir2phy(void *address);
extern unsigned int cdxalloc_phy2vir(void *address);
#endif

static void cedarv_set_ve_ref_count(int ref_count);

typedef struct CEDARV_OSAL_CONTEXT{
	int fd;
	int ref_count;
	unsigned int ve_version;
	cedarv_env_info_t  env_info;
}cedarv_osal_context_t;

cedarv_osal_context_t *cedarv_osal_ctx = NULL;
pthread_mutex_t g_mutex_cedarv_osal = PTHREAD_MUTEX_INITIALIZER;

#define WALL_CLOCK_FREQ (50*1000)
static signed long long wall_clock; //unit in HZ [freq]
unsigned int avs_cnt_last;

#ifdef __LINKFORLINUX
unsigned long ss_ssss_init[4] = {0x58c8d6b5,0x4cb6f930,0x722e480a,0x28bc13fb};
unsigned long ss_ssss[4]; //{0x90ef178a,0x7598479c,0x71afa8da,0x3bb60d56}
/*void get_sys_env()
{
	unsigned long txt_in[4] ={0x58c8d6c6,0x722e480a,0x4cb6f930,0x18bc13fa};
	//{0x90ef178a,0x7598479c,0x71afa8da,0x3bb60d56};//{0x72613c21,0x0000f000,0x0000f001,0x00000000};
	unsigned long res[4];
	//ss_xxxx(txt_in, res, 16);//only for link to export true {0x72613c21,0x0000f000,0x0000f001,0x00000000},
	printf("==================tout: {0x%08x,0x%08x,0x%08x,0x%08x},\n",res[0],res[1],res[2],res[3]);
}*/
#endif


#define __reg_value(addr) cedarx_reg_op(0, addr, 0)
#define __reg_read(addr)  cedarx_reg_op(0, addr, 0)
#define __reg_write(addr, value) cedarx_reg_op(1, addr, value)


int cedarx_reg_op(int rw, int addr, int value)
{
	struct cedarv_regop cedarv_reg;

	cedarv_reg.addr = addr;

	if(rw == 0) { //read
		return ioctl(cedarv_osal_ctx->fd, IOCTL_READ_REG, (unsigned long)&cedarv_reg);
	}
	else { //write
		cedarv_reg.value = value;
		ioctl(cedarv_osal_ctx->fd, IOCTL_WRITE_REG, (unsigned long)&cedarv_reg);
	}

	return 0;
}

int cedarx_hardware_init(int mode)
{
	int ret = 0;

	pthread_mutex_lock(&g_mutex_cedarv_osal);
	if(cedarv_osal_ctx == NULL){
		cedarv_osal_ctx = (cedarv_osal_context_t*)malloc(sizeof(cedarv_osal_context_t));
		if(!cedarv_osal_ctx){
			LOGE("malloc error!");
			pthread_mutex_unlock(&g_mutex_cedarv_osal);
			return -1;
		}

		memset(cedarv_osal_ctx,0,sizeof(cedarv_osal_context_t));

		//cedarv_osal_ctx->total_count++;

		cedarv_osal_ctx->fd = open("/dev/cedar_dev", O_RDWR);
		if(cedarv_osal_ctx->fd == -1){
			LOGE("Open file /dev/cedar_dev failed \n");
			pthread_mutex_unlock(&g_mutex_cedarv_osal);
			return -1;
		}

		ioctl(cedarv_osal_ctx->fd, IOCTL_GET_ENV_INFO, (unsigned long)&cedarv_osal_ctx->env_info);

		cedarv_osal_ctx->env_info.address_macc = (unsigned int)mmap(NULL, 2048, PROT_READ | PROT_WRITE, MAP_SHARED,
				cedarv_osal_ctx->fd, (int)cedarv_osal_ctx->env_info.address_macc);
//		LOGD("@@@@@@@@--address-macc: 0x%p phymem_start: 0x%p phymem_total_size:0x%x\n",cedarv_osal_ctx->env_info.address_macc,
//						cedarv_osal_ctx->env_info.phymem_start,cedarv_osal_ctx->env_info.phymem_total_size);
		wall_clock = 0;
		avs_cnt_last = 0;

		avs_counter_config();
		avs_counter_reset();
		avs_counter_start();

		//ioctl(cedarv_osal_ctx->fd, IOCTL_ENABLE_IRQ, 0);

	#ifdef __LINKFORLINUX
		unsigned long t = ss_ssss_init[1];
		memcpy(ss_ssss,ss_ssss_init,sizeof(unsigned long)*4);

		ss_ssss[1] = ss_ssss[2];//for encyption
		ss_ssss[2] = t;//for encyption

		ss_ssss[0] += 0x11;//for encyption
		ss_ssss[3] -= 0x10000001;
	#endif
#ifdef USE_ION_MEM_ALLOCATOR
		LOGD("use ion_alloc_open");
		ion_alloc_open();
#elif USE_SUNXI_MEM_ALLOCATOR
		LOGD("use sunxi_alloc_open");
		sunxi_alloc_open();
#else
		cdxalloc_open();
#endif

	#if defined (__CHIP_VERSION_F20) //TODO: move it to video decoder

    #else 
		cedarv_set_ve_ref_count(0); //.lys we must reset it to zero to fix refcount error when mediaserver crash
		cedarv_request_ve_core();
	#endif

		cedarv_reset_ve_core(); //reset ve here

	#if defined(__CHIP_VERSION_A31)
		cedarv_set_ve_freq(360);
	#endif

		{ //don't touch below code!
			cedarv_osal_ctx->ve_version = *((unsigned int *)(cedarv_osal_ctx->env_info.address_macc+0xf0)) >> 16;
			if(cedarv_osal_ctx->ve_version == 0x1625)
			{
				int chip_version,chip_id;
				int treg0,treg1,treg2,lcd_on;
				int ddddd = 800+111;
				int eeeee = 600+111;

				chip_version =	(__reg_value(0xf1c15000)>>16)&0x7;
				treg1 = __reg_value(0xf1c20064)&(1<<4); //ahb on
#if 0
				if(chip_version == 1 && treg1)
				{
					treg0 = __reg_value(0xf1c0c048) & 0x07ff;
					treg2 = (__reg_value(0xf1c0c048) & 0x07ff0000)>>16;
					//LOGV("xxxd=0x%x  vvvddddddd=0x%x",treg0, treg2);

					if(treg0 * treg2 > (ddddd-111) *(eeeee-111)) {
						close(cedarv_osal_ctx->fd);
						free(cedarv_osal_ctx);
					}
				}
#else
				chip_id = (__reg_value(0xf1c23808)>>12) & 0x0f;

				if(chip_version == 1 && treg1) {
					/* chip is a13 */
					//resolution constraint
					treg0 = __reg_value(0xf1c0c048) & 0x07ff;
					treg2 = (__reg_value(0xf1c0c048) & 0x07ff0000)>>16;
					//LOGV("xxxd=0x%x  vvvddddddd=0x%x",treg0, treg2);
                    ddddd = 1024+111;
                    eeeee = 768 +111;
					if(treg0 * treg2 > (ddddd-111) *(eeeee-111)) {
						close(cedarv_osal_ctx->fd);
						free(cedarv_osal_ctx);
					}
				} else if (chip_version == 0) {
					if(chip_id == 3 || chip_id == 0) {
						/* chip is a12 */ //nothing to do
#if 1 //modify by xiechr 20120801
						//resolution constraint
						ddddd = 1024; //1024;
                        eeeee = 768+32; //768+32;
						treg0 = __reg_value(0xf1c0c048) & 0x07ff;
						treg2 = (__reg_value(0xf1c0c048) & 0x07ff0000)>>16;
						//LOGV("a12 : xxxd=0x%x  vvvddddddd=0x%x",treg0, treg2);
                        //LOGV("limit_w=0x%x,  limit_h=0x%x", ddddd, eeeee);

						//if(treg0 * treg2 > (ddddd-111) *(eeeee-111)) {
						if(treg0 * treg2 > (ddddd) *(eeeee)) {
							close(cedarv_osal_ctx->fd);
							free(cedarv_osal_ctx);
						}
#endif
					} else if(chip_id == 7 && treg1) {
						/* chip is a10s */
						lcd_on = __reg_value(0xf1c0c040) & (1<<31);
						if(lcd_on) {
							//LCD on, make system error
							close(cedarv_osal_ctx->fd);
							free(cedarv_osal_ctx);
						}
					} else {
						/* unknown chip id, not support, make system error */
						close(cedarv_osal_ctx->fd);
						free(cedarv_osal_ctx);
					}
				} else {
					/* unknown chip, not support, make system error */
					close(cedarv_osal_ctx->fd);
					free(cedarv_osal_ctx);
				}
#endif
			}
#if 0
            else if (cedarv_osal_ctx->ve_version == 0x1623)
            {
                int chip_version,chip_id;
				int treg0,treg2;    //treg1,lcd_on;
				int ddddd = 800+111;
				int eeeee = 600+111;
                int reg_value;

				//chip_version =	(__reg_value(0xf1c15000)>>16)&0x7;
				//treg1 = __reg_value(0xf1c20064)&(1<<4); //ahb on
                reg_value = __reg_value(0xf1c00024);
                __reg_write(0xf1c00024, (chip_version | (1<<15)));
                chip_version =	(__reg_value(0xf1c00024)>>16);
				chip_id = (__reg_value(0xf1c23808)>>12) & 0x0f;
                if (chip_version == 0x1651)
                {
                    /* chip is a20 */
    				//resolution constraint
    				treg0 = __reg_value(0xf1c0c048) & 0x07ff;
    				treg2 = (__reg_value(0xf1c0c048) & 0x07ff0000)>>16;
    				//LOGV("xxxd=0x%x  vvvddddddd=0x%x",treg0, treg2);
                    ddddd = 1024+111;
                    eeeee = 768 +111;
    				if(treg0 * treg2 > (ddddd-111) *(eeeee-111)) {
    					close(cedarv_osal_ctx->fd);
    					free(cedarv_osal_ctx);
    				}
                }
                
            }
#endif
		}

//		{
//			int i;
//			volatile int *ptr = (int*)cedarv_osal_ctx->env_info.address_macc;
//			LOGV("--------- register of macc -----------\n");
//			for(i=0;i<64;i++){
//				LOGV("%08x ",ptr[i]);
//				if(((i+1)%8)==0)
//					LOGV("\n");
//			}
//		}
	}//end if(cedarv_osal_ctx == NULL)

	cedarv_osal_ctx->ref_count++;
	LOGD("init hw ref count:%d",cedarv_osal_ctx->ref_count);

_err_out:
	pthread_mutex_unlock(&g_mutex_cedarv_osal);

	return ret;
}

int cedarx_hardware_exit(int mode)
{
	pthread_mutex_lock(&g_mutex_cedarv_osal);

	cedarv_osal_ctx->ref_count--;
	LOGD("exit hw ref count:%d",cedarv_osal_ctx->ref_count);

	if(cedarv_osal_ctx->ref_count == 0){

#ifndef __CHIP_VERSION_F20 //TODO: move it to video decoder
	cedarv_release_ve_core();
#endif

#ifdef USE_ION_MEM_ALLOCATOR
		ion_alloc_close();
#elif USE_SUNXI_MEM_ALLOCATOR
		sunxi_alloc_close();
#else
		cdxalloc_close();
#endif

		munmap((void *)cedarv_osal_ctx->env_info.address_macc, 2048);

		if(cedarv_osal_ctx->fd != -1){
			close(cedarv_osal_ctx->fd);
		}

		if(cedarv_osal_ctx){
			free(cedarv_osal_ctx);
			cedarv_osal_ctx = NULL;
		}
	}

	pthread_mutex_unlock(&g_mutex_cedarv_osal);

    return 0;
}

void cedarx_register_print(char *func, int line)
{
	int i;
	volatile int *ptr = (int*)cedarv_osal_ctx->env_info.address_macc;

	if(cedarv_osal_ctx != NULL) {
		LOGD("--------- register of macc base:%p Func:%s Line:%d-----------\n",ptr,func,line);
		for(i=0;i<16;i++){
			LOGD("row %2d: %08x %08x %08x %08x",i,ptr[0],ptr[1],ptr[2],ptr[3]);
			ptr += 4;
		}
	}
}

#if (defined(__CHIP_VERSION_A31) || defined(__CHIP_VERSION_A23))
#else
#define IOCTL_WAIT_VE_DE IOCTL_WAIT_VE
#endif

int cedarv_wait_ve_ready()
{
    int    status;

	//printf("wait ve start!\n"); //printf("getchar()!\n");getchar();
	status = ioctl(cedarv_osal_ctx->fd, IOCTL_WAIT_VE_DE, 1);  //.tbd driver timeout set it to -1
	if(status <= 0)
	{   
		//ioctl(cedarv_osal_ctx->fd, IOCTL_RESET_VE, 0);
	}
	//printf("interrupt ret value: 0x%08x\n",status);

	return (status - 1);
}

int cedarv_wait_ve_enc_ready()
{
#ifdef __CHIP_VERSION_A31

    int    status;

	status = ioctl(cedarv_osal_ctx->fd, IOCTL_WAIT_VE_EN, 2);  //.tbd driver timeout set it to -1
	if(status <= 0)
	{
		//* reset the ve.
		ioctl(cedarv_osal_ctx->fd, IOCTL_RESET_VE, 0);
	}

	return (status - 1);
#else
    return cedarv_wait_ve_ready();
#endif
}

void cedarv_disable_ve_core()
{
	ioctl(cedarv_osal_ctx->fd, IOCTL_DISABLE_VE, 0);
}

int cedarv_f23_ic_version()
{
#ifndef	__CHIP_VERSION_F20
	if(ioctl(cedarv_osal_ctx->fd, IOCTL_GET_IC_VER, 0) == 0x0A10000B)
		return 1;
	else
#endif
		return 0;
}

void cedarv_enable_ve_core()
{
	ioctl(cedarv_osal_ctx->fd, IOCTL_ENABLE_VE, 0);
}

void cedarv_request_ve_core()
{
	ioctl(cedarv_osal_ctx->fd, IOCTL_ENGINE_REQ, 0);
}

void cedarv_release_ve_core()
{
	ioctl(cedarv_osal_ctx->fd, IOCTL_ENGINE_REL, 0);
}

void cedarv_set_ve_freq(int freq) //MHz
{
	if(cedarv_osal_ctx->ve_version == 0x1625) {
		if(freq > 240) {
			freq = 240;
		}
	}

	ioctl(cedarv_osal_ctx->fd, IOCTL_SET_VE_FREQ, freq);
}

void cedarv_reset_ve_core()
{
	ioctl(cedarv_osal_ctx->fd, IOCTL_RESET_VE, 0);
}

static void cedarv_set_ve_ref_count(int ref_count)
{
	ioctl(cedarv_osal_ctx->fd, IOCTL_SET_REFCOUNT, ref_count);
}

unsigned int cedarv_get_macc_base_address()
{
	return cedarv_osal_ctx->env_info.address_macc;
}

//===============================================================================================
unsigned int cedarv_address_vir2phy(void *addr)
{
#ifdef USE_ION_MEM_ALLOCATOR
	return ion_alloc_vir2phy(addr);
#elif USE_SUNXI_MEM_ALLOCATOR
	return sunxi_alloc_vir2phy(addr);
#else
	return cdxalloc_vir2phy(addr);
#endif
}

unsigned int cedarv_address_phy2vir(void *addr)
{
#ifdef USE_ION_MEM_ALLOCATOR
	return ion_alloc_phy2vir(addr);
#elif USE_SUNXI_MEM_ALLOCATOR
	return sunxi_alloc_phy2vir(addr);
#else
	return cdxalloc_phy2vir(addr);
#endif
}

unsigned int cedara_address_vir2phy(void *addr)
{
#ifdef USE_ION_MEM_ALLOCATOR
	return ion_alloc_vir2phy(addr);
#elif USE_SUNXI_MEM_ALLOCATOR
	return sunxi_alloc_vir2phy(addr);
#else
	return cdxalloc_vir2phy(addr);
#endif
}

void *cedar_sys_phymalloc(unsigned int size, int align)
{
#ifdef USE_ION_MEM_ALLOCATOR
	return ion_alloc_alloc(size);
#elif USE_SUNXI_MEM_ALLOCATOR
	return sunxi_alloc_alloc(size);
#else
	return cdxalloc_alloc(size);
#endif
}

void *cedar_sys_phymalloc_map(unsigned int size, int align)
{
	LOGD("cedar_sys_phymalloc_map");
#ifdef USE_ION_MEM_ALLOCATOR
	LOGD("cedar_sys_phymalloc_map1");
	return ion_alloc_alloc(size);
#elif USE_SUNXI_MEM_ALLOCATOR
	LOGD("cedar_sys_phymalloc_map2");
	return sunxi_alloc_alloc(size);
#else
	LOGD("cedar_sys_phymalloc_map3");
	return cdxalloc_alloc(size);
#endif
}

void cedar_sys_phyfree(void *buf)
{
#ifdef USE_ION_MEM_ALLOCATOR
	ion_alloc_free(buf);
#elif USE_SUNXI_MEM_ALLOCATOR
	sunxi_alloc_free(buf);
#else
	cdxalloc_free(buf);
#endif
}

void cedar_sys_phyfree_map(void *buf)
{
#ifdef USE_ION_MEM_ALLOCATOR
	ion_alloc_free(buf);
#elif USE_SUNXI_MEM_ALLOCATOR
	sunxi_alloc_free(buf);
#else
	cdxalloc_free(buf);
#endif
}

void *cedara_phymalloc_map(unsigned int size, int align)
{
#ifdef USE_ION_MEM_ALLOCATOR
	return ion_alloc_alloc(size);
#elif USE_SUNXI_MEM_ALLOCATOR
	return sunxi_alloc_alloc(size);
#else
	return cdxalloc_alloc(size);
#endif
}


void cedara_phyfree_map(void *buf)
{
#ifdef USE_ION_MEM_ALLOCATOR
	ion_alloc_free(buf);
#elif USE_SUNXI_MEM_ALLOCATOR
	sunxi_alloc_free(buf);
#else
	cdxalloc_free(buf);
#endif
}

void cedarx_cache_op(long start, long end, int flag)
{
#ifdef USE_ION_MEM_ALLOCATOR
	ion_flush_cache(start, end - start + 1);
#elif USE_SUNXI_MEM_ALLOCATOR
    sunxi_flush_cache(start, end - start + 1);
#else
	cedarv_cache_range cr;

	cr.start = start;
	cr.end = end;
	if((unsigned long)(end - start) > 1024*1024*64) {
		LOGE("flush cache fail, range error!");
		return;
	}

	//LOGV("flush start: 0x%x 0x%x len:%d",start, end, end-start);
	ioctl(cedarv_osal_ctx->fd, IOCTL_FLUSH_CACHE, &cr);
#endif
}

void cedarx_cache_op_all(long start, long end, int flag)
{
#ifdef USE_ION_MEM_ALLOCATOR
	ion_flush_cache_all();
#elif USE_SUNXI_MEM_ALLOCATOR
    sunxi_flush_cache_all();
#else
    int ret = 0;
	ret = ioctl(cedarv_osal_ctx->fd, IOCTL_TEST_VERSION, 0);

	if(end - start > 256*1024 && (ret == NEW_VERSION)) {
		
		ioctl(cedarv_osal_ctx->fd, IOCTL_FLUSH_CACHE_ALL, 0);
	} else {
		cedarx_cache_op(start, end, flag);
	}
#endif
}

//===============================================================================================

unsigned long ss_ssss[4];
unsigned long ss_xxxx(unsigned long* text_in, unsigned long* text_out, unsigned long text_size)
{
//	unsigned long i, j;
//	unsigned long io_buf[16];
//	unsigned long key_size=256;
//	printf("xxxxxxxxxxxxxxxxxxxxxxxxx\n\n\n"); //TODO: add encryption
	text_out[0] = 0x72613c21;
	text_out[1] = 0x0000f000;
	text_out[2] = 0x0000f001;
	text_out[3] = 0x00000000;
	return 1;
}

//===============================================================================================

long long gettimeofday_curr()
{
	struct timeval now;
	gettimeofday(&now, NULL);
	return now.tv_sec * 1000000 + now.tv_usec;
}

void avs_counter_config()
{
	ioctl(cedarv_osal_ctx->fd, IOCTL_CONFIG_AVS2, 0);
}

void avs_counter_start()
{
	ioctl(cedarv_osal_ctx->fd, IOCTL_START_AVS2, 0);
}

void avs_counter_pause()
{
	ioctl(cedarv_osal_ctx->fd, IOCTL_START_AVS2, 0);
}

void avs_counter_reset()
{
	wall_clock = 0;
	ioctl(cedarv_osal_ctx->fd, IOCTL_RESET_AVS2, 0);
}

void avs_counter_adjust_abs(int val){
	ioctl(cedarv_osal_ctx->fd, IOCTL_ADJUST_AVS2_ABS, val);
}

void avs_counter_get_time_us(signed long long *curr)
{
	unsigned int avs_cnt_curr;

	pthread_mutex_lock(&g_mutex_cedarv_osal);

	avs_cnt_curr = ioctl(cedarv_osal_ctx->fd, IOCTL_GETVALUE_AVS2, 0);

	if((avs_cnt_curr ^ avs_cnt_last)>>31) { //wrap around
		wall_clock += 0xffffffffULL;
	}

	avs_cnt_last = avs_cnt_curr;
	*curr = (wall_clock + (unsigned long long)avs_cnt_curr) * (1000000 / WALL_CLOCK_FREQ);//((wall_clock + avs_cnt_curr)*1000 / (WALL_CLOCK_FREQ/1000));

	//printf("avs_cnt_curr:0x%x wall:%lld val:%lld\n",avs_cnt_curr,wall_clock,val);
	pthread_mutex_unlock(&g_mutex_cedarv_osal);
}

