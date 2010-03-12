#include <test_item.h>
#include <genlib.h>
#include <global.h>
#include "timer-regs.h"
#include <platform.h>
#include "dependency.h"

#if defined (CONFIG_SQ8000)
#include <scu.h>
#include "../../platform/arch/scu-reg.h"
#endif

static u32
sq_timer_read(u32 reg, u32 base)
{
	u32 val;

	base = base + reg;
	val = ioread32(base);
	return val;
}

static void
sq_timer_write(u32 reg, u32 val, u32 base)
{
	base = base + reg;
	iowrite32(val, base);
}

static void sq_timer_isr_0(void *param);
static void sq_timer_isr_1(void *param);
static void sq_timer_isr_2(void *param);

static u32 sq_timer_base;
static u32 sq_timer_conf[3];
static u32 sq_timer_cur_idx;
static u32 sq_timer_apb_clk;
static volatile u32 sq_timer_sec;
static u32 sq_timer_chain_mode = 0;
static int sq_timer_int_flag = 0;

extern struct test_item_container sq_timer_main_test_container;

extern int
TIMERTesting(int autotest)
{
	int ret = 0;

#ifdef CONFIG_SQ8000
	sq_scu_dev_enable(SCU_DEVCON_TMR_GPIO);
#endif

	sq_timer_base = SQ_APB0_TIMER;
	sq_timer_apb_clk = sq_get_apb_clock();
	printf("Sq Timer: current apb clock is %d\n", sq_timer_apb_clk);

	/* Disable the timers */
	sq_timer_write(SQ_TMR0CON, SQ_TIMER_DIS, sq_timer_base);
	sq_timer_write(SQ_TMR1CON, SQ_TIMER_DIS, sq_timer_base);
	sq_timer_write(SQ_TMR2CON, SQ_TIMER_DIS, sq_timer_base);

	/* Register the interrupt service routines */
	request_irq(SQ_INTC_TMR0_0, sq_timer_isr_0, NULL);
	request_irq(SQ_INTC_TMR0_1, sq_timer_isr_1, NULL);
	request_irq(SQ_INTC_TMR0_2, sq_timer_isr_2, NULL);

	ret = test_item_ctrl(&sq_timer_main_test_container, autotest);

	/* Unregister the interrupt service routines */
	free_irq(SQ_INTC_TMR0_0);
	free_irq(SQ_INTC_TMR0_1);
	free_irq(SQ_INTC_TMR0_2);

#ifdef CONFIG_SQ8000
	sq_scu_dev_disable(SCU_DEVCON_TMR_GPIO);
#endif

	return ret;
}

extern struct test_item_container sq_timer_mode_0_timer_test_container;

extern int 
sq_timer_mode_0_test(int autotest)
{
	int ret = 0;

	extern struct test_item sq_timer_main_test_items[];

	sq_timer_chain_mode = 0;

	/* If chain mode is used, we need to reset mode to 0 */
	if ((1 == sq_timer_main_test_items[1].enable) ||
	    (1 == sq_timer_main_test_items[2].enable))
		sq_timer_write(SQ_TMRMODE, SQ_TIMER_CHAIN_MODE_0, sq_timer_base);

	ret = test_item_ctrl(&sq_timer_mode_0_timer_test_container, autotest);
	return ret;
}

extern int sq_timer_mode_1(int autotest)
{
	int err_flag = 0;

	sq_timer_chain_mode = 1;
	sq_timer_write(SQ_TMRMODE, SQ_TIMER_CHAIN_MODE_1, sq_timer_base);
	sq_timer_conf[0] = SQ_TIMER_EN |
		SQ_TIMER_CNT_MODE_PERIODICAL |
		SQ_TIMER_PRESCALE_FACTOR_1_1 |
		SQ_TIMER_INT_MASK_DIS |
		SQ_TIMER_INT_EDGE_TRIG;
	sq_timer_conf[1] = SQ_TIMER_EN |
		SQ_TIMER_CNT_MODE_FREE_RUNNING |
		SQ_TIMER_PRESCALE_FACTOR_1_1 |
		SQ_TIMER_INT_MASK_DIS |
		SQ_TIMER_INT_LEVEL_TRIG;
	sq_timer_write(SQ_TMR0LR, sq_timer_apb_clk*2, sq_timer_base);
	sq_timer_write(SQ_TMR1LR, 2, sq_timer_base);
	sq_timer_sec = 0;
	printf("Timer will count 4 seconds at once\n");
	sq_timer_int_flag = 0;

	/* Enable the timer */
	sq_timer_write(SQ_TMR0CON, sq_timer_conf[0], sq_timer_base);
	sq_timer_write(SQ_TMR1CON, sq_timer_conf[1], sq_timer_base);

	if (sq_wait_for_int(&sq_timer_int_flag, 10)) {
		printf("Timeout\n");
		err_flag = -1;
	} else
		printf("%d second\n", sq_timer_sec);

	/* Disable the timer 0*/
	sq_timer_write(SQ_TMR0CON, SQ_TIMER_DIS, sq_timer_base);

	/* Disable the timer 1*/
	sq_timer_write(SQ_TMR1CON, SQ_TIMER_DIS, sq_timer_base);

	return err_flag;
}

