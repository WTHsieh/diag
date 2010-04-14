//TSC2000 Test Function 
#include "spi-master.h"
#include "regs-tsc2000.h"
#if defined(CONFIG_PDK) || defined(CONFIG_PC7210)
#include "../GPIO/gpio.h"
#endif

#define	AR_START		(1 << 7)
#define	AR_A2A1A0_d_y		(1 << 4)	/* differential */
#define	AR_A2A1A0_d_z1		(3 << 4)	/* differential */
#define	AR_A2A1A0_d_z2		(4 << 4)	/* differential */
#define	AR_A2A1A0_d_x		(5 << 4)	/* differential */
#define	AR_A2A1A0_temp0		(0 << 4)	/* non-differential */
#define	AR_A2A1A0_vbatt		(2 << 4)	/* non-differential */
#define	AR_A2A1A0_vaux		(6 << 4)	/* non-differential */
#define	AR_A2A1A0_temp1		(7 << 4)	/* non-differential */
#define	AR_8_BIT		(1 << 3)
#define	AR_12_BIT		(0 << 3)
#define	AR_VREF			(1 << 2)	/* non-differential */
#define	AR_VDD			(0 << 2)	/* differential */
#define	AR_PD10_PDOWN		(0 << 0)	/* lowpower mode + penirq */
#define	AR_PD10_ADC_ON		(1 << 0)	/* ADC on */
#define	AR_PD10_REF_ON		(2 << 0)	/* vREF on + penirq */
#define	AR_PD10_ALL_ON		(3 << 0)	/* ADC + vREF on */

#define	MAX_12BIT		((1<<12)-1)

/* leave ADC powered up (disables penirq) between differential samples */
//#define	READ_12BIT_VDD(x, adc, vref) (AR_START | AR_A2A1A0_d_ ## x \
//	| AR_12_BIT | AR_VDD | \
//	(adc ? AR_PD10_ADC_ON : 0) | (vref ? AR_PD10_REF_ON : 0))

#define	READ_8BIT_VDD(x, adc, vref) (AR_START | AR_A2A1A0_d_ ## x \
	| AR_8_BIT | AR_VDD | \
	(adc ? AR_PD10_ADC_ON : 0) | (vref ? AR_PD10_REF_ON : 0))


#define USE_16BIT 1

#if (USE_16BIT == 0)

#define	READ_12BIT_VDD(x, adc, vref) (AR_START | AR_A2A1A0_d_ ## x \
	| AR_12_BIT | AR_VDD | AR_PD10_PDOWN)


#define	READ_TMP1()	(AR_START | AR_A2A1A0_temp0 | AR_12_BIT | AR_VDD | AR_PD10_ALL_ON)
#define	READ_TMP2()	(AR_START | AR_A2A1A0_temp1 | AR_12_BIT | AR_VDD | AR_PD10_ALL_ON)
#define	READ_BAT()	(AR_START | AR_A2A1A0_vbatt | AR_12_BIT | AR_VDD | AR_PD10_ALL_ON)
#define	READ_AUX()	(AR_START | AR_A2A1A0_vaux  | AR_12_BIT | AR_VDD | AR_PD10_ALL_ON)

#define	READ_Y(vref)	(READ_12BIT_VDD(y,  1, vref))
#define	READ_Z1(vref)	(READ_12BIT_VDD(z1, 1, vref))
#define	READ_Z2(vref)	(READ_12BIT_VDD(z2, 1, vref))
#define	READ_X(vref)	(READ_12BIT_VDD(x,  1, vref))

#else
#define	READ_TMP1()	(AR_START | AR_A2A1A0_temp0 | AR_12_BIT | AR_VDD | AR_PD10_ALL_ON)
#define	READ_TMP2()	(AR_START | AR_A2A1A0_temp1 | AR_12_BIT | AR_VDD | AR_PD10_ALL_ON)
#define	READ_BAT()	(AR_START | AR_A2A1A0_vbatt | AR_12_BIT | AR_VDD | AR_PD10_ALL_ON)
#define	READ_AUX()	(AR_START | AR_A2A1A0_vaux  | AR_12_BIT | AR_VDD | AR_PD10_ALL_ON)

#define	READ(x)	(AR_START | AR_A2A1A0_d_ ## x | AR_12_BIT | AR_VDD | AR_PD10_ALL_ON/*AR_VDD | AR_PD10_PDOWN*/)

