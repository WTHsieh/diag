#ifndef __RTC_REG_H_
#define __RTC_REG_H_                     1

#include "platform.h"
#include "irqs.h"


#ifndef RTC_REG_BASE
#define RTC_REG_BASE		SQ_APB0_RTC
#endif

#ifndef RTC_INT
#define RTC_INT			SQ_INTC_RTC
#endif


#define SQ_RTC_TIME     		(RTC_REG_BASE + 0x0000)
#define SQ_RTC_DATE     		(RTC_REG_BASE + 0x0004)
#define SQ_RTC_TALRM    		(RTC_REG_BASE + 0x0008)
#define SQ_RTC_DALRM    		(RTC_REG_BASE + 0x000C)
#define SQ_RTC_CTRL     		(RTC_REG_BASE + 0x0010)
#define SQ_RTC_RESET    		(RTC_REG_BASE + 0x0014)
#define SQ_RTC_PWOFF    		(RTC_REG_BASE + 0x0018)
#define SQ_RTC_RTCPWFAIL		(RTC_REG_BASE + 0x001C)

//SQ_RTC_TIME		0x00
#define SQ_RTC_TIME_SOS           	0
#define SQ_RTC_TIME_S             	4
#define SQ_RTC_TIME_TS            	8
#define SQ_RTC_TIME_M            	11
#define SQ_RTC_TIME_TM           	15
#define SQ_RTC_TIME_H            	18
#define SQ_RTC_TIME_TH           	22
#define SQ_RTC_TIME_DOW          	24

//SQ_RTC_DATE		0x04
#define SQ_RTC_DATE_D             	0
#define SQ_RTC_DATE_TD            	4
#define SQ_RTC_DATE_M             	6
#define SQ_RTC_DATE_TM           	10
#define SQ_RTC_DATE_Y            	11
#define SQ_RTC_DATE_TY           	15
#define SQ_RTC_DATE_C            	19
#define SQ_RTC_DATE_TC           	23

//SQ_RTC_TALRM		0x08
#define SQ_RTC_TALRM_CSOS		(1<<27)
#define SQ_RTC_TALRM_CS		(1<<28)
#define SQ_RTC_TALRM_CM		(1<<29)
#define SQ_RTC_TALRM_CH		(1<<30)
#define SQ_RTC_TALRM_CDOW		(1<<31)


//SQ_RTC_DALRM		0x0C
#define SQ_RTC_DALRM_CD		(1<<27)
#define SQ_RTC_DALRM_CM		(1<<28)
#define SQ_RTC_DALRM_CY		(1<<29)
#define SQ_RTC_DALRM_CC		(1<<30)

//SQ_RTC_CTRL 		0x10
//LDK 3 bit 28&29 No Use ,Ryan check with TY 2006/11/09
#define SQ_RTC_CTRL_DIV        	(1<<0)
#define SQ_RTC_CTRL_SOS	       	(1<<27)
#define SQ_RTC_CTRL_INT_OUT_EN		(1<<28)
#define SQ_RTC_CTRL_ALRM_PWON 		(1<<29)
#define SQ_RTC_CTRL_ALRM          	(1<<30)
#define SQ_RTC_CTRL_EN            	(1<<31)

//SQ_RTC_RESET		0x14
#define SQ_RTC_RESET_CRESET		(1<<0)
#define SQ_RTC_RESET_RTCRESET		(1<<1)

//SQ_RTC_PWOFF		0x18
#define SQ_RTC_PWOFF_INDT			(1<<0)
#define SQ_RTC_PWOFF_PWOFF			(1<<1)

//SQ_RTC_PWFAIL	0x1c
#define SQ_RTC_RTCPWFAIL_RTCPWFAIL		(1<<0)



//#define SQ_RTC_DIVIDER		(32768)
//#define SQ_RTC_DIVIDER		(4)
#define RTC_CLK_FREQ		(32768)
#define RTC_DIVIDER		((RTC_CLK_FREQ /16)-1)

#define RTC_IS_GOOD		(readw(SQ_RTC_PWOFF) & SQ_RTC_PWOFF_INDT)
#define RTC_IS_PWFAIL		(readw(SQ_RTC_RTCPWFAIL) & SQ_RTC_RTCPWFAIL_RTCPWFAIL)

#define RESET_RTC_CIRCUIT()	writew(readw(SQ_RTC_RESET) | SQ_RTC_RESET_CRESET, SQ_RTC_RESET)
#define RESET_RTC_COUNTER()	writew(readw(SQ_RTC_RESET) | SQ_RTC_RESET_RTCRESET, SQ_RTC_RESET)

#define EN_RTC_ALARM_POWER_ON()		writew(readw(SQ_RTC_CTRL) | SQ_RTC_CTRL_ALRM_PWON, SQ_RTC_CTRL)
#define ENTER_RTC_POWER_OFF_MODE()													\
		do {																		\
			writew(readw(SQ_RTC_CTRL) | SQ_RTC_CTRL_INT_OUT_EN, SQ_RTC_CTRL);	\
			writew(readw(SQ_RTC_PWOFF) | SQ_RTC_PWOFF_PWOFF, SQ_RTC_PWOFF);		\
		} while (0)
		

// macro lib
#define RTC_CTRL_SET_FLAG(x)     (writew(readw(SQ_RTC_CTRL) |  (x), SQ_RTC_CTRL))
#define RTC_CTRL_CLR_FLAG(x)     (writew(readw(SQ_RTC_CTRL) & ~(x), SQ_RTC_CTRL))

#define RTC_SET_DIVIDER(x)       (writew((readw(SQ_RTC_CTRL) & 0xf8000000) | (x), SQ_RTC_CTRL))
#define RTC_GET_DIVIDER()        (readw(SQ_RTC_CTRL) & BIT_MASK(27))
#define RTC_BSOS_EN()            RTC_CTRL_SET_FLAG( SQ_RTC_CTRL_SOS )
#define RTC_BSOS_DIS()           RTC_CTRL_CLR_FLAG( SQ_RTC_CTRL_SOS )
#define RTC_ALRM_EN()            RTC_CTRL_SET_FLAG( SQ_RTC_CTRL_ALRM )
#define RTC_ALRM_DIS()           RTC_CTRL_CLR_FLAG( SQ_RTC_CTRL_ALRM )
#define RTC_EN()                 RTC_CTRL_SET_FLAG( SQ_RTC_CTRL_EN )
#define RTC_DIS()                RTC_CTRL_CLR_FLAG( SQ_RTC_CTRL_EN )


#endif 










