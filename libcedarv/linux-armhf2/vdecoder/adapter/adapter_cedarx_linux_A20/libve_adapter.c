
#include "libve_adapter.h"
#include "os_adapter.h"
#include "vdecoder_config.h"

#ifdef __OS_ANDROID
#include <cutils/properties.h>
#define PROP_CHIP_VERSION_KEY  "media.cedarx.chipver"
#endif


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

void ve_init_clock(void);
void ve_release_clock(void);

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
	cedarv_reset_ve_core();
}


static void ve_enable_clock(u8 enable, u32 speed)
{
    if(enable)
    {
    	cedarv_enable_ve_core();
    	//cedarv_set_ve_freq(speed / 1000000);
    }
    else
    {
    	cedarv_disable_ve_core();
    }
}


static void ve_enable_intr(u8 enable)
{
	;
}


static s32 ve_wait_intr(void)
{
	return cedarv_wait_ve_ready();
}


static u32 ve_get_reg_base_addr(void)
{
    return cedarv_get_macc_base_address();	//* physic address is 0x01c0e000.
}


static memtype_e ve_get_memtype(void)
{
#ifdef __OS_ANDROID
	char prop_value[4];
	int chip_ver;
	property_get(PROP_CHIP_VERSION_KEY, prop_value, "3");
	chip_ver = atoi(prop_value);
	if(chip_ver == 5) {
		return MEMTYPE_DDR3_16BITS; //TODO:
	}
#endif
    return MEMTYPE_DDR3_32BITS; //TODO:
}


void ve_init_clock(void)
{
    return;
}


void ve_release_clock(void)
{
    return;
}

