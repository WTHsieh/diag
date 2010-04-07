/********************************************************************************
* File Name     : LCD/lcd-ctrl.c 
* Author        : Ryan Chen 
* Description   : file description ¡K.
* 
* Copyright (C) Sq Tech. Corp.
* 
*
*   Version      : 0,0,0,1
*   Description  : lcd ctrl diagnostic program 
*   History       :  
*      1. 2007/12/10 Ryan Chen create this file 
*     
*
**********************************************************************************/
#include <genlib.h>
#include <test_item.h>
#include "lcd-ctrl.h"
#include "regs-lcd.h"
#include "ampire-panel.h"
#include "dependency.h"

#if defined (CONFIG_SQ8000)
#include <scu.h>
#include "../../platform/arch/scu-reg.h"
#endif


//#define CONFIG_SQ_LCD_DEBUG
#ifdef CONFIG_SQ_LCD_DEBUG
	#define LCD_DBG(fmt, args...) printf("SQ_LCD: " fmt, ## args)
#else
	#define LCD_DBG(fmt, args...)
#endif


#define DEFAULT_WIDTH	(320)
#define DEFAULT_HEIGHT	(240)

#define COLOR_BAR_BASE0		(SQ_MM_DDR_SDR_BANK1 + 0x200000)
#define COLOR_BAR_BASE1		(SQ_MM_DDR_SDR_BANK1 + 0x300000)
#define LCD_LUT_BASE			(SQ_MM_DDR_SDR_BANK1 + 0x400000)

#define DEFAULT_BAR_HEIGHT	(DEFAULT_HEIGHT/8)

struct sq_lcd{
	u32 htiming;		// Horizontal timing
	u32 vtiming;		// Vertical timing
	u32 pixelclock;	// pixel clock
	u32 bpp16mode;		// colour sensitivity 16 bit
	u32 lummode;	//lum mode
	u32 bpp;			// mode 0: 16Bpp  mode 1: 24bpp mode 3 : LUT, mode 4: YUV	
	u32 yuv;			//YUV format support	
	u32 swap;
	u32 yuv_format;	//mode 0; yuv420, mode 1 yuv 422
};


struct sq_lcd sq_lcd_set = {
	.htiming = 0x14242140,
	.vtiming = 0x040f04f0,
#if defined (CONFIG_PC7210)
	.pixelclock = 0xa,
#elif defined (CONFIG_SQ8000)
	.pixelclock = 4,
#else
	.pixelclock = 2,		//default 2
#endif
	.bpp16mode = 0,
	.bpp = 0,
	.yuv = 0,
	.swap = 0,
	.yuv_format = 0,
};

const u32 BPP24_RGB[8]=
{
		COLOR_BALCK,
		COLOR_RED, 
		COLOR_GREEN,
		COLOR_BLUE, 
		COLOR_CYAN,
		COLOR_MAGENTA, 
		COLOR_YELLOW,  
		COLOR_WHITE,  
};

const u16 BPP16_RGB_R[8] =
{
		COLOR_BALCK_16BPP_R,
		COLOR_RED_16BPP_R,
		COLOR_GREEN_16BPP_R,
		COLOR_BLUE_16BPP_R,
		COLOR_CYAN_16BPP_R,
		COLOR_MAGENTA_16BPP_R,
		COLOR_YELLOW_16BPP_R,
		COLOR_WHITE_16BPP_R
};

const u16 BPP16_RGB_G[8] =
{
		COLOR_BALCK_16BPP_G,
		COLOR_RED_16BPP_G,
		COLOR_GREEN_16BPP_G,
		COLOR_BLUE_16BPP_G,
		COLOR_CYAN_16BPP_G,
		COLOR_MAGENTA_16BPP_G,
		COLOR_YELLOW_16BPP_G,
		COLOR_WHITE_16BPP_G
};

const u16 BPP16_RGB_B[8] =
{
		COLOR_BALCK_16BPP_B,
		COLOR_RED_16BPP_B,
		COLOR_GREEN_16BPP_B,
		COLOR_BLUE_16BPP_B,
		COLOR_CYAN_16BPP_B,
		COLOR_MAGENTA_16BPP_B,
		COLOR_YELLOW_16BPP_B,
		COLOR_WHITE_16BPP_B
};


const u16 BPP16_LUM_0[8] =
{
		COLOR_BALCK_LUM_0,
		COLOR_RED_LUM_0,
		COLOR_GREEN_LUM_0,
		COLOR_BLUE_LUM_0,
		COLOR_CYAN_LUM_0,
		COLOR_MAGENTA_LUM_0,
		COLOR_YELLOW_LUM_0,
		COLOR_WHITE_LUM_0
};

const u16 BPP16_LUM_R[8] =
{
		COLOR_BALCK_LUM_R,
		COLOR_RED_LUM_R,
		COLOR_GREEN_LUM_R,
		COLOR_BLUE_LUM_R,
		COLOR_CYAN_LUM_R,
		COLOR_MAGENTA_LUM_R,
		COLOR_YELLOW_LUM_R,
		COLOR_WHITE_LUM_R
};

const u16 BPP16_LUM_G[8] =
{
		COLOR_BALCK_LUM_G,
		COLOR_RED_LUM_G,
		COLOR_GREEN_LUM_G,
		COLOR_BLUE_LUM_G,
		COLOR_CYAN_LUM_G,
		COLOR_MAGENTA_LUM_G,
		COLOR_YELLOW_LUM_G,
		COLOR_WHITE_LUM_G
};

const u16 BPP16_LUM_B[8] =
{
		COLOR_BALCK_LUM_B,
		COLOR_RED_LUM_B,
		COLOR_GREEN_LUM_B,
		COLOR_BLUE_LUM_B,
		COLOR_CYAN_LUM_B,
		COLOR_MAGENTA_LUM_B,
		COLOR_YELLOW_LUM_B,
		COLOR_WHITE_LUM_B
};

const u16 BPP16_LUM_RGB[8] =
{
		COLOR_BALCK_LUM_RGB,
		COLOR_RED_LUM_RGB,
		COLOR_GREEN_LUM_RGB,
		COLOR_BLUE_LUM_RGB,
		COLOR_CYAN_LUM_RGB,
		COLOR_MAGENTA_LUM_RGB,
		COLOR_YELLOW_LUM_RGB,
		COLOR_WHITE_LUM_RGB
};

struct yuv_format
{
	u16 Y;
	u16 Cb;
	u16 Cr;
}; 

static u32 sq_lcd_base = SQ_LCD_BASE;
static u32 sq_lcd_irq = SQ_LCD_IRQ;

static inline u32 sq_lcd_read(u32 reg)
{
	u32 val;
	val = ioread32(sq_lcd_base + reg);
	return val;
}

static inline void sq_lcd_write(u32 val, u32 reg)
{
	iowrite32(val, sq_lcd_base + reg);
}