#define	READ_X()	(READ(x))
#define	READ_Y()	(READ(y))
#define	READ_Z1()	(READ(z1))
#define	READ_Z2()	(READ(z2))
//#define	READ_X(vref)	(READ_12BIT_VDD(x,  1, vref))

#endif
#define	PWRDOWN		0x80 //(READ_12BIT_VDD(y,  0, 0))	/* LAST */


/* single-ended samples need to first power up reference voltage;
 * we leave both ADC and VREF powered
 */
#define	READ_12BIT_VREF(x) (AR_START | AR_A2A1A0_ ## x \
	| AR_12_BIT | AR_VREF)

#define	REF_ON	(READ_12BIT_VDD(x, 1, 1))
#define	REF_OFF	(READ_12BIT_VDD(y, 0, 0))

volatile unsigned int touch_count;
volatile unsigned int touch_flag;
volatile unsigned int touch_dont_care;

/*
static int
sq_spi_ar7646_write(u8 cmd)
{
	printf("cmd=0x%x\n",cmd);
	if(sq_spi_transfer(&cmd, NULL, SET_TX_RX_LEN(1, 0)))
		return -1;

	return 0;
}

static int
sq_spi_ar7646_read(u8 cmd, u16 *buf)
{
	printf("cmd=0x%x - ",cmd);
	
	if (sq_spi_transfer(cmd, buf, SET_TX_RX_LEN(1, 2)))
		return -1;
	
	printf("read=0x%x\n",*buf);	
	return 0;
}
*/


#if (USE_16BIT == 0)


static int ar7646_shift(u8 *rbuf)
{
   u16 x;
   //printf("h:%x, l: %x ",rbuf[0],rbuf[1]);
			
   x = rbuf[1];  
   //printf("v=%x ",x);  
   
   x += (u16)(rbuf[0]&0x7f)<<8;   
   //printf("v=%x ",x); 
   
   x = x >> 3;
   //printf("v=%x \n",x);   
   return x;

}

static int
sq_spi_ar7646_cmd(u8 cmd, u16 *data)
{
	u8 buf[3];
/*	
	buf[0] = 0xd3;	
	if(sq_spi_transfer(buf, NULL, SET_TX_RX_LEN(1, 0)))
		return -1;
*/
	*data = 0;	
	buf[0] = cmd;	
	if(sq_spi_transfer(buf, data, SET_TX_RX_LEN(1, 2)))
		return -1;		
/*	
	buf[0] = 0xd0;
	if(sq_spi_transfer(buf, NULL, SET_TX_RX_LEN(1, 0)))
		return -1;
*/	
	*data = ar7646_shift(data);	
        printf(" v=%x \n",*data);	
	return 0;
}




static int
sq_spi_ar7646_cmd1(u16 *x, u16 *y, u16 *z1, u16 *z2)
{
	
	u8 buf[3];
	u8 rbuf[2];	
	buf[0] = 0xd3;	
	if(sq_spi_transfer(buf, NULL, SET_TX_RX_LEN(1, 0)))
		return -1;	

	buf[0] = READ_Y(AR_VDD);
	if(sq_spi_transfer(buf, rbuf, SET_TX_RX_LEN(1, 2)))
		return -1;	
	
	*y = ar7646_shift(rbuf);		
	
	buf[0] = READ_X(AR_VDD);
	if(sq_spi_transfer(buf, rbuf, SET_TX_RX_LEN(1, 2)))
		return -1;	
	
	*x = ar7646_shift(rbuf);	
			
	buf[0] = READ_Z1(AR_VDD);
	if(sq_spi_transfer(buf, rbuf, SET_TX_RX_LEN(1, 2)))
		return -1;	
	
	*z1 = ar7646_shift(rbuf);	
 			
	buf[0] = READ_Z2(AR_VDD);
	if(sq_spi_transfer(buf, rbuf, SET_TX_RX_LEN(1, 2)))
		return -1;		
	
	*z2 = ar7646_shift(rbuf);	
	
	buf[0] = 0xd0;	
	if(sq_spi_transfer(buf, NULL, SET_TX_RX_LEN(1, 0)))
		return -1;	
	
	return 0;
}



#else



