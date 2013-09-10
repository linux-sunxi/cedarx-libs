
#include "libve_adapter.h"
#include "fbm.h"
#include "vbv.h"
#include "os_adapter.h"
#include "vdecoder_config.h"

#define INTC_IRQNO_MACC         (INTC_IRQNO_VE)

//****************************************************************************//
//************************ Instance of FBM Interface *************************//
//****************************************************************************//
IFBM_t IFBM = 
{
    fbm_release,
    fbm_decoder_request_frame,
    fbm_decoder_return_frame,
    fbm_decoder_share_frame,
    fbm_init_ex
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
    sys_print,
    sys_sleep,
};


//****************************************************************************//
//************************ Instance of OS Interface **************************//
//****************************************************************************//

static __krnl_event_t*	sem_ve_clk_adjust = NULL;
static __hdle           h_ve_dram_gate    = 0;
static __hdle           h_ve_ahb_gate     = 0;
static __hdle           h_ve_clock        = 0;
static __hdle           h_ve_sram         = 0;
static __krnl_event_t*  sem_intr          = NULL;

void ve_init_clock(void);
void ve_release_clock(void);
s32  ve_interrupt_service(void);

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
    //disable macc to access sdram
    esCLK_MclkOnOff(h_ve_dram_gate, CLK_OFF);

    //disable ve clock
    if(h_ve_clock)
    {
        esCLK_MclkReset(h_ve_clock, 0);
        esCLK_MclkReset(h_ve_clock, 1);
    }

    //enable macc to access sdram
    esCLK_MclkOnOff(h_ve_dram_gate, CLK_ON);

    return;
}


static void ve_enable_clock(u8 enable, u32 speed)
{
    if(enable)
    {
        if(h_ve_clock)
        {
            //enable ve module clock
            esCLK_MclkOnOff(h_ve_clock, CLK_ON);
    
            //the clock should be multiple of 6Mhz
            speed /= 6*1000*1000;
            speed *= 6*1000*1000;
            esCLK_SetSrcFreq(CCMU_SYS_CLK_VE_PLL, speed);
        }
    }
    else
    {
        if(h_ve_clock)
        {
            //enable ve module clock
            esCLK_MclkOnOff(h_ve_clock, CLK_OFF);
        }
    }
}


static void ve_enable_intr(u8 enable)
{
	if(enable)
	{
	    esINT_EnableINT(INTC_IRQNO_MACC);
	}
	else
	{
	    esINT_DisableINT(INTC_IRQNO_MACC);
	}
}


static s32 ve_wait_intr(void)
{
	u8 err;

	esKRNL_SemPend(sem_intr, 50, &err);
	
	if(err != 0)
		return -1;
	else
		return 0;
}


static u32 ve_get_reg_base_addr(void)
{
    return 0xf1c0e000;	//* physic address is 0x01c0e000.
}


static memtype_e ve_get_memtype(void)
{
    __dram_para_t   tmpDramPara;
    esKSRV_GetDramCfgPara(&tmpDramPara);
    
    switch(tmpDramPara.dram_type)
    {
        case DRAM_TYPE_DDR:
        {
            return MEMTYPE_DDR1_16BITS;
        }
        case DRAM_TYPE_DDR2:
        {
            return MEMTYPE_DDR2_32BITS;
        }
        case DRAM_TYPE_DDR3:
        {
            return MEMTYPE_DDR3_32BITS;
        }
        default:
        {  
            return MEMTYPE_DDR2_32BITS;
        }
    }
}


static s32 cb_ve_clock_change(u32 cmd, s32 aux)
{
    switch(cmd)
    {
        case CLK_CMD_SCLKCHG_REQ:
       {
            u8 err;

            //* request semaphore to check if it can adjust speed now
            esKRNL_SemPend(sem_ve_clk_adjust, 0, &err);
            return EPDK_OK;
        }

        case CLK_CMD_SCLKCHG_DONE:
        {
            //* release semaphore
            esKRNL_SemPost(sem_ve_clk_adjust);
            return EPDK_OK;
        }

        default:
            return EPDK_FAIL;
    }
}