int Memory_Color_Set(int i)
{
	if (sq_lcd_set.swap == 0)
	{
		if (sq_lcd_set.bpp == 1)
		{
			Sq_ColorFSFill (BPP24_RGB[i], COLOR_BAR_BASE0);
		}
		else if (sq_lcd_set.bpp == 2)
		{
			Sq_LUT_Fill(i,COLOR_BAR_BASE0);
		}
		else if (sq_lcd_set.bpp == 3)	//YUV420 format
		{
			//Sq_LUT_Fill(i,COLOR_BAR_BASE0);
		}
		else
		{
			if(sq_lcd_set.bpp16mode == 1)
			{
				Sq_Color_16BPP_FSFill (BPP16_RGB_R[i],COLOR_BAR_BASE0);
			}
			else if(sq_lcd_set.bpp16mode == 2)
			{
				Sq_Color_16BPP_FSFill (BPP16_RGB_G[i],COLOR_BAR_BASE0);
			}
			else if(sq_lcd_set.bpp16mode == 3)
			{
				Sq_Color_16BPP_FSFill (BPP16_RGB_B[i],COLOR_BAR_BASE0);
			}
			else
			{
				if(sq_lcd_set.lummode == SQ_LCD_CTRL0_LUMCONFIG_0)
				{
					Sq_Color_16BPP_FSFill (BPP16_LUM_0[i],COLOR_BAR_BASE0);
				}
				else if(sq_lcd_set.lummode == SQ_LCD_CTRL0_LUMCONFIG_R)
				{
					Sq_Color_16BPP_FSFill (BPP16_LUM_R[i],COLOR_BAR_BASE0);
				}
				else if(sq_lcd_set.lummode == SQ_LCD_CTRL0_LUMCONFIG_G)
				{
					Sq_Color_16BPP_FSFill (BPP16_LUM_G[i],COLOR_BAR_BASE0);
				}
				else if(sq_lcd_set.lummode == SQ_LCD_CTRL0_LUMCONFIG_B)
				{
					Sq_Color_16BPP_FSFill (BPP16_LUM_B[i],COLOR_BAR_BASE0);
				}
				else
				{
					Sq_Color_16BPP_FSFill (BPP16_LUM_RGB[i],COLOR_BAR_BASE0);
				}				
			}
		}
	}
	else
	{
		if (sq_lcd_set.bpp == 1)
		{
			Sq_ColorFSFill (BPP24_RGB[i],COLOR_BAR_BASE0);
			Sq_ColorFSFill (BPP24_RGB[i],COLOR_BAR_BASE1);			
		}
		else if (sq_lcd_set.bpp == 2)
		{
			Sq_LUT_Fill(i,COLOR_BAR_BASE0);
			Sq_LUT_Fill(i,COLOR_BAR_BASE1);			
		}
		else
		{
			if(sq_lcd_set.bpp16mode == 1)
			{
				Sq_Color_16BPP_FSFill (BPP16_RGB_R[i],COLOR_BAR_BASE0);
				Sq_Color_16BPP_FSFill (BPP16_RGB_R[i],COLOR_BAR_BASE1);				
			}
			else if(sq_lcd_set.bpp16mode == 2)
			{
				Sq_Color_16BPP_FSFill (BPP16_RGB_G[i],COLOR_BAR_BASE0);			
				Sq_Color_16BPP_FSFill (BPP16_RGB_G[i],COLOR_BAR_BASE1);
			}
			else if(sq_lcd_set.bpp16mode == 3)
			{
				Sq_Color_16BPP_FSFill (BPP16_RGB_B[i],COLOR_BAR_BASE0);
				Sq_Color_16BPP_FSFill (BPP16_RGB_B[i],COLOR_BAR_BASE1);				
			}
			else
			{
				if(sq_lcd_set.lummode == SQ_LCD_CTRL0_LUMCONFIG_0)
				{
					Sq_Color_16BPP_FSFill (BPP16_LUM_0[i],COLOR_BAR_BASE0);
					Sq_Color_16BPP_FSFill (BPP16_LUM_0[i],COLOR_BAR_BASE1);
				}
				else if(sq_lcd_set.lummode == SQ_LCD_CTRL0_LUMCONFIG_R)
				{
					Sq_Color_16BPP_FSFill (BPP16_LUM_R[i],COLOR_BAR_BASE0);
					Sq_Color_16BPP_FSFill (BPP16_LUM_R[i],COLOR_BAR_BASE1);
				}
				else if(sq_lcd_set.lummode == SQ_LCD_CTRL0_LUMCONFIG_G)
				{
					Sq_Color_16BPP_FSFill (BPP16_LUM_G[i],COLOR_BAR_BASE0);
					Sq_Color_16BPP_FSFill (BPP16_LUM_G[i],COLOR_BAR_BASE1);
				}
				else if(sq_lcd_set.lummode == SQ_LCD_CTRL0_LUMCONFIG_B)
				{
					Sq_Color_16BPP_FSFill (BPP16_LUM_B[i],COLOR_BAR_BASE0);
					Sq_Color_16BPP_FSFill (BPP16_LUM_B[i],COLOR_BAR_BASE1);
				}
				else
				{
					Sq_Color_16BPP_FSFill (BPP16_LUM_RGB[i],COLOR_BAR_BASE0);
					Sq_Color_16BPP_FSFill (BPP16_LUM_RGB[i],COLOR_BAR_BASE1);
				}				
			
			}
		}
	}
	return 0;
	
}

int rgb2yuv_convert(int rgb_format, struct yuv_format *yuv_color)
{
//   Convert Range 16~240 
//	LCD_DBG("rgb2yuv_convert %x \n",rgb_format);
	yuv_color->Y=0.299 * ((rgb_format>>16)&0xFF) + 0.587*((rgb_format>>8)&0xFF) + 0.114*(rgb_format&0xFF);
//	LCD_DBG("Y : %x \n",yuv_color->Y);
	if(yuv_color->Y>240)
		yuv_color->Y = 240;
	if(yuv_color->Y<16)
		yuv_color->Y = 16;
	yuv_color->Cb=-0.172*((rgb_format>>16)&0xFF) - 0.339*((rgb_format>>8)&0xFF) + 0.511*(rgb_format&0xFF) + 128;
//	LCD_DBG("Cb : %x \n",yuv_color->Cb);
	if(yuv_color->Cb>240)
		yuv_color->Cb = 240;
	if(yuv_color->Cb<16)
		yuv_color->Cb = 16;
	
	yuv_color->Cr=0.511*((rgb_format>>16)&0xFF) - 0.428*((rgb_format>>8)&0xFF) - 0.083*(rgb_format&0xFF) + 128;
//	LCD_DBG("Cr : %x \n",yuv_color->Cr);

	if(yuv_color->Cr>240)
		yuv_color->Cr = 240;
	if(yuv_color->Cr<16)
		yuv_color->Cr = 16;
	
	return 0;
}

int YUV_ColorFS_Bottom_fill(struct yuv_format *yuv_color, u32 color_base)
{
	unsigned int height = DEFAULT_HEIGHT;
	unsigned int width = DEFAULT_WIDTH;

	int i, j;

	u8 *y = (u8 *) color_base+(height*width)/2;
	u8 *u;
	u8 *v;	

	if (sq_lcd_set.yuv_format == 0)	
		u = (u8 *) (color_base +  (height*width) + (height*width)/8);
	else
		u = (u8 *) (color_base +  (height*width) + (height*width)/4);
	

	if (sq_lcd_set.yuv_format == 0)
		v = (u8 *)  (color_base +  height*width + (height*width)/4 + (height*width)/8);
	else
		v = (u8 *)  (color_base +  height*width + (height*width)/2 + (height*width)/4);

	if(sq_lcd_set.yuv_format == 0)	//420
	{
		LCD_DBG("YUV420 \n");
		//Y1,Y2,Y3,Y4
		for (i = 0; i < height/2/2; i++)
		{
			for (j = 0; j < width/2; j++)
			{

				*y = yuv_color->Y; //Y1
				*(y+1) = yuv_color->Y; //Y2
				*(y+320) = yuv_color->Y; //Y3
				*(y+321) = yuv_color->Y; //Y4

				y+=2;
				//U
				*u = yuv_color->Cb;
				u++;
				//V
				*v = yuv_color->Cr;
				v++;
				
			}
			y+=320;
		}
		
	}		//422
	else
	{
		LCD_DBG("YUV422 \n");
		//Y1,Y2,Y3,Y4
		for (i = 0; i < height/2; i++)
		{
			for (j = 0; j < width/2; j++)
			{
				*y = yuv_color->Y; //Y1
				*(y+1) = yuv_color->Y; //Y2

				y+=2;
				//U
				*u = yuv_color->Cb;
				u++;
				//V
				*v = yuv_color->Cr;
				v++;
			}
		}

	}
	
	return 0;
}

int YUV_ColorFS_fill(struct yuv_format *yuv_color, u32 color_base)
{
	unsigned int height = DEFAULT_HEIGHT;
	unsigned int width = DEFAULT_WIDTH;

	int i, j;

	u8 *p = (u8 *) color_base;

	if(sq_lcd_set.yuv_format == 0)	//420
	{
		LCD_DBG("YUV420 \n");
		for (i = 0; i < height; i++)
		{
			  for (j = 0; j < width; j++)
			*p++ = yuv_color->Y;
		}
		*p = (color_base +  height*width);
		LCD_DBG("p : %x \n",p);
		for (i = 0; i < height/2; i++)
		{
			  for (j = 0; j < width/2; j++)
			*p++ = yuv_color->Cb;
		}
		*p = (color_base +  height*width + height*width/4);
		LCD_DBG("p : %x \n",p);
		for (i = 0; i < height/2; i++)
		{
			  for (j = 0; j < width/2; j++)
			*p++ = yuv_color->Cr;
		}
		
	}		//422
	else
	{
		LCD_DBG("YUV422 \n");
		for (i = 0; i < height; i++)
		{
			  for (j = 0; j < width; j++)
			*p++ = yuv_color->Y;
		}
		*p = (color_base +  height*width);
		LCD_DBG("p : %x \n",p);
		for (i = 0; i < height/2; i++)
		{
			  for (j = 0; j < width; j++)
			*p++ = yuv_color->Cb;
		}
		*p = (color_base +  height*width + height*width/2);
		LCD_DBG("p : %x \n",p);
		for (i = 0; i < height/2; i++)
		{
			  for (j = 0; j < width; j++)
			*p++ = yuv_color->Cr;
		}
	
	}
	
	return 0;
}

int yuv_color_fill(int i)
{
	struct yuv_format yuv_color;
	
	rgb2yuv_convert(BPP24_RGB[i], &yuv_color);
	if(sq_lcd_set.swap == 0)	
	{
		YUV_ColorFS_fill(&yuv_color, COLOR_BAR_BASE0);
		
	}
	else
	{
		YUV_ColorFS_fill(&yuv_color, COLOR_BAR_BASE0);
		YUV_ColorFS_fill(&yuv_color, COLOR_BAR_BASE1);		
	}
	return 0;
}

int yuv_color_fill_black(int autotest)
{
	yuv_color_fill(0);
	//Enable CLCD
	sq_lcd_write(sq_lcd_read(SQ_LCD_CTRL0)| SQ_LCD_CTRL0_ENABLE, SQ_LCD_CTRL0);
	
	return 0;	
}

int yuv_color_fill_red(int autotest)
{
	yuv_color_fill(1);		
	//Enable CLCD
	sq_lcd_write(sq_lcd_read(SQ_LCD_CTRL0)| SQ_LCD_CTRL0_ENABLE, SQ_LCD_CTRL0);

	return 0;
}

int yuv_color_fill_green(int autotest)
{
	yuv_color_fill(2);	
	//Enable CLCD
	sq_lcd_write(sq_lcd_read(SQ_LCD_CTRL0)| SQ_LCD_CTRL0_ENABLE, SQ_LCD_CTRL0);

	return 0;
}

