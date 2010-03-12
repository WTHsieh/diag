#ifndef	_irqs_h_included_
#define	_irqs_h_included_

#include "interrupt.h"

/* Interrupt Vector	INTC0 */
#define	SQ_INTC_EXTERNAL6		31
#define	SQ_INTC_EXTERNAL5		30
#define	SQ_INTC_EXTERNAL4		29
#define	SQ_INTC_EXTERNAL3		28
#define	SQ_INTC_EXTERNAL2		27
#define	SQ_INTC_EXTERNAL1		26
#define	SQ_INTC_EXTERNAL0		25
#define SQ_INTC_NAND				24
#define	SQ_INTC_A2PCI			23
#define	SQ_INTC_VOP				22
#define	SQ_INTC_VIP				21
#define	SQ_INTC_MPEG4_ENC		20
#define	SQ_INTC_MPEG4_DEC		19
#define SQ_INTC_IDE       		18
#define SQ_INTC_UHC          	17
#define SQ_INTC_UDC       	  	16
#define SQ_INTC_MAC1       	 	15
#define SQ_INTC_MAC0        		14
#define SQ_INTC_A2A_bri_DMA		13
#define SQ_INTC_HDMA         	12
#define SQ_INTC_SPI       	  	11
#define	SQ_INTC_SDMMC		10
#define SQ_INTC_SCU       		 9
#define SQ_INTC_RTC          	 8
#define SQ_INTC_AHB0_MAILBOX  	 7
#define SQ_INTC_INTC1        	 6
#define SQ_INTC_GPIO0         	 5
#define SQ_INTC_TMR0_2       	 4
#define SQ_INTC_TMR0_1       	 3
#define SQ_INTC_TMR0_0       	 2
#define SQ_INTC_UART1        	 1
#define SQ_INTC_UART0        	 0

/* Interrupt Vector	INTC1 */
#define SQ_INTC_AHB1_MAILBOX		 7
#define SQ_INTC_I2S       		 6
#define SQ_INTC_GPIO1         	 5
#define SQ_INTC_TMR1_2        	 4
#define SQ_INTC_TMR1_1        	 3
#define SQ_INTC_TMR1_0       	 2
#define SQ_INTC_I2C           	 1
#define SQ_INTC_UART2         	 0


#endif	//_irqs_h_included_
