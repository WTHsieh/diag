#include <type.h>
#include <io.h>
#include <genlib.h>
#include <dma/dma.h>
#include <interrupt.h>
#include "a2a-regs.h"


static int sq_a2a_request(u32 ch, struct sq_dma *dma);
static void sq_a2a_free(u32 ch, struct sq_dma *dma);
static void sq_a2a_enable(u32 ch, struct sq_dma *dma);
static void sq_a2a_disable(u32 ch, struct sq_dma *dma);
static void sq_a2a_isr_0(void *dma);
static void sq_a2a_isr_1(void *dma);

static u32 sq_a2a_channel[] = {0, 1};

struct sq_dma_ops sq_a2a_ops = {
	.request = sq_a2a_request,
	.free = sq_a2a_free,
	.enable = sq_a2a_enable,
	.disable = sq_a2a_disable,
};


struct sq_dma sq_a2a_channel_0 = {
	.dma_name = "SQ A2A Channel 0",
	.base_addr = SQ_BASE_A2A,
	.irq = SQ_INTC_A2A,
	.private_data = &sq_a2a_channel[0],
	.ops = &sq_a2a_ops,
};

struct sq_dma sq_a2a_channel_1 = {
	.dma_name = "SQ A2A Channel 1",
	.base_addr = SQ_BASE_A2A,
	.irq = SQ_INTC_A2A,
	.private_data = &sq_a2a_channel[1],
	.ops = &sq_a2a_ops,
};

static int
sq_a2a_request(u32 ch, struct sq_dma *dma)
{
	int ret = 0;
	u32 inter_ch = *((u32 *)dma->private_data);

	if (0 == inter_ch) 
		request_irq(dma->irq, sq_a2a_isr_0, dma);
	else if (1 == inter_ch)
		request_irq(dma->irq, sq_a2a_isr_1, dma);
	if (ret)
		printf("A2A: failed to request interrupt\n");
	return ret;
}

static void 
sq_a2a_free(u32 ch, struct sq_dma *dma)
{
	free_irq(dma->irq);
}

static void 
sq_a2a_enable(u32 ch, struct sq_dma *dma)
{
	u32 inter_ch = *((u32 *)dma->private_data);
	u32 conf = SQ_A2A_AUTORELOAD_DIS |
		SQ_A2A_HW_DMA_DIS |
		SQ_A2A_INT_MASK_N |
		SQ_A2A_FLY_DIS |
		SQ_A2A_TX_MODE_SINGLE |
		SQ_A2A_HDREQ0(0) |
		SQ_A2A_DIR_SRC_INC |
		SQ_A2A_DIR_DST_INC |
		SQ_A2A_DATA_SIZE_BYTE |
		SQ_A2A_SWDMA_OP_NO |
		SQ_A2A_DMA_MODE_SW;

	switch (dma->burst_type) {
	case SQ_DMA_BURST_SINGLE:
		conf |= SQ_A2A_TX_MODE_SINGLE;
		break;
	case SQ_DMA_BURST_INCR4:
		conf |= SQ_A2A_TX_MODE_INCR4;
		break;
	case SQ_DMA_BURST_INCR8:
		conf |= SQ_A2A_TX_MODE_INCR8;
		break;
	case SQ_DMA_BURST_INCR16:
		conf |= SQ_A2A_TX_MODE_INCR16;
		break;
	}
	conf |= SQ_A2A_HDREQ0(dma->ext_hdreq);
	if (SQ_DMA_DIR_FIXED == dma->src_dir)
		conf |= SQ_A2A_DIR_SRC_FIXED;
	if (SQ_DMA_DIR_FIXED == dma->dst_dir)
		conf |= SQ_A2A_DIR_DST_FIXED;		
	if (SQ_DMA_FLY_WRITE == dma->fly_op)
		conf |= SQ_A2A_FLY_EN;
	switch (dma->data_size) {
	case SQ_DMA_DATA_BYTE:
		conf |= SQ_A2A_DATA_SIZE_BYTE;
		break;
	case SQ_DMA_DATA_HALFWORD:
		conf |= SQ_A2A_DATA_SIZE_HALFWORD;
		break;
	case SQ_DMA_DATA_WORD:
		conf |= SQ_A2A_DATA_SIZE_WORD;
		break;
	}	
	if (SQ_DMA_MODE_HW == dma->mode){
		conf |= SQ_A2A_DMA_MODE_HW;
		conf |= SQ_A2A_HW_DMA_EN;
	}else{
		conf |= SQ_A2A_DMA_MODE_SW;
		conf |= SQ_A2A_SWDMA_OP_START;
	}
	switch (inter_ch) {
	case 0:    
		sq_a2a_write(SQ_A2A_ISRC0, dma->src_addr , dma->base_addr);
		sq_a2a_write(SQ_A2A_IDST0, dma->dst_addr , dma->base_addr);
		sq_a2a_write(SQ_A2A_ICNT0, dma->tx_cnt , dma->base_addr);
		sq_a2a_write(SQ_A2A_CON0, conf, dma->base_addr);
		break;
	case 1:
		sq_a2a_write(SQ_A2A_ISRC1, dma->src_addr , dma->base_addr);
		sq_a2a_write(SQ_A2A_IDST1, dma->dst_addr , dma->base_addr);
		sq_a2a_write(SQ_A2A_ICNT1, dma->tx_cnt , dma->base_addr);
		sq_a2a_write(SQ_A2A_CON1, conf, dma->base_addr);
		break;
	default:
		printf("A2A: unknown channel number %d\n", ch);
	}
}