int yuv_color_fill_blue(int autotest)
{
	yuv_color_fill(3);	
	//Enable CLCD
	sq_lcd_write(sq_lcd_read(SQ_LCD_CTRL0)| SQ_LCD_CTRL0_ENABLE, SQ_LCD_CTRL0);

	return 0;
}

int yuv_color_fill_cyan(int autotest)
{
	yuv_color_fill(4);	
	//Enable CLCD
	sq_lcd_write(sq_lcd_read(SQ_LCD_CTRL0)| SQ_LCD_CTRL0_ENABLE, SQ_LCD_CTRL0);

	return 0;
}

int yuv_color_fill_magenta(int autotest)
{
	yuv_color_fill(5);	
	//Enable CLCD
	sq_lcd_write(sq_lcd_read(SQ_LCD_CTRL0)| SQ_LCD_CTRL0_ENABLE, SQ_LCD_CTRL0);

	return 0;
}

int yuv_color_fill_yellow(int autotest)
{
	yuv_color_fill(6);	
	//Enable CLCD
	sq_lcd_write(sq_lcd_read(SQ_LCD_CTRL0)| SQ_LCD_CTRL0_ENABLE, SQ_LCD_CTRL0);

	return 0;
}

int yuv_color_fill_white(int autotest)
{
	yuv_color_fill(7);	
	//Enable CLCD
	sq_lcd_write(sq_lcd_read(SQ_LCD_CTRL0)| SQ_LCD_CTRL0_ENABLE, SQ_LCD_CTRL0);
	

	return 0;
}

int YUV_Color_Bar_V_Fill(u32 color_base)
{
	struct yuv_format yuv_color;
	
	unsigned int height = DEFAULT_HEIGHT;
	unsigned int width = DEFAULT_WIDTH;

	int i;
	int j;

	u8 *y = (u8 *) color_base;
	u8 *u = (u8 *) (color_base +  height*width);
	u8 *v;
	if (sq_lcd_set.yuv_format == 0)
		v = (u8 *)  (color_base +  height*width + height*width/4);
	else
		v = (u8 *)  (color_base +  height*width + height*width/2);

 
		
		
		if(sq_lcd_set.yuv_format == 0)	//420
		{
			LCD_DBG("YUV420 \n");
			//Y1,Y2,Y3,Y4
			for (i = 0; i < height/2; i++)
			{
				rgb2yuv_convert(BPP24_RGB[i/15], &yuv_color);			
				for (j = 0; j < width/2; j++)
				{

					*y = yuv_color.Y; //Y1
					*(y+1) = yuv_color.Y; //Y2
					*(y+320) = yuv_color.Y; //Y3
					*(y+321) = yuv_color.Y; //Y4

					y+=2;
					//U
					*u = yuv_color.Cb;
					u++;
					//V
					*v = yuv_color.Cr;
					v++;
					
				}
				y+=320;
				
 				
			}
			
		}		//422
		else
		{
			LCD_DBG("YUV422 \n");
			//Y1,Y2,Y3,Y4
			for (i = 0; i < height; i++)
			{
				rgb2yuv_convert(BPP24_RGB[i/30], &yuv_color);			
				for (j = 0; j < width/2; j++)
				{

					*y = yuv_color.Y; //Y1
					*(y+1) = yuv_color.Y; //Y2

					y+=2;
					//U
					*u = yuv_color.Cb;
					u++;
					//V
					*v = yuv_color.Cr;
					v++;
					
				}
			
 				
			}
			
			
		}
 
	return 0;
}

int YUV_Color_Bar_H_Fill(u32 color_base)
{
	struct yuv_format yuv_color;
	
	unsigned int height = DEFAULT_HEIGHT;
	unsigned int width = DEFAULT_WIDTH;

	int i;
	int j;
	int l;

	u8 *y = (u8 *) color_base;
	u8 *u = (u8 *) (color_base +  height*width);
	u8 *v;
	if (sq_lcd_set.yuv_format == 0)
		v = (u8 *)  (color_base +  height*width + height*width/4);
	else
		v = (u8 *)  (color_base +  height*width + height*width/2);


	if(sq_lcd_set.yuv_format == 0)	//420
	{
		LCD_DBG("YUV420 \n");
		for (i = 0; i < height/2; i++)
		{
			
			for (l = 0; l < 8; l++)
			{
				rgb2yuv_convert(BPP24_RGB[l], &yuv_color);			
				for (j = 0; j < 40/2; j++)
				{

					*y = yuv_color.Y; //Y1
					*(y+1) = yuv_color.Y; //Y2
					*(y+320) = yuv_color.Y; //Y3
					*(y+321) = yuv_color.Y; //Y4

					y+=2;
					//U
					*u = yuv_color.Cb;
					u++;
					//V
					*v = yuv_color.Cr;
					v++;
					
				}
			}
			
			y+=320;
			
				
		}
		
	}		//422
	else
	{
		LCD_DBG("YUV422 \n");
		//Y1,Y2,Y3,Y4
		for (i = 0; i < height; i++)
		{
			for (l = 0; l < 8; l++)
			{
				rgb2yuv_convert(BPP24_RGB[l], &yuv_color);			
				for (j = 0; j < 40/2; j++)
				{

					*y = yuv_color.Y; //Y1
					*(y+1) = yuv_color.Y; //Y2

					y+=2;
					//U
					*u = yuv_color.Cb;
					u++;
					//V
					*v = yuv_color.Cr;
					v++;
					
				}
			}
		
				
		}
		
		
	}
 
	return 0;
}

int YUV420_Grid_Fill(int height, int color_offset, struct yuv_format *yuv_base)
{
#if 0
	int i,l,j;
	struct yuv_format yuv_color;	
	
	for (i = 0; i < height/2; i++)
	{
			for (l = 0; l < 8; l++)
			{
				rgb2yuv_convert(BPP24_RGB[l], &yuv_color);			
				for (j = 0; j < 40/2; j++)
				{
					*(yuv_base->Y) = yuv_color.Y; //Y1
					*(yuv_base->Y+1) = yuv_color.Y; //Y2
					*(yuv_base->Y+320) = yuv_color.Y; //Y3
					*(yuv_base->Y+321) = yuv_color.Y; //Y4

					yuv_base->Y+=2;
					//U
					*(yuv_base->Cb) = yuv_color.Cb;
					yuv_base->Cb++;
					//V
					*(yuv_base->Cr) = yuv_color.Cr;
					yuv_base->Cr++;
					
				}
			}
			yuv_base->Y+=320;
	}
#endif
	return 0;
}

int YUV_Color_Grid_Fill(u32 color_base)
{
	struct yuv_format yuv_color;
	
	unsigned int height = DEFAULT_HEIGHT;
	unsigned int width = DEFAULT_WIDTH;

	int i;
	int j;
	int l;
	int m;
	u32 tmprgb1[8];
	u32 tmprgb2[9];

	u8 *y = (u8 *) color_base;
	u8 *u = (u8 *) (color_base +  height*width);
	u8 *v;

	for(j=0;j<8;j++) 
		tmprgb1[j] = BPP24_RGB[j];
	
	if (sq_lcd_set.yuv_format == 0)
		v = (u8 *)  (color_base +  height*width + height*width/4);
	else
		v = (u8 *)  (color_base +  height*width + height*width/2);


	if(sq_lcd_set.yuv_format == 0)	//420
	{
		LCD_DBG("YUV420 \n");
		for (m = 0;m<8;m++)
		{
			for (i = 0; i < height/2/8; i++)
			{
					for (l = 0; l < 8; l++)
					{
						rgb2yuv_convert(tmprgb1[l], &yuv_color);			
						for (j = 0; j < 40/2; j++)
						{

							*y = yuv_color.Y; //Y1
							*(y+1) = yuv_color.Y; //Y2
							*(y+320) = yuv_color.Y; //Y3
							*(y+321) = yuv_color.Y; //Y4

							y+=2;
							//U
							*u = yuv_color.Cb;
							u++;
							//V
							*v = yuv_color.Cr;
							v++;
							
						}
					}
					y+=320;
			}
			//shit color
			for(j=0;j<8;j++) 
				tmprgb2[j+1] = tmprgb1[j];
			tmprgb2[0] = tmprgb2[8];
			for(j=0;j<8;j++) 
				tmprgb1[j] = tmprgb2[j];
			
		}

		
	}		//422
	else
	{
		LCD_DBG("YUV422 \n");
		for (m = 0;m<8;m++)
		{
			for (i = 0; i < height/8; i++)
			{
					for (l = 0; l < 8; l++)
					{
						rgb2yuv_convert(tmprgb1[l], &yuv_color);			
						for (j = 0; j < 40/2; j++)
						{

							*y = yuv_color.Y; //Y1
							*(y+1) = yuv_color.Y; //Y2

							y+=2;
							//U
							*u = yuv_color.Cb;
							u++;
							//V
							*v = yuv_color.Cr;
							v++;
							
						}
					}
			}
			//shit color
			for(j=0;j<8;j++) 
				tmprgb2[j+1] = tmprgb1[j];
			tmprgb2[0] = tmprgb2[8];
			for(j=0;j<8;j++) 
				tmprgb1[j] = tmprgb2[j];
			
		}
		
		
	}
 
	return 0;
}
int yuv_color_bar_v_fill()
{
	if(sq_lcd_set.swap == 0)	
	{
		YUV_Color_Bar_V_Fill(COLOR_BAR_BASE0);
	}
	else
	{
		YUV_Color_Bar_V_Fill(COLOR_BAR_BASE0);	
		YUV_Color_Bar_V_Fill(COLOR_BAR_BASE1);		
	}
	//Enable CLCD
	sq_lcd_write(sq_lcd_read(SQ_LCD_CTRL0)| SQ_LCD_CTRL0_ENABLE, SQ_LCD_CTRL0);
	
	return 0;
}

