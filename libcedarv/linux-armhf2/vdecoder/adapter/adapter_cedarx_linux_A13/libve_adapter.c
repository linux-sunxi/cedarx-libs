
#include "libve_adapter.h"
#include "fbm.h"
#include "vbv.h"
#include "os_adapter.h"
#include "vdecoder_config.h"

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
    return MEMTYPE_DDR3_32BITS; //TODO:
}


void ve_init_clock(void)
{
//    u8 err;
//
//    //* create semaphore for VE interrupt.
//    sem_intr = esKRNL_SemCreate(0);
//    if(sem_intr == NULL)
//    	return -1;
//
//    //* create semaphore to sync adjust clock operation
//    sem_ve_clk_adjust = esKRNL_SemCreate(1);
//    if(sem_ve_clk_adjust == NULL)
//    {
//    	esKRNL_SemDel(sem_intr, OS_DEL_ALWAYS, &err);
//    	sem_intr = NULL;
//        return -1;
//    }
//
//    //* request handle of VE module clock
//    h_ve_clock = esCLK_Open(CCMU_MCLK_MACC, cb_ve_clock_change, &err);
//
//    //* request handle of AHB to VE clock gate.
//    h_ve_ahb_gate = esCLK_Open(CCMU_MCLK_AHB_MACC, 0, &err);
//
//    //* request handle of DRAM to VE clock gate.
//    h_ve_dram_gate = esCLK_Open(CCMU_MCLK_DRAM_MACC, 0, &err);
//
//    //* set MACC clock source to video PLL.
//    esCLK_SetFreq(h_ve_clock, CCMU_SCLK_MACCPLL, 1);
//
//    //* switch SRAM to MACC.
//    SRAM_REG_CFG &= 0x000000ff;
//
//    //* enable MACC module clock
//    esCLK_OnOff(h_ve_clock, CLK_ON);
//    esCLK_OnOff(h_ve_ahb_gate, CLK_ON);
//    esCLK_OnOff(h_ve_dram_gate, CLK_ON);
//
//	esINT_InsISR(INTC_IRQNO_MACC, (__pISR_t)ve_interrupt_service, 0);

    return;
}


void ve_release_clock(void)
{
//    u8 err;
//
//    //* release handle of VE clock gate.
//    if(h_ve_clock)
//    {
//        esCLK_OnOff(h_ve_clock, CLK_OFF);
//        esCLK_Close(h_ve_clock);
//        h_ve_clock = NULL;
//    }
//
//    //* release handle of AHB to VE clock gate.
//    if(h_ve_ahb_gate)
//    {
//        esCLK_OnOff(h_ve_ahb_gate, CLK_OFF);
//        esCLK_Close(h_ve_ahb_gate);
//        h_ve_ahb_gate = NULL;
//    }
//
//    //* release handle of DRAM to VE clock gate.
//    if(h_ve_dram_gate)
//    {
//        esCLK_OnOff(h_ve_dram_gate, CLK_OFF);
//        esCLK_Close(h_ve_dram_gate);
//        h_ve_dram_gate = 0;
//    }
//
//    //* delete semaphore
//    if(sem_ve_clk_adjust)
//    {
//        esKRNL_SemDel(sem_ve_clk_adjust, OS_DEL_ALWAYS, &err);
//        sem_ve_clk_adjust = NULL;
//    }
//
//    if(sem_intr)
//    {
//    	esKRNL_SemDel(sem_intr, OS_DEL_ALWAYS, &err);
//    	sem_intr = NULL;
//    }
//
//	esINT_UniISR(INTC_IRQNO_MACC, (__pISR_t)ve_interrupt_service);

    return;
}

