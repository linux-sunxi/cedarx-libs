#ifndef _MACH_AW_GPIO_H_
#define _MACH_AW_GPIO_H_

#define gpio_get_value	__gpio_get_value
#define gpio_set_value	__gpio_set_value
#define gpio_cansleep	__gpio_cansleep
#define gpio_to_irq     __gpio_to_irq

#define AW_GPIO_A_CHIP                  (0)
#define AW_GPIO_B_CHIP                  (1)
#define AW_GPIO_C_CHIP                  (2)
#define AW_GPIO_D_CHIP                  (3)
#define AW_GPIO_E_CHIP                  (4)
#define AW_GPIO_F_CHIP                  (5)
/* gpio bank size */                    
#define AW_GPIO_A_NR                    (20)
#define AW_GPIO_B_NR                    (18)
#define AW_GPIO_C_NR                    (20)
#define AW_GPIO_D_NR                    (28)
#define AW_GPIO_E_NR                    (12)
#define AW_GPIO_F_NR                    (1) 

/* register define */
#define GPIO_REG_BASE                   0x01c20800
#define GPIO_REG_VA_BASE    

enum aw15_gpio_reg_offset
{
    AW_GPIO_A_REG_OS = 0x00,
    AW_GPIO_B_REG_OS = 0x20,
    AW_GPIO_C_REG_OS = 0x40,
    AW_GPIO_D_REG_OS = 0x60,
    AW_GPIO_E_REG_OS = 0x84,
    AW_GPIO_F_REG_OS = 0x98,
};
#define AW_GPIO_BASE(_gpn)              (VA_PIO_BASE + _gpn##_REG_OS)

#define AW_GPIO_CFG_OS(_chip)           0x00                                                    // config 0 offset
#define AW_GPIO_DAT_OS(_chip)           (AW_GPIO_CFG_OS(_chip) + 4 + ((_chip->chip.ngpio)/8)*4)      // data offset
#define AW_GPIO_DRV_OS(_chip)           (AW_GPIO_DAT_OS(_chip) + 4)                             // driver 0 offset
#define AW_GPIO_PULL_OS(_chip)          (AW_GPIO_DRV_OS(_chip) + 4 + ((_chip->chip.ngpio)/16)*4)     // pull 0 offset

#define AW_GPIO_CFG(_chip, _offs)       (_chip->base + AW_GPIO_CFG_OS(_chip)  + ((_offs)/8)*4)
#define AW_GPIO_DAT(_chip, _offs)       (_chip->base + AW_GPIO_DAT_OS(_chip))
#define AW_GPIO_DRV(_chip, _offs)       (_chip->base + AW_GPIO_DRV_OS(_chip)  + ((_offs)/16)*4)
#define AW_GPIO_PULL(_chip, _offs)      (_chip->base + AW_GPIO_PULL_OS(_chip) + ((_offs)/16)*4)

#define GPIOF_REG_DRV         			(VA_PIO_BASE + 0x98)
#define GPIOF_REG_PULLUP      			(VA_PIO_BASE + 0x9c)

#define GPIO_REG_INT         			(VA_PIO_BASE + 0xa0)

#define AW_GPIO_NEXT(__gpio)            ((__gpio##_START) + (__gpio##_NR))

enum aw_gpio_number {
	AW_GPIO_A_START = 0,
	AW_GPIO_B_START = AW_GPIO_NEXT(AW_GPIO_A),
	AW_GPIO_C_START = AW_GPIO_NEXT(AW_GPIO_B),
	AW_GPIO_D_START = AW_GPIO_NEXT(AW_GPIO_C),
	AW_GPIO_E_START = AW_GPIO_NEXT(AW_GPIO_D),
	AW_GPIO_F_START = AW_GPIO_NEXT(AW_GPIO_E)
};

/* AW GPIO number definitions. */
#define AW_GPA(_nr)                     (AW_GPIO_A_START + (_nr))
#define AW_GPB(_nr)                     (AW_GPIO_B_START + (_nr))
#define AW_GPC(_nr)                     (AW_GPIO_C_START + (_nr))
#define AW_GPD(_nr)                     (AW_GPIO_D_START + (_nr))
#define AW_GPE(_nr)                     (AW_GPIO_E_START + (_nr))
#define AW_GPF(_nr)                     (AW_GPIO_F_START + (_nr))

/* the end of the awxx specific gpios */
#define AW15_GPIO_END	                (AW_GPF(AW_GPIO_F_NR) + 1)
#define AW_GPIO_END                     AW15_GPIO_END

/* define the number of gpios we need to the one after the GPQ() range */
#define ARCH_NR_GPIOS	                (AW_GPF(AW_GPIO_F_NR) + 1)

extern unsigned aw_gpio_getcfg(unsigned gpio);
extern int aw_gpio_setcfg(unsigned gpio, unsigned config);
extern unsigned aw_gpio_get_pull(unsigned gpio);
int aw_gpio_set_pull(unsigned gpio, unsigned pull);
extern unsigned aw_gpio_get_drv_level(unsigned gpio);
extern int aw_gpio_set_drv_level(unsigned gpio, unsigned level);

#include <asm-generic/gpio.h>

#endif