int yuv_color_bar_h_fill()
{
	if(sq_lcd_set.swap == 0)	
	{
		YUV_Color_Bar_H_Fill(COLOR_BAR_BASE0);
	}
	else
	{
		YUV_Color_Bar_H_Fill(COLOR_BAR_BASE0);
		YUV_Color_Bar_H_Fill(COLOR_BAR_BASE1);		
	}
	//Enable CLCD
	sq_lcd_write(sq_lcd_read(SQ_LCD_CTRL0)| SQ_LCD_CTRL0_ENABLE, SQ_LCD_CTRL0);
	
	return 0;
}

int yuv_color_bar_grid_fill()
{
	if(sq_lcd_set.swap == 0)	
	{
		YUV_Color_Grid_Fill(COLOR_BAR_BASE0);
	}
	else
	{
		YUV_Color_Grid_Fill(COLOR_BAR_BASE0);	
		YUV_Color_Grid_Fill(COLOR_BAR_BASE1);		
	}
	//Enable CLCD
	sq_lcd_write(sq_lcd_read(SQ_LCD_CTRL0)| SQ_LCD_CTRL0_ENABLE, SQ_LCD_CTRL0);
	
	return 0;
}

struct test_item_container yuv_color_fill_test_container;

int color_fill_black(int autotest)
{
	Memory_Color_Set(0);
	//Enable CLCD
	sq_lcd_write(sq_lcd_read(SQ_LCD_CTRL0)| SQ_LCD_CTRL0_ENABLE, SQ_LCD_CTRL0);

	return 0;
}

int color_fill_red(int autotest)
{
	Memory_Color_Set(1);
	//Enable CLCD
	sq_lcd_write(sq_lcd_read(SQ_LCD_CTRL0)| SQ_LCD_CTRL0_ENABLE, SQ_LCD_CTRL0);

	return 0;
}

int color_fill_green(int autotest)
{
	Memory_Color_Set(2);
	//Enable CLCD
	sq_lcd_write(sq_lcd_read(SQ_LCD_CTRL0)| SQ_LCD_CTRL0_ENABLE, SQ_LCD_CTRL0);

	return 0;
}

int color_fill_blue(int autotest)
{
	Memory_Color_Set(3);
	//Enable CLCD
	sq_lcd_write(sq_lcd_read(SQ_LCD_CTRL0)| SQ_LCD_CTRL0_ENABLE, SQ_LCD_CTRL0);

	return 0;
}

int color_fill_cyan(int autotest)
{
	Memory_Color_Set(4);
	//Enable CLCD
	sq_lcd_write(sq_lcd_read(SQ_LCD_CTRL0)| SQ_LCD_CTRL0_ENABLE, SQ_LCD_CTRL0);

	return 0;
}

int color_fill_magenta(int autotest)
{
	Memory_Color_Set(5);
	//Enable CLCD
	sq_lcd_write(sq_lcd_read(SQ_LCD_CTRL0)| SQ_LCD_CTRL0_ENABLE, SQ_LCD_CTRL0);

	return 0;
}

int color_fill_yellow(int autotest)
{
	Memory_Color_Set(6);
	//Enable CLCD
	sq_lcd_write(sq_lcd_read(SQ_LCD_CTRL0)| SQ_LCD_CTRL0_ENABLE, SQ_LCD_CTRL0);

	return 0;
}

int color_fill_white(int autotest)
{
	Memory_Color_Set(7);
	//Enable CLCD
	sq_lcd_write(sq_lcd_read(SQ_LCD_CTRL0)| SQ_LCD_CTRL0_ENABLE, SQ_LCD_CTRL0);

	return 0;
}

int Sq_LUT_Fill(int index , u32 color_base)
{
  unsigned int height = DEFAULT_HEIGHT;
  unsigned int width = DEFAULT_WIDTH;
  int i, j;
  
  u8 *p = (u8 *) color_base;

  for (i = 0; i < height; i++)
    {
      for (j = 0; j < width; j++)
	*p++ = index;
    }
  return 0;
  
}

int Sq_Color_16BPP_FSFill (int color , u32 color_base)
{
  unsigned int height = DEFAULT_HEIGHT;
  unsigned int width = DEFAULT_WIDTH;
  int i, j;
  
  u16 *p = (u16 *) color_base;

  for (i = 0; i < height; i++)
    {
      for (j = 0; j < width; j++)
	*p++ = color;
    }
  return 0;
}

int Sq_ColorFSFill (u32 color , u32 color_base)
{
  unsigned int height = DEFAULT_HEIGHT;
  unsigned int width = DEFAULT_WIDTH;
  int i, j;
  
  unsigned int *p = (unsigned int *) color_base;

  for (i = 0; i < height; i++)
    {
      for (j = 0; j < width; j++)
	*p++ = color;
    }

  return 0;
}

int Sq_CLCD_ColorBar (void)
{
	unsigned int height = DEFAULT_HEIGHT;
	unsigned int width = DEFAULT_WIDTH;
	unsigned int bar_height = DEFAULT_BAR_HEIGHT;
	int i, j, color;
	unsigned int *p = (unsigned int *) COLOR_BAR_BASE0;

	for (i = 0, color = 0; i < height; i++)
	{
		for (j = 0; j < width; j++)
		switch (color)
		{
			case 0:
			*p++ = COLOR_RED;
			break;
			case 1:
			*p++ = COLOR_MAGENTA;
			break;
			case 2:
			*p++ = COLOR_YELLOW;
			break;
			case 3:
			*p++ = COLOR_GREEN;
			break;
			case 4:
			*p++ = COLOR_BLUE;
			break;
			case 5:
			*p++ = COLOR_CYAN;
			break;
			case 6:
			*p++ = COLOR_WHITE;
			break;
			case 7:
			*p++ = COLOR_BALCK;
			break;
		}
		if (i % bar_height == 0 && i != 0)
			color++;
	}
	return 0;
}

int Sq_16BPP_ColorBar (void)
{
	unsigned int height = DEFAULT_HEIGHT;
	unsigned int width = DEFAULT_WIDTH;
	unsigned int bar_height = DEFAULT_BAR_HEIGHT;
	int i, j, color;
	u16 *p = (u16 *) COLOR_BAR_BASE0;

	for (i = 0, color = 0; i < height; i++)
	{
		for (j = 0; j < width; j++)
			*p++ = BPP16_RGB_R[color];

		if (i % bar_height == 0 && i != 0)
			color++;
	}
	return 0;
}

