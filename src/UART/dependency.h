#include <platform.h>
#include <irqs.h>

/* Set the IP's base address */
#ifndef SQ_APB0_UART0
#ifdef SQ_APB0_UART
#define SQ_APB0_UART0 SQ_APB0_UART
#else
#error "UART IP base address is not defined"
#endif
#endif

#ifndef SQ_APB0_UART0
#define SQ_APB0_UART0 SQ_APB0_UART0
#define SQ_UART_IP_0_TEST 0
#else
#define SQ_UART_IP_0_TEST 1
#endif

#ifndef SQ_APB0_UART1
#define SQ_APB0_UART1 SQ_APB0_UART0
#define SQ_UART_IP_1_TEST 0
#else
#define SQ_UART_IP_1_TEST 1
#endif

#ifndef SQ_APB0_UART2
#define SQ_APB0_UART2 SQ_APB0_UART0
#define SQ_UART_IP_2_TEST 0
#else
#define SQ_UART_IP_2_TEST 1
#endif

#ifndef SQ_APB0_UART3
#define SQ_APB0_UART3 SQ_APB0_UART0
#define SQ_UART_IP_3_TEST 0
#else
#define SQ_UART_IP_3_TEST 1
#endif

/* Set the IP's irq */
#ifndef SQ_INTC_UART0
#ifdef SQ_INTC_UART
#define SCOLE_INTC_UART0 SQ_INTC_UART
#else
#error "UART IRQ is not defined"
#endif
#endif

#ifndef SQ_INTC_UART1
#define SQ_INTC_UART1 SQ_INTC_UART0
#endif

#ifndef SQ_INTC_UART2
#define SQ_INTC_UART2 SQ_INTC_UART0
#endif

#ifndef SQ_INTC_UART3
#define SQ_INTC_UART3 SQ_INTC_UART0
#endif

/* Configure the test menu */
#if defined(CONFIG_PANTHER7_HDMA) && (defined(CONFIG_PDK) || defined(CONFIG_PC7210) || defined(CONFIG_SQ8000))
#define SQ_UART_HWDMA_PANTHER7_HDMA_TEST 1
#else
#define SQ_UART_HWDMA_PANTHER7_HDMA_TEST 0
#endif

#ifdef CONFIG_UART_DISP
#if defined(CONFIG_PC7210) || defined(CONFIG_PDK)
#undef SQ_UART_IP_1_TEST
#define SQ_UART_IP_1_TEST 0
#else
#undef SQ_UART_IP_0_TEST
#define SQ_UART_IP_0_TEST 0
#endif
#endif

#if defined(CONFIG_PC9002) || defined(CONFIG_SQ8000)
#define SQ_UART_GPS_0_TEST 0 
#define SQ_UART_GPS_1_TEST 1
#define SQ_UART_GPS_2_TEST 0
#define SQ_UART_GPS_3_TEST 0
#else
#define SQ_UART_GPS_0_TEST 0 
#define SQ_UART_GPS_1_TEST 0
#define SQ_UART_GPS_2_TEST 0
#define SQ_UART_GPS_3_TEST 0
#endif

#if defined(CONFIG_PC9002) || defined(CONFIG_SQ8000)
#define SQ_UART_EXT_0_TEST 1
#define SQ_UART_EXT_1_TEST 0
#define SQ_UART_EXT_2_TEST 0
#define SQ_UART_EXT_3_TEST 0
#elif defined(CONFIG_PC7210)
#define SQ_UART_EXT_0_TEST 0
#define SQ_UART_EXT_1_TEST 1
#define SQ_UART_EXT_2_TEST 0
#define SQ_UART_EXT_3_TEST 0
#else
#define SQ_UART_EXT_0_TEST 1 
#define SQ_UART_EXT_1_TEST 1
#define SQ_UART_EXT_2_TEST 0
#define SQ_UART_EXT_3_TEST 0
#endif
