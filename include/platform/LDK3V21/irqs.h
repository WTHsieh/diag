#ifndef	_irqs_h_included_
#define	_irqs_h_included_

#include "interrupt.h"

#define	SQ_INTC_EXTERNAL18		31
#define	SQ_INTC_SDMMC			30
#define SQ_INTC_A2PDMA			29
#define	SQ_INTC_I2C		 		28
//MPEG4 INT define
#define	SQ_INTC_VOP				27
#define	SQ_INTC_VIP				26
#define	SQ_INTC_MPEG4_ENC		25
#define	SQ_INTC_MPEG4_DEC		24
//#define	SQ_INTC_CPU_WBERR	23
#define SQ_INTC_A2A_bri_DMA		23
#define	SQ_INTC_EXTERNAL12		22
#define	SQ_INTC_EXTERNAL11		21
#define	SQ_INTC_EXTERNAL10		20
#define	SQ_INTC_EXTERNAL9		19
#define	SQ_INTC_EXTERNAL8		18
#define	SQ_INTC_EXTERNAL7		17
#define	SQ_INTC_EXTERNAL6		16
#define	SQ_INTC_EXTERNAL5		15
#define	SQ_INTC_EXTERNAL4		14
#define	SQ_INTC_EXTERNAL3		13
#define	SQ_INTC_EXTERNAL2		12
#define	SQ_INTC_EXTERNAL1		11
#define SQ_INTC_EXTERNAL0		10
#define SQ_INTC_UHC			SQ_INTC_EXTERNAL0
#define SQ_INTC_UDC				9
#define SQ_INTC_MAC1   	      	8
#define SQ_INTC_MAC0				7
#define SQ_INTC_RTC				6
#define SQ_INTC_TMR0_2			5
#define SQ_INTC_TMR0_1			4
#define SQ_INTC_TMR0_0			3
#define SQ_INTC_UART1			2
#define SQ_INTC_UART0			1
#define SQ_INTC_SPI				0

#endif	//_irqs_h_included_
