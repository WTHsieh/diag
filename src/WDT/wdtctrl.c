#include <platform.h>
#include <irqs.h>
#include "regs-wdt.h"
#if defined (CONFIG_SQ8000)
#include <scu.h>
#endif

extern int
wdt_watching(int autotest)
{
	int ret = 0, pseudo_flag = 0;

#if defined(CONFIG_CDK) || defined(CONFIG_PC9002) || defined(CONFIG_SCDK)
	iowrite32(ioread32(SQ_APB0_SCU + 0x28) | (0x1 << 13), SQ_APB0_SCU + 0x28);
#endif
#ifdef CONFIG_SQ8000
	sq_scu_wdt_reset_enable(1);
#endif 

	// set reload reg, prescaler
	iowrite32(sq_get_apb_clock() / 60 * 3, SQ_WDTLR);
	WDT_PRESCALE(PRESCALE_64);

	printf("WDT: The system will reset now!!\n");

	WDT_RST_EN();
	WDT_EN();

	if (sq_wait_for_int(&pseudo_flag, 5)) {
		printf("Timeout!! The system does not reset!!\n");
		ret = -1;
	}

	return ret;
}
