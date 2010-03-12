#include <test_item.h>
#include <genlib.h>
#include "protocol.h"
#include <dma/dma.h>
#include <clock.h>
#include "sdmmc-regs.h"
#include "dependency.h"
#ifdef CONFIG_PC9002
#include "MP_GPIO/regs-mp-gpio.h"
#include "MP_GPIO/mp-gpio.h"
#endif

#ifdef CONFIG_SQ8000
#include <scu.h>
#endif

#undef PDEBUG
#ifdef SDMMC_DEBUG
#define PDEBUG(fmt, args...) printf(fmt, ## args)
#else
#define PDEBUG(fmt, args...)
#endif

#define TO_THRESHOLD_CNT (1024 * 1024)
#define TEST_SECT_NUM 4
#define TEST_MEM_PATTERN_ADDR 0x00a00000
#define TEST_MEM_CMPR_ADDR (TEST_MEM_PATTERN_ADDR + (TEST_SECT_NUM * 512))

/* SD's commands */
#define SD_ERASE_WR_BLK_START 32 /* ac [31:0] data address R1 */
#define SD_ERASE_WR_BLK_END 33	/* ac [31:0] data address R1 */

static inline void
sq_sdmmc_write(u32 reg, u32 val, u32 base)
{
	base = base + reg;
	iowrite32(val, base);
}

static inline u32
sq_sdmmc_read(u32 reg, u32 base)
{
	u32 val;

	base = base + reg;
	val = ioread32(base);
	return val;
}

#define SQ_SDMMC_DATABUF_1_XFER_WIDTH_MASK 0x3
#define SQ_SDMMC_DATABUF_2_XFER_WIDTH_MASK 0x30
#define SQ_SDMMC_DATABUF_SWAP_MASK 0x300
#define SQ_SDMMC_MMU_SWAP_MASK 0x3FF

static inline void
sq_sdmmc_mmu_swap(u32 base)
{
	u32 val;
	u32 databuf_swap_val;
	u32 buf_2_xfer_width;
	u32 buf_1_xfer_width;

	val = sq_sdmmc_read(SQ_SDMMC_MMU_CTRL, base);
	databuf_swap_val = (val & SQ_SDMMC_DATABUF_SWAP_MASK) ^ SQ_SDMMC_DATABUF_SWAP_MASK;
	buf_1_xfer_width = (val & SQ_SDMMC_DATABUF_2_XFER_WIDTH_MASK) >> 4;
	buf_2_xfer_width = (val & SQ_SDMMC_DATABUF_1_XFER_WIDTH_MASK) << 4;
	val &= ~SQ_SDMMC_MMU_SWAP_MASK;
	sq_sdmmc_write(SQ_SDMMC_MMU_CTRL,
			  val |
			  databuf_swap_val |
			  SQ_SDMMC_DATABUF_2_POINTER_RST |
			  SQ_SDMMC_DATABUF_2_POINTER_END_SIGNAL_LOW |
			  buf_2_xfer_width |
			  SQ_SDMMC_DATABUF_1_POINTER_RST |
			  SQ_SDMMC_DATABUF_1_POINTER_END_SIGNAL_LOW |
			  buf_1_xfer_width, 
			  base);
}

static inline void 
sq_sdmmc_delay(u32 count)
{
	u32 i;

	for (i = 0; i < count; i++) {;}
}

static u32 sq_sdmmc_to_cnt = 0;

static inline void
sq_sdmmc_card_busy_end(u32 base)
{
	u32 rdata;

	sq_sdmmc_to_cnt = 0;
	do {
		if (sq_sdmmc_to_cnt > TO_THRESHOLD_CNT) {
			printf("\nSq SD/MMC: waiting for card's busy signal is timeout\n");
			return;
		}
		rdata = sq_sdmmc_read(SQ_SDMMC_SD_CARDA, base);
		sq_sdmmc_to_cnt++;
	} while (SQ_SDMMC_CARD_BUSY_SIGNAL_HIGH != (rdata & SQ_SDMMC_CARD_BUSY_SIGNAL_HIGH));
}

/*
 *  SD Card State Machine
 *  */
enum sd_card_state_machine{
	IDLE,			/* idle state */
	READY,			/* ready state */
	IDENT,			/* identification state */
	STDY,			/* stand-by state */
	TRAN,			/* transfer state */
	DATA,			/* sending-data state */
	RCV,			/* receive-data state */
	PRG,			/* programming state */
	DIS			/* disconnect state */
};

/*
 *  Card type
 *  */
enum card_type {
	SD_CARD,
	MMC
};

/*
 *  Interrupt Bit
 *  */
enum int_bit {
	CMD_RESP_INT = (1 << 2),
	DATA_XFER_INT = (1 << 1),
	CARD_DET_INT = 1
};

/* Test pattern's width */
enum {
	PATTERN_WIDTH_BYTE = 0,
	PATTERN_WIDTH_HALFWORD,
	PATTERN_WIDTH_WORD
};

/*
 *  The internal representation of our device.
 *  */
struct sq_sdmmc_device {
	int size;		/* device size in sectors */
	u32 card_rca;		/* card's relative card address */
	u32 card_res_0;		/* card's response arguments -0 */
	u32 card_res_1;		/* card's response arguments -1 */
	u32 card_res_2;		/* card's response arguments -2 */
	u32 card_res_3;		/* card's response arguments -3 */
	u32 card_state;		/* card's state machine */
	u8 card_type;		/* card's type */
	volatile u8 card_int;	/* card's interrupt status */
	u8 alive_flag;		/* card's alive flag */
	u32 mmu_setting;	/* host controller's mmu setting */
	int bus_width;		/* card's transfer bus width */
	u32 pattern_width;	/* test pattern width */
	u8 media_changed;	/* flag a media change? */
};

static int (*sq_sdmmc_transfer)(int autotest);
static int sq_sdmmc_single_block_sw_dma_transfer(int autotest);
static int sq_sdmmc_multiple_block_sw_dma_transfer(int autotest);
static int sq_sdmmc_single_block_transfer(int autotest);
static int sq_sdmmc_multiple_block_transfer(int autotest);
static void sq_sdmmc_device_initialize(struct sq_sdmmc_device *dev);
static void sq_sdmmc_isr(void *data);
static int sq_sdmmc_fill_pattern(int pattern_width, u32 block_num);
static int sq_sdmmc_compare_pattern(int pattern_width, u32 block_num);
static void sq_sdmmc_make_test_pattern_dma(u8 *mem, int pattern_width, u32 block_num);
static int sq_sdmmc_compare_memory(u8 *mem, u8 *mem_cmpr, u32 cnt, int skip_cmpr_result);
static u32 sq_sdmmc_send_cmd(struct sq_sdmmc_device *dev, u16 cmd_abbr, u32 arg);
static u32 sq_sdmmc_report_command_response_error(void);
static int sq_sdmmc_data_transfer_end(struct sq_sdmmc_device *dev);
static void sq_sdmmc_report_data_transfer_error(void);
static int sq_sdmmc_card_initialize(struct sq_sdmmc_device *dev, int only_ident);
static void sq_sdmmc_dma_complete(void *data);

static struct sq_dma_notifier sq_sdmmc_dma_notifier = {
	.complete = sq_sdmmc_dma_complete,
};

static struct sq_sdmmc_device sq_sdmmc_dev;
static volatile u32 sq_sdmmc_sw_dma_complete_flag = 0;
static u32 sq_sdmmc_base;
static u32 sq_sdmmc_burst_type;
static u32 sq_sdmmc_data_size;
static u32 sq_sdmmc_dma_channel_num;

extern struct test_item_container sq_sdmmc_sd_main_test_container;
extern struct test_item_container sq_sdmmc_mmc_main_test_container;

extern int
SDTesting(int autotest)
{
	int ret = 0;
#ifdef CONFIG_PC9002
	sq_mp_gpio_set_port_num_value(PA,5,0);
#endif

#ifdef CONFIG_SQ8000
	sq_scu_dev_enable(SQ_DEVCON_SDMMC);	
#endif
	sq_sdmmc_base = SQ_APB0_SDMMC0;
	sq_sdmmc_device_initialize(&sq_sdmmc_dev);
	request_irq(SQ_INTC_SDMMC0, sq_sdmmc_isr, &sq_sdmmc_dev);
	if (0 == sq_sdmmc_dev.alive_flag) {
		printf("\nSq SD/MMC host: card is not exist\n");
		return -1;
	}
	if (sq_sdmmc_dev.media_changed) {
		if (sq_sdmmc_card_initialize(&sq_sdmmc_dev, 0)) {
			printf("\nSq SD/MMC host: card initialization is fail\n");
			return -1;
		}
	}
	if (SD_CARD == sq_sdmmc_dev.card_type)
		ret = test_item_ctrl(&sq_sdmmc_sd_main_test_container, autotest);
	else if (MMC == sq_sdmmc_dev.card_type) {
		sq_sdmmc_dev.bus_width = SQ_SDMMC_DATA_XFER_BUS_WIDTH_LINE_1;
		ret = test_item_ctrl(&sq_sdmmc_mmc_main_test_container, autotest);
	}
	else {
		printf("\nSq SD/MMC host: unknown card type\n");
		return -1;
	}

	/* Send CMD0 to go to idle state */
	if (sq_sdmmc_send_cmd(&sq_sdmmc_dev, MMC_GO_IDLE_STATE, 0)) {
		printf("\nSq SD/MMC host: cannot go to \"Idle State\", please re-insert the card\n");
		return -1;
	}

	free_irq(SQ_INTC_SDMMC0);
	return ret;
}

/*
 *  SD
 *  */
extern struct test_item_container sq_sdmmc_sd_xfer_test_container;

extern int
sq_sdmmc_sd_bus_1_transfer_test(int autotest)
{
	int ret = 0;

	sq_sdmmc_dev.bus_width = SQ_SDMMC_DATA_XFER_BUS_WIDTH_LINE_1;
	ret = test_item_ctrl(&sq_sdmmc_sd_xfer_test_container, autotest);
	return ret;
}

extern int
sq_sdmmc_sd_bus_4_transfer_test(int autotest)
{
	int ret = 0;

	sq_sdmmc_dev.bus_width = SQ_SDMMC_DATA_XFER_BUS_WIDTH_LINE_4;
	ret = test_item_ctrl(&sq_sdmmc_sd_xfer_test_container, autotest);
	return ret;
}

extern int
sq_sdmmc_sd_erase(int autotest)
{
	int i;
	u32 rdata;

	if (0 == sq_sdmmc_dev.alive_flag) {
		printf("sd/mmc host: card is not exist\n");
		return -1;
	}
	if (sq_sdmmc_dev.media_changed) {
		if (sq_sdmmc_card_initialize(&sq_sdmmc_dev, 0)) {
			printf("sd/mmc host: card initialization is fail\n");
			return -1;
		}
	}
	
	/* Send CMD55 to indicate that the next command is and application specific command */
	if (sq_sdmmc_send_cmd(&sq_sdmmc_dev, MMC_APP_CMD, sq_sdmmc_dev.card_rca))
		return -1;

	/* Send ACMD6 to select given bus width */
	if (sq_sdmmc_send_cmd(&sq_sdmmc_dev, SD_APP_SET_BUS_WIDTH, SD_BUS_WIDTH_1))
		return -1;

	/* Send CMD 16 to set the block size */
	if (sq_sdmmc_send_cmd(&sq_sdmmc_dev, MMC_SET_BLOCKLEN, 512))
		return -1;

	sq_sdmmc_write(SQ_SDMMC_MMU_CTRL, SQ_SDMMC_MMU_DATA_WIDTH_BYTE, sq_sdmmc_base);

	/* Fill the test pattern into MMU buffer */
	sq_sdmmc_fill_pattern(PATTERN_WIDTH_BYTE, 0);

	/* Send CMD24 to write single block */
	if (sq_sdmmc_send_cmd(&sq_sdmmc_dev, MMC_WRITE_BLOCK, 0))
		return -1;
	sq_sdmmc_dev.card_state = RCV;

	sq_sdmmc_mmu_swap(sq_sdmmc_base);

	/* Set the transfer setting and start a writing transmission */
	sq_sdmmc_write(SQ_SDMMC_SD_DATAT,
			  SQ_SDMMC_DATA_XFER_SIGNAL_BEGIN |
			  SQ_SDMMC_DATA_XFER_DIR_WRITE |
			  SQ_SDMMC_DATA_XFER_BUS_WIDTH_LINE_1 |
			  SQ_SDMMC_DATA_XFER_DMA_DIS |
			  SQ_SDMMC_DATA_XFER_CYC_SINGLE,
			  sq_sdmmc_base);

	/* Wait for the writing transmission to be complete */
	if (sq_sdmmc_data_transfer_end(&sq_sdmmc_dev))
		return -1;
	sq_sdmmc_dev.card_state = PRG;

	/* Wait for the card to depart from the busy state */
	sq_sdmmc_card_busy_end(sq_sdmmc_base);
	sq_sdmmc_dev.card_state = TRAN;

	/* Send CMD32 to set the address of the first write block to be erased */
	if (sq_sdmmc_send_cmd(&sq_sdmmc_dev, SD_ERASE_WR_BLK_START, 0))
		return -1;

	/* Send CMD 33 to set the address of the last write block of the continuous range to be erased */
	if (sq_sdmmc_send_cmd(&sq_sdmmc_dev, SD_ERASE_WR_BLK_END, 512))
		return -1;

	/* Send CMD38 to erase all previously selected write blocks */
	if (sq_sdmmc_send_cmd(&sq_sdmmc_dev, MMC_ERASE, 0x00000000))
		return -1;
	sq_sdmmc_dev.card_state = PRG;

	/* Wait for the card to depart from the busy state */
	sq_sdmmc_card_busy_end(sq_sdmmc_base);
	sq_sdmmc_dev.card_state = TRAN;

	sq_sdmmc_write(SQ_SDMMC_MMU_CTRL, SQ_SDMMC_MMU_DATA_WIDTH_BYTE, sq_sdmmc_base);

	/* Set the transfer settnig and start a reading transmission */
	sq_sdmmc_write(SQ_SDMMC_SD_DATAT,
			  SQ_SDMMC_DATA_XFER_SIGNAL_BEGIN |
			  SQ_SDMMC_DATA_XFER_DIR_READ |
			  SQ_SDMMC_DATA_XFER_BUS_WIDTH_LINE_1 |
			  SQ_SDMMC_DATA_XFER_DMA_DIS |
			  SQ_SDMMC_DATA_XFER_CYC_SINGLE,
			  sq_sdmmc_base);

	/* Send CMD17 to read a single block */
	if (sq_sdmmc_send_cmd(&sq_sdmmc_dev, MMC_READ_SINGLE_BLOCK, 0))
		return -1;
	sq_sdmmc_dev.card_state = DATA;

	/* Wait for the reading transmission to be complete */
	if (sq_sdmmc_data_transfer_end(&sq_sdmmc_dev))
		return -1;
	sq_sdmmc_dev.card_state = TRAN;

	sq_sdmmc_mmu_swap(sq_sdmmc_base);

	/* Check whether the  data of MMU buffer has been erased to 0x00 or 0xFF  */
	for (i = 0; i < 512 - 1; i++) {
		rdata = sq_sdmmc_read(SQ_SDMMC_MMU_DATA, sq_sdmmc_base);
		if ((rdata != 0x00) && (rdata != 0xFF)) {
			printf("\n Sq SD/MMC host: data compare failed!\n");
			return -1;
		}
	}

	return 0;
}

extern struct test_item_container sq_sdmmc_sd_misc_command_test_container;