extern int
sq_spi_transfer2(void *tx_buf, void *rx_buf, u32 len);

static int
sq_spi_ar7646_cmd2(u16 *x, u16 *y)
{
	
	u16 buf;
	u16 rbuf;	
	
	buf = 0xd3;	
	if(sq_spi_transfer2(&buf, &rbuf, SET_TX_RX_LEN(1, 0)))
		return -1;	
	
	//printf("X    "); 
	buf = READ(x) ;		
	if(sq_spi_transfer2(&buf, &rbuf, SET_TX_RX_LEN(1, 1)))
		return -1;	
	*x = rbuf ;		
	
	//printf("Y    "); 
	//buf = READ(x) ;		
	buf = READ(y);	
	if(sq_spi_transfer2(&buf, &rbuf, SET_TX_RX_LEN(1, 1)))
		return -1;		
	*y = rbuf;	
	
	printf("Z1   ");
	//buf = READ(x) ;		
	buf = READ(z1);	
	if(sq_spi_transfer2(&buf, &rbuf, SET_TX_RX_LEN(1, 1)))
		return -1;
	
	printf("Z2   "); 
	//buf = READ(x) ;		
	buf = READ(z2);	
	if(sq_spi_transfer2(&buf, &rbuf, SET_TX_RX_LEN(1, 1)))
		return -1;	

	printf("AUX  "); 
	//buf = READ(x) ;		
	buf = READ_AUX();
	if(sq_spi_transfer2(&buf, &rbuf, SET_TX_RX_LEN(1, 1)))
		return -1;

	printf("BAT  "); 
	//buf = READ(x) ;		
	buf = READ_BAT();
	if(sq_spi_transfer2(&buf, &rbuf, SET_TX_RX_LEN(1, 1)))
		return -1;

	printf("TMP1 "); 
	//buf = READ(x) ;		
	buf = READ_TMP1();
	if(sq_spi_transfer2(&buf, &rbuf, SET_TX_RX_LEN(1, 1)))
		return -1;

	printf("TMP2 "); 
	//buf = READ(x) ;		
	buf = READ_TMP2();
	if(sq_spi_transfer2(&buf, &rbuf, SET_TX_RX_LEN(1, 1)))
		return -1;	

	//printf("EINT "); 	
	buf = 0xd0;
	if(sq_spi_transfer2(&buf, &rbuf, SET_TX_RX_LEN(1, 0)))
		return -1;	
	
	return 0;
}


static int
sq_spi_ar7646_all_cmd(void)
{
	u16 x,y,z1,z2,tmp1,tmp2,vin,vbat,xx,yy;
	u16 buf;
	u16 rbuf;	
/*
	buf = 0xe7;	
	if(sq_spi_transfer2(&buf, &rbuf, SET_TX_RX_LEN(1, 0)))
		return -1;	
*/	
	buf = READ(x) ;		
	if(sq_spi_transfer2(&buf, &x, SET_TX_RX_LEN(1, 1)))
		return -1;			
		
	buf = READ(y);	
	if(sq_spi_transfer2(&buf, &y, SET_TX_RX_LEN(1, 1)))
		return -1;			
		
	buf = READ(z1);	
	if(sq_spi_transfer2(&buf, &z1, SET_TX_RX_LEN(1, 1)))
		return -1;
			
	buf = READ(z2);	
	if(sq_spi_transfer2(&buf, &z2, SET_TX_RX_LEN(1, 1)))
		return -1;	

	buf = READ_AUX();
	if(sq_spi_transfer2(&buf, &vin, SET_TX_RX_LEN(1, 1)))
		return -1;
		
	buf = READ_BAT();
	if(sq_spi_transfer2(&buf, &vbat, SET_TX_RX_LEN(1, 1)))
		return -1;
		
	buf = READ_TMP1();
	if(sq_spi_transfer2(&buf, &tmp1, SET_TX_RX_LEN(1, 1)))
		return -1;
	
	buf = READ_TMP2();
	if(sq_spi_transfer2(&buf, &tmp2, SET_TX_RX_LEN(1, 1)))
		return -1;	
	
	buf = 0xe0;
	if(sq_spi_transfer2(&buf, &rbuf, SET_TX_RX_LEN(1, 1)))
		return -1;
	xx = ((x-0x60) * 320) / (0xfb0-0x60);
	yy = ((y-0x80) * 240) / (0xf20-0x80);	
//	touch_dont_care = 1;
	printf("X:0x%x,Y:0x%x,Z1:0x%x,Z2:0x%x,AUX:0x%x,BAT:0x%x,T1:0x%x,T2:0x%x,xx=%d,yy=%d\n",x,y,z1,z2,vin,vbat,tmp1,tmp2,xx,yy);		
	return 0;
}

