#include <test_item.h>
#include <genlib.h>
#include <io.h>
#include <type.h>
#include <dma/dma.h>
#include "i2s-regs.h"
#include "i2s-ctrl.h"
#include "dependency.h"
#include "codec-ctrl.h"
#if defined (CONFIG_SQ8000)
#include <scu.h>
#include "../../platform/arch/scu-reg.h"
#endif

#define I2S_DEBUG_ON

static u32 sq_i2s_base = SQ_I2S_BASE;
static u32 sq_i2s_irq = SQ_I2S_IRQ;

static void inline 
sq_i2s_write(u32 val, u32 reg)
{
	iowrite32(val, sq_i2s_base + reg);
}

static u32 inline 
sq_i2s_read(u32 reg)
{
	u32 val;

	val = ioread32(sq_i2s_base + reg);

	return val;
}

#ifdef CONFIG_UDA1342TS
static u32 frequecy_setting = SQ_I2S_OVERSAMPLING_RATE_64 | SQ_I2S_RATIO(12);
#else	//mosa 6335
static u32 frequecy_setting = SQ_I2S_OVERSAMPLING_RATE_32 | SQ_I2S_RATIO(12);
#endif

static int sq_i2s_capture_mode_test = 0;
static int sq_i2s_loopback_test = 0;
static int (*sq_i2s_hwdma_panther7_hdma_internal_loopback)(int autotest);
static int vol_change = 0;
static int vol = MAX_VOLUME / 2;

extern int sq_i2s_play_pcm_normal(int autotest);
extern int sq_i2s_play_pcm_hwdma_panther7_hdma(int autotest);
extern int sq_i2s_capture_pcm_normal(int autotest);
extern int sq_i2s_capture_pcm_hwdma_panther7_hdma(int autotest);
static int sq_i2s_normal_internal_loopback(int autotest);
static int sq_i2s_hwdma_panther7_hdma_direct(int autotest);
static int sq_i2s_hwdma_panther7_hdma_ring_buffer(int autotest);
static void sq_i2s_loopback_isr(void *data);
static void sq_i2s_play_pcm_isr(void *data);
static void sq_i2s_capture_isr(void *data);
static void sq_i2s_tx_dma_page_interrupt(void *data);
static void sq_i2s_rx_dma_page_interrupt(void *data);
static void sq_i2s_tx_dma_play_complete(void *data);
static void sq_i2s_rx_dma_capture_complete(void *data);

static struct sq_dma_notifier sq_i2s_tx_dma_notifier = {
	.complete = sq_i2s_tx_dma_play_complete,
	.page_interrupt = sq_i2s_tx_dma_page_interrupt,
};

static struct sq_dma_notifier sq_i2s_rx_dma_notifier = {
	.page_interrupt = sq_i2s_rx_dma_page_interrupt,
};

static struct sq_dma_notifier sq_i2s_tx_dma_play_notifier = {
	.complete = sq_i2s_tx_dma_play_complete,
};

static struct sq_dma_notifier sq_i2s_rx_dma_capture_notifier = {
	.complete = sq_i2s_rx_dma_capture_complete,
};

static u32 sq_i2s_tx_dma_ch_num, sq_i2s_rx_dma_ch_num;
static u32 sq_i2s_tx_dma_ext_hdreq, sq_i2s_rx_dma_ext_hdreq;
static u32 sq_i2s_dma_burst;
static u32 sq_i2s_base;
static u32 sq_i2s_bus_if, sq_i2s_sample_res, sq_i2s_oversample_rate;
static u32 sq_i2s_tx_mode_sel, sq_i2s_rx_mode_sel;


extern struct test_item_container sq_i2s_play_pcm_mode_test_container;
extern struct test_item_container sq_i2s_capture_mode_test_container;

extern int 
sq_i2s_play_pcm_8_test(int autotest)
{
	int ret = 0;

	sq_i2s_sample_res = SQ_I2S_SAMPLE_RES_8;

	if(sq_i2s_capture_mode_test == 1)
		ret = test_item_ctrl(&sq_i2s_capture_mode_test_container, autotest);
	else
		ret = test_item_ctrl(&sq_i2s_play_pcm_mode_test_container, autotest);
	
	return ret;
}

extern int 
sq_i2s_play_pcm_16_test(int autotest)
{
	int ret = 0;

	sq_i2s_sample_res = SQ_I2S_SAMPLE_RES_16;

	if(sq_i2s_capture_mode_test == 1)
		ret = test_item_ctrl(&sq_i2s_capture_mode_test_container, autotest);
	else
		ret = test_item_ctrl(&sq_i2s_play_pcm_mode_test_container, autotest);

	return ret;
}

extern struct test_item_container sq_i2s_hwdma_panther7_hdma_burst_test_container;
extern int 
sq_i2s_hwdma_panther7_hdma_direct_test(int autotest)
{
	int ret = 0;

	sq_i2s_hwdma_panther7_hdma_internal_loopback = sq_i2s_hwdma_panther7_hdma_direct;

	ret = test_item_ctrl(&sq_i2s_hwdma_panther7_hdma_burst_test_container, autotest);
	return ret;
}

extern int 
sq_i2s_hwdma_panther7_hdma_ring_buffer_test(int autotest)
{
	int ret = 0;

	sq_i2s_hwdma_panther7_hdma_internal_loopback = sq_i2s_hwdma_panther7_hdma_ring_buffer;

	ret = test_item_ctrl(&sq_i2s_hwdma_panther7_hdma_burst_test_container, autotest);
	return ret;
}

extern int 
sq_i2s_hwdma_panther7_hdma_burst_single(int autotest)
{
	int ret = 0;

	sq_i2s_dma_burst = SQ_DMA_BURST_SINGLE;
	ret = sq_i2s_hwdma_panther7_hdma_internal_loopback(autotest);
	return ret;
}

extern int 
sq_i2s_hwdma_panther7_hdma_burst_incr4(int autotest)
{
	int ret = 0;

	sq_i2s_dma_burst = SQ_DMA_BURST_INCR4;
	ret = sq_i2s_hwdma_panther7_hdma_internal_loopback(autotest);
	return ret;
}

extern int 
sq_i2s_hwdma_panther7_hdma_burst_incr8(int autotest)
{
	int ret = 0;

	sq_i2s_dma_burst = SQ_DMA_BURST_INCR8;
	ret = sq_i2s_hwdma_panther7_hdma_internal_loopback(autotest);
	return ret;
}

extern int 
sq_i2s_hwdma_panther7_hdma_burst_incr16(int autotest)
{
	int ret = 0;

	sq_i2s_dma_burst = SQ_DMA_BURST_INCR16;
	ret = sq_i2s_hwdma_panther7_hdma_internal_loopback(autotest);
	return ret;
}

extern int sq_i2s_play_pcm_normal_test(int autotest)
{
	int ret = 0;

	sq_audio_control_function = NULL;
	ret = sq_i2s_play_pcm_normal(autotest);
	return ret;
	
}

extern int sq_i2s_play_pcm_hwdma_panther7_hdma_test(int autotest)
{
	int ret = 0;

	ret = sq_i2s_play_pcm_hwdma_panther7_hdma(autotest);
	return ret;
}

extern struct test_item_container sq_i2s_adc_control_test_container;

extern int sq_i2s_capture_pcm_normal_test(int autotest)
{
	int ret = 0;

	/* Record */
	sq_audio_control_function = NULL;
	ret = sq_i2s_capture_pcm_normal(autotest);

	/* Play */
	sq_audio_control_function = NULL;
	ret = sq_i2s_play_pcm_normal(autotest);

	return ret;
}

extern int sq_i2s_capture_pcm_hwdma_panther7_hdma_test(int autotest)
{
	int ret = 0;
	/* Record */
	ret = sq_i2s_capture_pcm_hwdma_panther7_hdma(autotest);
	/* Play */
	ret = sq_i2s_play_pcm_hwdma_panther7_hdma(autotest);
	return ret;
}

static void sq_i2s_make_test_pattern(u8 *buf, u32 size);
static int sq_i2s_compare_memory(u8 *mem, u8 *cmpr_mem, u32 size,
				    int skip_cmpr_result);
static void sq_i2s_access_ring_buffer(u32 buf_pos, int dir);


#define PATTERN_BUF_ADDR 0x00a00000
#define PATTERN_BUF_SIZE 2048
#define PCM_BUF_ADDR 0x00800000
#define PCM_BUF_SIZE (1 * 1024 * 1024) /* 1MB */
#define DMA_BUF_SIZE (320 * 1024) /* 320K */
#define RING_BUF_ADDR 0x00700000
#define RING_BUF_SIZE (64 * 1024) /* 64K */
#define PERIOD_SIZE (16 * 1024)	/* 16k */
#define PERIODS 4

static u32 sq_i2s_conf;
static u8 *sq_i2s_pattern_buf = (u8 *)PATTERN_BUF_ADDR;
static u8 *sq_i2s_cmpr_buf = (u8 *)(PATTERN_BUF_ADDR + PATTERN_BUF_SIZE);
static u8 *sq_i2s_pcm_buf = (u8 *)PCM_BUF_ADDR;
static u8 *sq_i2s_ring_buf = (u8 *)RING_BUF_ADDR;
static u8 *sq_i2s_dma_buf = (u8 *)(PCM_BUF_ADDR + (2 * DMA_BUF_SIZE));
static u32 sq_i2s_isr_tx_buf_idx = 0;
static u32 *sq_i2s_isr_tx_buf_32 = 0;
static u32 sq_i2s_isr_rx_buf_idx = 0;
static u32 *sq_i2s_isr_rx_buf_32 = 0;
static u32 sq_i2s_isr_tx_total_len = 0;
static u32 sq_i2s_isr_rx_total_len = 0;
static volatile int sq_i2s_tx_complete_flag = 0;
static volatile int sq_i2s_rx_complete_flag = 0;
static volatile int sq_i2s_tx_complete_interrupt_flag = 0;
static volatile int sq_i2s_rx_complete_interrupt_flag = 0;
static u32 sq_i2s_pcm_buf_pos;
static u32 sq_i2s_ring_buf_pos;
static u32 sq_i2s_dma_ring_buf_pos;
static int sq_i2s_period_int_flag;

extern struct test_item_container sq_i2s_sample_res_test_container;