extern int
sq_sdmmc_sd_misc_command_test(int autotest)
{
	int ret = 0;

	ret = test_item_ctrl(&sq_sdmmc_sd_misc_command_test_container, autotest);
	return ret;
}

extern struct test_item_container sq_sdmmc_xfer_width_test_container;

extern int
sq_sdmmc_sd_single_block_transfer_test(int autotest)
{
	int ret = 0;

	sq_sdmmc_transfer = sq_sdmmc_single_block_transfer;
	ret = test_item_ctrl(&sq_sdmmc_xfer_width_test_container, autotest);
	return ret;
}

extern int
sq_sdmmc_sd_multiple_block_transfer_test(int autotest)
{
	int ret = 0;

	sq_sdmmc_transfer = sq_sdmmc_multiple_block_transfer;
	ret = test_item_ctrl(&sq_sdmmc_xfer_width_test_container, autotest);
	return ret;
}

extern struct test_item_container sq_sdmmc_swdma_burst_type_test_container;

extern int
sq_sdmmc_sd_single_block_sw_dma_a2a_transfer_test(int autotest)
{
	int ret = 0;

	sq_sdmmc_transfer = sq_sdmmc_single_block_sw_dma_transfer;
	sq_sdmmc_dma_channel_num = SQ_A2A_CH_0;
	sq_request_dma(sq_sdmmc_dma_channel_num, &sq_sdmmc_dma_notifier);
	ret = test_item_ctrl(&sq_sdmmc_swdma_burst_type_test_container, autotest);
	sq_disable_dma(sq_sdmmc_dma_channel_num);
	sq_free_dma(sq_sdmmc_dma_channel_num);
	return ret;
}

extern int
sq_sdmmc_sd_multiple_block_sw_dma_a2a_transfer_test(int autotest)
{
	int ret = 0;

	sq_sdmmc_transfer = sq_sdmmc_multiple_block_sw_dma_transfer;
	sq_sdmmc_dma_channel_num = SQ_A2A_CH_0;
	sq_request_dma(sq_sdmmc_dma_channel_num, &sq_sdmmc_dma_notifier);
	ret = test_item_ctrl(&sq_sdmmc_swdma_burst_type_test_container, autotest);
	sq_disable_dma(sq_sdmmc_dma_channel_num);
	sq_free_dma(sq_sdmmc_dma_channel_num);
	return ret;
}

extern int 
sq_sdmmc_sd_single_block_sw_dma_hdma_transfer_test(int autotest)
{
	int ret = 0;

	sq_sdmmc_transfer = sq_sdmmc_single_block_sw_dma_transfer;
	sq_sdmmc_dma_channel_num = SQ_HDMA_CH_0;
	sq_request_dma(sq_sdmmc_dma_channel_num, &sq_sdmmc_dma_notifier);
	ret = test_item_ctrl(&sq_sdmmc_swdma_burst_type_test_container, autotest);
	sq_disable_dma(sq_sdmmc_dma_channel_num);
	sq_free_dma(sq_sdmmc_dma_channel_num);
	return ret;
}

extern int 
sq_sdmmc_sd_multiple_block_sw_dma_hdma_transfer_test(int autotest)
{
	int ret = 0;

	sq_sdmmc_transfer = sq_sdmmc_multiple_block_sw_dma_transfer;
	sq_sdmmc_dma_channel_num = SQ_HDMA_CH_0;
	sq_request_dma(sq_sdmmc_dma_channel_num, &sq_sdmmc_dma_notifier);
	ret = test_item_ctrl(&sq_sdmmc_swdma_burst_type_test_container, autotest);
	sq_disable_dma(sq_sdmmc_dma_channel_num);
	sq_free_dma(sq_sdmmc_dma_channel_num);
	return ret;
}

extern int 
sq_sdmmc_sd_single_block_sw_dma_panther7_hdma_transfer_test(int autotest)
{
	int ret = 0;

	sq_sdmmc_transfer = sq_sdmmc_single_block_sw_dma_transfer;
	sq_sdmmc_dma_channel_num = PANTHER7_HDMA_CH_0;
	sq_request_dma(sq_sdmmc_dma_channel_num, &sq_sdmmc_dma_notifier);
	ret = test_item_ctrl(&sq_sdmmc_swdma_burst_type_test_container, autotest);
	sq_disable_dma(sq_sdmmc_dma_channel_num);
	sq_free_dma(sq_sdmmc_dma_channel_num);
	return ret;
}

extern int 
sq_sdmmc_sd_multiple_block_sw_dma_panther7_hdma_transfer_test(int autotest)
{
	int ret = 0;

	sq_sdmmc_transfer = sq_sdmmc_multiple_block_sw_dma_transfer;
	sq_sdmmc_dma_channel_num = PANTHER7_HDMA_CH_0;
	sq_request_dma(sq_sdmmc_dma_channel_num, &sq_sdmmc_dma_notifier);
	ret = test_item_ctrl(&sq_sdmmc_swdma_burst_type_test_container, autotest);
	sq_disable_dma(sq_sdmmc_dma_channel_num);
	sq_free_dma(sq_sdmmc_dma_channel_num);
	return ret;
}

extern int
sq_sdmmc_sd_single_partial_read(int autotest)
{
	u8 *mem_cmpr = (u8 *)TEST_MEM_CMPR_ADDR;
	u32 bus_width_def;
	u32 rblock_size = 1;
	u32 rdata, i;

	if (0 == sq_sdmmc_dev.alive_flag) {
		printf("\nSq SD/MMC host: card is not exist\n");
		return -1;
	}
	if (sq_sdmmc_dev.media_changed) {
		if (sq_sdmmc_card_initialize(&sq_sdmmc_dev, 0)) {
			printf("\nSq SD/MMC host: card initialization is fail\n");
			return -1;
		}
	}
	if (SQ_SDMMC_DATA_XFER_BUS_WIDTH_LINE_1 == sq_sdmmc_dev.bus_width)
		bus_width_def = SD_BUS_WIDTH_1;
	else if (SQ_SDMMC_DATA_XFER_BUS_WIDTH_LINE_4 == sq_sdmmc_dev.bus_width)
		bus_width_def = SD_BUS_WIDTH_4;
	else {
		printf("\nSq SD/MMC host: undefined bus width\n");
		return -1;
	}
	
	/* Send CMD55 to indicate that the next command is and application specific command */
	if (sq_sdmmc_send_cmd(&sq_sdmmc_dev, MMC_APP_CMD, sq_sdmmc_dev.card_rca))
		return -1;

	/* Send ACMD6 to select given bus width */
	if (sq_sdmmc_send_cmd(&sq_sdmmc_dev, SD_APP_SET_BUS_WIDTH, bus_width_def))
		return -1;

	/* Fill the test pattern into the main memory */
	memset(mem_cmpr, 0x0, 512);
	sq_sdmmc_make_test_pattern_dma(mem_cmpr, PATTERN_WIDTH_BYTE, 0); 

	/* Send CMD16 to set the block size */
	if (sq_sdmmc_send_cmd(&sq_sdmmc_dev, MMC_SET_BLOCKLEN, 512))
		return -1;

	sq_sdmmc_write(SQ_SDMMC_MMU_CTRL, SQ_SDMMC_MMU_DATA_WIDTH_BYTE, sq_sdmmc_base);

	/* Fill the test pattern into MMU buffer1 */
	sq_sdmmc_fill_pattern(PATTERN_WIDTH_BYTE, 0);

	/* Send CMD24 to write single block */
	if (sq_sdmmc_send_cmd(&sq_sdmmc_dev, MMC_WRITE_BLOCK, 0))
		return -1;
	sq_sdmmc_dev.card_state = RCV;

	sq_sdmmc_mmu_swap(sq_sdmmc_base);

	/* Set the transfer setting and start a writing transmission */
	sq_sdmmc_write(SQ_SDMMC_SD_DATAT,
			  SQ_SDMMC_DATA_XFER_SIGNAL_BEGIN |
			  SQ_SDMMC_DATA_XFER_DIR_WRITE |
			  sq_sdmmc_dev.bus_width |
			  SQ_SDMMC_DATA_XFER_DMA_DIS |
			  SQ_SDMMC_DATA_XFER_CYC_SINGLE,
			  sq_sdmmc_base);

	/* Wait for the writing transmission to be complete */
	if (sq_sdmmc_data_transfer_end(&sq_sdmmc_dev))
		return -1;
	sq_sdmmc_dev.card_state = PRG;

	/* Wait for the card to depart from the busy state */
	sq_sdmmc_card_busy_end(sq_sdmmc_base);
	sq_sdmmc_dev.card_state = TRAN;

	/* Do the single block partial reading process */
	while (rblock_size < 512) {
		/* Set the MMU buffer pointer */
		sq_sdmmc_write(SQ_SDMMC_MMU_PNRI, rblock_size-1, sq_sdmmc_base);
		sq_sdmmc_write(SQ_SDMMC_MMU_PNRII, rblock_size-1, sq_sdmmc_base); 
	  
		/* Send CMD16 to set the block size */
		if (sq_sdmmc_send_cmd(&sq_sdmmc_dev, MMC_SET_BLOCKLEN, rblock_size))
			return -1;

		sq_sdmmc_write(SQ_SDMMC_MMU_CTRL, SQ_SDMMC_MMU_DATA_WIDTH_BYTE, sq_sdmmc_base);

		/* Set the transfer setting and start a reading transmission */
		sq_sdmmc_write(SQ_SDMMC_SD_DATAT,
				  SQ_SDMMC_DATA_XFER_SIGNAL_BEGIN |
				  SQ_SDMMC_DATA_XFER_DIR_READ |
				  sq_sdmmc_dev.bus_width |
				  SQ_SDMMC_DATA_XFER_DMA_DIS |
				  SQ_SDMMC_DATA_XFER_CYC_SINGLE,
				  sq_sdmmc_base);

		/* Send CMD17 to read a single block */
		if (sq_sdmmc_send_cmd(&sq_sdmmc_dev, MMC_READ_SINGLE_BLOCK, 0))
			return -1;
		sq_sdmmc_dev.card_state = DATA;

		/* Wait for the reading transmisson to be complete */
		if (sq_sdmmc_data_transfer_end(&sq_sdmmc_dev))
			return -1;
		sq_sdmmc_dev.card_state = TRAN;

		sq_sdmmc_mmu_swap(sq_sdmmc_base);

		/* Compare data of MMU buffer1 with the test pattern written before */
		for (i = 0; i < rblock_size; i++) {
			rdata = sq_sdmmc_read(SQ_SDMMC_MMU_DATA, sq_sdmmc_base);
			if (mem_cmpr[i] != rdata) {
				printf("\nSq SD/MMC host: data compare is fail\n");
				return -1;
			}
		}

		rblock_size++; 	/* increase the read block size */
	}
	/*
	 *  Restore initial setting
	 *  */
	/* Send CMD16 to set the block length */
	if (sq_sdmmc_send_cmd(&sq_sdmmc_dev, MMC_SET_BLOCKLEN, 512))
		return -1;

	/* Initialize the MMU buffer pointer */
	sq_sdmmc_write(SQ_SDMMC_MMU_PNRI, 0x000001ff, sq_sdmmc_base);
	sq_sdmmc_write(SQ_SDMMC_MMU_PNRII, 0x000001ff, sq_sdmmc_base);

	return 0;
}

extern int
sq_sdmmc_sd_multiple_partial_read(int autotest)
{
	u8 *mem_cmpr =(u8 *)TEST_MEM_CMPR_ADDR;
	u32 bus_width_def;
	u32 rblock_size = 1;
	u32 rdata, i, j;

	if (0 == sq_sdmmc_dev.alive_flag) {
		printf("\nSq SD/MMC host: card is not exist\n");
		return -1;
	}
	if (sq_sdmmc_dev.media_changed) {
		if (sq_sdmmc_card_initialize(&sq_sdmmc_dev, 0)) {
			printf("\nSq SD/MMC host: card initialization is fail\n");
			return -1;
		}
	}
	if (SQ_SDMMC_DATA_XFER_BUS_WIDTH_LINE_1 == sq_sdmmc_dev.bus_width)
		bus_width_def = SD_BUS_WIDTH_1;
	else if (SQ_SDMMC_DATA_XFER_BUS_WIDTH_LINE_4 == sq_sdmmc_dev.bus_width)
		bus_width_def = SD_BUS_WIDTH_4;
	else {
		printf("\nSq SD/MMC host: undefined bus width\n");
		return -1;
	}
	
	/* Send CMD55 to indicate that the next command is and application specific command */
	if (sq_sdmmc_send_cmd(&sq_sdmmc_dev, MMC_APP_CMD, sq_sdmmc_dev.card_rca))
		return -1;

	/* Send ACMD6 to select given bus width */
	if (sq_sdmmc_send_cmd(&sq_sdmmc_dev, SD_APP_SET_BUS_WIDTH, bus_width_def))
		return -1;

	/* Fill the test pattern into the main memory */
	memset(mem_cmpr, 0x0, 512);
	sq_sdmmc_make_test_pattern_dma(mem_cmpr, PATTERN_WIDTH_BYTE, 0);

	/* Send CMD16 to set the block size */
	if (sq_sdmmc_send_cmd(&sq_sdmmc_dev, MMC_SET_BLOCKLEN, 512))
		return -1;

	sq_sdmmc_write(SQ_SDMMC_MMU_CTRL, SQ_SDMMC_MMU_DATA_WIDTH_BYTE, sq_sdmmc_base);

	/* Fill the test pattern into MMU buffer */
	sq_sdmmc_fill_pattern(PATTERN_WIDTH_BYTE, 0);

	/* Send CMD24 to write single block */
	if (sq_sdmmc_send_cmd(&sq_sdmmc_dev, MMC_WRITE_BLOCK, 0))
		return -1;
	sq_sdmmc_dev.card_state = RCV;

	sq_sdmmc_mmu_swap(sq_sdmmc_base);

	/* Set the transfer setting and start a writing transmisson */
	sq_sdmmc_write(SQ_SDMMC_SD_DATAT,
			  SQ_SDMMC_DATA_XFER_SIGNAL_BEGIN |
			  SQ_SDMMC_DATA_XFER_DIR_WRITE |
			  sq_sdmmc_dev.bus_width |
			  SQ_SDMMC_DATA_XFER_DMA_DIS |
			  SQ_SDMMC_DATA_XFER_CYC_SINGLE,
			  sq_sdmmc_base);

	/* Wait for the writing transmisson to be complete */
	if (sq_sdmmc_data_transfer_end(&sq_sdmmc_dev))
		return -1;
	sq_sdmmc_dev.card_state = PRG;

	/* Wait for the card to depart from busy state */
	sq_sdmmc_card_busy_end(sq_sdmmc_base);
	sq_sdmmc_dev.card_state = TRAN;

	/* Do the multiple block partial reading process */
	while (rblock_size <= 256) {
		/* Send CMD16 to set the block size */
		if (sq_sdmmc_send_cmd(&sq_sdmmc_dev, MMC_SET_BLOCKLEN, rblock_size))
			return -1;

		/* Set the MMU buffer pointer */
		sq_sdmmc_write(SQ_SDMMC_MMU_PNRI, rblock_size-1, sq_sdmmc_base);
		sq_sdmmc_write(SQ_SDMMC_MMU_PNRII, rblock_size-1, sq_sdmmc_base); 

		sq_sdmmc_write(SQ_SDMMC_MMU_CTRL, SQ_SDMMC_MMU_DATA_WIDTH_BYTE, sq_sdmmc_base);

		/* Set the transfer setting and start a reading transmission */
		sq_sdmmc_write(SQ_SDMMC_SD_DATAT,
				  SQ_SDMMC_DATA_XFER_SIGNAL_BEGIN |
				  SQ_SDMMC_DATA_XFER_DIR_READ |
				  sq_sdmmc_dev.bus_width |
				  SQ_SDMMC_DATA_XFER_DMA_DIS |
				  SQ_SDMMC_DATA_XFER_CYC_MULTIPLE,
				  sq_sdmmc_base);

		/* Send CMD18 to read multiple blocks */
		if (sq_sdmmc_send_cmd(&sq_sdmmc_dev, MMC_READ_MULTIPLE_BLOCK, 0))
			return -1;
		sq_sdmmc_dev.card_state = DATA;

		/* Wait for the reading transmisson to be complete */
		if (sq_sdmmc_data_transfer_end(&sq_sdmmc_dev))
			return -1;

		sq_sdmmc_mmu_swap(sq_sdmmc_base);

		/* Set the transfer settting and start a reading transmission */
		sq_sdmmc_write(SQ_SDMMC_SD_DATAT,
				  SQ_SDMMC_DATA_XFER_SIGNAL_BEGIN |
				  SQ_SDMMC_DATA_XFER_DIR_READ |
				  sq_sdmmc_dev.bus_width |
				  SQ_SDMMC_DATA_XFER_DMA_DIS |
				  SQ_SDMMC_DATA_XFER_CYC_SINGLE,
				  sq_sdmmc_base);

		/* Compare data of MMU buffer1 with the test pattern written before */
		for (i = 0, j = 0; i < rblock_size; i++, j++) {
			rdata = sq_sdmmc_read(SQ_SDMMC_MMU_DATA, sq_sdmmc_base);
			if (mem_cmpr[j] != rdata) {
				printf("\nSq SD/MMC host: data compare is fail\n");
				return -1;
			}
		}

		/* Wait for the reading transmission to be complete */
		if (sq_sdmmc_data_transfer_end(&sq_sdmmc_dev))
			return -1;

		/* Send CMD12 to stop reading multiple blocks */
		if (sq_sdmmc_send_cmd(&sq_sdmmc_dev, MMC_STOP_TRANSMISSION, 0x00000000))
			return -1;
		sq_sdmmc_dev.card_state = TRAN;

		sq_sdmmc_mmu_swap(sq_sdmmc_base);

		/* Compare data of MMU buffer2 with the test pattern written before */
		for (i = 0; i < rblock_size; i++, j++) {
			rdata = sq_sdmmc_read(SQ_SDMMC_MMU_DATA, sq_sdmmc_base);
			if (mem_cmpr[j] != rdata) {
				printf("\nSq SD/MMC host: data compare failed\n");
				return -1;
			}
		}
		rblock_size++;	/* increase the read block size */
	}

	/*
	 *  Restore initial setting
	 *  */
	/* Send CMD16 to set the block length */
	if (sq_sdmmc_send_cmd(&sq_sdmmc_dev, MMC_SET_BLOCKLEN, 512))
		return -1;

	/* Initialize the MMU buffer pointer */
	sq_sdmmc_write(SQ_SDMMC_MMU_PNRI, 0x000001ff, sq_sdmmc_base);
	sq_sdmmc_write(SQ_SDMMC_MMU_PNRII, 0x000001ff, sq_sdmmc_base);

	return 0;
}