void sq_cade_lcd_isr(void* pparam)
{
	int intr;
	u32_t* pcnt = (u32_t*) pparam;
	
	intr = sq_lcd_read(SQ_LCD_INTR_STS);

	if(intr & SQ_LCD_INTR_DMA_ERR)
	{
		printf("SQ_LCD_INTR_DMA_ERR \n");
	}

	if(intr & SQ_LCD_INTR_OUT_HALT)
	{
		printf("SQ_LCD_INTR_OUT_HALT \n");
	}

	if(intr & SQ_LCD_INTR_FIFO_EMPTY)
	{
		printf("SQ_LCD_INTR_FIFO_EMPTY \n");
		//Reset CLCD
		sq_lcd_write(sq_lcd_read(SQ_LCD_CTRL0) & ~SQ_LCD_CTRL0_ENABLE, SQ_LCD_CTRL0);
		sq_lcd_write(sq_lcd_read(SQ_LCD_CTRL0) | SQ_LCD_CTRL0_ENABLE, SQ_LCD_CTRL0);
	}

	if(intr & SQ_LCD_INTR_LUT_COMP)
	{
		printf("SQ_LCD_INTR_LUT_COMP \n");
	}

	if(intr & SQ_LCD_INTR_PAGE0_READ)
	{
		if (sq_lcd_set.swap == 1)
		{
			if (sq_lcd_set.bpp == 1) //24 bpp
			{
//				if (!(((*pcnt)/5)%8) % 2)
					Sq_ColorFSFill (BPP24_RGB[((*pcnt)/5)%8],COLOR_BAR_BASE0);
			}
			else if (sq_lcd_set.bpp == 2)
			{
				Sq_LUT_Fill (((*pcnt)/5)%8,COLOR_BAR_BASE0);
			}
			else 
			{
//				if (!(((*pcnt)/5)%8) % 2) {
					if(sq_lcd_set.bpp16mode == 1)
						Sq_Color_16BPP_FSFill (BPP16_RGB_R[((*pcnt)/5)%8],COLOR_BAR_BASE0);
					else if(sq_lcd_set.bpp16mode == 2) 
						Sq_Color_16BPP_FSFill (BPP16_RGB_G[((*pcnt)/5)%8],COLOR_BAR_BASE0);
					else if (sq_lcd_set.bpp16mode == 3)
						Sq_Color_16BPP_FSFill (BPP16_RGB_B[((*pcnt)/5)%8],COLOR_BAR_BASE0);
					else
					{
						if(sq_lcd_set.lummode == SQ_LCD_CTRL0_LUMCONFIG_0)
						{
							Sq_Color_16BPP_FSFill (BPP16_LUM_0[((*pcnt)/5)%8],COLOR_BAR_BASE0);
						}
						else if(sq_lcd_set.lummode == SQ_LCD_CTRL0_LUMCONFIG_R)
						{
							Sq_Color_16BPP_FSFill (BPP16_LUM_R[((*pcnt)/5)%8],COLOR_BAR_BASE0);
						}
						else if(sq_lcd_set.lummode == SQ_LCD_CTRL0_LUMCONFIG_G)
						{
							Sq_Color_16BPP_FSFill (BPP16_LUM_G[((*pcnt)/5)%8],COLOR_BAR_BASE0);
						}
						else if(sq_lcd_set.lummode == SQ_LCD_CTRL0_LUMCONFIG_B)
						{
							Sq_Color_16BPP_FSFill (BPP16_LUM_B[((*pcnt)/5)%8],COLOR_BAR_BASE0);
						}
						else
						{
							Sq_Color_16BPP_FSFill (BPP16_LUM_RGB[((*pcnt)/5)%8],COLOR_BAR_BASE0);
						}					
					}
//				}
			}
		}
		else		//no swap
		{
			if (sq_lcd_set.bpp == 1) //24 bpp
				Sq_ColorFSFill (BPP24_RGB[((*pcnt)/5)%8],COLOR_BAR_BASE0);
			else if (sq_lcd_set.bpp == 2)
			{
				Sq_LUT_Fill (((*pcnt)/5)%8,COLOR_BAR_BASE0);
			}
			else
			{
				if(sq_lcd_set.bpp16mode == 1)
					Sq_Color_16BPP_FSFill (BPP16_RGB_R[((*pcnt)/5)%8],COLOR_BAR_BASE0);
				else if(sq_lcd_set.bpp16mode == 2) 
					Sq_Color_16BPP_FSFill (BPP16_RGB_G[((*pcnt)/5)%8],COLOR_BAR_BASE0);
				else if (sq_lcd_set.bpp16mode == 3)
					Sq_Color_16BPP_FSFill (BPP16_RGB_B[((*pcnt)/5)%8],COLOR_BAR_BASE0);
				else
				{
					if(sq_lcd_set.lummode == SQ_LCD_CTRL0_LUMCONFIG_0)
					{
						Sq_Color_16BPP_FSFill (BPP16_LUM_0[((*pcnt)/5)%8],COLOR_BAR_BASE0);
					}
					else if(sq_lcd_set.lummode == SQ_LCD_CTRL0_LUMCONFIG_R)
					{
						Sq_Color_16BPP_FSFill (BPP16_LUM_R[((*pcnt)/5)%8],COLOR_BAR_BASE0);
					}
					else if(sq_lcd_set.lummode == SQ_LCD_CTRL0_LUMCONFIG_G)
					{
						Sq_Color_16BPP_FSFill (BPP16_LUM_G[((*pcnt)/5)%8],COLOR_BAR_BASE0);
					}
					else if(sq_lcd_set.lummode == SQ_LCD_CTRL0_LUMCONFIG_B)
					{
						Sq_Color_16BPP_FSFill (BPP16_LUM_B[((*pcnt)/5)%8],COLOR_BAR_BASE0);
					}
					else
					{
						Sq_Color_16BPP_FSFill (BPP16_LUM_RGB[((*pcnt)/5)%8],COLOR_BAR_BASE0);
					}
				
				}
			}
			

			
		}
		(*pcnt)++;
	}
	if(intr & SQ_LCD_INTR_PAGE1_READ)
	{
		if (sq_lcd_set.swap == 1)
		{
			if (sq_lcd_set.bpp == 1) //24 bpp
			{
//				if ((((*pcnt)/5)%8) % 2)
					Sq_ColorFSFill (BPP24_RGB[((*pcnt)/5)%8],COLOR_BAR_BASE1);
			}
			else if (sq_lcd_set.bpp == 2)
			{
				Sq_LUT_Fill (((*pcnt)/5)%8,COLOR_BAR_BASE1);
			}
			else 
			{
//				if ((((*pcnt)/5)%8) % 2) {
					if(sq_lcd_set.bpp16mode == 1)
						Sq_Color_16BPP_FSFill (BPP16_RGB_R[((*pcnt)/5)%8],COLOR_BAR_BASE1);
					else if(sq_lcd_set.bpp16mode == 2) 
						Sq_Color_16BPP_FSFill (BPP16_RGB_G[((*pcnt)/5)%8],COLOR_BAR_BASE1);
					else if(sq_lcd_set.bpp16mode ==3)
						Sq_Color_16BPP_FSFill (BPP16_RGB_B[((*pcnt)/5)%8],COLOR_BAR_BASE1);
					else 
					{
						if(sq_lcd_set.lummode == SQ_LCD_CTRL0_LUMCONFIG_0)
						{
							Sq_Color_16BPP_FSFill (BPP16_LUM_0[((*pcnt)/5)%8],COLOR_BAR_BASE1);
						}
						else if(sq_lcd_set.lummode == SQ_LCD_CTRL0_LUMCONFIG_R)
						{
							Sq_Color_16BPP_FSFill (BPP16_LUM_R[((*pcnt)/5)%8],COLOR_BAR_BASE1);
						}
						else if(sq_lcd_set.lummode == SQ_LCD_CTRL0_LUMCONFIG_G)
						{
							Sq_Color_16BPP_FSFill (BPP16_LUM_G[((*pcnt)/5)%8],COLOR_BAR_BASE1);				
						}
						else if(sq_lcd_set.lummode == SQ_LCD_CTRL0_LUMCONFIG_B)
						{
							Sq_Color_16BPP_FSFill (BPP16_LUM_B[((*pcnt)/5)%8],COLOR_BAR_BASE1);				
						}
						else
						{
							Sq_Color_16BPP_FSFill (BPP16_LUM_RGB[((*pcnt)/5)%8],COLOR_BAR_BASE1);				
						}
					
					}
//				}
			}
		}
		else //no swap
		{
			
			printf("SQ_LCD_INTR_PAGE1_READ error !! ctrl0: %x \n",sq_lcd_read(SQ_LCD_CTRL0));
			
		}
		(*pcnt)++;		
	}
	if(intr & SQ_LCD_INTR_LUT_COMP)
	{
		printf("SQ_LCD_INTR_LUT_COMP \n");
		(*pcnt) = 1;
		free_irq(SQ_LCD_IRQ);
	}	

	if((*pcnt) > 80)
		free_irq(SQ_LCD_IRQ);

	
	
}

void sq_lcd_yuv_isr(void* pparam)
{
	int intr;
	struct yuv_format yuv_color;
	u32_t* pcnt = (u32_t*) pparam;
	
	intr = sq_lcd_read(SQ_LCD_INTR_STS);

	if(intr & SQ_LCD_INTR_DMA_ERR)
	{
		printf("SQ_LCD_INTR_DMA_ERR \n");
	}

	if(intr & SQ_LCD_INTR_OUT_HALT)
	{
		printf("SQ_LCD_INTR_OUT_HALT \n");
	}

	if(intr & SQ_LCD_INTR_FIFO_EMPTY)
	{
		printf("SQ_LCD_INTR_FIFO_EMPTY \n");
		//Reset CLCD
		sq_lcd_write(sq_lcd_read(SQ_LCD_CTRL0) & ~SQ_LCD_CTRL0_ENABLE, SQ_LCD_CTRL0);
		sq_lcd_write(sq_lcd_read(SQ_LCD_CTRL0) | SQ_LCD_CTRL0_ENABLE, SQ_LCD_CTRL0);
	}

	if(intr & SQ_LCD_INTR_LUT_COMP)
	{
		printf("SQ_LCD_INTR_LUT_COMP \n");
	}

	if(intr & SQ_LCD_INTR_PAGE0_READ)
	{
		LCD_DBG("SQ_LCD_INTR_PAGE0_READ \n");
		if (sq_lcd_set.swap == 1)
		{
			rgb2yuv_convert(BPP24_RGB[((*pcnt)/5)%8], &yuv_color);
			YUV_ColorFS_Bottom_fill(&yuv_color, COLOR_BAR_BASE0);
		}
		else		//no swap
		{
			rgb2yuv_convert(BPP24_RGB[((*pcnt)/5)%8], &yuv_color);
			YUV_ColorFS_Bottom_fill(&yuv_color, COLOR_BAR_BASE0);
		}
		(*pcnt)++;
	}
	if(intr & SQ_LCD_INTR_PAGE1_READ)
	{
		LCD_DBG("SQ_LCD_INTR_LUT_COMP \n");
		if (sq_lcd_set.swap == 1)
		{
			rgb2yuv_convert(BPP24_RGB[((*pcnt)/5)%8], &yuv_color);
			YUV_ColorFS_Bottom_fill(&yuv_color, COLOR_BAR_BASE1);
		}
		else		//no swap
		{
			printf("SQ_LCD_INTR_PAGE1_READ error !! ctrl0: %x \n",sq_lcd_read(SQ_LCD_CTRL0));
		}
		(*pcnt)++;
	}
	if(intr & SQ_LCD_INTR_LUT_COMP)
	{
		printf("SQ_LCD_INTR_LUT_COMP \n");
		(*pcnt) = 1;
		free_irq(SQ_LCD_IRQ);
	}	

	if((*pcnt) > 80)
		free_irq(SQ_LCD_IRQ);

	
	
}