extern int 
sq_i2s_oversample_rate_32_test(int autotest)
{
	int ret = 0;

	sq_i2s_oversample_rate = SQ_I2S_OVERSAMPLING_RATE_32;
	ret = test_item_ctrl(&sq_i2s_sample_res_test_container, autotest);
	return ret;
}

extern int 
sq_i2s_oversample_rate_64_test(int autotest)
{
	int ret = 0;

	sq_i2s_oversample_rate = SQ_I2S_OVERSAMPLING_RATE_64;
	ret = test_item_ctrl(&sq_i2s_sample_res_test_container, autotest);
	return ret;
}

extern int 
sq_i2s_oversample_rate_128_test(int autotest)
{
	int ret = 0;

	sq_i2s_oversample_rate = SQ_I2S_OVERSAMPLING_RATE_128;
	ret = test_item_ctrl(&sq_i2s_sample_res_test_container, autotest);
	return ret;
}

extern struct test_item_container sq_i2s_master_slave_test_container;

extern int 
sq_i2s_sample_resolution_8(int autotest)
{
	int ret = 0;

	sq_i2s_sample_res = SQ_I2S_SAMPLE_RES_8;
	ret = test_item_ctrl(&sq_i2s_master_slave_test_container, autotest);
	return ret;
}

extern int sq_i2s_sample_resolution_16(int autotest)
{
	int ret = 0;

	sq_i2s_sample_res = SQ_I2S_SAMPLE_RES_16;
	ret = test_item_ctrl(&sq_i2s_master_slave_test_container, autotest);
	return ret;
}

extern int sq_i2s_sample_resolution_20(int autotest)
{
	int ret = 0;

	sq_i2s_sample_res = SQ_I2S_SAMPLE_RES_20;
	ret = test_item_ctrl(&sq_i2s_master_slave_test_container, autotest);
	return ret;
}

extern int sq_i2s_sample_resolution_24(int autotest)
{
	int ret = 0;

	sq_i2s_sample_res = SQ_I2S_SAMPLE_RES_24;
	ret = test_item_ctrl(&sq_i2s_master_slave_test_container, autotest);
	return ret;
}

extern struct test_item_container sq_i2s_tx_test_container;

extern int 
sq_i2s_tx_master_rx_slave(int autotest)
{
	int ret = 0;

	sq_i2s_tx_mode_sel = SQ_I2S_MASTER;
	sq_i2s_rx_mode_sel = SQ_I2S_SLAVE;
	ret = test_item_ctrl(&sq_i2s_tx_test_container, autotest);
	return ret;
}

extern int 
sq_i2s_rx_master_tx_slave(int autotest)
{
	int ret = 0;

	sq_i2s_tx_mode_sel = SQ_I2S_SLAVE;
	sq_i2s_rx_mode_sel = SQ_I2S_MASTER;
	ret = test_item_ctrl(&sq_i2s_tx_test_container, autotest);
	return ret;
}

static int 
sq_i2s_normal_internal_loopback(autotest)
{
	u32 tmp;

	request_irq(sq_i2s_irq , sq_i2s_loopback_isr, NULL);	//set iirg enable & mask 
	
	
	/* Initialize the operation stop register */
	sq_i2s_conf = SQ_I2S_TX_N_RST |           // don't reset Tx logic
		SQ_I2S_RX_N_RST |                     // don't reset Rx logic
		SQ_I2S_HDMA_REQ_1_DIS |               // disable HDMA req1     
		SQ_I2S_HDMA_REQ_2_DIS |               // disable HDMA req2         
		SQ_I2S_HDMA_IF_1_TX |                 // HDMA req1 ch used for TX FIFO
		SQ_I2S_HDMA_IF_2_RX |                 // HDMA req2 ch used for RX FIFO
		SQ_I2S_OP_LOOPBACK |                  // set Loop-Back mode  
		SQ_I2S_TX_OP_STP |                    // set tansmitter Stop
		SQ_I2S_RX_OP_STP;                     // set receiver Stop 
		
	/* Stop the tx and rx operation */
	sq_i2s_write(sq_i2s_conf, SQ_I2S_OPR);

	/* Reset the Tx and Rx logic and FSM */
	sq_i2s_write( 
			sq_i2s_conf |
			SQ_I2S_TX_RST |                  // Reset Tx logic 
			SQ_I2S_RX_RST,                   // Reset Rx logic 
			SQ_I2S_OPR);

	/* Clear Rx fifo first */
	tmp = sq_i2s_read(SQ_I2S_FIFOSTS);
	while (!(tmp & SQ_I2S_RX_FIFO_EMPTY)) {
		printf("RX fifo reset not clear \n");
		sq_i2s_read(SQ_I2S_RXR);
		tmp = sq_i2s_read(SQ_I2S_FIFOSTS);
	}

	/* Set the transmitter */
	sq_i2s_write(
			SQ_I2S_TX_DEV_SEL_0 |             // Select Tx Device 0
			sq_i2s_oversample_rate |          // set Oversampling Rate
			SQ_I2S_RATIO(255) |               // set I2S Ratio
			sq_i2s_sample_res |               // set sample resolution 
			SQ_I2S_STEREO | 
			sq_i2s_bus_if |                   // set bus interface I2S or Right Left Justified  
			sq_i2s_tx_mode_sel,
			SQ_I2S_TXCTL);

	/* Set the receiver */
	sq_i2s_write(
			SQ_I2S_RX_FIFO_CLR |
			sq_i2s_oversample_rate |
			SQ_I2S_RATIO(255) |
			sq_i2s_sample_res |
			SQ_I2S_STEREO |
			sq_i2s_bus_if |
			sq_i2s_rx_mode_sel,
			SQ_I2S_RXCTL);

	/* Set the interrupt trigger level */
	sq_i2s_write(
			SQ_I2S_TX_INT_TRIG_LEV_ALMOST_EMPTY |
			SQ_I2S_RX_INT_TRIG_LEV_HALF_FULL,
			SQ_I2S_FIFOSTS);

	/* Set the interrupt enable */
	sq_i2s_write(
			SQ_I2S_TX_FIFO_TRIG_INT_EN |
			SQ_I2S_RX_FIFO_TRIG_INT_EN |
			SQ_I2S_RX_FIFO_OVR_INT_EN,                // set Rx Over-run int enable   
			SQ_I2S_IER);

	/* Clear pattern buffer and compare buffer */
	memset(sq_i2s_pattern_buf, 0x0, PATTERN_BUF_SIZE);
	memset(sq_i2s_cmpr_buf, 0x0, PATTERN_BUF_SIZE);

	/* Make the pattern */
	sq_i2s_make_test_pattern(sq_i2s_pattern_buf, PATTERN_BUF_SIZE);

	/* Reset the flags */
	sq_i2s_tx_complete_flag = 0;
	sq_i2s_rx_complete_flag = 0;	

	/* Reset the buffer index */
	sq_i2s_isr_tx_buf_idx = 0;
	sq_i2s_isr_rx_buf_idx = 0;
	sq_i2s_isr_tx_total_len = PATTERN_BUF_SIZE;

	sq_i2s_isr_tx_buf_32 = (u32 *)sq_i2s_pattern_buf;
	sq_i2s_isr_rx_buf_32 = (u32 *)sq_i2s_cmpr_buf;

	/* Pre-fill the tx fifo */
	while ((sq_i2s_read(SQ_I2S_FIFOSTS) & SQ_I2S_TX_FIFO_FULL) != 
	       SQ_I2S_TX_FIFO_FULL) {
		sq_i2s_write(
				sq_i2s_isr_tx_buf_32[sq_i2s_isr_tx_buf_idx++],
				SQ_I2S_TXR);
	}

	/* Start to transfer */
	if (SQ_I2S_MASTER == sq_i2s_tx_mode_sel)
		sq_i2s_write(
				sq_i2s_conf | SQ_I2S_TX_OP_STR,
				SQ_I2S_OPR);
	else if (SQ_I2S_MASTER == sq_i2s_rx_mode_sel)
		sq_i2s_write(
				sq_i2s_conf | SQ_I2S_RX_OP_STR,
				SQ_I2S_OPR);

	/* Wait for the transimission to be complete */
	if (sq_wait_for_int(&sq_i2s_rx_complete_flag, 30)) {
		printf("Sq I2S host: transimission is timeout\n");
		return -1;
	}

	/* Disable the interrupt */
	sq_i2s_write(
			SQ_I2S_TX_FIFO_TRIG_INT_DIS |
			SQ_I2S_RX_FIFO_TRIG_INT_DIS |
			SQ_I2S_RX_FIFO_OVR_INT_DIS,
			SQ_I2S_IER);


	/* Stop the transmission */
	sq_i2s_write(
			sq_i2s_conf,
			SQ_I2S_OPR);

	free_irq(sq_i2s_irq);
	
	if ((-1 == sq_i2s_tx_complete_flag) ||
	    (-1 == sq_i2s_rx_complete_flag)) {
		return -1;
	} else 
		return sq_i2s_compare_memory(sq_i2s_pattern_buf, sq_i2s_cmpr_buf, PATTERN_BUF_SIZE,
						autotest);
}

