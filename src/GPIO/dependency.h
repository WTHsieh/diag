#include <platform.h>
#include <irqs.h>

#ifdef SQ_GPIO_WITH_INT
#define SQ_GPIO_SUPPORT_INT 1
#else
#define SQ_GPIO_SUPPORT_INT 0
#endif

/* Set the IP's base address */
#ifndef SQ_APB0_GPIO0
#ifdef SQ_APB0_GPIO
#define SQ_APB0_GPIO0	SQ_APB0_GPIO
#else
#error "GPIO IP base address is not defined"
#endif
#endif

#ifndef SQ_APB0_GPIO1
#define SQ_APB0_GPIO1 SQ_APB0_GPIO0
#define SQ_GPIO_GP1	0
#else
#define SQ_GPIO_GP1	1
#endif

#ifndef SQ_APB0_GPIO2
#define SQ_APB0_GPIO2 SQ_APB0_GPIO0
#define SQ_GPIO_GP2	0
#else
#define SQ_GPIO_GP2	1
#endif

#ifndef SQ_APB0_GPIO3
#define SQ_APB0_GPIO3 SQ_APB0_GPIO0
#define SQ_GPIO_GP3	0
#else
#define SQ_GPIO_GP3	1
#endif

/* Set the IP's irq */
#ifdef SQ_GPIO_WITH_INT

#ifndef SQ_INTC_GPIO0
#ifdef SQ_INTC_GPIO
#define SQ_INTC_GPIO0	SQ_INTC_GPIO
#else
#error "GPIO IRQ is not defined"
#endif
#endif

#ifndef SQ_INTC_GPIO1
#define SQ_INTC_GPIO1		SQ_INTC_GPIO0
#endif

#endif //SQ_GPIO_WITH_INT

#if defined(CONFIG_PDK) || defined(CONFIG_PC7210) || defined(CONFIG_SQ8000)
#define SQ_GPIO_KPD_TEST		1
#else
#define SQ_GPIO_KPD_TEST		0
#endif

#ifdef CONFIG_SCDK
#define SCDK_FPGA_TEST 1
#else
#define SCDK_FPGA_TEST 0
#endif