extern int 
sq_sdmmc_swdma_burst_type_single_test(int autotest)
{
	int ret = 0;

	sq_sdmmc_burst_type = SQ_DMA_BURST_SINGLE;
#if defined(CONFIG_CDK) || defined(CONFIG_PC9002) || defined(CONFIG_SQ8000) || defined(CONFIG_SCDK)
	{
	  extern struct test_item sq_sdmmc_xfer_width_test_items[];
	  
	  sq_sdmmc_xfer_width_test_items[0].enable = 0;
	  sq_sdmmc_xfer_width_test_items[1].enable = 0;
	}
#endif
	ret = test_item_ctrl(&sq_sdmmc_xfer_width_test_container, autotest);
#if defined(CONFIG_CDK) || defined(CONFIG_PC9002) || defined(CONFIG_SQ8000) || defined(CONFIG_SCDK)
	{
	  extern struct test_item sq_sdmmc_xfer_width_test_items[];
	  
	  sq_sdmmc_xfer_width_test_items[0].enable = 1;
	  sq_sdmmc_xfer_width_test_items[1].enable = 1;
	}
#endif
	return ret;
}


extern int 
sq_sdmmc_swdma_burst_type_incr4_test(int autotest)
{
	int ret = 0;

	sq_sdmmc_burst_type = SQ_DMA_BURST_INCR4;
#if defined(CONFIG_CDK) || defined(CONFIG_PC9002) || defined(CONFIG_SQ8000) || defined(CONFIG_SCDK)
	{
	  extern struct test_item sq_sdmmc_xfer_width_test_items[];
	  
	  sq_sdmmc_xfer_width_test_items[0].enable = 0;
	  sq_sdmmc_xfer_width_test_items[1].enable = 0;
	}
#endif
	ret = test_item_ctrl(&sq_sdmmc_xfer_width_test_container, autotest);
#if defined(CONFIG_CDK) || defined(CONFIG_PC9002) || defined(CONFIG_SQ8000) || defined(CONFIG_SCDK)
	{
	  extern struct test_item sq_sdmmc_xfer_width_test_items[];
	  
	  sq_sdmmc_xfer_width_test_items[0].enable = 1;
	  sq_sdmmc_xfer_width_test_items[1].enable = 1;
	}
#endif
	return ret;
}
extern int 
sq_sdmmc_swdma_burst_type_incr8_test(int autotest)
{
	int ret = 0;

	sq_sdmmc_burst_type = SQ_DMA_BURST_INCR8;
#if defined(CONFIG_CDK) || defined(CONFIG_PC9002) || defined(CONFIG_SQ8000) || defined(CONFIG_SCDK)
	{
	  extern struct test_item sq_sdmmc_xfer_width_test_items[];
	  
	  sq_sdmmc_xfer_width_test_items[0].enable = 0;
	  sq_sdmmc_xfer_width_test_items[1].enable = 0;
	}
#endif
	ret = test_item_ctrl(&sq_sdmmc_xfer_width_test_container, autotest);
#if defined(CONFIG_CDK) || defined(CONFIG_PC9002) || defined(CONFIG_SQ8000) || defined(CONFIG_SCDK)
	{
	  extern struct test_item sq_sdmmc_xfer_width_test_items[];
	  
	  sq_sdmmc_xfer_width_test_items[0].enable = 1;
	  sq_sdmmc_xfer_width_test_items[1].enable = 1;
	}
#endif
	return ret;
}

extern int 
sq_sdmmc_swdma_burst_type_incr16_test(int autotest)
{
	int ret = 0;

	sq_sdmmc_burst_type = SQ_DMA_BURST_INCR16;
#if defined(CONFIG_CDK) || defined(CONFIG_PC9002) || defined(CONFIG_SQ8000) || defined(CONFIG_SCDK)
	{
	  extern struct test_item sq_sdmmc_xfer_width_test_items[];
	  
	  sq_sdmmc_xfer_width_test_items[0].enable = 0;
	  sq_sdmmc_xfer_width_test_items[1].enable = 0;
	}
#endif
	ret = test_item_ctrl(&sq_sdmmc_xfer_width_test_container, autotest);
#if defined(CONFIG_CDK) || defined(CONFIG_PC9002) || defined(CONFIG_SQ8000) || defined(CONFIG_SCDK)
	{
	  extern struct test_item sq_sdmmc_xfer_width_test_items[];
	  
	  sq_sdmmc_xfer_width_test_items[0].enable = 1;
	  sq_sdmmc_xfer_width_test_items[1].enable = 1;
	}
#endif
	return ret;
}

extern int 
sq_sdmmc_sd_misc_command_9(int autotest)
{
	int err = 0;

	if (0 == sq_sdmmc_dev.alive_flag) {
		printf("\nSq SD/MMC host: card is not exist\n");
		return -1;
	}

	/* Mark card as media changed */
	sq_sdmmc_dev.media_changed = 1;

	/* Send CMD0 to go to Idle State */
	if (sq_sdmmc_send_cmd(&sq_sdmmc_dev, MMC_GO_IDLE_STATE, 0)) {
		printf("\nSq SD/MMC host: cannot go to \"Idle State\", please re-insert the card\n");
		return -1;
	}

	if (sq_sdmmc_dev.media_changed) {
		if (sq_sdmmc_card_initialize(&sq_sdmmc_dev, 1)) {
			printf("\nSq SD/MMC host: card initialization is fail\n");
			return -1;
		}
	}
	if (sq_sdmmc_dev.card_type != SD_CARD) {
		printf("\nSq SD/MMC host: this card is not a SD card\n");
		return -1;
	}
	err = sq_sdmmc_send_cmd(&sq_sdmmc_dev, MMC_SEND_CSD, sq_sdmmc_dev.card_rca);
	if (err)
		goto out;
	sq_sdmmc_dev.card_res_3 = sq_sdmmc_read(SQ_SDMMC_SD_RESA3, sq_sdmmc_base);
	sq_sdmmc_dev.card_res_2 = sq_sdmmc_read(SQ_SDMMC_SD_RESA2, sq_sdmmc_base);
	sq_sdmmc_dev.card_res_1 = sq_sdmmc_read(SQ_SDMMC_SD_RESA1, sq_sdmmc_base);
	sq_sdmmc_dev.card_res_0 = sq_sdmmc_read(SQ_SDMMC_SD_RESA0, sq_sdmmc_base);
	printf("\nSq SD/MMC host: resa_3: 0x%08x\n", sq_sdmmc_dev.card_res_3);
	printf("Sq SD/MMC host: resa_2: 0x%08x\n", sq_sdmmc_dev.card_res_2);
	printf("Sq SD/MMC host: resa_1: 0x%08x\n", sq_sdmmc_dev.card_res_1);
	printf("Sq SD/MMC host: resa_0: 0x%08x\n", sq_sdmmc_dev.card_res_0);

out:
	/* Mark card as media changed */
	sq_sdmmc_dev.media_changed = 1;

	/* Send CMD0 to go to Idle State */
	if (sq_sdmmc_send_cmd(&sq_sdmmc_dev, MMC_GO_IDLE_STATE, 0)) {
		printf("\nSq SD/MMC host: cannot go to \"Idle State\", please re-insert the card\n");
		return -1;
	}

	return err;
}

extern int 
sq_sdmmc_sd_misc_command_10(int autotest)
{
	int err = 0;

	if (0 == sq_sdmmc_dev.alive_flag) {
		printf("\nSq SD/MMC host: card is not exist\n");
		return -1;
	}

	/* Mark card as media changed */
	sq_sdmmc_dev.media_changed = 1;

	/* Send CMD0 to go to Idle State */
	if (sq_sdmmc_send_cmd(&sq_sdmmc_dev, MMC_GO_IDLE_STATE, 0)) {
		printf("\nSq SD/MMC host: cannot go to \"Idle State\", please re-insert the card\n");
		return -1;
	}

	if (sq_sdmmc_dev.media_changed) {
		if (sq_sdmmc_card_initialize(&sq_sdmmc_dev, 1)) {
			printf("\nSq SD/MMC host: card initialization is fail\n");
			return -1;
		}
	}
	if (sq_sdmmc_dev.card_type != SD_CARD) {
		printf("\nSq SD/MMC host: this card is not a SD card\n");
		return -1;
	}
	err = sq_sdmmc_send_cmd(&sq_sdmmc_dev, MMC_SEND_CID, sq_sdmmc_dev.card_rca);
	if (err)
		goto out;
	sq_sdmmc_dev.card_res_3 = sq_sdmmc_read(SQ_SDMMC_SD_RESA3, sq_sdmmc_base);
	sq_sdmmc_dev.card_res_2 = sq_sdmmc_read(SQ_SDMMC_SD_RESA2, sq_sdmmc_base);
	sq_sdmmc_dev.card_res_1 = sq_sdmmc_read(SQ_SDMMC_SD_RESA1, sq_sdmmc_base);
	sq_sdmmc_dev.card_res_0 = sq_sdmmc_read(SQ_SDMMC_SD_RESA0, sq_sdmmc_base);
	printf("\nSq SD/MMC host: resa_3: 0x%08x\n", sq_sdmmc_dev.card_res_3);
	printf("Sq SD/MMC host: resa_2: 0x%08x\n", sq_sdmmc_dev.card_res_2);
	printf("Sq SD/MMC host: resa_1: 0x%08x\n", sq_sdmmc_dev.card_res_1);
	printf("Sq SD/MMC host: resa_0: 0x%08x\n", sq_sdmmc_dev.card_res_0);
out:
	/* Mark card as media changed */
	sq_sdmmc_dev.media_changed = 1;

	/* Send CMD0 to go to Idle State */
	if (sq_sdmmc_send_cmd(&sq_sdmmc_dev, MMC_GO_IDLE_STATE, 0)) {
		printf("\nSq SD/MMC host: cannot go to \"Idle State\", please re-insert the card\n");
		return -1;
	}

	return err;
}

extern int sq_sdmmc_sd_misc_command_13(int autotest)
{
	int err = 0;

	if (0 == sq_sdmmc_dev.alive_flag) {
		printf("\nSq SD/MMC host: card is not exist\n");
		return -1;
	}

	/* Mark card as media changed */
	sq_sdmmc_dev.media_changed = 1;

	/* Send CMD0 to go to Idle State */
	if (sq_sdmmc_send_cmd(&sq_sdmmc_dev, MMC_GO_IDLE_STATE, 0)) {
		printf("\nSq SD/MMC host: cannot go to \"Idle State\", please re-insert the card\n");
		return -1;
	}

	if (sq_sdmmc_dev.media_changed) {
		if (sq_sdmmc_card_initialize(&sq_sdmmc_dev, 1)) {
			printf("\nSq SD/MMC host: card initialization is fail\n");
			return -1;
		}
	}
	if (sq_sdmmc_dev.card_type != SD_CARD) {
		printf("\nSq SD/MMC host: this card is not a SD card\n");
		return -1;
	}
	err = sq_sdmmc_send_cmd(&sq_sdmmc_dev, MMC_SEND_STATUS, sq_sdmmc_dev.card_rca);
	if (err)
		goto out;
	sq_sdmmc_dev.card_res_3 = sq_sdmmc_read(SQ_SDMMC_SD_RESA3, sq_sdmmc_base);
	sq_sdmmc_dev.card_res_2 = sq_sdmmc_read(SQ_SDMMC_SD_RESA2, sq_sdmmc_base);
	sq_sdmmc_dev.card_res_1 = sq_sdmmc_read(SQ_SDMMC_SD_RESA1, sq_sdmmc_base);
	sq_sdmmc_dev.card_res_0 = sq_sdmmc_read(SQ_SDMMC_SD_RESA0, sq_sdmmc_base);
	printf("\nSq SD/MMC host: resa_3: 0x%08x\n", sq_sdmmc_dev.card_res_3);
	printf("Sq SD/MMC host: resa_2: 0x%08x\n", sq_sdmmc_dev.card_res_2);
	printf("Sq SD/MMC host: resa_1: 0x%08x\n", sq_sdmmc_dev.card_res_1);
	printf("Sq SD/MMC host: resa_0: 0x%08x\n", sq_sdmmc_dev.card_res_0);
out:
	/* Mark card as media changed */
	sq_sdmmc_dev.media_changed = 1;

	/* Send CMD0 to go to Idle State */
	if (sq_sdmmc_send_cmd(&sq_sdmmc_dev, MMC_GO_IDLE_STATE, 0)) {
		printf("\nSq SD/MMC host: cannot  go to \"Idle State\", please re-insert the card\n");
		return -1;
	}

	return err;     
}