static int 
sq_i2s_hwdma_panther7_hdma_direct(int autotest)
{
	u32 tmp;

        sq_i2s_tx_dma_ch_num = PANTHER7_HDMA_CH_0;
        sq_i2s_rx_dma_ch_num = PANTHER7_HDMA_CH_1;
        sq_i2s_tx_dma_ext_hdreq = TX_DMA_EXT_HDREQ;
        sq_i2s_rx_dma_ext_hdreq = RX_DMA_EXT_HDREQ;

#ifdef I2S_DEBUG_ON
		printf("sq_i2s_hwdma_p7_hdma_direct Start\n");
#endif 


	sq_request_dma(sq_i2s_tx_dma_ch_num, &sq_i2s_tx_dma_notifier);
	sq_request_dma(sq_i2s_rx_dma_ch_num, &sq_i2s_rx_dma_notifier);

	/* Initialize the tx and rx operation configuration */
	sq_i2s_conf = SQ_I2S_TX_N_RST |
		SQ_I2S_RX_N_RST |
		SQ_I2S_HDMA_REQ_1_EN |
		SQ_I2S_HDMA_REQ_2_EN |
		SQ_I2S_HDMA_IF_1_TX |
		SQ_I2S_HDMA_IF_2_RX |
		SQ_I2S_OP_LOOPBACK |
		SQ_I2S_TX_OP_STP |
		SQ_I2S_RX_OP_STP;

	/* Stop the tx and rx operation */
	sq_i2s_write(sq_i2s_conf, SQ_I2S_OPR);

	/* Reset the Tx and Rx logic and FSM */
	sq_i2s_write(
			sq_i2s_conf |
			SQ_I2S_TX_RST |
			SQ_I2S_RX_RST,
			SQ_I2S_OPR);

	/* Clear Rx fifo first */
	tmp = sq_i2s_read(SQ_I2S_FIFOSTS);
	while (!(tmp & SQ_I2S_RX_FIFO_EMPTY)) {
		sq_i2s_read(SQ_I2S_RXR);
		tmp = sq_i2s_read(SQ_I2S_FIFOSTS);
	}

	/* Set the transmitter */
	sq_i2s_write(
			SQ_I2S_TX_DEV_SEL_0 |
			sq_i2s_oversample_rate |
			SQ_I2S_RATIO(255) |
			sq_i2s_sample_res |
			SQ_I2S_STEREO |
			sq_i2s_bus_if |
			sq_i2s_tx_mode_sel,
			SQ_I2S_TXCTL);

	/* Set the receiver */
	sq_i2s_write(
			SQ_I2S_RX_FIFO_CLR |
			sq_i2s_oversample_rate |
			SQ_I2S_RATIO(255) |
			sq_i2s_sample_res |
			SQ_I2S_STEREO |
			sq_i2s_bus_if |
			sq_i2s_rx_mode_sel,
			SQ_I2S_RXCTL);

	/* Set the interrupt trigger level */
	sq_i2s_write(
			SQ_I2S_TX_INT_TRIG_LEV_HALF_FULL |
			SQ_I2S_RX_INT_TRIG_LEV_HALF_FULL,
			SQ_I2S_FIFOSTS);

	/* Disable all the interrupts */
	sq_i2s_write(
			SQ_I2S_TX_FIFO_TRIG_INT_DIS |
			SQ_I2S_RX_FIFO_TRIG_INT_DIS |
			SQ_I2S_RX_FIFO_OVR_INT_DIS,
			SQ_I2S_IER);

	/* Clear pattern buffer and compare buffer */
	memset(sq_i2s_pcm_buf, 0x00, DMA_BUF_SIZE);
	memset(sq_i2s_dma_buf, 0x00, DMA_BUF_SIZE);

	/* Make the pattern */
	sq_i2s_make_test_pattern(sq_i2s_pcm_buf, DMA_BUF_SIZE);

	/* Reset the flags */
	sq_i2s_tx_complete_flag = 0;
	sq_i2s_rx_complete_flag = 0;	

	/* Configure the hardware dma settng of HDMA for tx channels */
	sq_disable_dma(sq_i2s_tx_dma_ch_num);
	sq_set_dma_mode(sq_i2s_tx_dma_ch_num, SQ_DMA_MODE_SLICE);
	sq_set_dma_ext_hdreq_number(sq_i2s_tx_dma_ch_num, sq_i2s_tx_dma_ext_hdreq);
	sq_set_dma_burst_type(sq_i2s_tx_dma_ch_num, sq_i2s_dma_burst);
	sq_set_dma_source_address(sq_i2s_tx_dma_ch_num, (u32)sq_i2s_pcm_buf);
	sq_set_dma_destination_address(sq_i2s_tx_dma_ch_num, SQ_I2S_TXR+sq_i2s_base);
	sq_set_dma_source_direction(sq_i2s_tx_dma_ch_num, SQ_DMA_DIR_INCR);
	sq_set_dma_destination_direction(sq_i2s_tx_dma_ch_num, SQ_DMA_DIR_FIXED);
	sq_set_dma_data_size(sq_i2s_tx_dma_ch_num, SQ_DMA_DATA_WORD);
	sq_set_dma_transfer_count(sq_i2s_tx_dma_ch_num, PERIOD_SIZE);   // tx_cnt = 16 * 1024
	sq_set_dma_slice_count(sq_i2s_tx_dma_ch_num, FIFO_DEPTH>>1);    // FIFO = 8
	sq_set_dma_page_number(sq_i2s_tx_dma_ch_num, 20);               // set page number = 20
	sq_set_dma_buffer_size(sq_i2s_tx_dma_ch_num, DMA_BUF_SIZE);     // 320 * 1024   = total transfer size = 16k * 20 (page number) 
		// please check [sq_i2s_tx_dma_play_complete] 
		// Transmit one page every page interrupt  

	/* Configure the hardware dma settng of HDMA for rx channels */
	sq_disable_dma(sq_i2s_rx_dma_ch_num);
	sq_set_dma_mode(sq_i2s_rx_dma_ch_num, SQ_DMA_MODE_SLICE);
	sq_set_dma_ext_hdreq_number(sq_i2s_rx_dma_ch_num, sq_i2s_rx_dma_ext_hdreq);
	sq_set_dma_burst_type(sq_i2s_rx_dma_ch_num, sq_i2s_dma_burst);
	sq_set_dma_source_address(sq_i2s_rx_dma_ch_num, SQ_I2S_RXR+sq_i2s_base);
	sq_set_dma_destination_address(sq_i2s_rx_dma_ch_num, (u32)sq_i2s_dma_buf);
	sq_set_dma_source_direction(sq_i2s_rx_dma_ch_num, SQ_DMA_DIR_FIXED);
	sq_set_dma_destination_direction(sq_i2s_rx_dma_ch_num, SQ_DMA_DIR_INCR);
	sq_set_dma_data_size(sq_i2s_rx_dma_ch_num, SQ_DMA_DATA_WORD);
	sq_set_dma_transfer_count(sq_i2s_rx_dma_ch_num, PERIOD_SIZE);
	sq_set_dma_slice_count(sq_i2s_rx_dma_ch_num, FIFO_DEPTH>>1);
	sq_set_dma_page_number(sq_i2s_rx_dma_ch_num, 20);
	sq_set_dma_buffer_size(sq_i2s_rx_dma_ch_num, DMA_BUF_SIZE);

	/* Start to transfer */
	if (SQ_I2S_MASTER == sq_i2s_tx_mode_sel)
		sq_i2s_write(
				sq_i2s_conf |SQ_I2S_TX_OP_STR,
				SQ_I2S_OPR);
	else if (SQ_I2S_MASTER == sq_i2s_rx_mode_sel)
		sq_i2s_write(
				sq_i2s_conf |SQ_I2S_RX_OP_STR,
				SQ_I2S_OPR);

	/* Enable the dma to run*/
	sq_enable_dma(sq_i2s_tx_dma_ch_num);
	sq_enable_dma(sq_i2s_rx_dma_ch_num);

	/* Wait for the transimission to be complete */
	/*20080114 JS Modify 30s to 60s */
	if (sq_wait_for_int(&sq_i2s_rx_complete_flag, 60)) {     // before  sq_i2s_rx_complete_flag =1
		printf("Sq I2S host: transimission is timeout\n");   // Tx will transmit one page 
		return -1;
	}

	/* Stop the tx and rx operation */
	sq_i2s_write(sq_i2s_conf, SQ_I2S_OPR);

	sq_disable_dma(sq_i2s_tx_dma_ch_num);
	sq_disable_dma(sq_i2s_rx_dma_ch_num);
	sq_free_dma(sq_i2s_tx_dma_ch_num);
	sq_free_dma(sq_i2s_rx_dma_ch_num);
#ifdef I2S_DEBUG_ON
	printf("sq_i2s_hwdma_p7_hdma_direct Finish\n");
#endif 

	if ((-1 == sq_i2s_tx_complete_flag) ||
	    (-1 == sq_i2s_rx_complete_flag)) {
		return -1;
	} else 
		return sq_i2s_compare_memory(sq_i2s_pcm_buf, sq_i2s_dma_buf, DMA_BUF_SIZE,
						autotest);
}

