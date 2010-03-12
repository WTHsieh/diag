#include <type.h>
#include <genlib.h>
#include <dma/dma.h>
#include <io.h>

#define xchg(ptr, x)				\
	((__typeof__(*(ptr)))__xchg((u32)(x), (ptr), sizeof(*(ptr))))

static inline u32
__xchg(u32 x, volatile void *ptr, u32 size)
{
	u32 ret;

	switch (size) {
	case 1:
		asm volatile ("@  __xchg1\n"
			      " swp %0, %1, [%2]"
			      : "=&r" (ret)
			      : "r" (x), "r" (ptr)
			      : "memory", "cc");
		break;
	case 4:
		asm volatile ("@ __xchg4\n"
			      " swp %0, %1, [%2]"
			      : "=&r" (ret)
			      : "r" (x), "r" (ptr)
			      : "memory", "cc");
		break;
	}
	return ret;
}

static struct sq_dma *sq_dma_channel[SQ_MAX_DMA_CHANNELS];

/*
 *  Request DMA channel
 *  We have to allocate an interrupt as well...
 *  */
extern int
sq_request_dma(u32 ch, struct sq_dma_notifier *notifier)
{
	struct sq_dma *dma = sq_dma_channel[ch];
	int ret;

	if ((ch >= SQ_MAX_DMA_CHANNELS) || !dma->ops)
		goto bad_dma;
	if (xchg(&dma->lock, 1) != 0)
		goto busy;
	dma->active = 0;
	ret = 0;
	dma->notifier = notifier;
	if (dma->ops->request)
		ret = dma->ops->request(ch, dma);
	if (ret)
		xchg(&dma->lock, 0);
	return ret;
bad_dma:
	printf("Sq DMA: trying to allocate DMA%d\n", ch);
busy:
	return -1;
}

/*
 *  Free DMA channel
 *  We have to free interrupt as well
 *  */
extern void
sq_free_dma(u32 ch)
{
	struct sq_dma *dma = sq_dma_channel[ch];

	if (ch >= SQ_MAX_DMA_CHANNELS || !dma->ops)
		goto bad_dma;
	if (dma->active) {
		printf("Sq DMA: freeing active DMA%d\n", ch);
		dma->ops->disable(ch, dma);
		dma->active = 0;
	}
	if (xchg(&dma->lock, 0) != 0) {
		dma->notifier = NULL;
		if (dma->ops->free)
			dma->ops->free(ch, dma);
		return;
	}
	printf("Sq DMA: trying to free free DMA%d\n", ch);
	return;
bad_dma:
	printf("Sq DMA: trying to free DMA%d\n", ch);
}

/*
 *  Enable DMA channel
 *  */
extern void 
sq_enable_dma(u32 ch)
{
	struct sq_dma *dma = sq_dma_channel[ch];

	if (!dma->lock)
		goto free_dma;
	if (0 == dma->active) {
		dma->active = 1;
		dma->ops->enable(ch, dma);
	}
	return;
free_dma:
	printf("Sq DMA%d: trying to enable free DMA\n", ch);
}

/*
 *  Disable DMA channel
 *  */
extern void
sq_disable_dma(u32 ch)
{
	struct sq_dma *dma = sq_dma_channel[ch];

	if (!dma->lock)
		goto free_dma;
	if (1 == dma->active) {
		dma->active = 0;
		dma->ops->disable(ch, dma);
	}
	return;
free_dma:
	printf("Sq DMA%d: trying to disable free DMA\n", ch);
}

/*
 *  Is the specified DAM channel active
 *  */
extern int
sq_dma_channel_active(u32 ch)
{
	return sq_dma_channel[ch]->active;
}

extern void
sq_set_dma_transfer_count(u32 ch, u32 cnt)
{
	sq_dma_channel[ch]->tx_cnt = cnt;
}

extern void
sq_set_dma_source_address(u32 ch, u32 addr)
{
	sq_dma_channel[ch]->src_addr = addr;
}

extern void
sq_set_dma_destination_address(u32 ch, u32 addr)
{
	sq_dma_channel[ch]->dst_addr = addr;
}

extern void
sq_set_dma_source_direction(u32 ch, int dir)
{
	sq_dma_channel[ch]->src_dir = dir;
}

extern void
sq_set_dma_destination_direction(u32 ch, int dir)
{
	sq_dma_channel[ch]->dst_dir = dir;
}

extern void
sq_set_dma_burst_type(u32 ch, int burst)
{
	sq_dma_channel[ch]->burst_type = burst;
}

extern void
sq_set_dma_data_size(u32 ch, int size)
{
	sq_dma_channel[ch]->data_size = size;
}

extern void
sq_set_dma_mode(u32 ch, int mode)
{
	sq_dma_channel[ch]->mode = mode;
}

extern void 
sq_set_dma_ext_hdreq_number(u32 ch, u32 num)
{
	sq_dma_channel[ch]->ext_hdreq = num;
}

extern void 
sq_set_dma_fly_operation(u32 ch, u32 fly_op)
{
	sq_dma_channel[ch]->fly_op = fly_op;
}

extern void
sq_set_dma_slice_count(u32 ch, u32 cnt)
{
	sq_dma_channel[ch]->slice_cnt = cnt;
}

extern void
sq_set_dma_page_number(u32 ch, u32 num)
{
	struct sq_dma *dma = sq_dma_channel[ch];

	if (!dma->lock)
		goto free_dma;
	dma->ops->set_page_number(ch, dma, num);
	return;
free_dma:
	printf("Sq DMA%d: trying to access free DMA\n", ch);
}

extern void
sq_set_dma_buffer_size(u32 ch, u32 size)
{
	sq_dma_channel[ch]->buf_size = size;
}

extern void
sq_dump_dma_register(u32 ch, u32 size)
{
	int i;
	
        printf("%s:\n", sq_dma_channel[ch]->dma_name);
        for (i = 0; i <= size; i += 4)
                printf("0x%08x:0x%08x\n", sq_dma_channel[ch]->base_addr+i, ioread32(sq_dma_channel[ch]->base_addr+i));
}


extern struct sq_dma sq_a2a_channel_0;
extern struct sq_dma sq_a2a_channel_1;
extern struct sq_dma sq_hdma_channel_0;
extern struct sq_dma sq_hdma_channel_1;
extern struct sq_dma sq_hdma_channel_2;
extern struct sq_dma sq_hdma_channel_3;
extern struct sq_dma panther7_hdma_channel_0;
extern struct sq_dma panther7_hdma_channel_1;

extern int
sq_init_dma(void)
{
	sq_dma_channel[SQ_A2A_CH_0] = &sq_a2a_channel_0;
	sq_dma_channel[SQ_A2A_CH_1] = &sq_a2a_channel_1;
	sq_dma_channel[SQ_HDMA_CH_0] = &sq_hdma_channel_0;
	sq_dma_channel[SQ_HDMA_CH_1] = &sq_hdma_channel_1;
	sq_dma_channel[SQ_HDMA_CH_2] = &sq_hdma_channel_2;
	sq_dma_channel[SQ_HDMA_CH_3] = &sq_hdma_channel_3;
	sq_dma_channel[PANTHER7_HDMA_CH_0] = &panther7_hdma_channel_0;
	sq_dma_channel[PANTHER7_HDMA_CH_1] = &panther7_hdma_channel_1;
	return 0;
}

extern void
sq_set_dma_base_irq(u32 ch, u32 base, u32 int_num)
{
	sq_dma_channel[ch]->base_addr = base;
	sq_dma_channel[ch]->irq = int_num;
}