static void 
sq_a2a_disable(u32 ch, struct sq_dma *dma)
{
	u32 inter_ch = *((u32 *)dma->private_data);
	u32 tmp;

		/* Clear channel 0 interrupt flag */
	tmp = sq_a2a_read(SQ_A2A_INT_STS, dma->base_addr);
	
	switch (inter_ch) {
	case 0:
			/* Clear channel 0 interrupt flag */
		tmp = sq_a2a_read(SQ_A2A_INT_STS, dma->base_addr);
			/* Clear channel 0 configuration register */
		sq_a2a_write(SQ_A2A_CON0, 0, dma->base_addr);
		sq_a2a_write(SQ_A2A_ISRC0, 0 , dma->base_addr);
		sq_a2a_write(SQ_A2A_IDST0, 0 , dma->base_addr);
		sq_a2a_write(SQ_A2A_ICNT0, 0, dma->base_addr);
		
		break;
	case 1:
			/* Clear channel 1 interrupt flag */
		tmp = sq_a2a_read(SQ_A2A_INT_STS, dma->base_addr);
		/* Clear channel 1 configuration register */
		sq_a2a_write(SQ_A2A_CON1, 0, dma->base_addr);
		sq_a2a_write(SQ_A2A_ISRC1, 0, dma->base_addr);
		sq_a2a_write(SQ_A2A_IDST1, 0, dma->base_addr);
		sq_a2a_write(SQ_A2A_ICNT1, 0, dma->base_addr);
		
		break;
	default:
		printf("A2A: unknown channel number %d\n", ch);
		return;
	}
}

static void
sq_a2a_isr_0(void *_dma)
{
	u32 int_stat;
	struct sq_dma *dma = _dma;
	int *err = (int *) dma->notifier->data;

	int_stat = sq_a2a_read(SQ_A2A_INT_STS, dma->base_addr);
	int_stat &= (SQ_A2A_CH0_INT_ACT | 
		     SQ_A2A_PRI_BUS_ERR |
		     SQ_A2A_SEC_BUS_ERR);

	if (int_stat & SQ_A2A_CH0_INT_ACT) {
		if (dma->notifier->complete)
			dma->notifier->complete(dma->notifier->data);
	}
	if (int_stat & SQ_A2A_PRI_BUS_ERR) {
		if (dma->notifier->error){
			*err = 1;
			dma->notifier->error(dma->notifier->data);
		}
	}
	if (int_stat & SQ_A2A_SEC_BUS_ERR) {
		if (dma->notifier->error){
			*err = 2;
			dma->notifier->error(dma->notifier->data);
		}
	}
}

static void
sq_a2a_isr_1(void *_dma)
{
	u32 int_stat;
	struct sq_dma *dma = _dma;

	int_stat = sq_a2a_read(SQ_A2A_INT_STS, dma->base_addr);
	int_stat &= (SQ_A2A_CH1_INT_ACT | 
		     SQ_A2A_PRI_BUS_ERR |
		     SQ_A2A_SEC_BUS_ERR);

	if (int_stat & SQ_A2A_CH1_INT_ACT) {
		if (dma->notifier->complete)
			dma->notifier->complete(dma->notifier->data);
	}
	if (int_stat & SQ_A2A_PRI_BUS_ERR) {
		if (dma->notifier->error)
			dma->notifier->error(dma->notifier->data);
	}
	if (int_stat & SQ_A2A_SEC_BUS_ERR) {
		if (dma->notifier->error)
			dma->notifier->error(dma->notifier->data);
	}
}