static int
sq_i2s_hwdma_panther7_hdma_ring_buffer(int autotest)
{
	u32 tmp;

        sq_i2s_tx_dma_ch_num = PANTHER7_HDMA_CH_0;
        sq_i2s_rx_dma_ch_num = PANTHER7_HDMA_CH_1;
        sq_i2s_tx_dma_ext_hdreq = TX_DMA_EXT_HDREQ;
        sq_i2s_rx_dma_ext_hdreq = RX_DMA_EXT_HDREQ;

	sq_request_dma(sq_i2s_tx_dma_ch_num, &sq_i2s_tx_dma_notifier);
	sq_request_dma(sq_i2s_rx_dma_ch_num, &sq_i2s_rx_dma_notifier);

	/* Initialize the tx and rx operation configuration */
	sq_i2s_conf = SQ_I2S_TX_N_RST |
		SQ_I2S_RX_N_RST |
		SQ_I2S_HDMA_REQ_1_EN |
		SQ_I2S_HDMA_REQ_2_EN |
		SQ_I2S_HDMA_IF_1_TX |
		SQ_I2S_HDMA_IF_2_RX |
		SQ_I2S_OP_LOOPBACK |
		SQ_I2S_TX_OP_STP |
		SQ_I2S_RX_OP_STP;

	/* Stop the tx and rx operation */
	sq_i2s_write(sq_i2s_conf, SQ_I2S_OPR);

	/* Reset the Tx and Rx logic and FSM */
	sq_i2s_write(
			sq_i2s_conf |
			SQ_I2S_TX_RST |
			SQ_I2S_RX_RST,
			SQ_I2S_OPR);

	/* Clear Rx fifo first */
	tmp = sq_i2s_read(SQ_I2S_FIFOSTS);
	while (!(tmp & SQ_I2S_RX_FIFO_EMPTY)) {
		sq_i2s_read(SQ_I2S_RXR);
		tmp = sq_i2s_read(SQ_I2S_FIFOSTS);
	}

	/* Set the transmitter */
	sq_i2s_write(
			SQ_I2S_TX_DEV_SEL_0 |
			sq_i2s_oversample_rate |
			SQ_I2S_RATIO(255) |
			sq_i2s_sample_res |
			SQ_I2S_STEREO |
			sq_i2s_bus_if |
			sq_i2s_tx_mode_sel,
			SQ_I2S_TXCTL);

	/* Set the receiver */
	sq_i2s_write(
			SQ_I2S_RX_FIFO_CLR |
			sq_i2s_oversample_rate |
			SQ_I2S_RATIO(255) |
			sq_i2s_sample_res |
			SQ_I2S_STEREO |
			sq_i2s_bus_if |
			sq_i2s_rx_mode_sel,
			SQ_I2S_RXCTL);

	/* Set the interrupt trigger level */
	sq_i2s_write(
			SQ_I2S_TX_INT_TRIG_LEV_HALF_FULL |
			SQ_I2S_RX_INT_TRIG_LEV_HALF_FULL,
			SQ_I2S_FIFOSTS);

	/* Disable all the interrupts */
	sq_i2s_write(
			SQ_I2S_TX_FIFO_TRIG_INT_DIS |
			SQ_I2S_RX_FIFO_TRIG_INT_DIS |
			SQ_I2S_RX_FIFO_OVR_INT_DIS,
			SQ_I2S_IER);

	/* Clear pattern buffer and compare buffer */
	memset(sq_i2s_pcm_buf, 0x00, DMA_BUF_SIZE);
	memset(sq_i2s_dma_buf, 0x00, DMA_BUF_SIZE);

	/* Make the pattern */
	sq_i2s_make_test_pattern(sq_i2s_pcm_buf, DMA_BUF_SIZE);

	/* Reset the flags */
	sq_i2s_tx_complete_flag = 0;
	sq_i2s_period_int_flag = 0;
	sq_i2s_rx_complete_flag = 0;	

	/* Configure the hardware dma settng of HDMA for tx channels */
	sq_disable_dma(sq_i2s_tx_dma_ch_num);
	sq_set_dma_mode(sq_i2s_tx_dma_ch_num, SQ_DMA_MODE_SLICE);
	sq_set_dma_ext_hdreq_number(sq_i2s_tx_dma_ch_num, sq_i2s_tx_dma_ext_hdreq);
	sq_set_dma_burst_type(sq_i2s_tx_dma_ch_num, sq_i2s_dma_burst);
	sq_set_dma_source_address(sq_i2s_tx_dma_ch_num, (u32)sq_i2s_ring_buf);
	sq_set_dma_destination_address(sq_i2s_tx_dma_ch_num, SQ_I2S_TXR+sq_i2s_base);
	sq_set_dma_source_direction(sq_i2s_tx_dma_ch_num, SQ_DMA_DIR_INCR);
	sq_set_dma_destination_direction(sq_i2s_tx_dma_ch_num, SQ_DMA_DIR_FIXED);
	sq_set_dma_data_size(sq_i2s_tx_dma_ch_num, SQ_DMA_DATA_WORD);
	sq_set_dma_transfer_count(sq_i2s_tx_dma_ch_num, PERIOD_SIZE);
	sq_set_dma_slice_count(sq_i2s_tx_dma_ch_num, FIFO_DEPTH>>1);
	sq_set_dma_page_number(sq_i2s_tx_dma_ch_num, PERIODS);
	sq_set_dma_buffer_size(sq_i2s_tx_dma_ch_num, RING_BUF_SIZE);

	/* Configure the hardware dma settng of HDMA for rx channels */
	sq_disable_dma(sq_i2s_rx_dma_ch_num);
	sq_set_dma_mode(sq_i2s_rx_dma_ch_num, SQ_DMA_MODE_SLICE);
	sq_set_dma_ext_hdreq_number(sq_i2s_rx_dma_ch_num, sq_i2s_rx_dma_ext_hdreq);
	sq_set_dma_burst_type(sq_i2s_rx_dma_ch_num, sq_i2s_dma_burst);
	sq_set_dma_source_address(sq_i2s_rx_dma_ch_num, SQ_I2S_RXR+sq_i2s_base);
	sq_set_dma_destination_address(sq_i2s_rx_dma_ch_num, (u32)sq_i2s_dma_buf);
	sq_set_dma_source_direction(sq_i2s_rx_dma_ch_num, SQ_DMA_DIR_FIXED);
	sq_set_dma_destination_direction(sq_i2s_rx_dma_ch_num, SQ_DMA_DIR_INCR);
	sq_set_dma_data_size(sq_i2s_rx_dma_ch_num, SQ_DMA_DATA_WORD);
	sq_set_dma_transfer_count(sq_i2s_rx_dma_ch_num, PERIOD_SIZE);
	sq_set_dma_slice_count(sq_i2s_rx_dma_ch_num, FIFO_DEPTH>>1);
	sq_set_dma_page_number(sq_i2s_rx_dma_ch_num, 20);
	sq_set_dma_buffer_size(sq_i2s_rx_dma_ch_num, DMA_BUF_SIZE);

	sq_i2s_pcm_buf_pos = 0;
	sq_i2s_ring_buf_pos = 0;
	sq_i2s_dma_ring_buf_pos = 0;
	sq_i2s_access_ring_buffer(sq_i2s_dma_ring_buf_pos, 0);

	/* Start to transfer */
	if (SQ_I2S_MASTER == sq_i2s_tx_mode_sel)
		sq_i2s_write(
				sq_i2s_conf |SQ_I2S_TX_OP_STR,
				SQ_I2S_OPR);
	else if (SQ_I2S_MASTER == sq_i2s_rx_mode_sel)
		sq_i2s_write(
				sq_i2s_conf |SQ_I2S_RX_OP_STR,
				SQ_I2S_OPR);

	/* Enable the dma to run*/
	sq_enable_dma(sq_i2s_tx_dma_ch_num);
	sq_enable_dma(sq_i2s_rx_dma_ch_num);

	/* Wait for the transimission of pcm to be complete */
	while (sq_i2s_pcm_buf_pos < DMA_BUF_SIZE) {
		if (sq_i2s_period_int_flag) {
			sq_i2s_period_int_flag = 0;
			sq_i2s_access_ring_buffer(sq_i2s_dma_ring_buf_pos, 0);
		}
	}

	/* Wait for the transimission to be complete */
	if (sq_wait_for_int(&sq_i2s_rx_complete_flag, 30)) {
		printf("Sq I2S host: transimission is timeout\n");
		return -1;
	}

	/* Stop the tx and rx operation */
	sq_i2s_write(sq_i2s_conf, SQ_I2S_OPR);

	sq_disable_dma(sq_i2s_tx_dma_ch_num);
	sq_disable_dma(sq_i2s_rx_dma_ch_num);
	sq_free_dma(sq_i2s_tx_dma_ch_num);
	sq_free_dma(sq_i2s_rx_dma_ch_num);

	return sq_i2s_compare_memory(sq_i2s_pcm_buf, sq_i2s_dma_buf, DMA_BUF_SIZE,
						autotest);
}

extern int 
sq_i2s_play_pcm_normal(int autotest)
{
	int ret;
	
	printf("\nPlaying PCM data from memory 0x%x for 0x%x bytes. \n", PCM_BUF_ADDR, PCM_BUF_SIZE);	
	request_irq(sq_i2s_irq, sq_i2s_play_pcm_isr, NULL);
#ifdef I2S_DEBUG_ON
	printf("sq_i2s_play_pcm_normal start \n");
#endif
	/* Initialize the operation start register */
	sq_i2s_conf = SQ_I2S_TX_N_RST |
		SQ_I2S_RX_N_RST |
		SQ_I2S_HDMA_REQ_1_DIS |
		SQ_I2S_HDMA_REQ_2_DIS |
		SQ_I2S_HDMA_IF_1_TX |
		SQ_I2S_HDMA_IF_2_RX |
		SQ_I2S_OP_NORMAL |
		SQ_I2S_TX_OP_STP |
		SQ_I2S_RX_OP_STP;

	/* Stop the tx operation */
	sq_i2s_write(
			sq_i2s_conf |
			(sq_i2s_read(SQ_I2S_OPR) & SQ_I2S_RX_OP_STR),
			SQ_I2S_OPR);
			
	/* Reset the Tx logic and FSM */
	sq_i2s_write(
			sq_i2s_conf |
			SQ_I2S_TX_RST |
			(sq_i2s_read(SQ_I2S_OPR) & SQ_I2S_RX_OP_STR),
			SQ_I2S_OPR);

	/* Set the transmitter */
	sq_i2s_write(
			SQ_I2S_TX_DEV_SEL_0 |
			frequecy_setting |
			sq_i2s_sample_res |
			SQ_I2S_STEREO |
			SQ_I2S_BUS_IF_I2S |
			SQ_I2S_MASTER,
			SQ_I2S_TXCTL);

	/* Set the interrupt trigger level */
	sq_i2s_write(
			SQ_I2S_TX_INT_TRIG_LEV_ALMOST_EMPTY |
			SQ_I2S_RX_INT_TRIG_LEV_HALF_FULL,
			SQ_I2S_FIFOSTS);

	/* Set the interrupt enable */
	sq_i2s_write(
			SQ_I2S_TX_FIFO_TRIG_INT_EN |
			SQ_I2S_RX_FIFO_TRIG_INT_DIS |
			SQ_I2S_RX_FIFO_OVR_INT_DIS,
			SQ_I2S_IER);

	sq_i2s_make_test_pattern(sq_i2s_pcm_buf, PCM_BUF_SIZE);	

	/* Reset the flags */
	sq_i2s_tx_complete_flag = 0;

	/* Reset the buffer index */
	sq_i2s_isr_tx_buf_idx = 0;
	sq_i2s_isr_tx_total_len = PCM_BUF_SIZE;

	sq_i2s_isr_tx_buf_32 = (u32 *)sq_i2s_pcm_buf;

#ifdef I2S_DEBUG_ON
	printf("transfer start  ......\n");
#endif
	/* Start to transfer */
	sq_i2s_write(
			sq_i2s_conf |
			SQ_I2S_TX_OP_STR |
			(sq_i2s_read(SQ_I2S_OPR) & SQ_I2S_RX_OP_STR),
			SQ_I2S_OPR);

	/* Enter into control function */
	if (sq_audio_control_function) {
		ret = sq_audio_control_function();
		if (ret)
			sq_i2s_tx_complete_flag = -1;

	}

	/* Wait for the transimission to be complete */
	while (!sq_i2s_tx_complete_flag)
		/* NOP */;
#ifdef I2S_DEBUG_ON
	printf("transfer finish	......\n");
#endif

	/* Disable the interrupt */
	sq_i2s_write(
			sq_i2s_read(SQ_I2S_IER) &
			~SQ_I2S_TX_FIFO_TRIG_INT_EN,
			SQ_I2S_IER);

	/* Stop the tx operation */
	sq_i2s_write(
			sq_i2s_conf |
			(sq_i2s_read(SQ_I2S_OPR) & SQ_I2S_RX_OP_STR),
			SQ_I2S_OPR);

	free_irq(sq_i2s_irq);
	
	if (-1 == sq_i2s_tx_complete_flag)
		return -1;
	return 0;
}