extern int sq_sdmmc_sd_misc_command_15(int autotest)
{
	if (0 == sq_sdmmc_dev.alive_flag) {
		printf("\nSq SD/MMC host: card is not exist\n");
		return -1;
	}

	/* Mark card as media changed */
	sq_sdmmc_dev.media_changed = 1;

	/* Send CMD0 to go to Idle State */
	if (sq_sdmmc_send_cmd(&sq_sdmmc_dev, MMC_GO_IDLE_STATE, 0)) {
		printf("\nSq SD/MMC host: cannot go to \"Idle State\", please re-insert the card\n");
		return -1;
	}

	if (sq_sdmmc_dev.media_changed) {
		if (sq_sdmmc_card_initialize(&sq_sdmmc_dev, 1)) {
			printf("\nSq SD/MMC host: card initialization is fail\n");
			return -1;
		}
	}
	if (sq_sdmmc_dev.card_type != SD_CARD) {
		printf("\nSq SD/MMC host: this card is not a SD card\n");
		return -1;
	}
	if (sq_sdmmc_send_cmd(&sq_sdmmc_dev, MMC_GO_INACTIVE_STATE, sq_sdmmc_dev.card_rca))
		return -1;

	/* Mark card as media changed */
	sq_sdmmc_dev.media_changed = 1;

	printf("\nSq SD/MMC host: card has gone to inactive state, please re-insert it again\n");
	return 0;     
}

extern int sq_sdmmc_sd_misc_app_command_51(int autotest)
{
	int err = 0;

	if (0 == sq_sdmmc_dev.alive_flag) {
		printf("\nSq SD/MMC host: card is not exist\n");
		return -1;
	}

	/* Mark card as media changed */
	sq_sdmmc_dev.media_changed = 1;

	/* Send CMD0 to go to Idle State */
	if (sq_sdmmc_send_cmd(&sq_sdmmc_dev, MMC_GO_IDLE_STATE, 0)) {
		printf("\nSq SD/MMC host: cannot go to \"Idle State\", please re-insert the card\n");
		return -1;
	}

	if (sq_sdmmc_dev.media_changed) {
		if (sq_sdmmc_card_initialize(&sq_sdmmc_dev, 0)) {
			printf("\nSq SD/MMC host: card initialization is fail\n");
			return -1;
		}
	}
	if (sq_sdmmc_dev.card_type != SD_CARD) {
		printf("\nSq SD/MMC host: this card is not a SD card\n");
		return -1;
	}

	err = sq_sdmmc_send_cmd(&sq_sdmmc_dev, MMC_APP_CMD, sq_sdmmc_dev.card_rca);
	if (err)
		goto out;
	err = sq_sdmmc_send_cmd(&sq_sdmmc_dev, SD_APP_SEND_SCR, 0);
	if (err)
		goto out;
	sq_sdmmc_dev.card_res_3 = sq_sdmmc_read(SQ_SDMMC_SD_RESA3, sq_sdmmc_base);
	sq_sdmmc_dev.card_res_2 = sq_sdmmc_read(SQ_SDMMC_SD_RESA2, sq_sdmmc_base);
	sq_sdmmc_dev.card_res_1 = sq_sdmmc_read(SQ_SDMMC_SD_RESA1, sq_sdmmc_base);
	sq_sdmmc_dev.card_res_0 = sq_sdmmc_read(SQ_SDMMC_SD_RESA0, sq_sdmmc_base);
	printf("\nSq SD/MMC host: resa_3: 0x%08x\n", sq_sdmmc_dev.card_res_3);
	printf("Sq SD/MMC host: resa_2: 0x%08x\n", sq_sdmmc_dev.card_res_2);
	printf("Sq SD/MMC host: resa_1: 0x%08x\n", sq_sdmmc_dev.card_res_1);
	printf("Sq SD/MMC host: resa_0: 0x%08x\n", sq_sdmmc_dev.card_res_0);
out:
	/* Mark card as media changed */
	sq_sdmmc_dev.media_changed = 1;

	/* Send CMD0 to go to Idle State */
	if (sq_sdmmc_send_cmd(&sq_sdmmc_dev, MMC_GO_IDLE_STATE, 0)) {
		printf("\nSq SD/MMC host: cannot go to \"Idle State\", please re-insert the card\n");
		return -1;
	}

	return err;
}

extern int 
sq_sdmmc_transfer_width_byte(int autotest)
{
	int ret = 0;

	sq_sdmmc_dev.mmu_setting = SQ_SDMMC_MMU_DATA_WIDTH_BYTE;
	sq_sdmmc_dev.pattern_width = PATTERN_WIDTH_BYTE;
	ret = sq_sdmmc_transfer(autotest);
	return ret;
}

extern int sq_sdmmc_transfer_width_halfword(int autotest)
{
	int ret = 0;

	sq_sdmmc_dev.mmu_setting = SQ_SDMMC_MMU_DATA_WIDTH_HALFWORD;
	sq_sdmmc_dev.pattern_width = PATTERN_WIDTH_HALFWORD;
	ret = sq_sdmmc_transfer(autotest);
	return ret;
}

extern int sq_sdmmc_transfer_width_word(int autotest)
{
	int ret  = 0;

	sq_sdmmc_dev.mmu_setting = SQ_SDMMC_MMU_DATA_WIDTH_WORD;
	sq_sdmmc_dev.pattern_width = PATTERN_WIDTH_WORD;
	ret = sq_sdmmc_transfer(autotest);
	return ret;
}

/*
 *  MMC
 *  */
extern int
sq_sdmmc_mmc_single_block_transfer_test(int autotest)
{
	int ret = 0;

	sq_sdmmc_transfer = sq_sdmmc_single_block_transfer;
	ret = test_item_ctrl(&sq_sdmmc_xfer_width_test_container, autotest);
	return ret;
}

extern int
sq_sdmmc_mmc_multiple_block_transfer_test(int autotest)
{
	int ret = 0;

	sq_sdmmc_transfer = sq_sdmmc_multiple_block_transfer;
	ret = test_item_ctrl(&sq_sdmmc_xfer_width_test_container, autotest);
	return ret;
}

extern int
sq_sdmmc_mmc_single_block_sw_dma_a2a_transfer_test(int autotest)
{
	int ret = 0;

	sq_sdmmc_transfer = sq_sdmmc_single_block_sw_dma_transfer;
	sq_sdmmc_dma_channel_num = SQ_A2A_CH_0;
	sq_request_dma(sq_sdmmc_dma_channel_num, &sq_sdmmc_dma_notifier);
	ret = test_item_ctrl(&sq_sdmmc_swdma_burst_type_test_container, autotest);
	sq_disable_dma(sq_sdmmc_dma_channel_num);
	sq_free_dma(sq_sdmmc_dma_channel_num);
	return ret;
}

extern int
sq_sdmmc_mmc_multiple_block_sw_dma_a2a_transfer_test(int autotest)
{
	int ret = 0;

	sq_sdmmc_transfer = sq_sdmmc_multiple_block_sw_dma_transfer;
	sq_sdmmc_dma_channel_num = SQ_A2A_CH_0;
	sq_request_dma(sq_sdmmc_dma_channel_num, &sq_sdmmc_dma_notifier);
	ret = test_item_ctrl(&sq_sdmmc_swdma_burst_type_test_container, autotest);
	sq_disable_dma(sq_sdmmc_dma_channel_num);
	sq_free_dma(sq_sdmmc_dma_channel_num);
	return ret;
}

extern int
sq_sdmmc_mmc_single_block_sw_dma_hdma_transfer_test(int autotest)
{
	int ret = 0;

	sq_sdmmc_transfer = sq_sdmmc_single_block_sw_dma_transfer;
	sq_sdmmc_dma_channel_num = SQ_HDMA_CH_0;
	sq_request_dma(sq_sdmmc_dma_channel_num, &sq_sdmmc_dma_notifier);
	ret = test_item_ctrl(&sq_sdmmc_swdma_burst_type_test_container, autotest);
	sq_disable_dma(sq_sdmmc_dma_channel_num);
	sq_free_dma(sq_sdmmc_dma_channel_num);
	return ret;
}

extern int
sq_sdmmc_mmc_multiple_block_sw_dma_hdma_transfer_test(int autotest)
{
	int ret = 0;

	sq_sdmmc_transfer = sq_sdmmc_multiple_block_sw_dma_transfer;
	sq_sdmmc_dma_channel_num = SQ_HDMA_CH_0;
	sq_request_dma(sq_sdmmc_dma_channel_num, &sq_sdmmc_dma_notifier);
	ret = test_item_ctrl(&sq_sdmmc_swdma_burst_type_test_container, autotest);
	sq_disable_dma(sq_sdmmc_dma_channel_num);
	sq_free_dma(sq_sdmmc_dma_channel_num);
	return ret;
}

extern int
sq_sdmmc_mmc_single_block_sw_dma_panther7_hdma_transfer_test(int autotest)
{
	int ret = 0;

	sq_sdmmc_transfer = sq_sdmmc_single_block_sw_dma_transfer;
	sq_sdmmc_dma_channel_num = PANTHER7_HDMA_CH_0;
	sq_request_dma(sq_sdmmc_dma_channel_num, &sq_sdmmc_dma_notifier);
	ret = test_item_ctrl(&sq_sdmmc_swdma_burst_type_test_container, autotest);
	sq_disable_dma(sq_sdmmc_dma_channel_num);
	sq_free_dma(sq_sdmmc_dma_channel_num);
	return ret;
}

extern int
sq_sdmmc_mmc_multiple_block_sw_dma_panther7_hdma_transfer_test(int autotest)
{
	int ret = 0;

	sq_sdmmc_transfer = sq_sdmmc_multiple_block_sw_dma_transfer;
	sq_sdmmc_dma_channel_num = PANTHER7_HDMA_CH_0;
	sq_request_dma(sq_sdmmc_dma_channel_num, &sq_sdmmc_dma_notifier);
	ret = test_item_ctrl(&sq_sdmmc_swdma_burst_type_test_container, autotest);
	sq_disable_dma(sq_sdmmc_dma_channel_num);
	sq_free_dma(sq_sdmmc_dma_channel_num);
	return ret;
}


static int sq_sdmmc_single_block_transfer(int autotest)
{
	u32 bus_width_def;
	u32 cur_addr_w = 0, cur_addr_r = 0;
	u32 block_num_w = 0, block_num_r = 0;
	u32 nsect_w = TEST_SECT_NUM, nsect_r = TEST_SECT_NUM;

	if (0 == sq_sdmmc_dev.alive_flag) {
		printf("\nSq SD/MMC host: card is not exist\n");
		return -1;
	}
	if (sq_sdmmc_dev.media_changed) {
		if (sq_sdmmc_card_initialize(&sq_sdmmc_dev, 0)) {
			printf("\nSq SD/MMC host: card initialization is fail\n");
			return -1;
		}
	}
	if (SQ_SDMMC_DATA_XFER_BUS_WIDTH_LINE_1 == sq_sdmmc_dev.bus_width)
		bus_width_def = SD_BUS_WIDTH_1;
	else if (SQ_SDMMC_DATA_XFER_BUS_WIDTH_LINE_4 == sq_sdmmc_dev.bus_width)
		bus_width_def = SD_BUS_WIDTH_4;
	else {
		if (sq_sdmmc_dev.bus_width != -1) {
			printf("\nSq SD/MMC host: undefined bus width\n");
			return -1;
		}
	}
	if (SD_CARD == sq_sdmmc_dev.card_type) {
		/* Send CMD55 to indicate that the next command is an application specific command */
		if (sq_sdmmc_send_cmd(&sq_sdmmc_dev, MMC_APP_CMD, sq_sdmmc_dev.card_rca))
			return -1;

		/* Send ACMD6 to select given bus width */
		if (sq_sdmmc_send_cmd(&sq_sdmmc_dev, SD_APP_SET_BUS_WIDTH, bus_width_def))
			return -1;
	}

	sq_sdmmc_write(SQ_SDMMC_MMU_CTRL, sq_sdmmc_dev.mmu_setting, sq_sdmmc_base);
	while (nsect_w > 0) {
		sq_sdmmc_fill_pattern(sq_sdmmc_dev.pattern_width, block_num_w);

		/* Send CMD24 to write single block */
		if (sq_sdmmc_send_cmd(&sq_sdmmc_dev, MMC_WRITE_BLOCK, cur_addr_w))
			return -1;
		sq_sdmmc_dev.card_state = RCV;

		sq_sdmmc_mmu_swap(sq_sdmmc_base);
		sq_sdmmc_write(SQ_SDMMC_SD_DATAT,
				  SQ_SDMMC_DATA_XFER_SIGNAL_BEGIN |
				  SQ_SDMMC_DATA_XFER_DIR_WRITE |
				  sq_sdmmc_dev.bus_width |
				  SQ_SDMMC_DATA_XFER_DMA_DIS |
				  SQ_SDMMC_DATA_XFER_CYC_SINGLE,
				  sq_sdmmc_base);

		/* Wait for the writing transmission to be complete */
		if (sq_sdmmc_data_transfer_end(&sq_sdmmc_dev))
			return -1;
		sq_sdmmc_dev.card_state = PRG;

		/* Wait for the card to depart from the busy state */
		sq_sdmmc_card_busy_end(sq_sdmmc_base);
		sq_sdmmc_dev.card_state = TRAN;

		cur_addr_w += 512;
		block_num_w++;
		nsect_w--;
		sq_sdmmc_mmu_swap(sq_sdmmc_base);
	}

	sq_sdmmc_write(SQ_SDMMC_MMU_CTRL, sq_sdmmc_dev.mmu_setting, sq_sdmmc_base);
	while (nsect_r > 0) {
		sq_sdmmc_write(SQ_SDMMC_SD_DATAT,
				  SQ_SDMMC_DATA_XFER_SIGNAL_BEGIN |
				  SQ_SDMMC_DATA_XFER_DIR_READ |
				  sq_sdmmc_dev.bus_width |
				  SQ_SDMMC_DATA_XFER_DMA_DIS |
				  SQ_SDMMC_DATA_XFER_CYC_SINGLE,
				  sq_sdmmc_base);

		/* Send CMD17 to read single block */
		if (sq_sdmmc_send_cmd(&sq_sdmmc_dev, MMC_READ_SINGLE_BLOCK, cur_addr_r))
			return -1;
		sq_sdmmc_dev.card_state = DATA;

		/* Wait for the reading transmission to be complete */
		if (sq_sdmmc_data_transfer_end(&sq_sdmmc_dev))
			return -1;
		sq_sdmmc_dev.card_state = TRAN;

		sq_sdmmc_mmu_swap(sq_sdmmc_base);
		if (sq_sdmmc_compare_pattern(sq_sdmmc_dev.pattern_width, block_num_r))
			return -1;

		cur_addr_r += 512;
		block_num_r++;
		nsect_r--;
	}

	return 0;
}