extern int sq_timer_mode_2(int autotest)
{
	int err_flag = 0;

	sq_timer_chain_mode = 2;
	sq_timer_write(SQ_TMRMODE, SQ_TIMER_CHAIN_MODE_2, sq_timer_base);
	sq_timer_conf[0] = SQ_TIMER_EN |
		SQ_TIMER_CNT_MODE_PERIODICAL |
		SQ_TIMER_PRESCALE_FACTOR_1_1 |
		SQ_TIMER_INT_MASK_DIS |
		SQ_TIMER_INT_EDGE_TRIG;
	sq_timer_conf[1] = SQ_TIMER_EN |
		SQ_TIMER_CNT_MODE_PERIODICAL |
		SQ_TIMER_PRESCALE_FACTOR_1_1 |
		SQ_TIMER_INT_MASK_DIS |
		SQ_TIMER_INT_EDGE_TRIG;
	sq_timer_conf[2] = SQ_TIMER_EN |
		SQ_TIMER_CNT_MODE_FREE_RUNNING |
		SQ_TIMER_PRESCALE_FACTOR_1_1 |
		SQ_TIMER_INT_MASK_DIS |
		SQ_TIMER_INT_LEVEL_TRIG;
	sq_timer_write(SQ_TMR0LR, sq_timer_apb_clk*2, sq_timer_base);
	sq_timer_write(SQ_TMR1LR, 2, sq_timer_base);
	sq_timer_write(SQ_TMR2LR, 2, sq_timer_base);
	sq_timer_sec = 0;
	printf("Timer will count 8 seconds at once\n");

	sq_timer_int_flag = 0;

	/* Enable the timer */
	sq_timer_write(SQ_TMR0CON, sq_timer_conf[0], sq_timer_base);
	sq_timer_write(SQ_TMR1CON, sq_timer_conf[1], sq_timer_base);
	sq_timer_write(SQ_TMR2CON, sq_timer_conf[2], sq_timer_base);

	if (sq_wait_for_int(&sq_timer_int_flag, 15)) {
		printf("Timeout\n");
		err_flag = -1;
	} else
		printf("%d second\n", sq_timer_sec);

	/* Disable the timer 0*/
	sq_timer_write(SQ_TMR0CON, SQ_TIMER_DIS, sq_timer_base);

	/* Disable the timer 1*/
	sq_timer_write(SQ_TMR1CON, SQ_TIMER_DIS, sq_timer_base);

	/* Disable the timer 1*/
	sq_timer_write(SQ_TMR2CON, SQ_TIMER_DIS, sq_timer_base);

	return err_flag;
}

extern struct test_item_container sq_timer_cnt_mode_test_container;

extern int
sq_timer_mode_0_timer_0_test(int autotest)
{
	int ret = 0;

	sq_timer_cur_idx = 0;
	ret = test_item_ctrl(&sq_timer_cnt_mode_test_container, autotest);
	return ret;
}

extern int 
sq_timer_mode_0_timer_1_test(int autotest)
{
	int ret = 0;

	sq_timer_cur_idx = 1;
	ret = test_item_ctrl(&sq_timer_cnt_mode_test_container, autotest);
	return ret;
}

extern int 
sq_timer_mode_0_timer_2_test(int autotest)
{
	int ret = 0;

	sq_timer_cur_idx = 2;
	ret = test_item_ctrl(&sq_timer_cnt_mode_test_container, autotest);
	return ret;
}