extern int 
sq_i2s_play_pcm_hwdma_panther7_hdma(int autotest)
{
	printf("\nPlaying PCM data from memory 0x%x for 0x%x bytes. \n", PCM_BUF_ADDR, PCM_BUF_SIZE);	
	sq_i2s_tx_dma_ch_num = PANTHER7_HDMA_CH_0;
	sq_i2s_tx_dma_ext_hdreq = TX_DMA_EXT_HDREQ;
	sq_request_dma(sq_i2s_tx_dma_ch_num, &sq_i2s_tx_dma_play_notifier);

	/* Initialize the operation start register */
	sq_i2s_conf = SQ_I2S_TX_N_RST |
		SQ_I2S_RX_N_RST |
		SQ_I2S_HDMA_REQ_1_EN |
		SQ_I2S_HDMA_REQ_2_DIS |
		SQ_I2S_HDMA_IF_1_TX |
		SQ_I2S_HDMA_IF_2_RX |
		SQ_I2S_OP_NORMAL |
		SQ_I2S_TX_OP_STP |
		SQ_I2S_RX_OP_STP;

	/* Stop the tx operation */
	sq_i2s_write(
			sq_i2s_conf |
			(sq_i2s_read(SQ_I2S_OPR) & SQ_I2S_RX_OP_STR),
			SQ_I2S_OPR);
			
	/* Reset the Tx logic and FSM */
	sq_i2s_write(
			sq_i2s_conf |
			SQ_I2S_TX_RST |
			(sq_i2s_read(SQ_I2S_OPR) & SQ_I2S_RX_OP_STR),
			SQ_I2S_OPR);

	/* Set the transmitter */
	sq_i2s_write(
			SQ_I2S_TX_DEV_SEL_0 |
			frequecy_setting |
			sq_i2s_sample_res |
			SQ_I2S_STEREO |
			SQ_I2S_BUS_IF_I2S |
			SQ_I2S_MASTER,
			SQ_I2S_TXCTL);

	/* Set the interrupt trigger level */
	sq_i2s_write(
			SQ_I2S_TX_INT_TRIG_LEV_HALF_FULL |
			SQ_I2S_RX_INT_TRIG_LEV_HALF_FULL,
			SQ_I2S_FIFOSTS);

	/* Disable all interrupts */
	sq_i2s_write(
			SQ_I2S_TX_FIFO_TRIG_INT_DIS |
			SQ_I2S_RX_FIFO_TRIG_INT_DIS |
			SQ_I2S_RX_FIFO_OVR_INT_DIS,
			SQ_I2S_IER);

	/* Reset the flags */
	sq_i2s_tx_complete_flag = 0;
	sq_i2s_tx_complete_interrupt_flag = 0;	
	sq_i2s_period_int_flag = 0;

	/* Configure the hardware dma settng of HDMA for tx channels */
	sq_disable_dma(sq_i2s_tx_dma_ch_num);
	sq_set_dma_mode(sq_i2s_tx_dma_ch_num, SQ_DMA_MODE_SLICE);
	sq_set_dma_ext_hdreq_number(sq_i2s_tx_dma_ch_num, sq_i2s_tx_dma_ext_hdreq);
	sq_set_dma_burst_type(sq_i2s_tx_dma_ch_num, PCM_BURST_TYPE);
	sq_set_dma_source_address(sq_i2s_tx_dma_ch_num, RING_BUF_ADDR);
	sq_set_dma_destination_address(sq_i2s_tx_dma_ch_num, SQ_I2S_TXR+sq_i2s_base);
	sq_set_dma_source_direction(sq_i2s_tx_dma_ch_num, SQ_DMA_DIR_INCR);
	sq_set_dma_destination_direction(sq_i2s_tx_dma_ch_num, SQ_DMA_DIR_FIXED);
	sq_set_dma_data_size(sq_i2s_tx_dma_ch_num, SQ_DMA_DATA_WORD);
	sq_set_dma_transfer_count(sq_i2s_tx_dma_ch_num, PERIOD_SIZE);
	sq_set_dma_slice_count(sq_i2s_tx_dma_ch_num, FIFO_DEPTH>>1);
	sq_set_dma_page_number(sq_i2s_tx_dma_ch_num, PERIODS);	/* repeat to play pcm again */
	sq_set_dma_buffer_size(sq_i2s_tx_dma_ch_num, RING_BUF_SIZE);

	sq_i2s_pcm_buf_pos = 0;
	sq_i2s_ring_buf_pos = 0;
	sq_i2s_dma_ring_buf_pos = 0;
	sq_i2s_access_ring_buffer(sq_i2s_dma_ring_buf_pos, 0);

	/* Start to transfer */
	sq_i2s_write(
			sq_i2s_conf |
			SQ_I2S_TX_OP_STR |
			(sq_i2s_read(SQ_I2S_OPR) & SQ_I2S_RX_OP_STR),
			SQ_I2S_OPR);

	/* Enable the dma to run*/
	sq_enable_dma(sq_i2s_tx_dma_ch_num);

	/* Wait for the transimission of pcm to be complete */
	while (sq_i2s_pcm_buf_pos < PCM_BUF_SIZE) {
		// 20080714 cyli add for INR
		if (vol_change) {
			audio_dac_master_volume(vol);
			vol_change = 0;
		}
		if (sq_i2s_period_int_flag) {
			sq_i2s_period_int_flag = 0;
			sq_i2s_access_ring_buffer(sq_i2s_dma_ring_buf_pos, 0);
		}
	}

	sq_i2s_tx_complete_flag = 1;
	while (sq_i2s_tx_complete_interrupt_flag !=1);
	/* Stop the transmission */
	sq_i2s_write(
			sq_i2s_conf |
			(sq_i2s_read(SQ_I2S_OPR) & SQ_I2S_RX_OP_STR),
			SQ_I2S_OPR);

	sq_disable_dma(sq_i2s_tx_dma_ch_num);
	sq_free_dma(sq_i2s_tx_dma_ch_num);
	return 0;
}

extern int 
sq_i2s_capture_pcm_normal(int autotest)
{
	int ret;

	printf("sq_i2s_capture_pcm_normal \n");
	request_irq(sq_i2s_irq, sq_i2s_capture_isr, NULL);

	/* Initialize the operation start register */
	sq_i2s_conf = SQ_I2S_TX_N_RST |
		SQ_I2S_RX_N_RST |
		SQ_I2S_HDMA_REQ_1_DIS |
		SQ_I2S_HDMA_REQ_2_DIS |
		SQ_I2S_HDMA_IF_1_TX |
		SQ_I2S_HDMA_IF_2_RX |
		SQ_I2S_OP_NORMAL |
		SQ_I2S_TX_OP_STP |
		SQ_I2S_RX_OP_STP;

	/* Stop the Rx operation */
	sq_i2s_write(
			sq_i2s_conf |
			(sq_i2s_read(SQ_I2S_OPR) & (~SQ_I2S_RX_OP_STR)),
			SQ_I2S_OPR);
			
	/* Reset the Rx logic and FSM */
	sq_i2s_write(
			sq_i2s_conf |
			SQ_I2S_RX_RST |
			(sq_i2s_read(SQ_I2S_OPR)),
			SQ_I2S_OPR);

	/* Set the receiver */
	sq_i2s_write(
			SQ_I2S_RX_FIFO_CLR |
			frequecy_setting |
			sq_i2s_sample_res |
			SQ_I2S_STEREO |
			SQ_I2S_BUS_IF_LJ |
			SQ_I2S_MASTER,
			SQ_I2S_RXCTL);

	/* Set the interrupt trigger level */
	sq_i2s_write(
			SQ_I2S_TX_INT_TRIG_LEV_ALMOST_EMPTY |
			SQ_I2S_RX_INT_TRIG_LEV_HALF_FULL,
			SQ_I2S_FIFOSTS);

	/* Set the interrupt enable */
	sq_i2s_write(
			SQ_I2S_TX_FIFO_TRIG_INT_DIS |
			SQ_I2S_RX_FIFO_TRIG_INT_EN |
			SQ_I2S_RX_FIFO_OVR_INT_EN,
			SQ_I2S_IER);

	/* Reset the flags */
	sq_i2s_rx_complete_flag = 0;

	/* Reset the buffer index */
	sq_i2s_isr_rx_buf_idx = 0;
	sq_i2s_isr_rx_total_len = PCM_BUF_SIZE;
	
	sq_i2s_isr_rx_buf_32 = (u32 *)sq_i2s_pcm_buf;

	/* Start to transfer */
	sq_i2s_write(
			sq_i2s_conf |
			SQ_I2S_RX_OP_STR |
			(sq_i2s_read(SQ_I2S_OPR)),
			SQ_I2S_OPR);

	/* Enter into control function */
	if (sq_audio_control_function) {
		ret = sq_audio_control_function();
		if (ret)
			sq_i2s_rx_complete_flag = -1;
	}

	/* Wait for the transmission to be complete */
	while (!sq_i2s_rx_complete_flag)
		/* NOP */;

	/* Disable the interrupt */
	sq_i2s_write(
			sq_i2s_read(SQ_I2S_IER) &
			~(SQ_I2S_RX_FIFO_TRIG_INT_EN |
			  SQ_I2S_RX_FIFO_OVR_INT_EN),
			SQ_I2S_IER);

	/* Stop the Rx operation */
	sq_i2s_write(
			sq_i2s_conf |
			(sq_i2s_read(SQ_I2S_OPR) & SQ_I2S_TX_OP_STR),
			SQ_I2S_OPR);

	free_irq(sq_i2s_irq);
	
	if (-1 == sq_i2s_rx_complete_flag)
		return -1;
	return 0;
}