#endif
/* 
Driver design note:
interrupt&pressed=0=>disable ts irq,timer=1ms,get xy,pressed=1,enable ts irq,timer=1ms
interrupt&pressed=1=>check if PSM,pressed=0/1

kernel timer routine:only when pressed=1&PSM=0=>pressed=0 & reenable ts irq, no longer to set the routine
otherwise, set the routine repeatedly, the routine is in charge of get_xy & button=UP/DOWN

ts_isr is only a trigger event to setup the timer routine

STS have to be checked before get_xy!
*/
void
ar7646_isr (void *pparam)
{

	u16 buf;
/*
	if(touch_dont_care == 1) { 
		touch_dont_care = 0;
		touch_flag = 0;
	} else {
		buf = 0xe7;	
		sq_spi_transfer2(&buf, NULL, SET_TX_RX_LEN(1, 0));	  
		touch_flag = 1;
	}
*/
	buf = 0xe7;	
	sq_spi_transfer2(&buf, NULL, SET_TX_RX_LEN(1, 0));	
	touch_flag = 1;

}

set_spi_char_len(len){
	sq_spi_write((sq_spi_read(SQ_SPI_SSCR) & ~0x7800) | 
			len ,
			SQ_SPI_SSCR);
}


extern int
sq_spi_ar7646_touch(int autotest)
{
	u8 divisor;
	u16 rx_buf[1] = {0};
	u32 i;
	u16 x,y,z1,z2;	
	touch_count=0;
	touch_flag=0;

	/* Reset SPI controller */
	sq_spi_write(
			sq_spi_read(SQ_SPI_FWCR) | 
			SQ_SPI_SOFT_RST,
			SQ_SPI_FWCR);

	/* Configure SPI controller */
	sq_spi_write(
//#ifdef CONFIG_PC9220
			SQ_SPI_MASTER_SIGNAL_CTL_HW |
			SQ_SPI_MASTER_SIGNAL_ACT_NO |		
			SQ_SPI_MODE_MASTER |
//#endif			
			SQ_SPI_SOFT_N_RST |
			SQ_SPI_TXRX_N_RUN |
			SQ_SPI_CLK_IDLE_AST |
			SQ_SPI_TXRX_SIMULT_DIS |
			SQ_SPI_CPOL_0 |
			SQ_SPI_CPHA_0 |//SQ_SPI_CPHA_1
			SQ_SPI_TX_MSB_FIRST |//SQ_SPI_TX_MSB_FIRST
			SQ_SPI_OP_NORMAL,
			SQ_SPI_FWCR);

	/* Enable SPI interrupt */
	sq_spi_write(
			SQ_SPI_IER_RXFIFO_INT_EN |
			SQ_SPI_IER_RXFIFO_OVR_INT_EN |
			SQ_SPI_IER_RX_COMPLETE_INT_EN,
			SQ_SPI_IER);

	/* Configure FIFO and clear Tx & Rx FIFO */
	sq_spi_write(
			SQ_SPI_RXFIFO_INT_TRIGGER_LEVEL_4 |
			SQ_SPI_TXFIFO_INT_TRIGGER_LEVEL_4 |
			SQ_SPI_RXFIFO_CLR |
			SQ_SPI_TXFIFO_CLR,
			SQ_SPI_FCR);
    
	/* Set the SPI slaves select and characteristic control register */
	divisor = sq_spi_calculate_divisor(400000);//3000000 /* 3.0 MHz clock rate */
#if (USE_16BIT == 0)
	sq_spi_write(
			SQ_SPI_CHAR_LEN_8 |   //8 16
			SQ_SPI_SLAVE_SEL_0 |
			SQ_SPI_CLK_DIV(divisor),
			SQ_SPI_SSCR);
	/* Set per char length */
	sq_spi_set_current_mode(MODE_CHAR_LEN_8);//8
#else
	sq_spi_write(
			SQ_SPI_CHAR_LEN_16 |   //8 16
			SQ_SPI_SLAVE_SEL_0 |
			SQ_SPI_CLK_DIV(divisor),
			SQ_SPI_SSCR);
	/* Set per char length */
	sq_spi_set_current_mode(MODE_CHAR_LEN_16);//8
#endif
	
	/* Config SPI clock delay */
	sq_spi_write(
			SQ_SPI_PBTXRX_DELAY_4 |//SQ_SPI_PBTXRX_DELAY_NONE
			SQ_SPI_PBCT_DELAY_4 |//SQ_SPI_PBCT_DELAY_NONE
			SQ_SPI_PBCA_DELAY_4,//SQ_SPI_PBCA_DELAY_1_2
			SQ_SPI_DLYCR);
						

	// enable interrupt
#if defined(CONFIG_PDK) 
	//cyli 20071122 modify
	//set PA5 as low level triggle interrupt
	if (sq_request_gpio_irq(SET_GPIO_PIN_NUM(PA, 5), tsc2000_isr, GPIO_INT_SENSE_LEVEL | GPIO_INT_SINGLE_EDGE | GPIO_INT_EVENT_LO, NULL)) {
		printf("touch_screen_test: GPIO pin[%d] is busy!\n", SET_GPIO_PIN_NUM(PA, 5));
		return -1;
	}
#elif defined(CONFIG_PC7210)	//20080123 Leonid add
	//set PF6 as low level triggle interrupt
	if (sq_request_gpio_irq(SET_GPIO_PIN_NUM(PF, 6), tsc2000_isr, GPIO_INT_SENSE_LEVEL | GPIO_INT_SINGLE_EDGE | GPIO_INT_EVENT_LO, NULL)) {
		printf("touch_screen_test: GPIO pin[%d] is busy!\n", SET_GPIO_PIN_NUM(PF, 6));
		return -1;
	}
#else

#ifdef CONFIG_PC9220
	sq_scu_dev_enable(sq_DEVCON_EXT_INT0);
#endif

	request_irq(TSC2000_INTR, ar7646_isr, null);
#endif
#if (USE_16BIT == 0)
   sq_spi_ar7646_cmd(0x80,rx_buf);//e7
#else
	rx_buf[0] = 0x80;
	sq_spi_transfer2(&rx_buf, NULL, SET_TX_RX_LEN(1, 0));
#endif

	printf("=======================\n");
	printf("====Please Touch Panel ====\n");
	printf("=======================\n");
	touch_flag=0;	
	//test

	touch_flag=0;
	while(1) {
		if(touch_flag != 0) {
			printf("[P]\n");
			touch_flag=0;
			sq_spi_ar7646_all_cmd();

		}	
				
	}

	while(1)
	{
		
	  	if(touch_flag != 0)
		{
			#if (USE_16BIT == 1)
			sq_spi_ar7646_all_cmd();
			//sq_spi_ar7646_cmd1(&x,&y,&z1,&z2);
			/*
			sq_spi_ar7646_cmd2(&x,&y);			
			printf("X: %x, Y: %x\n",x,y);	
			*/
			#else


			sq_spi_ar7646_cmd(READ_Y(AR_VDD),&y);
			sq_spi_ar7646_cmd(READ_X(AR_VDD),&x);			
		
			sq_spi_ar7646_cmd(READ_Z1(AR_VDD),&z1);
			sq_spi_ar7646_cmd(READ_Z2(AR_VDD),&z2);
									
			printf("X: %x, Y: %x, Z1: %x, Z2: %x \n",x,y,z1,z2);
			sq_spi_ar7646_cmd(0xd0,rx_buf);//e7		
			#endif

			
			touch_flag=0;
			touch_count++;
		
		}
		if(touch_count == 100)
			break;
	}
	return 0;
	
	// enable interrupt
#if defined(CONFIG_PDK) || defined(CONFIG_PC7210)
	//cyli 20071122 modify
	//set PA5 as low level triggle interrupt
	sq_free_gpio_irq(SET_GPIO_PIN_NUM(PA, 5));
#else
#ifdef CONFIG_PC9220
	sq_scu_dev_disable(sq_DEVCON_EXT_INT0);
#endif
	free_irq(TSC2000_INTR);
#endif
	
}
