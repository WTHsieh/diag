#ifndef __SQ_DMA_H_INCLUDED
#define __SQ_DMA_H_INCLUDED

#include <type.h>

#define SQ_MAX_DMA_CHANNELS 8

enum sq_dma_channel_no{
	SQ_A2A_CH_0 = 0,
	SQ_A2A_CH_1,
	SQ_HDMA_CH_0,
	SQ_HDMA_CH_1,
	SQ_HDMA_CH_2,
	SQ_HDMA_CH_3,
	PANTHER7_HDMA_CH_0,
	PANTHER7_HDMA_CH_1
};

struct sq_dma_notifier {
	void (*complete)(void *data);
	void (*error)(void *data);
	void (*page_interrupt)(void *data);
	void (*page_accumulation_overflow)(void *data);
	void *data;
};

struct sq_dma;
struct sq_dma_ops {
	int (*request)(u32 ch, struct sq_dma *dma);
	void (*free)(u32 ch, struct sq_dma *dma);
	void (*enable)(u32 ch, struct sq_dma *dma);
	void (*disable)(u32 ch, struct sq_dma *dma);
	void (*set_page_number)(u32 ch, struct sq_dma *dma, u32 num);
};

struct sq_dma{
	u32 src_addr;		/* source address */
	u32 dst_addr;		/* destination address */
	u8 src_dir;		/* source direction */
	u8 dst_dir;		/* destination direction */
	u32 tx_cnt;		/* transfer count */
	u32 burst_type;		/* burst type */
	u32 data_size;		/* data size */
	u8 mode;		/* transfer mode */
	u32 slice_cnt;		/* slice count */
	u32 buf_size;		/* buffer size */
	u32 fly_op;		/* on the fly mode	*/
	u32 ext_hdreq;		/* external hdreq source selection */
	const char *dma_name;	/* dma name */
	const char *device_id;	/* device name */
	u8 active;		/* transfer active */
	u32 lock;		/* device is allocated */
	u32 base_addr;		/* controller base address */
	u32 irq;		/* controller IRQ */
	void *private_data;	/* private data */
	struct sq_dma_ops *ops;
	struct sq_dma_notifier *notifier;
};

/* DMA mode */
enum SQ_DMA_MODE {
	SQ_DMA_MODE_SW,
	SQ_DMA_MODE_HW,
	SQ_DMA_MODE_SLICE,
};

/* DMA burst type */
enum SQ_DMA_BURST {
	SQ_DMA_BURST_SINGLE,
	SQ_DMA_BURST_INCR4,
	SQ_DMA_BURST_INCR8,
	SQ_DMA_BURST_INCR16,
};

/* DMA source/destination direction */
enum SQ_DMA_DIRECTION {
	SQ_DMA_DIR_INCR,
	SQ_DMA_DIR_FIXED,
};

/* DMA data size */
enum SQ_DMA_DATA_SIZE {
	SQ_DMA_DATA_BYTE,
	SQ_DMA_DATA_HALFWORD,
	SQ_DMA_DATA_WORD,
};

/* DMA on the fly mode enable/disable */
enum SQ_DMA_FLY {
	SQ_DMA_FLY_DIS,
	SQ_DMA_FLY_READ,
	SQ_DMA_FLY_WRITE,
};

/* Request a DMA channel */
extern int sq_request_dma(u32 ch, struct sq_dma_notifier *notifier);

/* Free a DMA channel */
extern void sq_free_dma(u32 ch);

/* Enable DMA channel */
extern void sq_enable_dma(u32 ch);

/* Disable DMA for this channel */
extern void sq_disable_dma(u32 ch);

/* Test whether the specified channel has an active DMA transfer */
extern int sq_dma_channel_active(u32 ch);

/* Set DMA transfer count */
extern void sq_set_dma_transfer_count(u32 ch, u32 cnt);

/* Set DMA source address */
extern void sq_set_dma_source_address(u32 ch, u32 addr);

/* Set DMA destination address */
extern void sq_set_dma_destination_address(u32 ch, u32 addr);

/* Set DMA source direction */
extern void sq_set_dma_source_direction(u32 ch, int dir);

/* Set DMA destination direction */
extern void sq_set_dma_destination_direction(u32 ch, int dir);

/* Set DMA burst type */
extern void sq_set_dma_burst_type(u32 ch, int burst);

/* Set DMA data size */
extern void sq_set_dma_data_size(u32 ch, int size);

/* Set DMA mode */
extern void sq_set_dma_mode(u32 ch, int mode);

/* Set DMA external hdreq source selection */
extern void sq_set_dma_ext_hdreq_number(u32 ch, u32 num);

/* Set DMA on the fly mode enable/disable */
extern void sq_set_dma_fly_operation(u32 ch, u32 fly_op);

/* Set DMA slice count */
extern void sq_set_dma_slice_count(u32 ch, u32 cnt);

/* Set DMA page number */
extern void sq_set_dma_page_number(u32 ch, u32 num);

/* Set DMA buffer size */
extern void sq_set_dma_buffer_size(u32 ch, u32 size);

extern void sq_dump_dma_register(u32 ch, u32 size);

extern int sq_init_dma(void);

extern void sq_set_dma_base_irq(u32 ch, u32 base, u32 int_num);
#endif
