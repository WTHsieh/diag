//TSC2000 Test Function 
#include "spi-master.h"
#include "regs-tsc2000.h"
#if defined(CONFIG_PDK) || defined(CONFIG_PC7210)
#include "../GPIO/gpio.h"
#endif

volatile unsigned int touch_count;
volatile unsigned int touch_flag;

static int
sq_spi_tsc2000_write(u16 addr, u16 data)
{
	u16 tx_buf[2] = {0};

	//Commnad
	tx_buf[0] =  addr & WRITE_CMD;
	//Data
	tx_buf[1] = data;

	if(sq_spi_transfer(tx_buf, NULL, SET_TX_RX_LEN(2, 0)))
		return -1;

	return 0;
}

static int
sq_spi_tsc2000_read(u16 addr, u16 *buf, u32 len)
{
	u16 tx_buf[1] = {0};

	tx_buf[0] = addr | READ_CMD;
	
	if (sq_spi_transfer(tx_buf, buf, SET_TX_RX_LEN(1, len)))
		return -1;
	return 0;
}


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
tsc2000_isr (void *pparam)
{
	touch_flag=1;

}

extern int
sq_spi_tsc2000_touch(int autotest)
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
#ifdef CONFIG_SQ8000
			SQ_SPI_MASTER_SIGNAL_CTL_HW |
			SQ_SPI_MASTER_SIGNAL_ACT_NO |		
			SQ_SPI_MODE_MASTER |
#endif			
			SQ_SPI_SOFT_N_RST |
			SQ_SPI_TXRX_N_RUN |
			SQ_SPI_CLK_IDLE_AST |
			SQ_SPI_TXRX_SIMULT_DIS |
			SQ_SPI_CPOL_0 |
			SQ_SPI_CPHA_1 |
			SQ_SPI_TX_MSB_FIRST |
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
	divisor = sq_spi_calculate_divisor(3000000); /* 3.0 MHz clock rate */

	sq_spi_write(
			SQ_SPI_CHAR_LEN_16 |
			SQ_SPI_SLAVE_SEL_0 |
			SQ_SPI_CLK_DIV(divisor),
			SQ_SPI_SSCR);

	/* Config SPI clock delay */
	sq_spi_write(
			SQ_SPI_PBTXRX_DELAY_NONE |
			SQ_SPI_PBCT_DELAY_NONE |
			SQ_SPI_PBCA_DELAY_1_2,
			SQ_SPI_DLYCR);

	/* Set per char length */
	sq_spi_set_current_mode(MODE_CHAR_LEN_16);

	//RESET tsc2000 & print register 
	sq_spi_tsc2000_write(TSC2000_REG_RESET,0xbb00);
	sq_spi_tsc2000_read(TSC2000_REG_RESET,rx_buf,1);
	printf("Reset value = %x \n",rx_buf);

	printf("Print TSC2000 initial value \n");
	for (i=0;i<5;i++) 
	{
		sq_spi_tsc2000_read(((0 << 11) | (i << 5)),rx_buf,1);
		printf("PAGE0 [%x]: %x \n",i,rx_buf[0]);
	}

	for (i=0;i<5;i++) 
	{
		sq_spi_tsc2000_read(((1 << 11) | (i << 5)),rx_buf,1);
		printf("PAGE1 [%x]: %x \n",i,rx_buf[0]);
	}

		
	sq_spi_tsc2000_write(TSC2000_REG_CONFIG, 0x003f);

	//sq_spi_tsc2000_write(TSC2000_REG_ADC, 0x86f0) //10 bit resolution
//	sq_spi_tsc2000_write(TSC2000_REG_ADC, 0x85f0); //8 bit resolution
	sq_spi_tsc2000_write(TSC2000_REG_ADC, 0x89f0); //8 bit resolution

	for (i=0;i<5;i++) 
	{
		sq_spi_tsc2000_read(((0 << 11) | (i << 5)),rx_buf,1);
		printf("PAGE0 [%x]: %x \n",i,rx_buf[0]);
	}

	for (i=0;i<5;i++) 
	{
		sq_spi_tsc2000_read(((1 << 11) | (i << 5)),rx_buf,1);
		printf("PAGE1 [%x]: %x \n",i,rx_buf[0]);
	}

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
#ifdef CONFIG_SQ8000
	sq_scu_dev_enable(SQ_DEVCON_EXT_INT0);
#endif
	request_irq(TSC2000_INTR, tsc2000_isr, null);
#endif


	printf("=======================\n");
	printf("====Please Touch Panel ====\n");
	printf("=======================\n");
	
	while(1)
	{
		if(touch_flag==1)
		{
			sq_spi_tsc2000_read(TSC2000_REG_X,&x,1);
			sq_spi_tsc2000_read(TSC2000_REG_Y,&y,1);
			sq_spi_tsc2000_read(TSC2000_REG_Z1,&z1,1);
			sq_spi_tsc2000_read(TSC2000_REG_Z2,&z2,1);
			printf("X: %x, Y: %x, Z1: %x, Z2: %x \n",x,y,z1,z2);
			touch_flag=0;
			touch_count++;
		
		}
		if(touch_count == 10)
			break;
	}
	return 0;
	
	// enable interrupt
#if defined(CONFIG_PDK) || defined(CONFIG_PC7210)
	//cyli 20071122 modify
	//set PA5 as low level triggle interrupt
	sq_free_gpio_irq(SET_GPIO_PIN_NUM(PA, 5));
#else
#ifdef CONFIG_SQ8000
	sq_scu_dev_disable(SQ_DEVCON_EXT_INT0);
#endif
	free_irq(TSC2000_INTR);
#endif
	
}