static int sq_sdmmc_multiple_block_transfer(int autotest)
{
	u32 bus_width_def;
	u32 cur_addr_w = 0, cur_addr_r = 0;
	u32 block_num_w = 0, block_num_r = 0;
	u32 nsect_w = TEST_SECT_NUM, nsect_r = TEST_SECT_NUM;

	if (0 == sq_sdmmc_dev.alive_flag) {
		printf("\nSq SD/MMC host: card is not exist\n");
		return -1;
	}
	if (sq_sdmmc_dev.media_changed) {
		if (sq_sdmmc_card_initialize(&sq_sdmmc_dev, 0)) {
			printf("\nSq SD/MMC host: card initialization is fail\n");
			return -1;
		}
	}
	if (SQ_SDMMC_DATA_XFER_BUS_WIDTH_LINE_1 == sq_sdmmc_dev.bus_width)
		bus_width_def = SD_BUS_WIDTH_1;
	else if (SQ_SDMMC_DATA_XFER_BUS_WIDTH_LINE_4 == sq_sdmmc_dev.bus_width)
		bus_width_def = SD_BUS_WIDTH_4;
	else {
		if (sq_sdmmc_dev.bus_width != -1) {
			printf("\nSq SD/MMC host: undefined bus width\n");
			return -1;
		}
	}
	if (SD_CARD == sq_sdmmc_dev.card_type) {
		/* Send CMD55 to indicate that the next command is an application specific command */
		if (sq_sdmmc_send_cmd(&sq_sdmmc_dev, MMC_APP_CMD, sq_sdmmc_dev.card_rca))
			return -1;

		/* Send ACMD6 to select given bus width */
		if (sq_sdmmc_send_cmd(&sq_sdmmc_dev, SD_APP_SET_BUS_WIDTH, bus_width_def))
			return -1;
	}

	/*
	 *  Write data into SD/MMC card
	 *  */
	sq_sdmmc_write(SQ_SDMMC_MMU_CTRL, sq_sdmmc_dev.mmu_setting, sq_sdmmc_base);
	sq_sdmmc_fill_pattern(sq_sdmmc_dev.pattern_width, block_num_w++);

	/* Send CMD25 to write multiple block */
	if (sq_sdmmc_send_cmd(&sq_sdmmc_dev, MMC_WRITE_MULTIPLE_BLOCK, cur_addr_w))
		return -1;
	sq_sdmmc_dev.card_state = RCV;

	while (nsect_w > 0) {
		sq_sdmmc_mmu_swap(sq_sdmmc_base);
		if (1 == nsect_w)	/* last block to write */
			sq_sdmmc_write(SQ_SDMMC_SD_DATAT,
					  SQ_SDMMC_DATA_XFER_SIGNAL_BEGIN |
					  SQ_SDMMC_DATA_XFER_DIR_WRITE |
					  sq_sdmmc_dev.bus_width |
					  SQ_SDMMC_DATA_XFER_DMA_DIS |
					  SQ_SDMMC_DATA_XFER_CYC_SINGLE,
					  sq_sdmmc_base);
		else
			sq_sdmmc_write(SQ_SDMMC_SD_DATAT,
					  SQ_SDMMC_DATA_XFER_SIGNAL_BEGIN |
					  SQ_SDMMC_DATA_XFER_DIR_WRITE |
					  sq_sdmmc_dev.bus_width |
					  SQ_SDMMC_DATA_XFER_DMA_DIS |
					  SQ_SDMMC_DATA_XFER_CYC_MULTIPLE,
					  sq_sdmmc_base);

		/* Last tranmission */
		if (1 == nsect_w)
			goto data_transfer;
		else
			sq_sdmmc_fill_pattern(sq_sdmmc_dev.pattern_width, block_num_w++);

	data_transfer:
		/* Wait for the writing transmission to be complete */
		if (sq_sdmmc_data_transfer_end(&sq_sdmmc_dev))
			return -1;

		nsect_w--;
	}

	/* Send CMD12 to stop the transimission */
	if (sq_sdmmc_send_cmd(&sq_sdmmc_dev, MMC_STOP_TRANSMISSION, 0x00000000))
		return -1;
	sq_sdmmc_dev.card_state = PRG;

	/* Wait for the card to depart from the busy state */
	sq_sdmmc_card_busy_end(sq_sdmmc_base);
	sq_sdmmc_dev.card_state = TRAN;

	/*
	 *  Read data from the SD/MMC card
	 *  */
	sq_sdmmc_write(SQ_SDMMC_MMU_CTRL, sq_sdmmc_dev.mmu_setting, sq_sdmmc_base);
	sq_sdmmc_write(SQ_SDMMC_SD_DATAT,
			  SQ_SDMMC_DATA_XFER_SIGNAL_BEGIN |
			  SQ_SDMMC_DATA_XFER_DIR_READ |
			  sq_sdmmc_dev.bus_width |
			  SQ_SDMMC_DATA_XFER_DMA_DIS |
			  SQ_SDMMC_DATA_XFER_CYC_MULTIPLE,
			  sq_sdmmc_base);

	/* Send CMD18 to read multiple block */
	if (sq_sdmmc_send_cmd(&sq_sdmmc_dev, MMC_READ_MULTIPLE_BLOCK, cur_addr_r))
		return -1;
	sq_sdmmc_dev.card_state = DATA;

	while (nsect_r > 0) {
		/* Wait for the reading transmission to be complete */
		if (sq_sdmmc_data_transfer_end(&sq_sdmmc_dev))
			return -1;

		sq_sdmmc_mmu_swap(sq_sdmmc_base);

		/* Last transmission */
		if (1 == nsect_r) {
			/* Send CMD12 to stop the transimission */
			if (sq_sdmmc_send_cmd(&sq_sdmmc_dev, MMC_STOP_TRANSMISSION, 0x00000000))
				return -1;
			sq_sdmmc_dev.card_state = TRAN;
			goto data_transfer_end;
		}

		if (2 == nsect_r)	/* last block to read */
			sq_sdmmc_write(SQ_SDMMC_SD_DATAT,
					  SQ_SDMMC_DATA_XFER_SIGNAL_BEGIN |
					  SQ_SDMMC_DATA_XFER_DIR_READ |
					  sq_sdmmc_dev.bus_width |
					  SQ_SDMMC_DATA_XFER_DMA_DIS |
					  SQ_SDMMC_DATA_XFER_CYC_SINGLE,
					  sq_sdmmc_base);
		else
			sq_sdmmc_write(SQ_SDMMC_SD_DATAT,
					  SQ_SDMMC_DATA_XFER_SIGNAL_BEGIN |
					  SQ_SDMMC_DATA_XFER_DIR_READ |
					  sq_sdmmc_dev.bus_width |
					  SQ_SDMMC_DATA_XFER_DMA_DIS |
					  SQ_SDMMC_DATA_XFER_CYC_MULTIPLE,
					  sq_sdmmc_base);
	data_transfer_end:
		//20080401 cyli fix for calling error function (change from sq_sdmmc_fill_pattern() to sq_sdmmc_compare_pattern())
		if (sq_sdmmc_compare_pattern(sq_sdmmc_dev.pattern_width, block_num_r++))
			return -1;

		nsect_r--;
	}

	return 0;
}

static void 
sq_sdmmc_dma_complete(void *data)
{
	sq_sdmmc_sw_dma_complete_flag = 1;
}

static int 
sq_sdmmc_single_block_sw_dma_transfer(int autotest)
{
	u32 cur_addr_w = 0; /* current written address */
	u32 cur_addr_r = 0; /* current read address */
	u32 bus_width_def;
	u32 block_num = 0;
	u32 nsect_w = TEST_SECT_NUM, nsect_r = TEST_SECT_NUM;
	u8 *mem_pattern_addr = (u8 *)TEST_MEM_PATTERN_ADDR;
	u8 *mem_cmpr_addr = (u8 *)TEST_MEM_CMPR_ADDR;
	int err = 0;
     
	if (0 == sq_sdmmc_dev.alive_flag) {
		printf("\nSq SD/MMC host: card is not exist\n");
		return -1;
	}
	if (sq_sdmmc_dev.media_changed) {
		if (sq_sdmmc_card_initialize(&sq_sdmmc_dev, 0)) {
			printf("\nSq SD/MMC host: card initialization is fail\n");
			return -1;
		}
	}
	if (SQ_SDMMC_DATA_XFER_BUS_WIDTH_LINE_1 == sq_sdmmc_dev.bus_width)
		bus_width_def = SD_BUS_WIDTH_1;
	else if (SQ_SDMMC_DATA_XFER_BUS_WIDTH_LINE_4 == sq_sdmmc_dev.bus_width)
		bus_width_def = SD_BUS_WIDTH_4;
	else {
		if (sq_sdmmc_dev.bus_width != -1) {
			printf("\nSq SD/MMC host: undefined bus width\n");
			return -1;
		}
	}
	if (SD_CARD == sq_sdmmc_dev.card_type) {
		/* Send CMD55 to indicate that the next command is an application specific command */
		if (sq_sdmmc_send_cmd(&sq_sdmmc_dev, MMC_APP_CMD, sq_sdmmc_dev.card_rca))
			return -1;

		/* Send ACMD6 to select given bus width */
		if (sq_sdmmc_send_cmd(&sq_sdmmc_dev, SD_APP_SET_BUS_WIDTH, bus_width_def))
			return -1;
	}

	switch (sq_sdmmc_dev.mmu_setting) {
	case SQ_SDMMC_MMU_DATA_WIDTH_BYTE:
		sq_sdmmc_data_size = SQ_DMA_DATA_BYTE;
		sq_sdmmc_dev.pattern_width = PATTERN_WIDTH_BYTE;
		break;
	case SQ_SDMMC_MMU_DATA_WIDTH_HALFWORD:
		sq_sdmmc_data_size = SQ_DMA_DATA_HALFWORD;
		sq_sdmmc_dev.pattern_width = PATTERN_WIDTH_HALFWORD;
		break;
	case SQ_SDMMC_MMU_DATA_WIDTH_WORD:
		sq_sdmmc_data_size = SQ_DMA_DATA_WORD;
		sq_sdmmc_dev.pattern_width = PATTERN_WIDTH_WORD;
		break;
	default:
		printf("\nSq SD/MMC host: unknown data size\n");
		return -1;
	}

	memset(mem_pattern_addr, 0x0, TEST_SECT_NUM*512);
	memset(mem_cmpr_addr, 0x0, TEST_SECT_NUM*512);

	/*
	 *  Write data into SD/MMC card
	 *  */
	sq_sdmmc_write(SQ_SDMMC_MMU_CTRL, sq_sdmmc_dev.mmu_setting, sq_sdmmc_base); 
	while (nsect_w > 0) {
		/* Fill the test pattern into memory */
		sq_sdmmc_make_test_pattern_dma(mem_pattern_addr, sq_sdmmc_dev.pattern_width, block_num++);
	  
		/* Set dma */
		sq_disable_dma(sq_sdmmc_dma_channel_num);
		sq_set_dma_mode(sq_sdmmc_dma_channel_num, SQ_DMA_MODE_SW);
		sq_set_dma_source_direction(sq_sdmmc_dma_channel_num, SQ_DMA_DIR_INCR);
		sq_set_dma_destination_direction(sq_sdmmc_dma_channel_num, SQ_DMA_DIR_FIXED);
		sq_set_dma_source_address(sq_sdmmc_dma_channel_num, (u32)mem_pattern_addr);
		sq_set_dma_destination_address(sq_sdmmc_dma_channel_num, SQ_SDMMC_MMU_DATA+sq_sdmmc_base);
		sq_set_dma_burst_type(sq_sdmmc_dma_channel_num, sq_sdmmc_burst_type);
		sq_set_dma_data_size(sq_sdmmc_dma_channel_num, sq_sdmmc_data_size);
		sq_set_dma_transfer_count(sq_sdmmc_dma_channel_num, 512);
		sq_sdmmc_sw_dma_complete_flag = 0;
		sq_enable_dma(sq_sdmmc_dma_channel_num);

		mem_pattern_addr += 512;

		/* Wait for dma to be complete */
		sq_sdmmc_to_cnt = 0;
		while (!sq_sdmmc_sw_dma_complete_flag) {
			if (sq_sdmmc_to_cnt > TO_THRESHOLD_CNT) {
				printf("\nSq SD/MMC host: software dma transmission is timeout\n");
				return -1;
			}
			sq_sdmmc_to_cnt++;
		}

		/* Send CMD24 to write single block */
		if (sq_sdmmc_send_cmd(&sq_sdmmc_dev, MMC_WRITE_BLOCK, cur_addr_w))
			return -1;
		sq_sdmmc_dev.card_state = RCV;

		sq_sdmmc_mmu_swap(sq_sdmmc_base);
		sq_sdmmc_write(SQ_SDMMC_SD_DATAT,
				  SQ_SDMMC_DATA_XFER_SIGNAL_BEGIN |
				  SQ_SDMMC_DATA_XFER_DIR_WRITE |
				  sq_sdmmc_dev.bus_width |
				  SQ_SDMMC_DATA_XFER_DMA_DIS |
				  SQ_SDMMC_DATA_XFER_CYC_SINGLE,
				  sq_sdmmc_base);

		/* Wait for the writing transmission to be complete */
		if (sq_sdmmc_data_transfer_end(&sq_sdmmc_dev))
			return -1;
		sq_sdmmc_dev.card_state = PRG;

		/* Wait for the card to depart from the busy state */
		sq_sdmmc_card_busy_end(sq_sdmmc_base);
		sq_sdmmc_dev.card_state = TRAN;

		cur_addr_w += 512;
		nsect_w--;
	}


	/*
	 *  Read data from the SD/MMC card
	 *  */

	/* Restore the memory address of pattern */
	mem_pattern_addr = (u8 *)TEST_MEM_PATTERN_ADDR;

	sq_sdmmc_write(SQ_SDMMC_MMU_CTRL, sq_sdmmc_dev.mmu_setting, sq_sdmmc_base);
	while (nsect_r > 0) {
		sq_sdmmc_write(SQ_SDMMC_SD_DATAT,
				  SQ_SDMMC_DATA_XFER_SIGNAL_BEGIN |
				  SQ_SDMMC_DATA_XFER_DIR_READ |
				  sq_sdmmc_dev.bus_width |
				  SQ_SDMMC_DATA_XFER_DMA_DIS |
				  SQ_SDMMC_DATA_XFER_CYC_SINGLE,
				  sq_sdmmc_base);

		/* Send CMD17 to read single block */
		if (sq_sdmmc_send_cmd(&sq_sdmmc_dev, MMC_READ_SINGLE_BLOCK, cur_addr_r))
			return -1;
		sq_sdmmc_dev.card_state = DATA;

		/* Wait for the reading transmission to be complete */
		if (sq_sdmmc_data_transfer_end(&sq_sdmmc_dev))
			return -1;
		sq_sdmmc_dev.card_state = TRAN;

		sq_sdmmc_mmu_swap(sq_sdmmc_base);
		
		/* Set dma */
		sq_disable_dma(sq_sdmmc_dma_channel_num);
		sq_set_dma_mode(sq_sdmmc_dma_channel_num, SQ_DMA_MODE_SW);
		sq_set_dma_source_direction(sq_sdmmc_dma_channel_num, SQ_DMA_DIR_FIXED);
		sq_set_dma_destination_direction(sq_sdmmc_dma_channel_num, SQ_DMA_DIR_INCR);
		sq_set_dma_source_address(sq_sdmmc_dma_channel_num, SQ_SDMMC_MMU_DATA+sq_sdmmc_base);
		sq_set_dma_destination_address(sq_sdmmc_dma_channel_num, (u32)mem_cmpr_addr);
		sq_set_dma_burst_type(sq_sdmmc_dma_channel_num, sq_sdmmc_burst_type);
		sq_set_dma_data_size(sq_sdmmc_dma_channel_num, sq_sdmmc_data_size);
		sq_set_dma_transfer_count(sq_sdmmc_dma_channel_num, 512);
		sq_sdmmc_sw_dma_complete_flag = 0;
		sq_enable_dma(sq_sdmmc_dma_channel_num);
	
		/* Wait for dma to be complete */
		sq_sdmmc_to_cnt = 0;
		while (!sq_sdmmc_sw_dma_complete_flag) {
			if (sq_sdmmc_to_cnt > TO_THRESHOLD_CNT) {
				printf("\nSq SD/MMC host: software dma transmission is timeout\n");
				return -1;
			}
			sq_sdmmc_to_cnt++;
		}

		err |= sq_sdmmc_compare_memory(mem_pattern_addr, mem_cmpr_addr, 512,
						  autotest);
		mem_cmpr_addr += 512;
		mem_pattern_addr += 512;
		cur_addr_r += 512;
		nsect_r--;
	}

	return err;
}