extern int 
sq_timer_cnt_free_running(int autotest)
{
	int timer_conf_idx;
	int timer_load_idx;
	int err_flag = 0;

	switch(sq_timer_cur_idx) {
	case 0:
		timer_conf_idx = SQ_TMR0CON;
		timer_load_idx = SQ_TMR0LR;
		break;
	case 1:
		timer_conf_idx = SQ_TMR1CON;
		timer_load_idx = SQ_TMR1LR;
		break;
	case 2:
		timer_conf_idx = SQ_TMR2CON;
		timer_load_idx = SQ_TMR2LR;
		break;
	default:
		printf("Unknown timer's configuration base\n");
		return -1;
	}
	sq_timer_conf[sq_timer_cur_idx] = SQ_TIMER_EN |
		SQ_TIMER_CNT_MODE_FREE_RUNNING |
		SQ_TIMER_PRESCALE_FACTOR_1_1 |
		SQ_TIMER_INT_MASK_DIS |
		SQ_TIMER_INT_LEVEL_TRIG;
	sq_timer_write(timer_load_idx, sq_timer_apb_clk*2, sq_timer_base);
	sq_timer_sec = 0;

	printf("Timer will count 2 seconds at once\n");
	sq_timer_int_flag = 0;

	/* Enable the timer */
	sq_timer_write(timer_conf_idx, sq_timer_conf[sq_timer_cur_idx], sq_timer_base);

	if (sq_wait_for_int(&sq_timer_int_flag, 5)) {
		printf("Timeout\n");
		err_flag = -1;
		goto out;
	} else
		printf("%d second\n", sq_timer_sec);
	if (sq_wait_for_int(&sq_timer_int_flag, 5))
		printf("Timeout\n");
	else {
		printf("%d second\n", sq_timer_sec);
		err_flag = -1;
	}

out:		
	/* Disable the timer */
	sq_timer_write(timer_conf_idx, SQ_TIMER_DIS, sq_timer_base);
			
	return err_flag;
}

extern int 
sq_timer_cnt_periodical(int autotest)
{
	int timer_conf_idx;
	int timer_load_idx;
	int err_flag = 0;

	switch(sq_timer_cur_idx) {
	case 0:
		timer_conf_idx = SQ_TMR0CON;
		timer_load_idx = SQ_TMR0LR;
		break;
	case 1:
		timer_conf_idx = SQ_TMR1CON;
		timer_load_idx = SQ_TMR1LR;
		break;
	case 2:
		timer_conf_idx = SQ_TMR2CON;
		timer_load_idx = SQ_TMR2LR;
		break;
	default:
		printf("Unknown timer's configuration base\n");
		return -1;
	}
	sq_timer_conf[sq_timer_cur_idx] = SQ_TIMER_EN |
		SQ_TIMER_CNT_MODE_PERIODICAL |
		SQ_TIMER_PRESCALE_FACTOR_1_1 |
		SQ_TIMER_INT_MASK_DIS |
		SQ_TIMER_INT_LEVEL_TRIG;
	sq_timer_write(timer_load_idx, sq_timer_apb_clk*2, sq_timer_base);
	sq_timer_sec = 0;

	printf("Timer will count 2 seconds and repeat it again\n");

	sq_timer_int_flag = 0;

	/* Enable the timer */
	sq_timer_write(timer_conf_idx, sq_timer_conf[sq_timer_cur_idx], sq_timer_base);

	if (sq_wait_for_int(&sq_timer_int_flag, 5)) {
		printf("Timeout\n");
		err_flag = -1;
		goto out;
	} else
		printf("%d second\n", sq_timer_sec);
	if (sq_wait_for_int(&sq_timer_int_flag, 5)) {
		printf("Timeout\n");
		err_flag = -1;
	}else 
		printf("%d second\n", sq_timer_sec);
	
out:
	/* Disable the timer */
	sq_timer_write(timer_conf_idx, SQ_TIMER_DIS, sq_timer_base);

	return err_flag;
}

static void 
sq_timer_isr_0(void *param)
{
	sq_timer_int_flag = 1;

	/* Clear interrupt */
	sq_timer_write(SQ_TMR0CON, 
			  sq_timer_read(SQ_TMR0CON, sq_timer_base) & (~SQ_TIMER_INT_FLAG),
			  sq_timer_base);

	sq_timer_sec += 2;
}

static void 
sq_timer_isr_1(void *param)
{
	sq_timer_int_flag = 1;

	/* Clear interrupt */
	sq_timer_write(SQ_TMR1CON, 
			  sq_timer_read(SQ_TMR1CON, sq_timer_base) & (~SQ_TIMER_INT_FLAG),
			  sq_timer_base);

	if (1 == sq_timer_chain_mode)
		sq_timer_sec = 4;
	else
		sq_timer_sec += 2;
}

static void 
sq_timer_isr_2(void *param)
{
	sq_timer_int_flag = 1;

	/* Clear interrupt */
	sq_timer_write(SQ_TMR2CON, 
			  sq_timer_read(SQ_TMR2CON, sq_timer_base) & (~SQ_TIMER_INT_FLAG),
			  sq_timer_base);

	if (2 == sq_timer_chain_mode)
		sq_timer_sec = 8;
	else
		sq_timer_sec += 2;
}