extern int 
sq_i2s_capture_pcm_hwdma_panther7_hdma(int autotest)
{
	sq_i2s_rx_dma_ch_num = PANTHER7_HDMA_CH_1;
	sq_i2s_rx_dma_ext_hdreq = RX_DMA_EXT_HDREQ;
	sq_request_dma(sq_i2s_rx_dma_ch_num, &sq_i2s_rx_dma_capture_notifier);

	/* Initialize the operation start register */
	sq_i2s_conf = SQ_I2S_TX_N_RST |
		SQ_I2S_RX_N_RST |
		SQ_I2S_HDMA_REQ_1_DIS |
		SQ_I2S_HDMA_REQ_2_EN |
		SQ_I2S_HDMA_IF_1_TX |
		SQ_I2S_HDMA_IF_2_RX |
		SQ_I2S_OP_NORMAL |
		SQ_I2S_TX_OP_STP |
		SQ_I2S_RX_OP_STP;

	/* Stop the Rx operation */
	sq_i2s_write(
			sq_i2s_conf |
			(sq_i2s_read(SQ_I2S_OPR) & SQ_I2S_TX_OP_STR),
			SQ_I2S_OPR);
			
	/* Reset the Rx logic and FSM */
	sq_i2s_write(
			sq_i2s_conf |
			SQ_I2S_RX_RST |
			(sq_i2s_read(SQ_I2S_OPR) & SQ_I2S_TX_OP_STR),
			SQ_I2S_OPR);

	/* Set the receiver */
	sq_i2s_write(
			SQ_I2S_RX_FIFO_CLR |
			frequecy_setting |
			sq_i2s_sample_res |
			SQ_I2S_STEREO |
			SQ_I2S_BUS_IF_I2S |
			SQ_I2S_MASTER,
			SQ_I2S_RXCTL);

	/* Set the interrupt trigger level */
	sq_i2s_write(
			SQ_I2S_TX_INT_TRIG_LEV_HALF_FULL |
			SQ_I2S_RX_INT_TRIG_LEV_HALF_FULL,
			SQ_I2S_FIFOSTS);

	/* Disable all interrupts */
	sq_i2s_write(
			SQ_I2S_TX_FIFO_TRIG_INT_DIS |
			SQ_I2S_RX_FIFO_TRIG_INT_DIS |
			SQ_I2S_RX_FIFO_OVR_INT_DIS,
			SQ_I2S_IER);

	/* Reset the flags */
	sq_i2s_rx_complete_flag = 0;
	sq_i2s_rx_complete_interrupt_flag = 0;	
	sq_i2s_period_int_flag = 0;

	/* Configure the hardware dma settng of HDMA for tx channels */
	sq_disable_dma(sq_i2s_rx_dma_ch_num);
	sq_set_dma_mode(sq_i2s_rx_dma_ch_num, SQ_DMA_MODE_SLICE);
	sq_set_dma_ext_hdreq_number(sq_i2s_rx_dma_ch_num, sq_i2s_rx_dma_ext_hdreq);
	sq_set_dma_burst_type(sq_i2s_rx_dma_ch_num, PCM_BURST_TYPE);
	sq_set_dma_source_address(sq_i2s_rx_dma_ch_num, SQ_I2S_RXR+sq_i2s_base);
	sq_set_dma_destination_address(sq_i2s_rx_dma_ch_num, RING_BUF_ADDR);
	sq_set_dma_source_direction(sq_i2s_rx_dma_ch_num, SQ_DMA_DIR_FIXED);
	sq_set_dma_destination_direction(sq_i2s_rx_dma_ch_num, SQ_DMA_DIR_INCR);
	sq_set_dma_data_size(sq_i2s_rx_dma_ch_num, SQ_DMA_DATA_WORD);
	sq_set_dma_transfer_count(sq_i2s_rx_dma_ch_num, PERIOD_SIZE);
	sq_set_dma_slice_count(sq_i2s_rx_dma_ch_num, FIFO_DEPTH>>1);
	sq_set_dma_page_number(sq_i2s_rx_dma_ch_num, PERIODS);	/* repeat to play pcm again */
	sq_set_dma_buffer_size(sq_i2s_rx_dma_ch_num, RING_BUF_SIZE);

	sq_i2s_pcm_buf_pos = 0;
	sq_i2s_ring_buf_pos = 0;
	sq_i2s_dma_ring_buf_pos = 0;

	/* Start to transfer */
	sq_i2s_write(
			sq_i2s_conf |
			SQ_I2S_RX_OP_STR |
			(sq_i2s_read(SQ_I2S_OPR) & SQ_I2S_TX_OP_STR),
			SQ_I2S_OPR);

	/* Enable the dma to run*/
	sq_enable_dma(sq_i2s_rx_dma_ch_num);

	/* Wait for the transimission of pcm to be complete */
	while (sq_i2s_pcm_buf_pos < PCM_BUF_SIZE) {
		if (sq_i2s_period_int_flag) {
			sq_i2s_period_int_flag = 0;
			sq_i2s_access_ring_buffer(sq_i2s_dma_ring_buf_pos, 1);
		}
	}
//	MSDELAY(1000);
	sq_i2s_rx_complete_flag = 1;
	/* Stop the Rx operation */
	while(sq_i2s_rx_complete_interrupt_flag != 1);

	sq_i2s_write(
			sq_i2s_conf |
			(sq_i2s_read(SQ_I2S_OPR) & SQ_I2S_TX_OP_STR),
			SQ_I2S_OPR);

	sq_disable_dma(sq_i2s_rx_dma_ch_num);
	sq_free_dma(sq_i2s_rx_dma_ch_num);

	return 0;
}

static void
sq_i2s_transmit(void)
{
	u32 i;
	for (i = 0; i < (FIFO_DEPTH >> 1); i++) {
		if (sq_i2s_isr_tx_total_len == (sq_i2s_isr_tx_buf_idx << 2)) {
			sq_i2s_tx_complete_flag = 1;
			
			if (sq_i2s_isr_tx_total_len == (sq_i2s_isr_rx_buf_idx << 2)) {
				sq_i2s_rx_complete_flag = 1;

				/* Disable the interrupt */
				sq_i2s_write(
						SQ_I2S_TX_FIFO_TRIG_INT_DIS |
						SQ_I2S_RX_FIFO_TRIG_INT_DIS |
						SQ_I2S_RX_FIFO_OVR_INT_DIS,
						SQ_I2S_IER);


				/* Stop the transmission */
				sq_i2s_write(
						sq_i2s_conf,
						SQ_I2S_OPR);

				return;
			} else
				break;
		} 
		sq_i2s_write(
				sq_i2s_isr_tx_buf_32[sq_i2s_isr_tx_buf_idx++],
				SQ_I2S_TXR);
	}

}

static void
sq_i2s_receive(void)
{
	u32 i;
	for (i = 0; i < (FIFO_DEPTH >> 1); i++) {
		if (sq_i2s_isr_tx_total_len == (sq_i2s_isr_rx_buf_idx << 2)) {
			sq_i2s_rx_complete_flag = 1;

			/* Disable the interrupt */
			sq_i2s_write(
					SQ_I2S_TX_FIFO_TRIG_INT_DIS |
					SQ_I2S_RX_FIFO_TRIG_INT_DIS |
					SQ_I2S_RX_FIFO_OVR_INT_DIS,
					SQ_I2S_IER);

			/* Stop the transmission */
			sq_i2s_write(
					sq_i2s_conf,
					SQ_I2S_OPR);

			return;
		}
		sq_i2s_isr_rx_buf_32[sq_i2s_isr_rx_buf_idx++] = sq_i2s_read(SQ_I2S_RXR);
	}
	
}

static void 
sq_i2s_loopback_isr(void *data)
{
	u32 int_stat;

	int_stat = sq_i2s_read(SQ_I2S_ISR);

	/* Check whether it is the rx fifo overrun interrupt */
	if (int_stat & SQ_I2S_RX_FIFO_OVR_INT) {
		printf("Sq I2S host: rx fifo is overrun\n");
		
		/* Clear the rx fifo */
		sq_i2s_write(
				sq_i2s_read(SQ_I2S_RXCTL) |
				SQ_I2S_RX_FIFO_CLR,
				SQ_I2S_RXCTL);

		sq_i2s_rx_complete_flag = -1;

		/* Disable the interrupt */
		sq_i2s_write(
				SQ_I2S_TX_FIFO_TRIG_INT_DIS |
				SQ_I2S_RX_FIFO_TRIG_INT_DIS |
				SQ_I2S_RX_FIFO_OVR_INT_DIS,
				SQ_I2S_IER);

		/* Stop the transmission */
		sq_i2s_write(
				sq_i2s_conf,
				SQ_I2S_OPR);

		return;
	}

	/* Check whether it is the tx fifo data trigger interrupt */
	if (int_stat & SQ_I2S_TX_FIFO_TRIG_INT) {
		sq_i2s_transmit();
	}

	/* Chech whether it is the rx fifo data trigger interrupt */
	if (int_stat & SQ_I2S_RX_FIFO_TRIG_INT) {
		sq_i2s_receive();
	}
}

static void 
sq_i2s_play_pcm_isr(void *data)
{
	u32 int_stat, fifo_stat;

	int_stat = sq_i2s_read(SQ_I2S_ISR);

	/* Check whether it is the tx fifo data trigger interrupt */
	if (int_stat & SQ_I2S_TX_FIFO_TRIG_INT) {
		do {
			if (sq_i2s_isr_tx_total_len == (sq_i2s_isr_tx_buf_idx << 2)) {
				sq_i2s_tx_complete_flag = 1;

				/* Disable the interrupt */
				sq_i2s_write(
						sq_i2s_read(SQ_I2S_IER) &
						~SQ_I2S_TX_FIFO_TRIG_INT_EN,
						SQ_I2S_IER);

				/* Stop the transmission */
				sq_i2s_write(
						sq_i2s_conf |
						(sq_i2s_read(SQ_I2S_OPR) & SQ_I2S_RX_OP_STR),
						SQ_I2S_OPR);

				break;
			} 
			sq_i2s_write(
					sq_i2s_isr_tx_buf_32[sq_i2s_isr_tx_buf_idx++],
					SQ_I2S_TXR);
			fifo_stat = sq_i2s_read(SQ_I2S_FIFOSTS);
		} while (!(fifo_stat & SQ_I2S_TX_FIFO_FULL));
	}
}