void ve_init_clock(void)
{
    u8 err;

    //* create semaphore for VE interrupt.
    sem_intr = esKRNL_SemCreate(0);
    if(sem_intr == NULL)
    	return;

    //* create semaphore to sync adjust clock operation
    sem_ve_clk_adjust = esKRNL_SemCreate(1);
    if(sem_ve_clk_adjust == NULL)
    {
    	esKRNL_SemDel(sem_intr, OS_DEL_ALWAYS, &err);
    	sem_intr = NULL;
        return;
    }

    //* request handle of VE module clock
    h_ve_clock = esCLK_OpenMclk(CCMU_MOD_CLK_VE);
    esCLK_MclkRegCb(CCMU_MOD_CLK_VE, cb_ve_clock_change);

    //* request handle of AHB to VE clock gate.
    h_ve_ahb_gate = esCLK_OpenMclk(CCMU_MOD_CLK_AHB_VE);

    //* request handle of DRAM to VE clock gate.
    h_ve_dram_gate = esCLK_OpenMclk(CCMU_MOD_CLK_SDRAM_VE);

    //* set MACC clock source to video PLL.
    esCLK_SetMclkSrc(h_ve_clock, CCMU_SYS_CLK_VE_PLL);
    esCLK_SetMclkDiv(h_ve_clock, 1);

    //* switch SRAM to MACC.
    h_ve_sram = esMEM_SramReqBlk(CSP_SRAM_ZONE_C1, SRAM_REQ_MODE_WAIT);
    esMEM_SramSwitchBlk(h_ve_sram, CSP_SRAM_MODULE_VE);

    //* enable MACC module clock
    esCLK_MclkOnOff(h_ve_clock, CLK_ON);
    esCLK_MclkOnOff(h_ve_ahb_gate, CLK_ON);
    esCLK_MclkOnOff(h_ve_dram_gate, CLK_ON);

	esINT_InsISR(INTC_IRQNO_MACC, (__pISR_t)ve_interrupt_service, 0);

    return;
}


void ve_release_clock(void)
{
    u8 err;
    
    //register call-back fucntion for adjust ve clock
    esCLK_MclkUnregCb(CCMU_MOD_CLK_VE, cb_ve_clock_change);

    //* release handle of VE clock gate.
    if(h_ve_clock)
    {
        esCLK_MclkOnOff(h_ve_clock, CLK_OFF);
        esCLK_CloseMclk(h_ve_clock);
        h_ve_clock = 0;
    }

    //* release handle of AHB to VE clock gate.
    if(h_ve_ahb_gate)
    {
        esCLK_MclkOnOff(h_ve_ahb_gate, CLK_OFF);
        esCLK_CloseMclk(h_ve_ahb_gate);
        h_ve_ahb_gate = 0;
    }

    //* release handle of DRAM to VE clock gate.
    if(h_ve_dram_gate)
    {
        esCLK_MclkOnOff(h_ve_dram_gate, CLK_OFF);
        esCLK_CloseMclk(h_ve_dram_gate);
        h_ve_dram_gate = 0;
    }

    //* delete semaphore
    if(sem_ve_clk_adjust)
    {
        esKRNL_SemDel(sem_ve_clk_adjust, OS_DEL_ALWAYS, &err);
        sem_ve_clk_adjust = NULL;
    }

	esINT_UniISR(INTC_IRQNO_MACC, (__pISR_t)ve_interrupt_service);
	
    if(sem_intr)
    {
    	esKRNL_SemDel(sem_intr, OS_DEL_ALWAYS, &err);
    	sem_intr = NULL;
    }
    
    if(h_ve_sram)
    {
        esMEM_SramRelBlk(h_ve_sram);
        h_ve_sram = 0;
    }

    return;
}


s32 ve_interrupt_service(void)
{
    volatile u32* ve_top_reg;
    volatile u32* ve_intr_ctrl_reg;
    u32 		  mode;
    OS_SEM_DATA   sem_data;

    ve_top_reg = (u32*)ve_get_reg_base_addr();

    mode = (*ve_top_reg) & 0xf;
    
	/* estimate Which video format */
    switch (mode)
    {
        case 0: //mpeg124
            //ve_status_reg = (int *)(addrs.regs_macc + 0x100 + 0x1c);
            ve_intr_ctrl_reg = (u32 *)(((u32)ve_top_reg) + 0x100 + 0x14);
            break;
        case 1: //h264
            //ve_status_reg = (int *)(addrs.regs_macc + 0x200 + 0x28);
            ve_intr_ctrl_reg = (u32 *)(((u32)ve_top_reg) + 0x200 + 0x20);
            break;
        case 2: //vc1
            //ve_status_reg = (int *)(addrs.regs_macc + 0x300 + 0x2c);
            ve_intr_ctrl_reg = (u32 *)(((u32)ve_top_reg) + 0x300 + 0x24);
            break;
        case 3: //rmvb
            //ve_status_reg = (int *)(addrs.regs_macc + 0x400 + 0x1c);
            ve_intr_ctrl_reg = (u32 *)(((u32)ve_top_reg) + 0x400 + 0x14);
            break;
        default:
            //ve_status_reg = (int *)(addrs.regs_macc + 0x100 + 0x1c);
            ve_intr_ctrl_reg = (u32 *)(((u32)ve_top_reg) + 0x100 + 0x14);
            break;
    }

    //disable interrupt
    if(mode == 0)
        *ve_intr_ctrl_reg = *ve_intr_ctrl_reg & (~0x7C);
    else
        *ve_intr_ctrl_reg = *ve_intr_ctrl_reg & (~0xF);
	

	if(esKRNL_SemQuery(sem_intr, &sem_data) == 0)
	{
		if(sem_data.OSCnt == 0)
			esKRNL_SemPost(sem_intr);
	}
    
    return 0;
}