static int 
sq_sdmmc_multiple_block_sw_dma_transfer(int autotest)
{
	u32 bus_width_def;
	u32 block_num = 0;
	u32 nsect_w = TEST_SECT_NUM, nsect_r = TEST_SECT_NUM;
	u8 *mem_pattern_addr = (u8 *)TEST_MEM_PATTERN_ADDR;
	u8 *mem_cmpr_addr = (u8 *)TEST_MEM_CMPR_ADDR;
	int err = 0;
     
	if (0 == sq_sdmmc_dev.alive_flag) {
		printf("\nSq SD/MMC host: card is not exist\n");
		return -1;
	}
	if (sq_sdmmc_dev.media_changed) {
		if (sq_sdmmc_card_initialize(&sq_sdmmc_dev, 0)) {
			printf("\nSq SD/MMC host: card initialization is fail\n");
			return -1;
		}
	}
	if (SQ_SDMMC_DATA_XFER_BUS_WIDTH_LINE_1 == sq_sdmmc_dev.bus_width)
		bus_width_def = SD_BUS_WIDTH_1;
	else if (SQ_SDMMC_DATA_XFER_BUS_WIDTH_LINE_4 == sq_sdmmc_dev.bus_width)
		bus_width_def = SD_BUS_WIDTH_4;
	else {
		if (sq_sdmmc_dev.bus_width != -1) {
			printf("\nSq SD/MMC host: undefined bus width\n");
			return -1;
		}
	}
	if (SD_CARD == sq_sdmmc_dev.card_type) {
		/* Send CMD55 to indicate that the next command is an application specific command */
		if (sq_sdmmc_send_cmd(&sq_sdmmc_dev, MMC_APP_CMD, sq_sdmmc_dev.card_rca))
			return -1;

		/* Send ACMD6 to select given bus width */
		if (sq_sdmmc_send_cmd(&sq_sdmmc_dev, SD_APP_SET_BUS_WIDTH, bus_width_def))
			return -1;
	}

	switch (sq_sdmmc_dev.mmu_setting) {
	case SQ_SDMMC_MMU_DATA_WIDTH_BYTE:
		sq_sdmmc_data_size = SQ_DMA_DATA_BYTE;
		sq_sdmmc_dev.pattern_width = PATTERN_WIDTH_BYTE;
		break;
	case SQ_SDMMC_MMU_DATA_WIDTH_HALFWORD:
		sq_sdmmc_data_size = SQ_DMA_DATA_HALFWORD;
		sq_sdmmc_dev.pattern_width = PATTERN_WIDTH_HALFWORD;
		break;
	case SQ_SDMMC_MMU_DATA_WIDTH_WORD:
		sq_sdmmc_data_size = SQ_DMA_DATA_WORD;
		sq_sdmmc_dev.pattern_width = PATTERN_WIDTH_WORD;
		break;
	default:
		printf("\nSq SD/MMC host: unknown data size\n");
		return -1;
	}

	memset(mem_pattern_addr, 0x0, TEST_SECT_NUM*512);
	memset(mem_cmpr_addr, 0x0, TEST_SECT_NUM*512);


	/*
	 *  Write data into SD/MMC card
	 *  */
	sq_sdmmc_write(SQ_SDMMC_MMU_CTRL, sq_sdmmc_dev.mmu_setting, sq_sdmmc_base);

	/* Fill the test pattern into memory */
	sq_sdmmc_make_test_pattern_dma(mem_pattern_addr, sq_sdmmc_dev.pattern_width, block_num++);


	/* Set dma */
	sq_disable_dma(sq_sdmmc_dma_channel_num);
	sq_set_dma_mode(sq_sdmmc_dma_channel_num, SQ_DMA_MODE_SW);
	sq_set_dma_source_direction(sq_sdmmc_dma_channel_num, SQ_DMA_DIR_INCR);
	sq_set_dma_destination_direction(sq_sdmmc_dma_channel_num, SQ_DMA_DIR_FIXED);
	sq_set_dma_source_address(sq_sdmmc_dma_channel_num, (u32)mem_pattern_addr);
	sq_set_dma_destination_address(sq_sdmmc_dma_channel_num, SQ_SDMMC_MMU_DATA+sq_sdmmc_base);
	sq_set_dma_burst_type(sq_sdmmc_dma_channel_num, sq_sdmmc_burst_type);
	sq_set_dma_data_size(sq_sdmmc_dma_channel_num, sq_sdmmc_data_size);
	sq_set_dma_transfer_count(sq_sdmmc_dma_channel_num, 512);
	sq_sdmmc_sw_dma_complete_flag = 0;
	sq_enable_dma(sq_sdmmc_dma_channel_num);

	/* Send CMD25 to write multiple block */
	if (sq_sdmmc_send_cmd(&sq_sdmmc_dev, MMC_WRITE_MULTIPLE_BLOCK, 0))
		return -1;
	sq_sdmmc_dev.card_state = RCV;

	while (nsect_w > 0) {
		/* Wait for dma to be complete */
		sq_sdmmc_to_cnt = 0;
		while (!sq_sdmmc_sw_dma_complete_flag) {
			if (sq_sdmmc_to_cnt > TO_THRESHOLD_CNT) {
				printf("\nSq SD/MMC host: software dma transmission is timeout\n");
				return -1;
			}
			sq_sdmmc_to_cnt++;
		}

		sq_sdmmc_mmu_swap(sq_sdmmc_base);

		if (1 == nsect_w)	/* last block to write */
			sq_sdmmc_write(SQ_SDMMC_SD_DATAT,
					  SQ_SDMMC_DATA_XFER_SIGNAL_BEGIN |
					  SQ_SDMMC_DATA_XFER_DIR_WRITE |
					  sq_sdmmc_dev.bus_width |
					  SQ_SDMMC_DATA_XFER_DMA_DIS |
					  SQ_SDMMC_DATA_XFER_CYC_SINGLE,
					  sq_sdmmc_base);
		else
			sq_sdmmc_write(SQ_SDMMC_SD_DATAT,
					  SQ_SDMMC_DATA_XFER_SIGNAL_BEGIN |
					  SQ_SDMMC_DATA_XFER_DIR_WRITE |
					  sq_sdmmc_dev.bus_width |
					  SQ_SDMMC_DATA_XFER_DMA_DIS |
					  SQ_SDMMC_DATA_XFER_CYC_MULTIPLE,
					  sq_sdmmc_base);

		/* Last tranmission */
		if (1 == nsect_w)
			goto data_transfer;
		else {
			mem_pattern_addr += 512;
			sq_sdmmc_make_test_pattern_dma(mem_pattern_addr, sq_sdmmc_dev.pattern_width, block_num++);

			/* Set dma */
			sq_disable_dma(sq_sdmmc_dma_channel_num);
			sq_set_dma_mode(sq_sdmmc_dma_channel_num, SQ_DMA_MODE_SW);
			sq_set_dma_source_direction(sq_sdmmc_dma_channel_num, SQ_DMA_DIR_INCR);
			sq_set_dma_destination_direction(sq_sdmmc_dma_channel_num, SQ_DMA_DIR_FIXED);
			sq_set_dma_source_address(sq_sdmmc_dma_channel_num, (u32)mem_pattern_addr);
			sq_set_dma_destination_address(sq_sdmmc_dma_channel_num, SQ_SDMMC_MMU_DATA+sq_sdmmc_base);
			sq_set_dma_burst_type(sq_sdmmc_dma_channel_num, sq_sdmmc_burst_type);
			sq_set_dma_data_size(sq_sdmmc_dma_channel_num, sq_sdmmc_data_size);
			sq_set_dma_transfer_count(sq_sdmmc_dma_channel_num, 512);
			sq_sdmmc_sw_dma_complete_flag = 0;
			sq_enable_dma(sq_sdmmc_dma_channel_num);
		}

	data_transfer:
		/* Wait for the writing transmission to be complete */
		if (sq_sdmmc_data_transfer_end(&sq_sdmmc_dev))
			return -1;

		nsect_w--;
	}

	/* Send CMD12 to stop the transimission */
	if (sq_sdmmc_send_cmd(&sq_sdmmc_dev, MMC_STOP_TRANSMISSION, 0x00000000))
		return -1;
	sq_sdmmc_dev.card_state = PRG;

	/* Wait for the card to depart from the busy state */
	sq_sdmmc_card_busy_end(sq_sdmmc_base);
	sq_sdmmc_dev.card_state = TRAN;


	/*
	 *  Read data from the SD/MMC card
	 *  */
	sq_sdmmc_write(SQ_SDMMC_MMU_CTRL, sq_sdmmc_dev.mmu_setting, sq_sdmmc_base);
	sq_sdmmc_write(SQ_SDMMC_SD_DATAT,
			  SQ_SDMMC_DATA_XFER_SIGNAL_BEGIN |
			  SQ_SDMMC_DATA_XFER_DIR_READ |
			  sq_sdmmc_dev.bus_width |
			  SQ_SDMMC_DATA_XFER_DMA_DIS |
			  SQ_SDMMC_DATA_XFER_CYC_MULTIPLE,
			  sq_sdmmc_base);

	/* Rset the memory address of pattern */
	mem_pattern_addr = (u8 *)TEST_MEM_PATTERN_ADDR;

	/* Send CMD18 to read multiple block */
	if (sq_sdmmc_send_cmd(&sq_sdmmc_dev, MMC_READ_MULTIPLE_BLOCK, 0))
		return -1;
	sq_sdmmc_dev.card_state = DATA;

	while (nsect_r > 0) {
		/* Wait for the reading transmission to be complete */
		if (sq_sdmmc_data_transfer_end(&sq_sdmmc_dev))
			return -1;

		sq_sdmmc_mmu_swap(sq_sdmmc_base);

		/* Set dma */
		sq_disable_dma(sq_sdmmc_dma_channel_num);
		sq_set_dma_mode(sq_sdmmc_dma_channel_num, SQ_DMA_MODE_SW);
		sq_set_dma_source_direction(sq_sdmmc_dma_channel_num, SQ_DMA_DIR_FIXED);
		sq_set_dma_destination_direction(sq_sdmmc_dma_channel_num, SQ_DMA_DIR_INCR);
		sq_set_dma_source_address(sq_sdmmc_dma_channel_num, SQ_SDMMC_MMU_DATA+sq_sdmmc_base);
		sq_set_dma_destination_address(sq_sdmmc_dma_channel_num, (u32)mem_cmpr_addr);
		sq_set_dma_burst_type(sq_sdmmc_dma_channel_num, sq_sdmmc_burst_type);
		sq_set_dma_data_size(sq_sdmmc_dma_channel_num, sq_sdmmc_data_size);
		sq_set_dma_transfer_count(sq_sdmmc_dma_channel_num, 512);
		sq_sdmmc_sw_dma_complete_flag = 0;
		sq_enable_dma(sq_sdmmc_dma_channel_num);

		/* Last transmission */
		if (1 == nsect_r) {
			/* Send CMD12 to stop the transimission */
			if (sq_sdmmc_send_cmd(&sq_sdmmc_dev, MMC_STOP_TRANSMISSION, 0x00000000))
				return -1;
			sq_sdmmc_dev.card_state = TRAN;
			goto hdma_dma_transfer_end;
		}

		if (2 == nsect_r)	/* last block to read */
			sq_sdmmc_write(SQ_SDMMC_SD_DATAT,
					  SQ_SDMMC_DATA_XFER_SIGNAL_BEGIN |
					  SQ_SDMMC_DATA_XFER_DIR_READ |
					  sq_sdmmc_dev.bus_width |
					  SQ_SDMMC_DATA_XFER_DMA_DIS |
					  SQ_SDMMC_DATA_XFER_CYC_SINGLE,
					  sq_sdmmc_base);
		else
			sq_sdmmc_write(SQ_SDMMC_SD_DATAT,
					  SQ_SDMMC_DATA_XFER_SIGNAL_BEGIN |
					  SQ_SDMMC_DATA_XFER_DIR_READ |
					  sq_sdmmc_dev.bus_width |
					  SQ_SDMMC_DATA_XFER_DMA_DIS |
					  SQ_SDMMC_DATA_XFER_CYC_MULTIPLE,
					  sq_sdmmc_base);
	hdma_dma_transfer_end:
		/* Wait for dma to be complete */
		sq_sdmmc_to_cnt = 0;
		while (!sq_sdmmc_sw_dma_complete_flag) {
			if (sq_sdmmc_to_cnt > TO_THRESHOLD_CNT) {
				printf("\nSq SD/MMC host: software dma transmission is timeout\n");
				return -1;
			}
			sq_sdmmc_to_cnt++;
		}

		err |= sq_sdmmc_compare_memory(mem_pattern_addr, mem_cmpr_addr, 512,
						  autotest);
		mem_cmpr_addr += 512;
		mem_pattern_addr += 512;
		nsect_r--;
	}

	return  err;
}

static void
sq_sdmmc_device_initialize(struct sq_sdmmc_device *dev)
{
	u32 rdata;

	dev->size = 0;
	dev->card_state = IDLE;
	dev->card_type = SD_CARD;
	dev->card_int = 0;
	dev->bus_width = 0;
	dev->media_changed = 1;
	rdata = sq_sdmmc_read(SQ_SDMMC_SD_CARDA, sq_sdmmc_base);
	if (SQ_SDMMC_CARD_DETECT_SIGNAL_HIGH == (rdata & SQ_SDMMC_CARD_DETECT_SIGNAL_HIGH)) {
#if defined(CONFIG_PC9002) || defined(CONFIG_SCDK) || defined(CONFIG_SQ8000)		
		dev->alive_flag = 1;
#else
		dev->alive_flag = 0;
#endif
	}
	else {
#if defined(CONFIG_PC9002) || defined(CONFIG_SCDK) || defined(CONFIG_SQ8000)
		dev->alive_flag= 0;
#else
		dev->alive_flag = 1;
#endif
	}
}