struct test_item_container color_fill_test_container;

int lcd_color_fill(int autotest)
{
	int ret;
	if (sq_lcd_set.yuv == 0)
		ret = test_item_ctrl(&color_fill_test_container, autotest);
	else //yuv format
		ret = test_item_ctrl(&yuv_color_fill_test_container, autotest);
	return ret;
}

int lcd_inter(int autotest)
{
	u32_t isr_cnt = 0;
	//initial INTR
	sq_lcd_write(0x3f, SQ_LCD_INTR_EN);

	yuv_color_fill(1);
	
	if(sq_lcd_set.yuv == 0)
		request_irq(SQ_LCD_IRQ, sq_cade_lcd_isr, (void*)&isr_cnt);	
	else
		request_irq(SQ_LCD_IRQ, sq_lcd_yuv_isr, (void*)&isr_cnt);	

	//Enable CLCD
	sq_lcd_write(sq_lcd_read(SQ_LCD_CTRL0)| SQ_LCD_CTRL0_ENABLE, SQ_LCD_CTRL0);

	while(isr_cnt < 80);

	free_irq(SQ_LCD_IRQ);
	//initial INTR
	sq_lcd_write(0, SQ_LCD_INTR_EN);
	
	sq_lcd_write(sq_lcd_read(SQ_LCD_CTRL0) & ~SQ_LCD_CTRL0_ENABLE, SQ_LCD_CTRL0);	

	return 0;
}


struct test_item_container lcd_intr_test_container;
int single_frame_test(int autotest)
{
	int ret;

	
	//Disable PAGE SWAP
	sq_lcd_set.swap = 0;	
	sq_lcd_write(sq_lcd_read(SQ_LCD_CTRL0)& ~SQ_LCD_CTRL0_PAGESWAP, SQ_LCD_CTRL0);

	sq_lcd_write(COLOR_BAR_BASE0, SQ_LCD_PAGE0_ADDR); // Set Page0 Address


	if (sq_lcd_set.bpp == 1)
	{
		Sq_ColorFSFill (BPP24_RGB[1],COLOR_BAR_BASE0);
	}
	else if (sq_lcd_set.bpp == 2)
	{
		Sq_LUT_Fill(1,COLOR_BAR_BASE0);
	}
	else
	{
		if(sq_lcd_set.bpp16mode == 1)
		{
			Sq_Color_16BPP_FSFill (BPP16_RGB_R[1],COLOR_BAR_BASE0);
		}
		else if(sq_lcd_set.bpp16mode == 2)
		{
			Sq_Color_16BPP_FSFill (BPP16_RGB_G[1],COLOR_BAR_BASE0);
		}
		else if(sq_lcd_set.bpp16mode == 3)
		{
			Sq_Color_16BPP_FSFill (BPP16_RGB_B[1],COLOR_BAR_BASE0);
		}
		else 
		{
			if(sq_lcd_set.lummode == SQ_LCD_CTRL0_LUMCONFIG_0)
			{
				Sq_Color_16BPP_FSFill (BPP16_LUM_0[1],COLOR_BAR_BASE0);
			}
			else if(sq_lcd_set.lummode == SQ_LCD_CTRL0_LUMCONFIG_R)
			{
				Sq_Color_16BPP_FSFill (BPP16_LUM_R[1],COLOR_BAR_BASE0);
			}
			else if(sq_lcd_set.lummode == SQ_LCD_CTRL0_LUMCONFIG_G)
			{
				Sq_Color_16BPP_FSFill (BPP16_LUM_G[1],COLOR_BAR_BASE0);
			}
			else if(sq_lcd_set.lummode == SQ_LCD_CTRL0_LUMCONFIG_B)
			{
				Sq_Color_16BPP_FSFill (BPP16_LUM_B[1],COLOR_BAR_BASE0);
			}
			else
			{
				Sq_Color_16BPP_FSFill (BPP16_LUM_RGB[1],COLOR_BAR_BASE0);
			}
		}
	}

	ret = test_item_ctrl(&lcd_intr_test_container, autotest);

	sq_lcd_write(sq_lcd_read(SQ_LCD_CTRL0) & ~SQ_LCD_CTRL0_ENABLE, SQ_LCD_CTRL0);	


	return ret;	
}

int double_frame_test(int autotest)
{
	int ret;

	//Enable PAGE SWAP
	sq_lcd_set.swap = 1;	
	sq_lcd_write(sq_lcd_read(SQ_LCD_CTRL0)| SQ_LCD_CTRL0_PAGESWAP, SQ_LCD_CTRL0);

	sq_lcd_write(COLOR_BAR_BASE0, SQ_LCD_PAGE0_ADDR);
	sq_lcd_write(COLOR_BAR_BASE1, SQ_LCD_PAGE1_ADDR);	

	if (sq_lcd_set.bpp == 1)
	{
		Sq_ColorFSFill (COLOR_RED,COLOR_BAR_BASE0);
		Sq_ColorFSFill (COLOR_BLUE,COLOR_BAR_BASE1);
	}
	else if (sq_lcd_set.bpp == 2)
	{
		Sq_LUT_Fill(1,COLOR_BAR_BASE0);
		Sq_LUT_Fill(1,COLOR_BAR_BASE1);
	}
	else
	{
		if(sq_lcd_set.bpp16mode == 1)
		{
			Sq_Color_16BPP_FSFill (BPP16_RGB_R[1],COLOR_BAR_BASE0);
			Sq_Color_16BPP_FSFill (BPP16_RGB_R[1],COLOR_BAR_BASE1);
		}
		else if(sq_lcd_set.bpp16mode == 2)
		{
			Sq_Color_16BPP_FSFill (BPP16_RGB_G[1],COLOR_BAR_BASE0);
			Sq_Color_16BPP_FSFill (BPP16_RGB_G[1],COLOR_BAR_BASE1);
		}
		else if(sq_lcd_set.bpp16mode == 3)
		{
			Sq_Color_16BPP_FSFill (BPP16_RGB_B[1],COLOR_BAR_BASE0);
			Sq_Color_16BPP_FSFill (BPP16_RGB_B[1],COLOR_BAR_BASE1);
		}
		else		//lum mode
		{
			if(sq_lcd_set.lummode == SQ_LCD_CTRL0_LUMCONFIG_0)
			{
				Sq_Color_16BPP_FSFill (BPP16_LUM_0[1],COLOR_BAR_BASE0);
				Sq_Color_16BPP_FSFill (BPP16_LUM_0[1],COLOR_BAR_BASE1);
			}
			else if(sq_lcd_set.lummode == SQ_LCD_CTRL0_LUMCONFIG_R)
			{
				Sq_Color_16BPP_FSFill (BPP16_LUM_R[1],COLOR_BAR_BASE0);
				Sq_Color_16BPP_FSFill (BPP16_LUM_R[1],COLOR_BAR_BASE1);
			}
			else if(sq_lcd_set.lummode == SQ_LCD_CTRL0_LUMCONFIG_G)
			{
				Sq_Color_16BPP_FSFill (BPP16_LUM_G[1],COLOR_BAR_BASE0);
				Sq_Color_16BPP_FSFill (BPP16_LUM_G[1],COLOR_BAR_BASE1);				
			}
			else if(sq_lcd_set.lummode == SQ_LCD_CTRL0_LUMCONFIG_B)
			{
				Sq_Color_16BPP_FSFill (BPP16_LUM_B[1],COLOR_BAR_BASE0);
				Sq_Color_16BPP_FSFill (BPP16_LUM_B[1],COLOR_BAR_BASE1);				
			}
			else
			{
				Sq_Color_16BPP_FSFill (BPP16_LUM_RGB[1],COLOR_BAR_BASE0);
				Sq_Color_16BPP_FSFill (BPP16_LUM_RGB[1],COLOR_BAR_BASE1);				
			}
		
		}
	}
	
	ret = test_item_ctrl(&lcd_intr_test_container, autotest);

	sq_lcd_write(sq_lcd_read(SQ_LCD_CTRL0) & ~SQ_LCD_CTRL0_ENABLE, SQ_LCD_CTRL0);		
	return ret;
}

struct test_item_container sq_lcd_controller_test_container;

int bpp16_red_test(int autotest)
{
	int ret = 0;
	sq_lcd_set.bpp16mode = 1;
	sq_lcd_write((sq_lcd_read(SQ_LCD_CTRL0) & (~SQ_LCD_CTRL0_COLOUR)) | SQ_LCD_CTRL0_COLOUR_RED, SQ_LCD_CTRL0);	
	ret = test_item_ctrl(&sq_lcd_controller_test_container, autotest);
		return ret;
}

int bpp16_green_test(int autotest)
{
	int ret = 0;
	sq_lcd_set.bpp16mode = 2;
	sq_lcd_write((sq_lcd_read(SQ_LCD_CTRL0) & (~SQ_LCD_CTRL0_COLOUR)) | SQ_LCD_CTRL0_COLOUR_GREEN, SQ_LCD_CTRL0);
	ret = test_item_ctrl(&sq_lcd_controller_test_container, autotest);
		return ret;
}

