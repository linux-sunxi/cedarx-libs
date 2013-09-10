/*
 *  arch/arm/mach-softwinner/include/mach/system.h
 *
 *  Copyright (C) 2003 ARM Limited
 *  Copyright (C) 2000 Deep Blue Solutions Ltd
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
#ifndef __ASM_ARCH_SYSTEM_H
#define __ASM_ARCH_SYSTEM_H

#include <linux/io.h>
#include <mach/hardware.h>
#include <mach/platform.h>
#include <mach/tmr_regs.h>
#include <asm/proc-fns.h>

static inline void arch_idle(void)
{
	/*
	 * This should do all the clock switching
	 * and wait for interrupt tricks
	 */
	cpu_do_idle();
}

static inline void arch_reset(char mode)
{
	u32 val;

	val = __raw_readl(TMRC_REG_TM2MOD);
	val = val | 0x03;   //enable watch dog reset and watchdog
	val = val & ~(7 << 3);  //0.5 second
	__raw_writel(val, TMRC_REG_TM2MOD);

	while(1);
	
}
extern struct sysdev_class sw_sysclass;

#endif