static void 
sq_i2s_capture_isr(void *data)
{
	u32 int_stat;
	u32 i;

	int_stat = sq_i2s_read(SQ_I2S_ISR);

	/* Check whether it is the rx fifo overrun interrupt */
	if (int_stat & SQ_I2S_RX_FIFO_OVR_INT) {
		printf("Sq I2S host: rx fifo is overrun\n");
		
		/* Clear the rx fifo */
		sq_i2s_write(
				sq_i2s_read(SQ_I2S_RXCTL) |
				SQ_I2S_RX_FIFO_CLR,
				SQ_I2S_RXCTL);

		sq_i2s_rx_complete_flag = -1;

		/* Disable the interrupt */
		sq_i2s_write(
				sq_i2s_read(SQ_I2S_IER) &
				~(SQ_I2S_RX_FIFO_TRIG_INT_EN |
				  SQ_I2S_RX_FIFO_OVR_INT_EN),
				SQ_I2S_IER);

		/* Stop the transmission */
		sq_i2s_write(
				sq_i2s_conf |
				(sq_i2s_read(SQ_I2S_OPR) & SQ_I2S_TX_OP_STR),
				SQ_I2S_OPR);

		return;
	}

	/* Chech whether it is the rx fifo data trigger interrupt */
	if (int_stat & SQ_I2S_RX_FIFO_TRIG_INT) {
		for (i = 0; i < (FIFO_DEPTH >> 1); i++) {
			if (sq_i2s_isr_rx_total_len == (sq_i2s_isr_rx_buf_idx << 2)) {
				sq_i2s_rx_complete_flag = 1;

				/* Disable the interrupt */
				sq_i2s_write(
						sq_i2s_read(SQ_I2S_IER) &
						~(SQ_I2S_RX_FIFO_TRIG_INT_EN |
						  SQ_I2S_RX_FIFO_OVR_INT_EN),
						SQ_I2S_IER);

				/* Stop the transmission */
				sq_i2s_write(
						sq_i2s_conf |
						(sq_i2s_read(SQ_I2S_OPR) & SQ_I2S_TX_OP_STR),
						SQ_I2S_OPR);

				return;
			}
			sq_i2s_isr_rx_buf_32[sq_i2s_isr_rx_buf_idx++] = sq_i2s_read(SQ_I2S_RXR);
		}
	}
}

static void 
sq_i2s_tx_dma_play_complete(void *data)
{
	if (sq_i2s_tx_complete_flag){
		sq_i2s_tx_complete_interrupt_flag = 1;	
#ifdef I2S_DEBUG_ON
		printf("TX DMA Play Complete\n");  
#endif  
		return;
	}	
	sq_i2s_dma_ring_buf_pos += PERIOD_SIZE;
	sq_i2s_dma_ring_buf_pos %= RING_BUF_SIZE;
	sq_i2s_period_int_flag = 1;
	sq_set_dma_page_number(sq_i2s_tx_dma_ch_num, 1);	/* repeat to play pcm again */
#ifdef I2S_DEBUG_ON
	printf("TX DMA Play Complete repeat\n");   
#endif
}

static void 
sq_i2s_rx_dma_capture_complete(void *data)
{
	if (sq_i2s_rx_complete_flag){
		sq_i2s_rx_complete_interrupt_flag = 1;
		return;
	}	
	sq_i2s_dma_ring_buf_pos += PERIOD_SIZE;
	sq_i2s_dma_ring_buf_pos %= RING_BUF_SIZE;
	sq_i2s_period_int_flag = 1;
	sq_set_dma_page_number(sq_i2s_rx_dma_ch_num, 1);	/* repeat to play pcm again */
}

static void
sq_i2s_tx_dma_page_interrupt(void *data)
{
	sq_i2s_tx_complete_flag = 1;
#ifdef I2S_DEBUG_ON
	printf("sq_i2s_tx_dma_page_interrupt\n");   
#endif	
}

static void 
sq_i2s_rx_dma_page_interrupt(void *data)
{
	sq_i2s_rx_complete_flag = 1;
	/* Disable the interrupt */
	sq_i2s_write(
			SQ_I2S_TX_FIFO_TRIG_INT_DIS |
			SQ_I2S_RX_FIFO_TRIG_INT_DIS |
			SQ_I2S_RX_FIFO_OVR_INT_DIS,
			SQ_I2S_IER);

	/* Stop the transmission */
	sq_i2s_write(
			sq_i2s_conf,
			SQ_I2S_OPR);
#ifdef I2S_DEBUG_ON
	printf("sq_i2s_rx_dma_page_interrupt\n");   
#endif		
}

static void
sq_i2s_make_test_pattern(u8 *buf, u32 size)
{
	int i;

	for (i = 0; i < size; i++)
		buf[i] = i + 1;
}

static int 
sq_i2s_compare_memory(u8 *mem, u8 *cmpr_mem, u32 size,
			 int skip_cmpr_result)
{
	int i;
        int err_flag = 0;
        u32 cmpr_idx = 0;

        for (i = 0; i < size; i++) {
		if (0 == cmpr_mem[i])
			cmpr_idx++;
		else 
			break;
        }
	printf("\n");
        if (cmpr_idx)
		printf("Sq I2S: skip %d byte\n", cmpr_idx);
	for (i = cmpr_idx; i < (size - cmpr_idx); i++) {
                if (mem[i] != cmpr_mem[i]) {
                        err_flag |= -1;
                        if (!skip_cmpr_result)
                                printf("Byte %d, 0x%02x (0x%08x) != 0x%02x (0x%08x)\n", i, mem[i], &mem[i],
                                       cmpr_mem[i], &cmpr_mem[i]);

                }
        }
        return err_flag;
}

static void
sq_i2s_access_ring_buffer(u32 buf_pos, int dir)
{
	int offset;
	int i;

	offset = buf_pos - sq_i2s_ring_buf_pos;
	if (0 == offset)
		offset = RING_BUF_SIZE;
	else if (offset < 0)
		offset += RING_BUF_SIZE;
	for (i = 0; i < offset; i++) {
		if (0 == dir)	/* Write to ring buffer */
			sq_i2s_ring_buf[sq_i2s_ring_buf_pos++] = sq_i2s_pcm_buf[sq_i2s_pcm_buf_pos++];
		else		/* Read from ring buffer */
			sq_i2s_pcm_buf[sq_i2s_pcm_buf_pos++] = sq_i2s_ring_buf[sq_i2s_ring_buf_pos++];
		sq_i2s_ring_buf_pos %= RING_BUF_SIZE;
	}
}

extern int 
sq_i2s_normal_test(int autotest)
{
	int ret = 0;
	ret = sq_i2s_normal_internal_loopback(autotest);	

	return ret;
}

extern struct test_item_container sq_i2s_tx_hwdma_panther7_hdma_test_container;

extern int
sq_i2s_hwdma_panther7_hdma_test(int autotest)
{
	int ret = 0;

	sq_i2s_tx_dma_ch_num = PANTHER7_HDMA_CH_0;
	sq_i2s_rx_dma_ch_num = PANTHER7_HDMA_CH_1;
	sq_i2s_tx_dma_ext_hdreq = TX_DMA_EXT_HDREQ;
	sq_i2s_rx_dma_ext_hdreq = RX_DMA_EXT_HDREQ;
	ret = test_item_ctrl(&sq_i2s_tx_hwdma_panther7_hdma_test_container, autotest);
	return ret;
}

extern struct test_item_container sq_i2s_oversample_rate_test_container;
extern struct test_item_container sq_i2s_play_pcm_sample_res_test_container;
/* bus type function start */
	
extern int 
sq_i2s_bus_i2s_test(int autotest)
{
	int ret = 0;

	sq_i2s_bus_if = SQ_I2S_BUS_IF_I2S;
	
	if(sq_i2s_loopback_test)
		ret = test_item_ctrl(&sq_i2s_oversample_rate_test_container, autotest);
	else
		ret = test_item_ctrl(&sq_i2s_play_pcm_sample_res_test_container, autotest);

	return ret;
}

extern int 
sq_i2s_bus_lsb_test(int autotest) 
{
	int ret = 0;

	sq_i2s_bus_if = SQ_I2S_BUS_IF_LJ;

	if(sq_i2s_loopback_test)
		ret = test_item_ctrl(&sq_i2s_oversample_rate_test_container, autotest);
	else
		ret = test_item_ctrl(&sq_i2s_play_pcm_sample_res_test_container, autotest);

	return ret;
}

extern int 
sq_i2s_bus_msb_test(int autotest)
{
	int ret = 0;

	sq_i2s_bus_if = SQ_I2S_BUS_IF_RJ;

	if(sq_i2s_loopback_test)
		ret = test_item_ctrl(&sq_i2s_oversample_rate_test_container, autotest);
	else
		ret = test_item_ctrl(&sq_i2s_oversample_rate_test_container, autotest);

	return ret;
}

/* bus type function end */

extern struct test_item_container sq_audio_codec_test_container;

extern int 
sq_adc_dac_function_test(int autotest)
{
	int ret = 0;
	ret = test_item_ctrl(&sq_audio_codec_test_container, autotest);
	return ret;

}

static u32 *sq_i2s_decode_pcm_buf = (u32 *)PCM_BUF_ADDR;			//for audio decode put raw data
static u32 *sq_i2s_decode_dma_buf = (u32 *)RING_BUF_ADDR;		//for dma transfer
static u32 sq_i2s_decode_dma_buf_pos;						//dma buff pos
static u32 sq_i2s_decode_pcm_buf_pos;						//decode buff pos

static void
sq_copy_decode_to_dma_buffer(u32 dma_buff_offset,u32 decode_buff_offset)
{

	u32 i;
//	printf("dma_buff_offset = %, decode_buffset = %x \n",dma_buff_offset,decode_buff_offset);
//	printf("copy decode = %x , dma offset = %x \n",decode_buff_offset*PERIOD_SIZE ,(dma_buff_offset%4) * PERIOD_SIZE);
	
	for (i = 0; i < PERIOD_SIZE/4; i++) {
		sq_i2s_decode_dma_buf[((dma_buff_offset%4) * PERIOD_SIZE)/4 + i] = sq_i2s_decode_pcm_buf[(decode_buff_offset*PERIOD_SIZE)/4 + i];
	}
}

