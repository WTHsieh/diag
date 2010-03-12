#ifndef	_irqs_h_included_
#define	_irqs_h_included_

#define	SQ_INTC_EXTERNAL1		30
#define	SQ_INTC_EXTERNAL0		29
#define SQ_INTC_GPIO			28
#define	SQ_INTC_NAND			27
#define	SQ_INTC_CLCD			25
#define	SQ_INTC_HDMA			22
#define	SQ_INTC_UHC			20
#define SQ_INTC_RTC			13
#define SQ_INTC_PWM1         	12
#define SQ_INTC_PWM0      	  	11
#define	SQ_INTC_TMR0_2		10
#define SQ_INTC_TMR0_1     		9
#define SQ_INTC_TMR0_0        	8
#define SQ_INTC_I2S        	 	6
#define SQ_INTC_I2C0        	 	5
#define SQ_INTC_I2C2       	 	4
#define SQ_INTC_SPI0      	 	3
#define SQ_INTC_I2C1    	   	2
#define SQ_INTC_UART1        	1
#define SQ_INTC_UART0        	0

#define SQ_INTC_MPS2			SQ_INTC_GPIO
#define PANTHER7_INTC_HDMA_0		SQ_INTC_HDMA

#endif	//_irqs_h_included_
