/*
 *  arch/arm/mach-softwinner/include/mach/irqs.h
 *
 *  Copyright (C) 2003 ARM Limited
 *  Copyright (C) 2000 Deep Blue Solutions Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <mach/platform.h>

#define NR_IRQS			64

/*----------- interrupt register list -------------------------------------------*/
                                         

/* registers */                            
                                          
/* mask */             
#define INTC_START		 0
#define INTC_IRQNO_FIQ           0  
#define INTC_IRQNO_UART0         1   
#define INTC_IRQNO_UART1         2  
#define INTC_IRQNO_UART2	 3  
#define INTC_IRQNO_UART3	 4  
#define INTC_IRQNO_RESERVE0      5  
#define INTC_IRQNO_IR            6  
#define INTC_IRQNO_TWI0          7  
#define INTC_IRQNO_TWI1          8  
#define INTC_IRQNO_RESERVE1      9  
#define INTC_IRQNO_SPI0          10
#define INTC_IRQNO_SPI1          11
#define INTC_IRQNO_RESERVE2      12  
#define INTC_IRQNO_SPDIF         13
#define INTC_IRQNO_AC97          14
#define INTC_IRQNO_TS            15
#define INTC_IRQNO_IIS           16 
#define INTC_IRQNO_RESERVE3      17 
#define INTC_IRQNO_RESERVE4      18 
#define INTC_IRQNO_RESERVE5      19 
#define INTC_IRQNO_RESERVE6      20 
#define INTC_IRQNO_PS2           21 
#define INTC_IRQNO_TIMER0        22 
#define INTC_IRQNO_TIMER1        23 
#define INTC_IRQNO_TIMER24       24 
#define INTC_IRQNO_TIMER5        25 
#define INTC_IRQNO_RESERVE7      26
#define INTC_IRQNO_DMA           27
#define INTC_IRQNO_PIO           28
#define INTC_IRQNO_TP            29
#define INTC_IRQNO_ACODEC        30
#define INTC_IRQNO_LRADC         31

#define INTC_IRQNO_SDMMC0        32
#define INTC_IRQNO_SDMMC1        33
#define INTC_IRQNO_SDMMC2        34
#define INTC_IRQNO_SDMMC3        35
#define INTC_IRQNO_MSTICK        36
#define INTC_IRQNO_NAND          37
#define INTC_IRQNO_USB0          38
#define INTC_IRQNO_USB1          39
#define INTC_IRQNO_RESERVE8      40
#define INTC_IRQNO_RESERVE9      41
#define INTC_IRQNO_CSI           42
#define INTC_IRQNO_TVECODE       43
#define INTC_IRQNO_LCDC          44
#define INTC_IRQNO_DE            45
#define INTC_IRQNO_SCALACC       46
#define INTC_IRQNO_RESERVE10     47
#define INTC_IRQNO_MACC          48
#define INTC_IRQNO_RESERVE11     49
#define INTC_IRQNO_SS            50
#define INTC_IRQNO_EMAC          51
#define INTC_IRQNO_SIRQ0         52
#define INTC_IRQNO_SIRQ1         53
#define INTC_IRQNO_RESERVE12     54
#define INTC_IRQNO_RESERVE13     55
#define INTC_IRQNO_RESERVE14     56
#define INTC_IRQNO_RESERVE15     57
#define INTC_IRQNO_RESERVE16     58
#define INTC_IRQNO_RESERVE17     59
#define INTC_IRQNO_RESERVE18     60
#define INTC_IRQNO_RESERVE19     61
#define INTC_IRQNO_RESERVE20     62
#define INTC_END		 63