static void 
sq_sdmmc_isr(void *data)
{
	struct sq_sdmmc_device *dev = (struct sq_sdmmc_device *)data;
	u32 rdata;

	rdata = sq_sdmmc_read(SQ_SDMMC_SD_INT, sq_sdmmc_base);

	if (SQ_SDMMC_CMD_RESP_XFER_INT_STAT_YES == (rdata & SQ_SDMMC_CMD_RESP_XFER_INT_STAT_YES)) {
		PDEBUG("\nSq SD/MMC host: command and response interrupt occured\n");

		/* Clear corresponding interrupt flag */
		sq_sdmmc_write(SQ_SDMMC_SD_INT,
				  sq_sdmmc_read(SQ_SDMMC_SD_INT, sq_sdmmc_base) & (~SQ_SDMMC_CMD_RESP_XFER_INT_STAT_YES),
				  sq_sdmmc_base);

		dev->card_int |= CMD_RESP_INT;
	}

	if (SQ_SDMMC_DATA_XFER_INT_STAT_YES == (rdata & SQ_SDMMC_DATA_XFER_INT_STAT_YES)) {
		PDEBUG("\nSq SD/MMC host: data transfer interrupt occured\n");

		/* Clear corresponding interrupt flag */
		sq_sdmmc_write(SQ_SDMMC_SD_INT,
				  sq_sdmmc_read(SQ_SDMMC_SD_INT, sq_sdmmc_base) & (~SQ_SDMMC_DATA_XFER_INT_STAT_YES),
				  sq_sdmmc_base);

		dev->card_int |= DATA_XFER_INT;
	}

	if (SQ_SDMMC_HOST_CARD_DETECT_INT_STAT_YES == (rdata & SQ_SDMMC_HOST_CARD_DETECT_INT_STAT_YES)) {
		PDEBUG("\nSq SD/MMC host: host card detect interrupt occured\n");

		/* Clear corresponding interrupt flag */
		sq_sdmmc_write(SQ_SDMMC_SD_INT,
				  sq_sdmmc_read(SQ_SDMMC_SD_INT, sq_sdmmc_base) & (~SQ_SDMMC_HOST_CARD_DETECT_INT_STAT_YES),
				  sq_sdmmc_base);

		dev->card_int |= CARD_DET_INT;
		rdata = sq_sdmmc_read(SQ_SDMMC_SD_CARDA, sq_sdmmc_base);
		if (SQ_SDMMC_CARD_DETECT_SIGNAL_HIGH == (rdata & SQ_SDMMC_CARD_DETECT_SIGNAL_HIGH)) {	//==  
		  
#if defined(CONFIG_PC9002) || defined(CONFIG_SCDK) || defined(CONFIG_SQ8000)
			printf("\nCONFIG_SQ8000 Sq SD/MMC host: card has been inserted\n");
			dev->alive_flag = 1;
			dev->media_changed = 1;
			
#else	

			printf("\nNOT SQ8000 Sq SD/MMC host: card has been removed\n");
			dev->alive_flag = 0;
			dev->media_changed = 0;

			// Disable card's power */
			sq_sdmmc_write(SQ_SDMMC_SD_CARDA,
					  SQ_SDMMC_CARD_SEL_DIS |
					  SQ_SDMMC_CARD_POWER_CTRL_SIGNAL_DIS |
					  SQ_SDMMC_CARD_DETECT_INT_EN,
					  sq_sdmmc_base);
#endif		  

		} else {
		  
#if defined(CONFIG_PC9002) || defined(CONFIG_SCDK) || defined(CONFIG_SQ8000)
			printf("\nCONFIG_SQ8000 Sq SD/MMC host: card has been removed\n");
			dev->alive_flag = 0;
			dev->media_changed = 0;

			// Disable card's power 
			sq_sdmmc_write(SQ_SDMMC_SD_CARDA,
					  SQ_SDMMC_CARD_SEL_DIS |
					  SQ_SDMMC_CARD_POWER_CTRL_SIGNAL_DIS |
					  SQ_SDMMC_CARD_DETECT_INT_EN,
					  sq_sdmmc_base);
#else

			printf("\nNOT SQ8000 Sq SD/MMC host: card has been inserted\n");
			dev->alive_flag = 1;
			dev->media_changed = 1;
#endif
		}
	}
}

static int 
sq_sdmmc_fill_pattern(int pattern_width, u32 block_num)
{
	u32 wdata, i;
	u8 remainder = block_num % 2;

	/* Check whether the block number is odd or even */
	if (1 == remainder) {
		switch (pattern_width) {
		case PATTERN_WIDTH_BYTE:
			for (i = 0; i < 256; i++)
				sq_sdmmc_write(SQ_SDMMC_MMU_DATA, i, sq_sdmmc_base);
			for (i = 256; i > 0; i--)
				sq_sdmmc_write(SQ_SDMMC_MMU_DATA, i-1, sq_sdmmc_base);
			break;
		case PATTERN_WIDTH_HALFWORD:
			for (i = 0; i < 128; i++) {
				wdata = 256 * (2 * i) + (2 * i + 1);
				sq_sdmmc_write(SQ_SDMMC_MMU_DATA, wdata, sq_sdmmc_base);				
			}
			for (i = 128; i > 0; i--) {
				wdata = 256 * (2 * i - 1) + (2 * i - 2);
				sq_sdmmc_write(SQ_SDMMC_MMU_DATA, wdata, sq_sdmmc_base);
			}
			break;
		case PATTERN_WIDTH_WORD:
			for (i = 0; i < 64; i++) {
				wdata = 16777216 * (4 * i) + 65536 * (4 * i + 1) + 256 * (4 * i + 2) + (4 * i + 3);
				sq_sdmmc_write(SQ_SDMMC_MMU_DATA, wdata, sq_sdmmc_base);
			}
			for (i = 64; i > 0; i--) {
				wdata = 16777216 * (4 * i - 1) + 65526 * (4 * i - 2) + 256 * (4 * i - 3) + (4 * i - 4);
				sq_sdmmc_write(SQ_SDMMC_MMU_DATA, wdata, sq_sdmmc_base);
			}
			break;
		default:
			goto undefine_error;
		}
	} else {
		switch (pattern_width) {
		case PATTERN_WIDTH_BYTE:
			for (i = 256; i > 0; i--)
				sq_sdmmc_write(SQ_SDMMC_MMU_DATA, i-1, sq_sdmmc_base);
			for (i = 0; i < 256; i++)
				sq_sdmmc_write(SQ_SDMMC_MMU_DATA, i, sq_sdmmc_base);
			break;
		case PATTERN_WIDTH_HALFWORD:
			for (i = 128; i > 0; i--) {
				wdata = 256 * (2 * i - 1) + (2 * i - 2);
				sq_sdmmc_write(SQ_SDMMC_MMU_DATA, wdata, sq_sdmmc_base);
			}
			for (i = 0; i < 128; i++) {
				wdata = 256 * (2 * i) + (2 * i + 1);
				sq_sdmmc_write(SQ_SDMMC_MMU_DATA, wdata, sq_sdmmc_base);
			}
			break;
		case PATTERN_WIDTH_WORD:
			for (i = 64; i > 0; i--) {
				wdata = 16777216 * (4 * i - 1) + 65526 * (4 * i - 2) + 256 * (4 * i - 3) + (4 * i - 4);
				sq_sdmmc_write(SQ_SDMMC_MMU_DATA, wdata, sq_sdmmc_base);
			}
			for (i = 0; i < 64; i++) {
				wdata = 16777216 * (4 * i) + 65536 * (4 * i + 1) + 256 * (4 * i + 2) + (4 * i + 3);
				sq_sdmmc_write(SQ_SDMMC_MMU_DATA, wdata, sq_sdmmc_base);
			}
			break;
		default:
			goto undefine_error;
		}
	}
	return 0;
undefine_error:
	printf("\nSq SD/MMC host: cause a undefined symbol error\n");
	goto ret_error;
ret_error:
	return -1;

}

static int 
sq_sdmmc_compare_pattern(int pattern_width, u32 block_num)
{
	u32 rdata, wdata, i;
	u8 remainder = block_num % 2;
     
	/* Check whether the block number is odd or even */
	if (1 == remainder) {
		switch (pattern_width) {
		case PATTERN_WIDTH_BYTE:
			for (i = 0; i < 256; i++) {
				rdata = sq_sdmmc_read(SQ_SDMMC_MMU_DATA, sq_sdmmc_base);
				if (rdata != i)
					goto compare_err;
			}
			for (i = 256; i > 0; i--) {
				rdata = sq_sdmmc_read(SQ_SDMMC_MMU_DATA, sq_sdmmc_base);
				if (rdata != (i - 1))
					goto compare_err;
			}
			break;
		case PATTERN_WIDTH_HALFWORD:
			for (i = 0; i < 128; i++) {
				wdata = 256 * (2 * i) + (2 * i + 1);
				rdata = sq_sdmmc_read(SQ_SDMMC_MMU_DATA, sq_sdmmc_base);
				if (rdata != wdata)
					goto compare_err;
			}
			for (i = 128; i > 0; i--) {
				wdata = 256 * (2 * i - 1) + (2 * i - 2);
				rdata = sq_sdmmc_read(SQ_SDMMC_MMU_DATA, sq_sdmmc_base);
				if (rdata != wdata)
					goto compare_err;
			}
			break;
		case PATTERN_WIDTH_WORD:
			for (i = 0; i < 64; i++) {
				wdata = 16777216 * (4 * i) + 65536 * (4 * i + 1) + 256 * (4 * i + 2) + (4 * i + 3);
				rdata = sq_sdmmc_read(SQ_SDMMC_MMU_DATA, sq_sdmmc_base);
				if (rdata != wdata)
					goto compare_err;
			}
			for (i = 64; i > 0; i--) {
				wdata = 16777216 * (4 * i - 1) + 65526 * (4 * i - 2) + 256 * (4 * i - 3) + (4 * i - 4);
				rdata = sq_sdmmc_read(SQ_SDMMC_MMU_DATA, sq_sdmmc_base);
				if (rdata != wdata)
					goto compare_err;
			}
			break;
		default:
			goto undefine_error;
		}
	} else {
		switch (pattern_width) {
		case PATTERN_WIDTH_BYTE:
			for (i = 256; i > 0; i--) {
				rdata = sq_sdmmc_read(SQ_SDMMC_MMU_DATA, sq_sdmmc_base);
				if (rdata != (i - 1))
					goto compare_err;
			}
			for (i = 0; i < 256; i++) {
				rdata = sq_sdmmc_read(SQ_SDMMC_MMU_DATA, sq_sdmmc_base);
				if (rdata != i)
					goto compare_err;
			}
			break;
		case PATTERN_WIDTH_HALFWORD:
			for (i = 128; i > 0; i--) {
				wdata = 256 * (2 * i - 1) + (2 * i - 2);
				rdata = sq_sdmmc_read(SQ_SDMMC_MMU_DATA, sq_sdmmc_base);
				if (rdata != wdata)
					goto compare_err;
			}
			for (i = 0; i < 128; i++) {
				wdata = 256 * (2 * i) + (2 * i + 1);
				rdata = sq_sdmmc_read(SQ_SDMMC_MMU_DATA, sq_sdmmc_base);
				if (rdata != wdata)
					goto compare_err;
			}
			break;
		case PATTERN_WIDTH_WORD:
			for (i = 64; i > 0; i--) {
				wdata = 16777216 * (4 * i - 1) + 65526 * (4 * i - 2) + 256 * (4 * i - 3) + (4 * i - 4);
				rdata = sq_sdmmc_read(SQ_SDMMC_MMU_DATA, sq_sdmmc_base);
				if (rdata != wdata)
					goto compare_err;
			}
			for (i = 0; i < 64; i++) {
				wdata = 16777216 * (4 * i) + 65536 * (4 * i + 1) + 256 * (4 * i + 2) + (4 * i + 3);
				rdata = sq_sdmmc_read(SQ_SDMMC_MMU_DATA, sq_sdmmc_base);
				if (rdata != wdata)
					goto compare_err;
			}
			break;
		default:
			goto undefine_error;
		}
	}
	return 0;
undefine_error:
	printf("\nSq SD/MMC host: cause a undefined symbol error\n");
	goto ret_error;
compare_err:
	printf("\nSq SD/MMC host: cause a compare error\n");
	goto ret_error;
ret_error:
	return -1;

}

static void 
sq_sdmmc_make_test_pattern_dma(u8 *mem, int pattern_width, u32_t block_num)
{
	u8 *mem_8 = NULL;
	u16 *mem_16 = NULL;
	u32 *mem_32 = NULL;
	u8 remainder = block_num % 2;
	u32 i, j = 0;

	/* Check whether the block number is odd or even */
	if (1 == remainder) {
		switch (pattern_width) {
		case PATTERN_WIDTH_BYTE:
			mem_8 = (u8 *)mem;
			for (i = 0; i < 256; i++)
				mem_8[j++] = i;
			for (i = 256; i > 0; i--)
				mem_8[j++] = i - 1;
			break;
		case PATTERN_WIDTH_HALFWORD:
			mem_16 = (u16 *)mem;
			for (i = 0; i < 128; i++) 
				mem_16[j++] = 256 * (2 * i) + (2 * i + 1);
			for (i = 128; i > 0; i--)
				mem_16[j++] = 256 * (2 * i - 1) + (2 * i - 2);
			break;
		case PATTERN_WIDTH_WORD:
			mem_32 = (u32 *)mem;
			for (i = 0; i < 64; i++) {
				mem_32[j++] = 16777216 * (4 * i) + 65536 * (4 * i + 1) + 256 * (4 * i + 2) + (4 * i + 3);
			}
			for (i = 64; i > 0; i--) {
				mem_32[j++] = 16777216 * (4 * i - 1) + 65526 * (4 * i - 2) + 256 * (4 * i - 3) + (4 * i - 4);
			}
			break;
		default:
			goto undefine_error;
		}
	} else {
		switch (pattern_width) {
		case PATTERN_WIDTH_BYTE:
			mem_8 = (u8 *)mem;
			for (i = 256; i > 0; i--)
				mem_8[j++] = i - 1;
			for (i = 0; i < 256; i++)
				mem_8[j++] = i;
			break;
		case PATTERN_WIDTH_HALFWORD:
			mem_16 = (u16 *)mem;
			for (i = 128; i > 0; i--)
				mem_16[j++] = 256 * (2 * i - 1) + (2 * i - 2);
			for (i = 0; i < 128; i++)
				mem_16[j++] = 256 * (2 * i) + (2 * i + 1);
			break;
		case PATTERN_WIDTH_WORD:
			mem_32 = (u32 *)mem;
			for (i = 64; i > 0; i--) {
				mem_32[j++] = 16777216 * (4 * i - 1) + 65526 * (4 * i - 2) + 256 * (4 * i - 3) + (4 * i - 4);
			}
			for (i = 0; i < 64; i++) {
				mem_32[j++] = 16777216 * (4 * i) + 65536 * (4 * i + 1) + 256 * (4 * i + 2) + (4 * i + 3);
			}
			break;
		default:
			goto undefine_error;
		}
	}
	return ;
undefine_error:
	printf("\nSq SD/MMC host: cause a undefined symbol error\n");
	return ;
}

static int
sq_sdmmc_compare_memory(u8 *mem, u8 *mem_cmpr, u32 cnt,
			   int skip_cmpr_result)
{
	int i, err = 0;
	u32 *mem_32 = (u32 *)mem;
	u32 *mem_cmpr_32 = (u32 *)mem_cmpr;
	
	for (i = 0; i < (cnt >> 2); i++) {
		if (mem_32[i] != mem_cmpr_32[i]) {
			err |= -1;
			if (!skip_cmpr_result)
				printf("Sq SD/MMC host: word%d 0x%08x (0x%08x) != 0x%08x (0x%08x)\n", i, mem_32[i], &mem_32[i], 
				       mem_cmpr_32[i], &mem_cmpr_32[i]);
		}
	}
	return err;
}