static void 
sq_i2s_tx_dma_play_fun_complete(void *data)
{

	if (sq_i2s_tx_complete_flag){
		sq_i2s_tx_complete_interrupt_flag = 1;	
		return;
	}	
//	printf("sq_i2s_decode_dma_buf_pos = %x : sq_i2s_decode_pcm_buf_pos = %x  ",sq_i2s_decode_dma_buf_pos,sq_i2s_decode_pcm_buf_pos);
	sq_copy_decode_to_dma_buffer(sq_i2s_decode_dma_buf_pos++,sq_i2s_decode_pcm_buf_pos++);
	
	sq_set_dma_page_number(sq_i2s_tx_dma_ch_num, 1);	/* repeat to play pcm again */	

}

static struct sq_dma_notifier sq_i2s_tx_dma_play_fun_notifier = {
	.complete = sq_i2s_tx_dma_play_fun_complete,
};

extern int 
sq_audio_codec_function_test(int autotest)
{
	int i;
	//with HDMA
	printf("\nPlaying PCM data from memory 0x%x for 0x%x bytes. \n", PCM_BUF_ADDR, PCM_BUF_SIZE);	
	sq_i2s_tx_dma_ch_num = PANTHER7_HDMA_CH_0;
	sq_i2s_tx_dma_ext_hdreq = TX_DMA_EXT_HDREQ;
	sq_request_dma(sq_i2s_tx_dma_ch_num, &sq_i2s_tx_dma_play_fun_notifier);

	/* Initialize the operation start register */
	sq_i2s_conf = SQ_I2S_TX_N_RST |
		SQ_I2S_RX_N_RST |
		SQ_I2S_HDMA_REQ_1_EN |
		SQ_I2S_HDMA_REQ_2_DIS |
		SQ_I2S_HDMA_IF_1_TX |
		SQ_I2S_HDMA_IF_2_RX |
		SQ_I2S_OP_NORMAL |
		SQ_I2S_TX_OP_STP |
		SQ_I2S_RX_OP_STP;

	/* Stop the tx operation */
	sq_i2s_write(
			sq_i2s_conf |
			(sq_i2s_read(SQ_I2S_OPR) & SQ_I2S_RX_OP_STR),
			SQ_I2S_OPR);
			
	/* Reset the Tx logic and FSM */
	sq_i2s_write(
			sq_i2s_conf |
			SQ_I2S_TX_RST |
			(sq_i2s_read(SQ_I2S_OPR) & SQ_I2S_RX_OP_STR),
			SQ_I2S_OPR);

	sq_i2s_sample_res = SQ_I2S_SAMPLE_RES_16;
	
	/* Set the transmitter */
	sq_i2s_write(
			SQ_I2S_TX_DEV_SEL_0 |
			frequecy_setting |
			sq_i2s_sample_res |
			SQ_I2S_STEREO |
			SQ_I2S_BUS_IF_I2S |
			SQ_I2S_MASTER,
			SQ_I2S_TXCTL);

	/* Set the interrupt trigger level */
	sq_i2s_write(
			SQ_I2S_TX_INT_TRIG_LEV_HALF_FULL |
			SQ_I2S_RX_INT_TRIG_LEV_HALF_FULL,
			SQ_I2S_FIFOSTS);

	/* Disable all interrupts */
	sq_i2s_write(
			SQ_I2S_TX_FIFO_TRIG_INT_DIS |
			SQ_I2S_RX_FIFO_TRIG_INT_DIS |
			SQ_I2S_RX_FIFO_OVR_INT_DIS,
			SQ_I2S_IER);

	sq_i2s_decode_dma_buf_pos = 0;
	sq_i2s_decode_pcm_buf_pos = 0;
	sq_i2s_tx_complete_flag = 0;

	//implement first 4 page size decode to memory 
	for (i=0;i<4;i++)
		sq_copy_decode_to_dma_buffer(sq_i2s_decode_dma_buf_pos++,sq_i2s_decode_pcm_buf_pos++);


	/* Configure the hardware dma settng of HDMA for tx channels */
	sq_disable_dma(sq_i2s_tx_dma_ch_num);
	sq_set_dma_mode(sq_i2s_tx_dma_ch_num, SQ_DMA_MODE_SLICE);
	sq_set_dma_ext_hdreq_number(sq_i2s_tx_dma_ch_num, sq_i2s_tx_dma_ext_hdreq);
	sq_set_dma_burst_type(sq_i2s_tx_dma_ch_num, PCM_BURST_TYPE);
	sq_set_dma_source_address(sq_i2s_tx_dma_ch_num, RING_BUF_ADDR);
	sq_set_dma_destination_address(sq_i2s_tx_dma_ch_num, SQ_I2S_TXR+sq_i2s_base);
	sq_set_dma_source_direction(sq_i2s_tx_dma_ch_num, SQ_DMA_DIR_INCR);
	sq_set_dma_destination_direction(sq_i2s_tx_dma_ch_num, SQ_DMA_DIR_FIXED);
	sq_set_dma_data_size(sq_i2s_tx_dma_ch_num, SQ_DMA_DATA_WORD);
	sq_set_dma_transfer_count(sq_i2s_tx_dma_ch_num, PERIOD_SIZE);
	sq_set_dma_slice_count(sq_i2s_tx_dma_ch_num, FIFO_DEPTH>>1);
	sq_set_dma_page_number(sq_i2s_tx_dma_ch_num, PERIODS);	/* repeat to play pcm again */
	sq_set_dma_buffer_size(sq_i2s_tx_dma_ch_num, RING_BUF_SIZE);


	/* Start to transfer */
	sq_i2s_write(
			sq_i2s_conf |
			SQ_I2S_TX_OP_STR |
			(sq_i2s_read(SQ_I2S_OPR) & SQ_I2S_RX_OP_STR),
			SQ_I2S_OPR);


	/* Enable the dma to run*/
	sq_enable_dma(sq_i2s_tx_dma_ch_num);

	
	/* Wait for the transimission of pcm to be complete */
	while (sq_i2s_decode_pcm_buf_pos* PERIOD_SIZE < PCM_BUF_SIZE) {
		//please implement decode to memory 
	}

	sq_i2s_tx_complete_flag = 1;
	while (sq_i2s_tx_complete_interrupt_flag !=1);
	/* Stop the transmission */
	sq_i2s_write(
			sq_i2s_conf |
			(sq_i2s_read(SQ_I2S_OPR) & SQ_I2S_RX_OP_STR),
			SQ_I2S_OPR);

	sq_disable_dma(sq_i2s_tx_dma_ch_num);
	sq_free_dma(sq_i2s_tx_dma_ch_num);


	return 0;
	
}

extern struct test_item_container sq_i2s_bus_type_test_container;

/* Level 1 Menu Start */

extern int 
sq_i2s_play_pcm_test(int autotest)
{
	int ret = 0;
	sq_i2s_capture_mode_test = 0;
	ret = test_item_ctrl(&sq_i2s_bus_type_test_container, autotest);
	return ret;
}

extern struct test_item_container sq_i2s_capture_mode_test_container;

extern int
sq_i2s_capture_test(int autotest)
{
	int ret = 0;
	sq_i2s_capture_mode_test = 1;
	ret = test_item_ctrl(&sq_i2s_bus_type_test_container, autotest);
	return ret;
}

extern struct test_item_container sq_i2s_input_ch_test_container;

extern int
sq_i2s_input_ch1_test(int autotest)
{
	int ret = 0;
	sq_adc_channel_contrl(1);
	ret = test_item_ctrl(&sq_i2s_bus_type_test_container, autotest);
	return ret;
}

extern int
sq_i2s_input_ch2_test(int autotest)
{
	int ret = 0;
	sq_adc_channel_contrl(2);	
	ret = test_item_ctrl(&sq_i2s_bus_type_test_container, autotest);
	return ret;
}

extern int 
sq_i2s_internal_loopback_test(int autotest)
{
	int ret = 0;
	sq_i2s_loopback_test = 1;
	ret = test_item_ctrl(&sq_i2s_bus_type_test_container, autotest);
	sq_i2s_loopback_test = 0;
	return ret;
}

/* Level 1 Menu End */

// 20080714 cyli add for INR
#ifdef CONFIG_INR_PC7230
#include <GPIO/gpio.h>
static void
volume_tuner_isr (void *pparam)
{
	int clockwise = sq_gpio_get_value_with_mask(PF, 0x20);

	if (clockwise)
		vol++;
	else
		vol--;

	if (vol > MAX_VOLUME)
		vol = MAX_VOLUME;
	if (vol < 0)
		vol = 0;

	vol_change = 1;
	printf("volume_tuner_isr(): vol = %d\n", vol);
}
#endif

extern struct test_item_container sq_i2s_main_test_container;

extern int 
I2STesting(int autotest)
{
	int ret = 0;
#ifdef CONFIG_SQ8000
	sq_scu_dev_enable(SQ_DEVCON_I2C);
	sq_scu_dev_enable(SQ_DEVCON_I2S_TX_RX);
#endif
	ret |= audio_dac_initialize();
	ret |= audio_adc_initialize();

	//audio_dac_master_volume(MAX_VOLUME);

// 20080714 cyli add for INR
#ifdef CONFIG_INR_PC7230
	// set audio source selection to I2S
	sq_gpio_set_value_with_mask(PE, 0x1b, 0x3f);				// PE[1:0] = [11]

	// set PF4 as low level triggle interrupt for volume_tuner_isr
	if (sq_request_gpio_irq(SET_GPIO_PIN_NUM(PF, 4), volume_tuner_isr, GPIO_INT_SENSE_EDGE | GPIO_INT_SINGLE_EDGE | GPIO_INT_EVENT_HI, NULL)) {
		printf("I2STesting(): GPIO pin[%d] is busy!\n", SET_GPIO_PIN_NUM(PF, 4));
		return -1;
	}
#endif

	ret |= test_item_ctrl(&sq_i2s_main_test_container, autotest);

#ifdef CONFIG_SQ8000
sq_scu_dev_disable(SQ_DEVCON_I2C);
sq_scu_dev_disable(SQ_DEVCON_I2S_TX_RX);
#endif

#ifdef CONFIG_INR_PC7230
	// free PF4
	sq_free_gpio_irq(SET_GPIO_PIN_NUM(PF, 4));
#endif

	return ret;
}

