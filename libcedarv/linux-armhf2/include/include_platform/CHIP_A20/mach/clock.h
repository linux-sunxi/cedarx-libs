#ifndef __ARCH_ARM_OMAP_CLOCK_H
#define __ARCH_ARM_OMAP_CLOCK_H

struct module;
struct clk;
typedef struct clk mux_t;

struct mux_sorces{
	struct clk*		port;
	int			reg_val;
};

struct mux_selecter{
	struct mux_sorces	src[3];
};

struct clk {
	struct list_head	child_list;
	struct list_head	sibling_list;
	struct module		*owner;
	const char		*name;
	int			id;
	struct clk		*parent;
	unsigned long		freq;
	__u32			flags;
	__s8			usecount;
	
	union{
		struct{
			void __iomem		*freq_reg;
			__u8			freq_bit;
			__u8			freq_bitsize;
			__u8			div_max;
			__u8			enable_bit;
			void __iomem		*enable_reg;
			int		(*set_freq)(struct clk *, unsigned long);
		} c;
		struct{
			void __iomem		*select_reg;
			__u8			select_bit;
			__u8			select_bitsize;
			__u8			reserve0;
			__u8			reserve1;
			struct mux_selecter	*mux_sel;
			int		(*set_mux)(struct clk *, char *);
		} m;
	} u;
	int			(*enable)(struct clk *);
	void			(*disable)(struct clk *);
#if defined(CONFIG_DEBUG_FS)
	struct dentry		*dent;	/* For visible tree hierarchy */
#endif
};


struct cpufreq_frequency_table;

struct clk_functions {
	int		(*clk_enable)(struct clk *clk);
	void		(*clk_disable)(struct clk *clk);
	int		(*clk_set_mux)(struct clk *clk, char* name);
	int		(*clk_set_freq)(struct clk *clk, unsigned long rate);
#ifdef CONFIG_CPU_FREQ
	void		(*clk_init_cpufreq_table)(struct cpufreq_frequency_table **);
#endif
};

/* Clock flags */
#define CF_MUX		(1 << 0)	/* is a clk mux */
#define CF_HALT		(1 << 1)	/* if the freq is changed/set to an invalid rate, CF_HALT will be set */
#define CF_DIVEXP	(1 << 2)	/* freq div is like 1/2/4/8/16...*/
#define CF_DIV		(0 << 2)	/* freq div is like 1/2/3/4/5/6... */
#define CF_PROPAGATES	(1 << 3)	/* freq change broad cast to its sub tree */

typedef enum __CCMU_ERR
{
	CCMU_VALID = 0                    ,
	CCMU_ZERO_FREQ                          ,
	CCMU_INVALID_SRC                        ,
	CCMU_TOO_SMALL_DIV                      ,
	CCMU_TOO_BIG_DIV                        ,
	CCMU_INVALID_MCLK                       ,
	CCMU_CORRESPODING_GATTING_OFF           ,
	CCMU_READ_DIV_ERROR                     ,
	CCMU_READ_MODULE_SRC_ERROR              ,
	CCMU_LOSC_DISABLE                       ,
	CCMU_HOSC_DISABLE                       ,
	CCMU_COREPLL_DISABLE                    ,
	CCMU_IDUPLL_DISABLE                     ,
	CCMU_IDUGATE_DISABLE                    ,
	CCMU_AUDIOPLL_DISABLE                   ,
	CCMU_VIDEOPLL_DISABLE                   ,
	CCMU_DRAMPLL_DISABLE                    ,
	CCMU_READ_FACTOR_ERROR                  ,
	CCMU_COREPLL_FACTOR_ERROR               ,

	CCMU_COREPLL_BYPASS_ON_FREQ_NONSTANDARD ,
	CCMU_COREPLL_BYPASS_ON_FACTOR_ERROR     ,
	CCMU_COREPLL_BYPASS_OFF_FREQ_LOW        ,
	CCMU_COREPLL_BYPASS_OFF_FREQ_HIGH       ,
	CCMU_COREPLL_BYPASS_OFF_FREQ_NONSTANDARD,

	CCMU_IDUPLL_BYPASS_ON_FREQ_NONSTANDARD  ,
	CCMU_IDUPLL_BYPASS_ON_FACTOR_ERROR     ,
	CCMU_IDUPLL_BYPASS_OFF_FREQ_LOW         ,
	CCMU_IDUPLL_BYPASS_OFF_FREQ_HIGH        ,
	CCMU_IDUPLL_BYPASS_OFF_FREQ_NONSTANDARD ,

	CCMU_UNABLE_CLOSE_USB                   ,

	CCMU_AUDIOPLL_NONSTANDARD               ,

	CCMU_VIDEOPLL_FREQ_LOW                  ,
	CCMU_VIDEOPLL_FREQ_HIGH                 ,
	CCMU_VIDEOLL_FREQ_NONSTANDARD           ,

	CCMU_DRAMPLL_FREQ_LOW                   ,
	CCMU_DRAMPLL_BYPASS_OFF_FREQ_LOW        ,
	CCMU_DRAMPLL_BYPASS_OFF_FREQ_HIGH       ,
	CCMU_DRAMPLL_BYPASS_OFF_FREQ_NONSTANDARD,

	CCMU_AHBCLK_NONSTANDARD                 ,

	CCMU_APBCLK_NONSTANDARD                 ,

	CCMU_AC320CLK_NONSTANDARD               ,

	CCMU_CORECLK_NONSTANDARD

}__ccmu_err_t;

extern int clk_enable(struct clk *clk);
extern int clk_enable(struct clk *clk);
extern void clk_disable(struct clk *clk);
extern void clk_put(struct clk *clk);
extern struct clk * clk_get(struct device *dev, const char *id);
extern int clk_set_rate(struct clk *clk, unsigned long rate);
extern int clk_set_mux(struct clk *clk, char* name);
extern int __init aw_clk_init(void);

#endif
