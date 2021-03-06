#ifndef _platform_H_
#define _platform_H_

#include <io.h>
#include <type.h>
#include <interrupt.h>

#define INTC_MAX_IRQ           		 	31			// interrupt source number
#define INTC_MAX_FIQ				31			// interrupt source number
#define MAX_SYSTEM_IRQ				31			// max. system interrupt source number

#define INTC_CONNECT_SYS_INT		SYS_INT_7	// interrupt controller connect which MIPS IP


#define EXT_OSC						24			// Ext OSC (MHz)
#define SQ_UART_EXT_CLK			(11059200*8)


#define MAPPING_MASK				0x1FFFFFFF
#define CACHE_BASE					0x60000000	// Cache Kenel Mode

#define KSEG_TO_PHY(x)				( ((int) (x)) & MAPPING_MASK )
#define PHY_TO_CACHE(x)				( ( ((int) (x)) & MAPPING_MASK ) | CACHE_BASE )

// memory mapped address
#define	SQ_MEMORY_ADDR_START			0x40000000
#define	SQ_MEMORY_ADDR_SIZE			0x04000000

#define	SQ_MEMORY_BANKS		2

#define SQ_MM_DDR_SDR_BANK0			0x00000000
#define SQ_MM_DDR_SDR_BANK1			0x40000000


#define	SQ_NORFLASH_BANKS		1

#define SQ_NOR_FLASH_BANK0        0x10000000	


// APB device base address define
#define SQ_APB0_ADC			0x19120000
#define SQ_APB0_SCU			0x19100000
#define SQ_APB0_WDT			0x190F0000
#define SQ_APB0_RTC			0x190E0000
#define SQ_APB0_GPIO0		0x190A0000
#define SQ_APB0_GPIO1		0x190B0000
#define SQ_APB0_GPIO2		0x190C0000
#define SQ_APB0_GPIO3		0x190D0000
#define SQ_APB0_TIMER_PWM	0x19090000
#define SQ_APB0_TIMER		0x19080000
#define SQ_APB0_SDMMC		0x19070000
#define SQ_APB0_I2S			0x19060000
#define SQ_APB0_I2C			0x19050000
#define SQ_APB0_SPI1			0x19040000
#define SQ_APB0_SPI0			0x19030000
#define SQ_APB0_UART2		0x19020000
#define SQ_APB0_UART1		0x19010000
#define SQ_APB0_UART0		0x19000000
#define SQ_AHB0_LCD			0x19110000

// AHB device base address define
#define SQ_AHB0_LCD			0x19110000
#define SQ_AHB0_VOP			0x18100000
#define SQ_AHB0_UHC1		0x180E0000
#define SQ_AHB0_HDMA		0x180C0000
#define SQ_AHB0_UDC			0x180A0000
#define SQ_AHB0_UHC0		0x18080000
#define SQ_AHB0_MAC0		0x18060000
#define SQ_AHB0_INTC		0x18040000
#define SQ_AHB0_ARBITER		0x18020000
#define SQ_AHB0_SDRSTMC	0x18000000	
#define SQ_AHB0_NAND		0x18140000	
#define SQ_AHB0_APB_BRI		0x19000000	
#define PANTHER7_AHB_0_HDMA_0	     SQ_AHB0_HDMA

#define	PHY_IC_PLUS

// GPIO
#define SQ_GPIO_WITH_INT
#define SQ_GPIO_FLASH_LED_TEST	0
#define GPIO_FLASH_LED_PA	1
#define GPIO_FLASH_LED_PB	0

#endif //_platform_H_