static u32
sq_sdmmc_send_cmd(struct sq_sdmmc_device *dev, u16 cmd_abbr, u32 arg)
{
	u32 rdata;
	u32 resp_xfer_type;
	int no_resp = 0;

	sq_sdmmc_write(SQ_SDMMC_SD_CMD, arg, sq_sdmmc_base);
	switch (cmd_abbr) {
	case MMC_APP_CMD:
	case SD_APP_SET_BUS_WIDTH:
	case SD_APP_SEND_SCR:
	case MMC_SET_BLOCKLEN:
	case MMC_WRITE_BLOCK:
	case MMC_READ_SINGLE_BLOCK:
	case MMC_READ_MULTIPLE_BLOCK:
	case MMC_WRITE_MULTIPLE_BLOCK:
	case MMC_SEND_STATUS:
	case SD_ERASE_WR_BLK_START:
	case SD_ERASE_WR_BLK_END:
		resp_xfer_type = SQ_SDMMC_RESP_XFER_TYPE_R1;
		break;
	case MMC_SELECT_CARD:
	case MMC_STOP_TRANSMISSION:
	case MMC_SET_WRITE_PROT:
	case MMC_CLR_WRITE_PROT:
	case MMC_ERASE:
		resp_xfer_type = SQ_SDMMC_RESP_XFER_TYPE_R1B;
		break;
	case MMC_ALL_SEND_CID:
	case MMC_SEND_CSD:
	case MMC_SEND_CID:
		resp_xfer_type = SQ_SDMMC_RESP_XFER_TYPE_R2;
		break;
	case MMC_SEND_OP_COND:
	case SD_APP_OP_COND:
		resp_xfer_type = SQ_SDMMC_RESP_XFER_TYPE_R3;
		break;
	case MMC_SET_RELATIVE_ADDR:
		resp_xfer_type = SQ_SDMMC_RESP_XFER_TYPE_R6;
		break;
	case MMC_GO_IDLE_STATE:
	case MMC_GO_INACTIVE_STATE:
		no_resp = 1;
		break;
	default:
		goto undefined_cmd;
	}
	if (no_resp)
		sq_sdmmc_write(SQ_SDMMC_SD_CMDREST,
				  SQ_SDMMC_CMD_XFER_SIGNAL_BEGIN |
				  SQ_SDMMC_RESP_XFER_SIGNAL_END |
				  SQ_SDMMC_RESP_XFER_TYPE_R1 |
				  SQ_SDMMC_CMD_RESP_XFER_STAT_N_ERR |
				  SQ_SDMMC_CMD_INDEX(cmd_abbr),
				  sq_sdmmc_base);
	else
		sq_sdmmc_write(SQ_SDMMC_SD_CMDREST,
				  SQ_SDMMC_CMD_XFER_SIGNAL_BEGIN |
				  SQ_SDMMC_RESP_XFER_SIGNAL_BEGIN |
				  resp_xfer_type |
				  SQ_SDMMC_CMD_RESP_XFER_STAT_N_ERR |
				  SQ_SDMMC_CMD_INDEX(cmd_abbr),
				  sq_sdmmc_base);

	/* Do command transfer checking process */
	sq_sdmmc_to_cnt = 0;
	while (0 == (CMD_RESP_INT & dev->card_int)) {
		if (sq_sdmmc_to_cnt > TO_THRESHOLD_CNT) {
			printf("\nSq SD/MMC host: comamnd%d and response it timeout\n", cmd_abbr);
			return -1;
		}
		sq_sdmmc_to_cnt++;
	}
	dev->card_int &= ~CMD_RESP_INT;

	/* Check whether command transfer cause errors */
	rdata = sq_sdmmc_read(SQ_SDMMC_SD_CMDREST, sq_sdmmc_base);
	if (SQ_SDMMC_CMD_RESP_XFER_STAT_ERR == (rdata & SQ_SDMMC_CMD_RESP_XFER_STAT_ERR))
		goto send_cmd_error;

	PDEBUG("\nSq SD/MMC host: sending CMD%d success\n", cmd_abbr);
	return 0;
undefined_cmd:
	printf("\nSq SD/MMC host: undefined command: CMD%d\n", cmd_abbr);
	return -1;
send_cmd_error:
	printf("\nSq SD/MMC host: sending CMD%d cause an error\n", cmd_abbr);
	return sq_sdmmc_report_command_response_error();
}

static u32
sq_sdmmc_report_command_response_error(void)
{
	u32 rdata;

	rdata = sq_sdmmc_read(SQ_SDMMC_SD_CMDRESA, sq_sdmmc_base);
	if (SQ_SDMMC_CARD_CMD_RESP_STAT_ERR == (rdata & SQ_SDMMC_CARD_CMD_RESP_STAT_ERR))
		printf("\nSq SD/MMC host: command and response cause a error\n");
	if (SQ_SDMMC_CARD_CMD_RESP_BUS_CONFLICT_ERR == (rdata & SQ_SDMMC_CARD_CMD_RESP_BUS_CONFLICT_ERR))
		printf("\nSq SD/MMC host: command and response cause a bus conflict error\n");
	if (SQ_SDMMC_CARD_RESP_TO_ERR == (rdata & SQ_SDMMC_CARD_RESP_TO_ERR))
		printf("\nSq SD/MMC host: command and response cause a timeout error\n");
	if (SQ_SDMMC_CARD_RESP_XFER_BIT_ERR == (rdata & SQ_SDMMC_CARD_RESP_XFER_BIT_ERR))
		printf("\nSq SD/MMC host: command and response cause a transmission bit error\n");
	if (SQ_SDMMC_CARD_RESP_IDX_ERR == (rdata & SQ_SDMMC_CARD_RESP_IDX_ERR))
		printf("\nSq SD/MMC host: command and response cause a index error\n");
	if (SQ_SDMMC_CARD_RESP_CRC_ERR == (rdata & SQ_SDMMC_CARD_RESP_CRC_ERR))
		printf("\nSq SD/MMC host: command and response cause a CRC error\n");
	if (SQ_SDMMC_CARD_RESP_END_BIT_ERR == (rdata & SQ_SDMMC_CARD_RESP_END_BIT_ERR))
		printf("\nSq SD/MMC host: command and response cause a end bit error\n");
	return rdata;
}

static int
sq_sdmmc_data_transfer_end(struct sq_sdmmc_device *dev)
{
	u32 rdata;

	sq_sdmmc_to_cnt = 0;
	while (0 == (DATA_XFER_INT & dev->card_int)) {
		if (sq_sdmmc_to_cnt > TO_THRESHOLD_CNT) {
			printf("\nSq SD/MMC host: data transfer is timeout\n");
			return -1;
		}
		sq_sdmmc_to_cnt++;
	}		
	dev->card_int &= ~DATA_XFER_INT;
	rdata = sq_sdmmc_read(SQ_SDMMC_SD_DATAT, sq_sdmmc_base);
	if (SQ_SDMMC_DATA_XFER_STAT_ERR == (rdata & SQ_SDMMC_DATA_XFER_STAT_ERR)) {
		sq_sdmmc_report_data_transfer_error();
		return -1;
	}
	return 0;
}

static void
sq_sdmmc_report_data_transfer_error(void)
{
	u32 rdata;

	rdata = sq_sdmmc_read(SQ_SDMMC_SD_DATAT, sq_sdmmc_base);
	if (SQ_SDMMC_DATA_XFER_STAT_ERR == (rdata & SQ_SDMMC_DATA_XFER_STAT_ERR))
		printf("\nSq SD/MMC host: data transfer status is error\n");
	if (SQ_SDMMC_DATA_XFER_BUS_CONFLICT_ERR == (rdata & SQ_SDMMC_DATA_XFER_BUS_CONFLICT_ERR))
		printf("\nSq SD/MMC host: data transfer cause a bus conflict error\n");
	if (SQ_SDMMC_DATA_XFER_TO_ERR == (rdata & SQ_SDMMC_DATA_XFER_TO_ERR))
		printf("\nSq SD/MMC host: data transfer status is error\n");
	if (SQ_SDMMC_DATA_XFER_CRC_ERR == (rdata & SQ_SDMMC_DATA_XFER_CRC_ERR))
		printf("\nSq SD/MMC host: data transfer cause a CRC error\n");
	if (SQ_SDMMC_READ_DATA_XFER_STR_BIT_ERR == (rdata & SQ_SDMMC_READ_DATA_XFER_STR_BIT_ERR))
		printf("\nSq SD/MMC host: read data transfer cause a start bit error\n");
	if (SQ_SDMMC_READ_DATA_XFER_END_BIT_ERR == (rdata & SQ_SDMMC_READ_DATA_XFER_END_BIT_ERR))	  
		printf("\nSq SD/MMC host: read data transfer cause a end bit error\n");
	if (SQ_SDMMC_WRITE_DATA_XFER_CRC_STAT_CRC_ERR == (rdata & SQ_SDMMC_WRITE_DATA_XFER_CRC_STAT_CRC_ERR))
		printf("\nSq SD/MMC host: write data transfer cause CRC status to be a CRC error\n");
	if (SQ_SDMMC_WRITE_DATA_XFER_CRC_STAT_NO_RESP == (rdata & SQ_SDMMC_WRITE_DATA_XFER_CRC_STAT_NO_RESP))
		printf("\nSq SD/MMC host: write data transfer cause CRC status to be a no response error\n");
}

static u32
sq_sdmmc_power(u32 base, u32 power)
{
	int i = 0;
	u32 val = 1;

	if (0 == power)
		return 1;
	else {
		for (i = 0; i < power; i++)
			val *= base;
		return val;
	}
}

static int 
sq_sdmmc_card_initialize(struct sq_sdmmc_device *dev, int only_ident)
{
	int i;
	u32 error;
	u32  rdata = 0;
	u32 divider = 0;
	u32 apb_clk;
	u32 sdmmc_clk = 0xffffffff;

	apb_clk = sq_get_apb_clock();

	/* Caculate the divider */
	while (1) {
		u32 tmp;

		tmp = sq_sdmmc_power(2, divider);
		tmp++;
		sdmmc_clk = apb_clk / tmp;
		if (sdmmc_clk > 25000000) {
			divider++;
			continue;
		} else
			break;
	}
	printf("\nSq SD/MMC Host: current sd/mmc bus clock is %d and divider is %d\n", sdmmc_clk, divider);

	sq_sdmmc_write(SQ_SDMMC_SD_CTRL,
			  SQ_SDMMC_CARD_POWER_CTRL_CPU |
			  SQ_SDMMC_CARD_DETECT_FUNC_MECH |
			  SQ_SDMMC_CARD_CLK_RUN |
			  SQ_SDMMC_CARD_CLK_DIVIDER(divider),
			  sq_sdmmc_base);
	sq_sdmmc_write(SQ_SDMMC_SD_CARDA,
			  SQ_SDMMC_CARD_SEL_EN |
			  SQ_SDMMC_CARD_POWER_CTRL_SIGNAL_EN |
			  SQ_SDMMC_CARD_DETECT_INT_EN,
			  sq_sdmmc_base);
	sq_sdmmc_write(SQ_SDMMC_SD_INT,
			  SQ_SDMMC_CMD_RESP_XFER_INT_STAT_NO |
			  SQ_SDMMC_DATA_XFER_INT_STAT_NO |
			  SQ_SDMMC_HOST_CARD_DETECT_INT_STAT_NO |
			  SQ_SDMMC_CMD_RESP_XFER_INT_EN |
			  SQ_SDMMC_DATA_XFER_INT_EN |
			  SQ_SDMMC_HOST_CARD_DETECT_INT_EN,
			  sq_sdmmc_base);

	/* Delay */
	for (i = 0; i <65536; i++);

	dev->card_state = IDLE;
	sq_sdmmc_to_cnt = 0;
	do {
		if (sq_sdmmc_to_cnt > TO_THRESHOLD_CNT) {
			printf("\nSq SD/MMC host: sd identification is timeout\n");
			return -1;
		}
		sq_sdmmc_to_cnt++;

		/* Send CMD55 */
		if ((error = sq_sdmmc_send_cmd(dev, MMC_APP_CMD, 0x00000000))) {
			if (SQ_SDMMC_CARD_RESP_TO_ERR == (error & SQ_SDMMC_CARD_RESP_TO_ERR))
				goto mmc_identification;
			else
				return -1;
		}
	  
		/* Send ACMD41, host support VDD voltage is 0x00ff8000 */
		if (sq_sdmmc_send_cmd(dev, SD_APP_OP_COND, 0x00ff8000))
			return -1;

		rdata = sq_sdmmc_read(SQ_SDMMC_SD_RESA3, sq_sdmmc_base);
	} while (0x00000000 == (rdata & 0x80000000));
	dev->card_type = SD_CARD;
	printf("\n Sq SD/MMC host: sd card has been identified\n");
	goto identity_end;
mmc_identification:
	sq_sdmmc_to_cnt = 0;
	do {
		if (sq_sdmmc_to_cnt > TO_THRESHOLD_CNT) {
			printf("\nSq SD/MMC: mmc identification is timeout\n");
			return -1;
		}
		sq_sdmmc_to_cnt++;
		if (sq_sdmmc_send_cmd(dev, MMC_SEND_OP_COND, 0x00ff8000))
			return -1;
		rdata = sq_sdmmc_read(SQ_SDMMC_SD_RESA3, sq_sdmmc_base);
	} while (0x00000000 == (rdata & 0x80000000));
	dev->card_type = MMC;
	printf("\nSq SD/MMC host: mmc has been identified\n");
identity_end:
	dev->card_state = READY;

	/* Send CMD2 */
	if (sq_sdmmc_send_cmd(dev, MMC_ALL_SEND_CID, 0x00000000))
		return -1;
	dev->card_state = IDENT;

	/* Send CMD3 */
	if (sq_sdmmc_send_cmd(dev, MMC_SET_RELATIVE_ADDR, 0x00000000))
		return -1;
	dev->card_state = STDY;

	/* Get card's relative address (RCA) */
	dev->card_rca = sq_sdmmc_read(SQ_SDMMC_SD_RESA3, sq_sdmmc_base) & 0xffff0000;

	/* Caculate the divider */
	divider = 0;
	sdmmc_clk = 0xffffffff;
	while (1) {
		u32 tmp;

		tmp = sq_sdmmc_power(2, divider);
		tmp++;
		sdmmc_clk = apb_clk / tmp;
		if (sdmmc_clk > 25000000) {
			divider++;
			continue;
		} else
			break;
	}
	printf("\nSq SD/MMC host: current sd/mmc bus clock is %d and divider is %d\n", sdmmc_clk, divider);


	/* Change clock rate to enter the data transfer mode */
	sq_sdmmc_write(SQ_SDMMC_SD_CTRL,
			  SQ_SDMMC_CARD_POWER_CTRL_CPU |
			  SQ_SDMMC_CARD_DETECT_FUNC_MECH |
			  SQ_SDMMC_CARD_CLK_RUN |
			  SQ_SDMMC_CARD_CLK_DIVIDER(divider),
			  sq_sdmmc_base);

	if (only_ident)
		return 0;

	/* Send CMD7 */
	// 20080408 cyli fix for mmc cmd7 response
	if (SD_CARD == dev->card_type) {
		if (sq_sdmmc_send_cmd(dev, MMC_SELECT_CARD, dev->card_rca))
			return -1;
	} else {
		if (sq_sdmmc_send_cmd(dev, MMC_SELECT_CARD, 0x0))
			return -1;
	}
	dev->card_state = TRAN;

	/* Send CMD16 to set the block length */
	if (sq_sdmmc_send_cmd(dev, MMC_SET_BLOCKLEN, 512))
		return -1;

	/* Initialize the MMU buffer pointer */
	sq_sdmmc_write(SQ_SDMMC_MMU_PNRI, 0x000001ff, sq_sdmmc_base);
	sq_sdmmc_write(SQ_SDMMC_MMU_PNRII, 0x000001ff, sq_sdmmc_base);

	dev->media_changed = 0;
	return 0;

}

