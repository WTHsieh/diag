#include <platform.h>
#include <irqs.h>

/* Set the IP's base address */
#ifndef SQ_LCD_BASE
#ifdef SQ_AHB0_LCD
#define SQ_LCD_BASE	SQ_AHB0_LCD
#else
#define SQ_LCD_BASE	SQ_AHB0_ES3
//#error "LCD IP base address is not defined"
#endif
#endif

/* Set the IP's irq */
 
#ifndef SQ_LCD_IRQ
#ifdef SQ_INTC_LCD
#define SQ_LCD_IRQ	SQ_INTC_LCD
#else
#define SQ_LCD_IRQ	SQ_INTC_EXTERNAL2
//#error "LCD IRQ is not defined"
#endif
#endif

/* Set IP version dependance */

#ifdef CONFIG_PDK
#define SQ_YUV_TEST 0
#else
#define SQ_YUV_TEST 1
#endif
 
#ifdef CONFIG_PC7210
#define LCD_YUV_MODE_TEST 0
#else
#define LCD_YUV_MODE_TEST 1
#endif
 
