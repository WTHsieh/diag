#include <platform.h>
#include <irqs.h>

#ifndef SQ_APB0_TIMER
#error "Timer IP base address is not defined"
#endif

#ifndef SQ_INTC_TMR0_0
#error "Tiimer IRQ0 is not defined"
#endif

#ifndef SQ_INTC_TMR0_1
#error "Tiimer IRQ1 is not defined"
#endif

#ifndef SQ_INTC_TMR0_2
#error "Tiimer IRQ2 is not defined"
#endif


#if defined(CONFIG_PDK) || defined(CONFIG_PC7210) || defined(CONFIG_SQ8000)
#define SQ_TIMER_CHAIN_MODE_1_TEST 1
#define SQ_TIMER_CHAIN_MODE_2_TEST 1
#else
#define SQ_TIMER_CHAIN_MODE_1_TEST 0
#define SQ_TIMER_CHAIN_MODE_2_TEST 0
#endif