int bpp16_blue_test(int autotest)
{
	int ret = 0;
	sq_lcd_set.bpp16mode = 3;

	sq_lcd_write((sq_lcd_read(SQ_LCD_CTRL0) & (~SQ_LCD_CTRL0_COLOUR))| SQ_LCD_CTRL0_COLOUR_BLUE, SQ_LCD_CTRL0);	
	ret = test_item_ctrl(&sq_lcd_controller_test_container, autotest);
		return ret;
}

int bpp16_lum0_test(int autotest)
{
	int ret = 0;
	sq_lcd_set.lummode = SQ_LCD_CTRL0_LUMCONFIG_0;

	sq_lcd_write((sq_lcd_read(SQ_LCD_CTRL0) & (~SQ_LCD_CTRL0_LUMCONFIG))| (sq_lcd_set.lummode << 17), SQ_LCD_CTRL0);	
	ret = test_item_ctrl(&sq_lcd_controller_test_container, autotest);
		return ret;

}

int bpp16_lumr_test(int autotest)
{
	int ret = 0;
	sq_lcd_set.lummode = SQ_LCD_CTRL0_LUMCONFIG_R;

	sq_lcd_write((sq_lcd_read(SQ_LCD_CTRL0) & (~SQ_LCD_CTRL0_LUMCONFIG))| (sq_lcd_set.lummode << 17), SQ_LCD_CTRL0);	
	ret = test_item_ctrl(&sq_lcd_controller_test_container, autotest);
		return ret;

}

int bpp16_lumg_test(int autotest)
{
	int ret = 0;
	sq_lcd_set.lummode = SQ_LCD_CTRL0_LUMCONFIG_G;

	sq_lcd_write((sq_lcd_read(SQ_LCD_CTRL0) & (~SQ_LCD_CTRL0_LUMCONFIG))| (sq_lcd_set.lummode << 17), SQ_LCD_CTRL0);	
	ret = test_item_ctrl(&sq_lcd_controller_test_container, autotest);
		return ret;

}

int bpp16_lumb_test(int autotest)
{
	int ret = 0;
	sq_lcd_set.lummode = SQ_LCD_CTRL0_LUMCONFIG_B;

	sq_lcd_write((sq_lcd_read(SQ_LCD_CTRL0) & (~SQ_LCD_CTRL0_LUMCONFIG))| (sq_lcd_set.lummode << 17), SQ_LCD_CTRL0);	
	ret = test_item_ctrl(&sq_lcd_controller_test_container, autotest);
		return ret;
}

int bpp16_lumrgb_test(int autotest)
{
	int ret = 0;
	sq_lcd_set.lummode = SQ_LCD_CTRL0_LUMCONFIG_RGB;

	sq_lcd_write((sq_lcd_read(SQ_LCD_CTRL0) & (~SQ_LCD_CTRL0_LUMCONFIG))| (sq_lcd_set.lummode << 17), SQ_LCD_CTRL0);	
	ret = test_item_ctrl(&sq_lcd_controller_test_container, autotest);
		return ret;
}

struct test_item_container sq_lcd_16bpp_lum_test_container;
int bpp16_lum_test(int autotest)
{
	int ret = 0;
	sq_lcd_set.bpp16mode = 0;
	sq_lcd_write((sq_lcd_read(SQ_LCD_CTRL0) & (~SQ_LCD_CTRL0_COLOUR)) , SQ_LCD_CTRL0);	
	ret = test_item_ctrl(&sq_lcd_16bpp_lum_test_container, autotest);
		return ret;
}

int tft_lut_test(int autotest)
{
	int color;
	int i=0;
	int ret = 0;
	u32_t isr_cnt = 0;
	
	sq_lcd_set.bpp = 2;
	sq_lcd_write(sq_lcd_read(SQ_LCD_CTRL0)| SQ_LCD_CTRL0_COLOURDEP | SQ_LCD_CTRL0_LUTEN, SQ_LCD_CTRL0); // LUT Depth 24bpp  
                                                                                                        // LUT enable       
	u32 *p = (u32 *) LCD_LUT_BASE;

	for (color = 0; color < 8; color++)
	{
		*p++ = BPP24_RGB[color];
	}

	sq_lcd_write(LCD_LUT_BASE , SQ_LCD_LUT_ADDR);  // Set LUT Address

	//initial INTR
	sq_lcd_write(0x3f, SQ_LCD_INTR_EN); // Enable All int  
	                                    // MDA Error,Output Halted,Output FIFO empty  
	                                    // LUT load Complete 
	                                    // page 1 load from memory 
	                                    // page 0 load from memory  	                                    
	request_irq(sq_lcd_irq, sq_cade_lcd_isr, (void*)&isr_cnt);	

	while(isr_cnt == 0)
	{
		i++;
		if(i>50000) {
			printf("Load LUT timeout\n");
			break;
		}
	}

	printf("free_irq \n");
	free_irq(SQ_LCD_IRQ);
	//Diable INTR
	sq_lcd_write(0, SQ_LCD_INTR_EN);
	
	ret = test_item_ctrl(&sq_lcd_controller_test_container, autotest);

	//Disable LUT 
	sq_lcd_write(sq_lcd_read(SQ_LCD_CTRL0) &  (~SQ_LCD_CTRL0_LUTEN), SQ_LCD_CTRL0);
	return ret;
}

struct test_item_container sq_lcd_controller_16bpp_test_container;

int tft_16bpp_test(int autotest)
{
	int ret = 0;

	sq_lcd_set.bpp = 0;
	sq_lcd_write(sq_lcd_read(SQ_LCD_CTRL0) & ~SQ_LCD_CTRL0_24BPP  ,SQ_LCD_CTRL0);

	ret = test_item_ctrl(&sq_lcd_controller_16bpp_test_container, autotest);

	sq_lcd_write(sq_lcd_read(SQ_LCD_CTRL0) & (~SQ_LCD_CTRL0_LUMCONFIG) & (~SQ_LCD_CTRL0_COLOUR) ,SQ_LCD_CTRL0);	
	return ret;
}

int tft_24bpp_test(int autotest)
{
	int ret = 0;

	//Enable 24 BPP
	sq_lcd_set.bpp = 1;
	sq_lcd_write(sq_lcd_read(SQ_LCD_CTRL0)| SQ_LCD_CTRL0_24BPP ,SQ_LCD_CTRL0);
	
	ret = test_item_ctrl(&sq_lcd_controller_test_container, autotest);
	return ret;
}

struct test_item_container sq_lcd_controller_bpp_container;

int lcd_rgb_mode_test(int autotest)
{
	int ret = 0;
	sq_lcd_set.yuv = 0;
	sq_lcd_write(sq_lcd_read(SQ_LCD_YUV2RGB_CTRL)& (~SQ_LCD_YUV2RGB_EN),SQ_LCD_YUV2RGB_CTRL);
	ret = test_item_ctrl(&sq_lcd_controller_bpp_container, autotest);
	return ret;
}

int yuv_single_frame_test(int autotest)
{
	int ret;
	unsigned int height = DEFAULT_HEIGHT;
	unsigned int width = DEFAULT_WIDTH;
	
	//Disable PAGE SWAP
	sq_lcd_set.swap = 0;	
	sq_lcd_write(sq_lcd_read(SQ_LCD_CTRL0)& ~SQ_LCD_CTRL0_PAGESWAP, SQ_LCD_CTRL0);

	if(sq_lcd_set.yuv_format == 1)
	{
		sq_lcd_write(COLOR_BAR_BASE0, SQ_LCD_Y_PAGE0_ADDR);
		sq_lcd_write(COLOR_BAR_BASE0 + (height*width), SQ_LCD_Cb_PAGE0_ADDR);
		sq_lcd_write(COLOR_BAR_BASE0 + (height*width) + (height*width)/2, SQ_LCD_Cr_PAGE0_ADDR);
	}
	else
	{
		sq_lcd_write(COLOR_BAR_BASE0, SQ_LCD_Y_PAGE0_ADDR);
		sq_lcd_write(COLOR_BAR_BASE0 + (height*width), SQ_LCD_Cb_PAGE0_ADDR);
		sq_lcd_write(COLOR_BAR_BASE0 + (height*width) + (height*width)/4, SQ_LCD_Cr_PAGE0_ADDR);
	}

	yuv_color_fill(1);	

	ret = test_item_ctrl(&lcd_intr_test_container, autotest);

	sq_lcd_write(sq_lcd_read(SQ_LCD_CTRL0) & ~SQ_LCD_CTRL0_ENABLE, SQ_LCD_CTRL0);	


	return ret;	
}

