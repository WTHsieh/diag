#ifndef _platform_H_
#define _platform_H_

#include <io.h>
#include <type.h>
#include <interrupt.h>

#define INTC_MAX_IRQ            	32			// interrupt source number
#define INTC_MAX_FIQ				32			// interrupt source number
#define MAX_SYSTEM_IRQ				32			// max. system interrupt source number

#define INTC_CONNECT_SYS_INT		SYS_INT_7	// interrupt controller connect which MIPS IP


#define EXT_OSC					33			// Ext OSC (MHz)
#define SQ_UART_EXT_CLK			24000000


#ifdef CONFIG_ARM9
#define MAPPING_MASK				0x1FFFFFFF
#define CACHE_BASE					0x80000000	// Cache Kenel Mode
#endif

#if defined(CONFIG_ARM7) || defined(CONFIG_ARM7_HI)
#define MAPPING_MASK				0x3FFFFFFF
#define CACHE_BASE					0x40000000	// Cache Kenel Mode
#endif

#define KSEG_TO_PHY(x)				( ((int) (x)) & MAPPING_MASK )
#define PHY_TO_CACHE(x)				( ( ((int) (x)) & MAPPING_MASK ) | CACHE_BASE )

// memory mapped address
#define	SQ_MEMORY_ADDR_START			0x40000000
#define	SQ_MEMORY_ADDR_SIZE			0x20000000

#define	SQ_MEMORY_BANKS	4

#define SQ_MM_DDR_SDR_BANK0			0x00000000
#define SQ_MM_DDR_SDR_BANK1			0x40000000	
#define SQ_MM_DDR_SDR_BANK2			0x50000000	
#define SQ_MM_DDR_SDR_BANK3			0x17000000	


#define	SQ_NORFLASH_BANKS	2

#define SQ_NOR_FLASH_BANK0               0x1F000000	
#define SQ_NOR_FLASH_BANK1               0x1F800000	


// APB device base address define
#define SQ_APB0_SDMMC	0x1E920000
#define SQ_APB0_I2C		0x1E900000
#define SQ_APB0_SCU		0x1E8E0000
#define SQ_APB0_WDT		0x1E8C0000
#define SQ_APB0_GPIO		0x1E8A0000
#define SQ_APB0_SPI		0x1E880000
#define SQ_APB0_UART1	0x1E860000
#define SQ_APB0_UART0	0x1E840000
#define SQ_APB0_RTC		0x1E820000
#define SQ_APB0_TIMER	0x1E800000


// AHB device base address define
#define SQ_AHB0_ES7		0x20000000	/*256MB */ 
#define SQ_AHB0_ES6		0x1E7A0000
#define SQ_AHB0_ES5		0x1E780000
#define SQ_AHB0_ES4		0x1E760000
#define SQ_AHB0_ES3		0x1E740000
#define SQ_AHB0_ES2		0x1E720000
#define SQ_AHB0_ES1		0x1E700000
#define SQ_AHB0_ES0		0x1E6E0000

#define SQ_BUS0_PCI		0x19CE0000
#define SQ_AHB0_UDC		0x19CC0000
#define SQ_AHB0_MAC0		0x19C80000
#define SQ_AHB0_MAC1		0x19CA0000
#define SQ_AHB0_INTC		0x19C40000
#define SQ_AHB0_SDRSTMC	0x19C20000	// for static
#define SQ_AHB0_DDRMC	0x19C00000	// for DDR


// A2A Bridge & DMA
#define	SQ_AHB0_A2A_DMA	0x1E7E0000
#define SQ_AHB0_UHC		0x1E6E0000

#define	PHY_RTL8201

// GPIO
//#define SQ_GPIO_WITH_INT
#define SQ_GPIO_FLASH_LED_TEST	0
#define GPIO_FLASH_LED_PA	0
#define GPIO_FLASH_LED_PB	0

#endif //_platform_H_
