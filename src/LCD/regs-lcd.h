#ifndef _sq_clcd_h_included_
#define _sq_clcd_h_included_

/* LCD control registers */
#define SQ_LCD_CTRL0						0x00
#define SQ_LCD_CTRL1						0x04
#define SQ_LCD_YUV2RGB_CTRL			0x08
#define SQ_LCD_H_TIMING					0x10
#define SQ_LCD_V_TIMING					0x14
#define SQ_LCD_STN_TIMING				0x18
#define SQ_LCD_LUT_ADDR					0x1C
#define SQ_LCD_PAGE0_ADDR				0x20
#define SQ_LCD_PAGE1_ADDR				0x24
#define SQ_LCD_LPAGE0_ADDR				0x28
#define SQ_LCD_LPAGE1_ADDR				0x2C
#define SQ_LCD_ENDIAN					0x30
#define SQ_LCD_INTR_STS					0x40
#define SQ_LCD_INTR_EN					0x44
#define SQ_LCD_INTR_DIS					0x48
#define SQ_LCD_INTR_MASK				0x4C
#define SQ_LCD_Y_PAGE0_ADDR			0x50
#define SQ_LCD_Cb_PAGE0_ADDR			0x54
#define SQ_LCD_Cr_PAGE0_ADDR			0x58
#define SQ_LCD_Y_PAGE1_ADDR			0x60
#define SQ_LCD_Cb_PAGE1_ADDR			0x64
#define SQ_LCD_Cr_PAGE1_ADDR			0x68
#define SQ_LCD_INTR_VER					0xFC


//OFFSET 0x00 CTRL0
#define SQ_LCD_CTRL0_PXCLK_POLAR		(1<<22)
#define SQ_LCD_CTRL0_COLOURSENSE		(1<<20)
#define SQ_LCD_CTRL0_COLOUR_RED		(1<<20)
#define SQ_LCD_CTRL0_COLOUR_GREEN		(2<<20)
#define SQ_LCD_CTRL0_COLOUR_BLUE		(3<<20)
#define SQ_LCD_CTRL0_COLOUR			(3<<20)
#define SQ_LCD_CTRL0_LUMCONFIG_RGB	(7)
#define SQ_LCD_CTRL0_LUMCONFIG_B		(1)
#define SQ_LCD_CTRL0_LUMCONFIG_G		(2)
#define SQ_LCD_CTRL0_LUMCONFIG_R		(4)
#define SQ_LCD_CTRL0_LUMCONFIG_0		(0)
#define SQ_LCD_CTRL0_LUMCONFIG			(7<<17)
#define SQ_LCD_CTRL0_SRAM				(1<<16)
#define SQ_LCD_CTRL0_PCLOCK				(1<<8)
#define SQ_LCD_CTRL0_COLOURDEP			(1<<7)
#define SQ_LCD_CTRL0_24BPP				(1<<7)

#define SQ_LCD_CTRL0_LUTEN				(1<<6)
#define SQ_LCD_CTRL0_PAGESWAP			(1<<5)
#define SQ_LCD_CTRL0_HSYNC				(1<<4)
#define SQ_LCD_CTRL0_VSYNC				(1<<3)
#define SQ_LCD_CTRL0_DTMG				(1<<2)
#define SQ_LCD_CTRL0_RGBHALT			(1<<1)
#define SQ_LCD_CTRL0_ENABLE			(1<<0)

//OFFSET 0x04 CTRL1 STN

//OFFSET 0x08 YUV format control
#define SQ_LCD_YUV2RGB_EN				(1<<0)
//yuv format support
#define SQ_LCD_YUV420					(0x0<<1)
#define SQ_LCD_YUV422					(0x1<<1)
//FIXME
//#define SQ_LCD_YUV420					(0x1<<1)
//#define SQ_LCD_YUV422					(0x0<<1)

//OFFSET 0x10 CTRL1 H_TIMING
#define SQ_LCD_HTIMING_FP				(1<<24)
#define SQ_LCD_HTIMING_BP				(1<<16)
#define SQ_LCD_HTIMING_HRESET			(1<<10)
#define SQ_LCD_HTIMING_HACTIVE			(1<<0)

//OFFSET 0x14 CTRL1 V_TIMING
#define SQ_LCD_VTIMING_FP				(1<<24)
#define SQ_LCD_VTIMING_BP				(1<<16)
#define SQ_LCD_VTIMING_VRESET			(1<<10)
#define SQ_LCD_VTIMING_VACTIVE			(1<<0)

//OFFSET 0x18 STN TIMING


//OFFSET 0x20 PAGE0

//OFFSET 0x24 PAGE1

//OFFSET 0x40 intr status
#define SQ_LCD_INTR_DMA_ERR			(1<<5)
#define SQ_LCD_INTR_OUT_HALT			(1<<4)
#define SQ_LCD_INTR_FIFO_EMPTY			(1<<3)
#define SQ_LCD_INTR_LUT_COMP			(1<<2)
#define SQ_LCD_INTR_PAGE1_READ			(1<<1)
#define SQ_LCD_INTR_PAGE0_READ			(1<<0)


#endif