int yuv_double_frame_test(int autotest)
{
	int ret;
	unsigned int height = DEFAULT_HEIGHT;
	unsigned int width = DEFAULT_WIDTH;
	
	//Enable PAGE SWAP
	sq_lcd_set.swap = 1;	
	sq_lcd_write(sq_lcd_read(SQ_LCD_CTRL0)| SQ_LCD_CTRL0_PAGESWAP, SQ_LCD_CTRL0);

	if(sq_lcd_set.yuv_format == 1)
	{
		sq_lcd_write(COLOR_BAR_BASE0, SQ_LCD_Y_PAGE0_ADDR);
		sq_lcd_write(COLOR_BAR_BASE0 + (height*width), SQ_LCD_Cb_PAGE0_ADDR);
		sq_lcd_write(COLOR_BAR_BASE0 + (height*width) + (height*width)/2, SQ_LCD_Cr_PAGE0_ADDR);
		sq_lcd_write(COLOR_BAR_BASE1, SQ_LCD_Y_PAGE1_ADDR);
		sq_lcd_write(COLOR_BAR_BASE1 + (height*width), SQ_LCD_Cb_PAGE1_ADDR);
		sq_lcd_write(COLOR_BAR_BASE1 + (height*width) + (height*width)/2, SQ_LCD_Cr_PAGE1_ADDR);
		
	}
	else
	{
		sq_lcd_write(COLOR_BAR_BASE0, SQ_LCD_Y_PAGE0_ADDR);
		sq_lcd_write(COLOR_BAR_BASE0 + (height*width), SQ_LCD_Cb_PAGE0_ADDR);
		sq_lcd_write(COLOR_BAR_BASE0 + (height*width) + (height*width)/4, SQ_LCD_Cr_PAGE0_ADDR);
		sq_lcd_write(COLOR_BAR_BASE1, SQ_LCD_Y_PAGE1_ADDR);
		sq_lcd_write(COLOR_BAR_BASE1 + (height*width), SQ_LCD_Cb_PAGE1_ADDR);
		sq_lcd_write(COLOR_BAR_BASE1 + (height*width) + (height*width)/4, SQ_LCD_Cr_PAGE1_ADDR);
		
	}

	yuv_color_fill(1);	

	ret = test_item_ctrl(&lcd_intr_test_container, autotest);

	sq_lcd_write(sq_lcd_read(SQ_LCD_CTRL0) & ~SQ_LCD_CTRL0_ENABLE, SQ_LCD_CTRL0);	


	return ret;	
}

struct test_item_container sq_lcd_yuv_frame_test_container;

int lcd_yuv420_mode_test(int autotest)
{
	int ret = 0;
	sq_lcd_set.yuv_format = 0;
	sq_lcd_write(sq_lcd_read(SQ_LCD_YUV2RGB_CTRL)| SQ_LCD_YUV420 ,SQ_LCD_YUV2RGB_CTRL);
	ret = test_item_ctrl(&sq_lcd_yuv_frame_test_container, autotest);
	sq_lcd_write(sq_lcd_read(SQ_LCD_YUV2RGB_CTRL)& ~SQ_LCD_YUV420 ,SQ_LCD_YUV2RGB_CTRL);
	return ret;
}

int lcd_yuv422_mode_test(int autotest)
{
	int ret = 0;
	sq_lcd_set.yuv_format = 1;
	sq_lcd_write(sq_lcd_read(SQ_LCD_YUV2RGB_CTRL)| SQ_LCD_YUV422 ,SQ_LCD_YUV2RGB_CTRL);
	ret = test_item_ctrl(&sq_lcd_yuv_frame_test_container, autotest);	
	sq_lcd_write(sq_lcd_read(SQ_LCD_YUV2RGB_CTRL)& ~SQ_LCD_YUV422 ,SQ_LCD_YUV2RGB_CTRL);
	return ret;
}

struct test_item_container sq_lcd_controller_yuv_container;

int lcd_yuv_mode_test(int autotest)
{
	int ret = 0;

	sq_lcd_set.yuv = 1;
	//TEMP ADD BUG FIXME
//	sq_lcd_write(sq_lcd_read(SQ_LCD_CTRL0)| SQ_LCD_CTRL0_24BPP,SQ_LCD_CTRL0);
	
	sq_lcd_write(sq_lcd_read(SQ_LCD_YUV2RGB_CTRL)| SQ_LCD_YUV2RGB_EN,SQ_LCD_YUV2RGB_CTRL);
	
	ret = test_item_ctrl(&sq_lcd_controller_yuv_container, autotest);
	sq_lcd_write(sq_lcd_read(SQ_LCD_YUV2RGB_CTRL) &  (~SQ_LCD_YUV2RGB_EN),SQ_LCD_YUV2RGB_CTRL);
	return ret;
}

struct test_item_container sq_lcd_controller_format_container;

int SQ_LCD_Testing (int autotest)
{
	int ret = 0;

#ifdef CONFIG_SQ8000
	sq_scu_dev_enable(SQ_DEVCON_PWM0);
	sq_scu_dev_enable(SQ_DEVCON_LCDC);
#endif

	//PWM Setting
#ifdef CONFIG_SQ8000
	writel (6, 0x19090004);
	writel (0xf, 0x19090008);
	writel (0x29, 0x1909000c);
#endif
#ifdef CONFIG_LDK3V21
	writel (6, 0x1e750004);
	writel (0xf, 0x1e750008);
	writel (0x29, 0x1e75000c);
#endif
#ifdef CONFIG_PC7210
	writel (6, 0x18024004);
	writel (0xf, 0x18024008);
	writel (0x29, 0x1802400c);	
	//20080123 leonid add for LCD clock
	iowrite32(((ioread32(0x1803c010) & 0xffff)), 0x1803c010);
#endif
	


	//Set Panel timing
	sq_lcd_write(sq_lcd_set.htiming, SQ_LCD_H_TIMING);
	sq_lcd_write(sq_lcd_set.vtiming, SQ_LCD_V_TIMING);
#ifdef CONFIG_SQ8000	
	sq_lcd_write(SQ_LCD_CTRL0_PXCLK_POLAR |(SQ_LCD_CTRL0_PCLOCK * sq_lcd_set.pixelclock)| SQ_LCD_CTRL0_HSYNC | SQ_LCD_CTRL0_VSYNC,SQ_LCD_CTRL0);
#else
	sq_lcd_write((SQ_LCD_CTRL0_PCLOCK * sq_lcd_set.pixelclock)| SQ_LCD_CTRL0_HSYNC | SQ_LCD_CTRL0_VSYNC,SQ_LCD_CTRL0);
#endif
	ret = test_item_ctrl(&sq_lcd_controller_format_container, autotest);

#ifdef CONFIG_SQ8000
	sq_scu_dev_disable(SQ_DEVCON_PWM0);
	sq_scu_dev_disable(SQ_DEVCON_LCDC);
#endif
	
	return ret;
}

extern void Sq_vip_lcd_initial(int output422)
{
#ifdef CONFIG_SQ8000
	sq_scu_dev_enable(SQ_DEVCON_PWM0);
	sq_scu_dev_enable(SQ_DEVCON_LCDC);
#endif

	//PWM Setting
#ifdef CONFIG_SQ8000
	writel (6, 0x19090004);
	writel (0xf, 0x19090008);
	writel (0x29, 0x1909000c);
#endif
#ifdef CONFIG_LDK3V21
	writel (6, 0x1e750004);
	writel (0xf, 0x1e750008);
	writel (0x29, 0x1e75000c);
#endif
#ifdef CONFIG_PC7210
	writel (6, 0x18024004);
	writel (0xf, 0x18024008);
	writel (0x29, 0x1802400c);	
	//20080123 leonid add for LCD clock
	iowrite32(((ioread32(0x1803c010) & 0xffff)), 0x1803c010);
#endif

	//Set Panel timing
	sq_lcd_write(sq_lcd_set.htiming, SQ_LCD_H_TIMING);
	sq_lcd_write(sq_lcd_set.vtiming, SQ_LCD_V_TIMING);
#ifdef CONFIG_SQ8000	
	sq_lcd_write(SQ_LCD_CTRL0_PXCLK_POLAR |(SQ_LCD_CTRL0_PCLOCK * sq_lcd_set.pixelclock)| SQ_LCD_CTRL0_HSYNC | SQ_LCD_CTRL0_VSYNC,SQ_LCD_CTRL0);
#else
	sq_lcd_write((SQ_LCD_CTRL0_PCLOCK * sq_lcd_set.pixelclock)| SQ_LCD_CTRL0_HSYNC | SQ_LCD_CTRL0_VSYNC,SQ_LCD_CTRL0);
#endif
	sq_lcd_set.yuv = 1;
	
	sq_lcd_write(sq_lcd_read(SQ_LCD_YUV2RGB_CTRL)| SQ_LCD_YUV2RGB_EN,SQ_LCD_YUV2RGB_CTRL);
	
	sq_lcd_set.yuv_format = 0;
	if(output422)
		sq_lcd_write(sq_lcd_read(SQ_LCD_YUV2RGB_CTRL)| SQ_LCD_YUV422 ,SQ_LCD_YUV2RGB_CTRL);
	else
		sq_lcd_write(sq_lcd_read(SQ_LCD_YUV2RGB_CTRL)& ~SQ_LCD_YUV422 ,SQ_LCD_YUV2RGB_CTRL);

	//Disable PAGE SWAP
	sq_lcd_set.swap = 0;	
	sq_lcd_write(sq_lcd_read(SQ_LCD_CTRL0)& ~SQ_LCD_CTRL0_PAGESWAP, SQ_LCD_CTRL0);

	sq_lcd_write(0x00a00000, SQ_LCD_Y_PAGE0_ADDR);
	sq_lcd_write(0x00a60000 , SQ_LCD_Cb_PAGE0_ADDR);
	sq_lcd_write(0x00a90000 , SQ_LCD_Cr_PAGE0_ADDR);

	//Enable CLCD
	sq_lcd_write(sq_lcd_read(SQ_LCD_CTRL0)| SQ_LCD_CTRL0_ENABLE, SQ_LCD_CTRL0);

}
extern void Sq_vip_lcd_stop()
{
	sq_lcd_write(sq_lcd_read(SQ_LCD_CTRL0) & ~SQ_LCD_CTRL0_ENABLE, SQ_LCD_CTRL0);
}
