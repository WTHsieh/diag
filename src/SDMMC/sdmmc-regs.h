#ifndef __SQ_SDMMC_H_INCLUDED
#define __SQ_SDMMC_H_INCLUDED

/*
 *  Registers for SD/MMC host controller
 *  */
#define SQ_SDMMC_MMU_CTRL 0x0000 /* MMU control register */
#define SQ_SDMMC_MMU_PNRI 0x0004 /* MMU pointer setting register */
#define SQ_SDMMC_CUR_PNRI 0x0008 /* MMU pointer1 current register */
#define SQ_SDMMC_MMU_PNRII 0x000C /* MMU pointer2 setting register */
#define SQ_SDMMC_CUR_PNRII 0x0010 /* MMU pointer2 current register */
#define SQ_SDMMC_MMU_ADDR 0x0014 /* MMU address setting register */
#define SQ_SDMMC_CUR_ADDR 0x0018 /* MMU address current register */
#define SQ_SDMMC_MMU_DATA 0x001C /* MMU data register */
#define SQ_SDMMC_SD_CTRL 0x0020 /* SD host control register */
#define SQ_SDMMC_SD_INT 0x0024 /* SD host interrupt register */
#define SQ_SDMMC_SD_CARDA 0x0028 /* SD cardA register */
#define SQ_SDMMC_SD_CARDB 0x002C /* sD cardB register */
#define SQ_SDMMC_SD_CMDREST 0x0030 /* SD command and response transfer register */
#define SQ_SDMMC_SD_CMDRESA 0x0034 /* SD cardA command and response transfer status register */
#define SQ_SDMMC_SD_CMDRESB 0x0038 /* SD cardB command and response transfer status register */
#define SQ_SDMMC_SD_DATAT 0x003C /* SD data transfer register */
#define SQ_SDMMC_SD_CMD 0x0040 /* SD command argument register */
#define SQ_SDMMC_SD_RESA3 0x0044 /* SD cardA response argument 3 register */
#define SQ_SDMMC_SD_RESA2 0x0048 /* SD cardA response argument 2 register */
#define SQ_SDMMC_SD_RESA1 0x004C /* SD cardA response argument 1 register */
#define SQ_SDMMC_SD_RESA0 0x0050 /* SD cardA response argument 0 register */
#define SQ_SDMMC_SD_RESB3 0x0054 /* SD cardB response argument 3 register */
#define SQ_SDMMC_SD_RESB2 0x0058 /* SD cardB response argument 2 register */
#define SQ_SDMMC_SD_RESB1 0x005C /* SD cardB response argument 1 register */
#define SQ_SDMMC_SD_RESB0 0x0060 /* SD cardB response argument 0 register */

/*
 *  SQ_SDMMC_MMU_CTRL
 *  */
/* Endian control when CPU access to data buffer */
#define SQ_SDMMC_LITTLE_ENDIAN_ACCESS 0x0 /* Little endian access */
#define SQ_SDMMC_BIG_ENDIAN_ACCESS (0x1 << 12) /* Big endian access */

/* MMU DMA transfer signal */
#define SQ_SDMMC_MMU_DMA_XFER_SIGNAL_END 0x0 /* MMU DMA transfer end */
#define SQ_SDMMC_MMU_DMA_XFER_SIGNAL_BEGIN (0x1 << 11) /* MMU DMA transfer begin */

/* MMU DMA transfer direction */
#define SQ_SDMMC_MMU_DMA_XFER_DIR_READ 0 /* MMU DMA transfer direction is read */
#define SQ_SDMMC_MMU_DMA_XFER_DIR_WRITE (0x1 << 10) /* MMU DMA transfer direction is write */

/* MMU bus control swap data buffer */
#define SQ_SDMMC_MMU0_BUS_CTRL_DATABUF_1 0x0 /* the MMU0 bus control data buffer1 and the MMU1 bus control data buffer2 */
#define SQ_SDMMC_MMU0_BUS_CTRL_DATABUF_2 (0x1 << 9) /* the MMU0 bus control data buffer2 and the MMU1 bus control data buffer1 */

/* CPU control swap data buffer */
#define SQ_SDMMC_HOST_CTRL_DATABUF_1 0x0 /* the host microprocessor control data buffer1 */
#define SQ_SDMMC_HOST_CTRL_DATABUF_2 (0x1 << 8) /* the host microprocessor control data buffer2 */

/* Reset data buffer2 pointer */
#define SQ_SDMMC_DATABUF_2_POINTER_KEEP 0x0 /* keep data buffer2 pointer */
#define SQ_SDMMC_DATABUF_2_POINTER_RST (0x1 << 7) /* reset data buffer2 pointer and return 0 automatically */

/* Data buffer2 pointer and signal */
#define SQ_SDMMC_DATABUF_2_POINTER_END_SIGNAL_LOW 0x0 /* data buffer2 pointer end signal is low */
#define SQ_SDMMC_DATABUF_2_POINTER_END_SIGNAL_HIGH (0x1 << 6) /* data buffer2 pointer end signal is high */

/* Indicate data buffer2 transfer width */
#define SQ_SDMMC_DATABUF_2_XFER_WIDTH_BYTE 0x0 /* data buffer2 transfer width is byte */
#define SQ_SDMMC_DATABUF_2_XFER_WIDTH_HALFWORD (0x1 << 4) /* data bufffer2 transfer width is halfword */
#define SQ_SDMMC_DATABUF_2_XFER_WIDTH_WORD (0x3 << 4) /* data buffer2 transfer width is word */

/* Reset data buffer1 pointer */
#define SQ_SDMMC_DATABUF_1_POINTER_KEEP 0x0 /* keep data buffer1 pointer */
#define SQ_SDMMC_DATABUF_1_POINTER_RST (0x1 << 3) /* reset data buffer1 pointer and return 0 automatically */

/* Data buffer1 pointer end signal */
#define SQ_SDMMC_DATABUF_1_POINTER_END_SIGNAL_LOW 0x0 /* data buffer1 pointer end signal is low */
#define SQ_SDMMC_DATABUF_1_POINTER_END_SINGAL_HIGH (0x1 << 2) /* data buffer1 pointer end signal is high */

/* Indicate data buffer1 transfer width */
#define SQ_SDMMC_DATABUF_1_XFER_WIDTH_BYTE 0x0 /* data buffer1 transfer width is byte */
#define SQ_SDMMC_DATABUF_1_XFER_WIDTH_HALFWORD 0x1 /* data buffer1 transfer width is halfword */
#define SQ_SDMMC_DATABUF_1_XFER_WIDTH_WORD 0x3 /* data buffer1 transfer width is word */

#define SQ_SDMMC_MMU_DATA_WIDTH_BYTE (SQ_SDMMC_MMU_DMA_XFER_SIGNAL_END | \
					 SQ_SDMMC_MMU_DMA_XFER_DIR_READ | \
					 SQ_SDMMC_MMU0_BUS_CTRL_DATABUF_2 | \
					 SQ_SDMMC_HOST_CTRL_DATABUF_1 | \
					 SQ_SDMMC_DATABUF_2_POINTER_RST | \
					 SQ_SDMMC_DATABUF_2_POINTER_END_SIGNAL_LOW | \
					 SQ_SDMMC_DATABUF_2_XFER_WIDTH_BYTE | \
					 SQ_SDMMC_DATABUF_1_POINTER_RST | \
					 SQ_SDMMC_DATABUF_1_POINTER_END_SIGNAL_LOW | \
					 SQ_SDMMC_DATABUF_1_XFER_WIDTH_BYTE)
#define SQ_SDMMC_MMU_DATA_WIDTH_HALFWORD (SQ_SDMMC_MMU_DMA_XFER_SIGNAL_END | \
					     SQ_SDMMC_MMU_DMA_XFER_DIR_READ | \
					     SQ_SDMMC_MMU0_BUS_CTRL_DATABUF_2 | \
					     SQ_SDMMC_HOST_CTRL_DATABUF_1 | \
					     SQ_SDMMC_DATABUF_2_POINTER_RST | \
					     SQ_SDMMC_DATABUF_2_POINTER_END_SIGNAL_LOW | \
					     SQ_SDMMC_DATABUF_2_XFER_WIDTH_BYTE | \
					     SQ_SDMMC_DATABUF_1_POINTER_RST | \
					     SQ_SDMMC_DATABUF_1_POINTER_END_SIGNAL_LOW | \
					     SQ_SDMMC_DATABUF_1_XFER_WIDTH_HALFWORD)
#define SQ_SDMMC_MMU_DATA_WIDTH_WORD (SQ_SDMMC_MMU_DMA_XFER_SIGNAL_END | \
					 SQ_SDMMC_MMU_DMA_XFER_DIR_READ | \
					 SQ_SDMMC_MMU0_BUS_CTRL_DATABUF_2 | \
					 SQ_SDMMC_HOST_CTRL_DATABUF_1 | \
					 SQ_SDMMC_DATABUF_2_POINTER_RST | \
					 SQ_SDMMC_DATABUF_2_POINTER_END_SIGNAL_LOW | \
					 SQ_SDMMC_DATABUF_2_XFER_WIDTH_BYTE | \
					 SQ_SDMMC_DATABUF_1_POINTER_RST | \
					 SQ_SDMMC_DATABUF_1_POINTER_END_SIGNAL_LOW | \
					 SQ_SDMMC_DATABUF_1_XFER_WIDTH_WORD)

/*
 *  SQ_MMU_ADDR
 *  */
#define SQ_SDMMC_SET_MMU_ADDR(x) ((x) & 0x00ffffff)

/*
 *  SQ_SDMMC_SD_CTRL
 *  */
/* Power control type for SD cards */
#define SQ_SDMMC_CARD_POWER_CTRL_CPU 0x0 /* the SD card power is controlled by CPU */
#define SQ_SDMMC_CARD_POWER_CTRL_CD (0x1 << 13) /* the SD card power is controlled by CD */

/* Card detect type for SD cards */
#define SQ_SDMMC_CARD_DETECT_FUNC_MECH 0x0 /* the card detect function is used by mechanism */
#define SQ_SDMMC_CARD_DETECT_FUNC_CDDAT3 (0x1 << 12) /* the card detect function is used by CD/DAT3 */

/* SD card clock stop register */
#define SQ_SDMMC_CARD_CLK_RUN 0x0	/* run the SD card clock */
#define SQ_SDMMC_CARD_CLK_STP (0x1 << 11) /* stop the SD card clock */

#define SQ_SDMMC_MAX_CLOCK_DIVIDER 0x000007FF

/* SD card clock divider register */
#define SQ_SDMMC_CARD_CLK_DIVIDER(x) (x & 0x7FF)

/*
 *  SQ_SDMMC_SD_INT
 *  */
/* Command and response transfer interrupt status */
#define SQ_SDMMC_CMD_RESP_XFER_INT_STAT_NO 0x0 /* command and response transfer interrupt status is no */
#define SQ_SDMMC_CMD_RESP_XFER_INT_STAT_YES (0x1 << 6) /* command and response transfer interrupt status is yes */

/* Data transfer interrupt status */
#define SQ_SDMMC_DATA_XFER_INT_STAT_NO 0x0 /* data transfer interrupt status is no */
#define SQ_SDMMC_DATA_XFER_INT_STAT_YES (0x1 << 5) /* data transfer interrupt status is yes */

/* Card detect interrupt status */
#define SQ_SDMMC_HOST_CARD_DETECT_INT_STAT_NO 0x0 /* card detect interrupt status is no */
#define SQ_SDMMC_HOST_CARD_DETECT_INT_STAT_YES (0x1 << 4) /* card detect interrupt status is yes */

/* Command and response transfer interrupt control */
#define SQ_SDMMC_CMD_RESP_XFER_INT_DIS 0x0 /* disable command and response transfer interrupt */
#define SQ_SDMMC_CMD_RESP_XFER_INT_EN (0x1 << 2) /* enable command and response transfer interrupt */

/* Data transfer interrupt control */
#define SQ_SDMMC_DATA_XFER_INT_DIS 0x0 /* disable data transfer interrupt */
#define SQ_SDMMC_DATA_XFER_INT_EN (0x1 << 1) /* enable data transfer interrupt */

/* card detect interrupt enable */
#define SQ_SDMMC_HOST_CARD_DETECT_INT_DIS 0x0 /* disable card detect interrupt */
#define SQ_SDMMC_HOST_CARD_DETECT_INT_EN 0x1	/* enable card detect interrupt */

/*
 *  SQ_SDMMC_SD_CARD
 *  */
/* Card select enable */
#define SQ_SDMMC_CARD_SEL_DIS 0x0 /* disable card select enable */
#define SQ_SDMMC_CARD_SEL_EN (0x1 << 6) /* enable card select */

/* Card power control signal */
#define SQ_SDMMC_CARD_POWER_CTRL_SIGNAL_DIS 0x0 /* disable card power control signal */
#define SQ_SDMMC_CARD_POWER_CTRL_SIGNAL_EN (0x1 << 5) /* enable card power control signal */

/* Card select interrupt enable */
#define SQ_SDMMC_CARD_DETECT_INT_DIS 0x0 /* disable card detect interrupt */
#define SQ_SDMMC_CARD_DETECT_INT_EN (0x1 << 4) /* enable card detect interrupt */

/* Card busy signal */
#define SQ_SDMMC_CARD_BUSY_SIGNAL_HIGH (0x1 << 2) /* card busy signal is high */

/* Card write protect signal */
#define SQ_SDMMC_CARD_WRITE_PROT_SIGNAL_HIGH (0x1 << 1) /* card write protect signal is high */

/* Card detect signal */
#define SQ_SDMMC_CARD_DETECT_SIGNAL_HIGH 0x00//0x1 /* card detect signal is high */

/*
 *  SQ_SDMMC_SD_CMDREST
 *  */
/* Command transfer signal */
#define SQ_SDMMC_CMD_XFER_SIGNAL_END 0x0 /* command transfer end */
#define SQ_SDMMC_CMD_XFER_SIGNAL_BEGIN (0x1 << 13) /* command transfer begins */

/* Response transfer signal */
#define SQ_SDMMC_RESP_XFER_SIGNAL_END 0x0 /* response transfer end */
#define SQ_SDMMC_RESP_XFER_SIGNAL_BEGIN (0x1 << 12) /* response transfer begins */

/* Response transfer type */
#define SQ_SDMMC_RESP_XFER_TYPE_R1 0x0	/* response transfer type is R1 */
#define SQ_SDMMC_RESP_XFER_TYPE_R1B (0x1 << 9) /* response transfer type is R1b */
#define SQ_SDMMC_RESP_XFER_TYPE_R2 (0x2 << 9) /* response transfer type is R2 */
#define SQ_SDMMC_RESP_XFER_TYPE_R3 (0x3 << 9) /* response transfer type is R3 */
#define SQ_SDMMC_RESP_XFER_TYPE_R6 (0x6 << 9) /* response transfer type is R6 */

/* Command and response transfer error status */
#define SQ_SDMMC_CMD_RESP_XFER_STAT_N_ERR 0x0 /* command and response transfer error status is no error */
#define SQ_SDMMC_CMD_RESP_XFER_STAT_ERR (0x1 << 8) /* command and response transfer error status is error */

/* SD commandu index */
#define SQ_SDMMC_CMD_INDEX(x) (x & 0x3F)

/*
 *  SQ_SDMMC_SD_CMDRES
 *  */
/* Card command transfer signal */
#define SQ_SDMMC_CARD_CMD_XFER_SIGNAL_BEGIN (0x1 << 8) /* card command transfer begins */
#define SQ_SDMMC_CARD_RESP_XFER_SIGNAL_BEGIN (0x1 << 7) /* card response transfer begins */
#define SQ_SDMMC_CARD_CMD_RESP_STAT_ERR (0x1 << 6) /* card command and response status is error */
#define SQ_SDMMC_CARD_CMD_RESP_BUS_CONFLICT_ERR (0x1 << 5) /* card command and response bus conflict is error */
#define SQ_SDMMC_CARD_RESP_TO_ERR (0x1 << 4) /* card response timeout is error */
#define SQ_SDMMC_CARD_RESP_XFER_BIT_ERR (0x1 << 3) /* card response transmission bit is error */
#define SQ_SDMMC_CARD_RESP_IDX_ERR (0x1 << 2) /* card response index is error */
#define SQ_SDMMC_CARD_RESP_CRC_ERR (0x1 << 1) /* card response CRC is error */
#define SQ_SDMMC_CARD_RESP_END_BIT_ERR 0x1 /* card response end bit is error */

/*
 *  SQ_SDMMC_SD_DATAT
 *  */
/* Data transfer signal */
#define SQ_SDMMC_DATA_XFER_SIGNAL_END 0x0 /* data transfer end */
#define SQ_SDMMC_DATA_XFER_SIGNAL_BEGIN (0x1 << 13) /* data transfer begins */

/* Data transfer direction */
#define SQ_SDMMC_DATA_XFER_DIR_READ 0x0 /* data transfer direction is read */
#define SQ_SDMMC_DATA_XFER_DIR_WRITE (0x1 << 12) /* data transfer direction is write */

/* Data transfer bus width */
#define SQ_SDMMC_DATA_XFER_BUS_WIDTH_LINE_1 0x0 /* data transfer bus width is line */
#define SQ_SDMMC_DATA_XFER_BUS_WIDTH_LINE_4 (0x1 << 11) /* data transfer bus width is lines */

/* Data transfer with DMA function */
#define SQ_SDMMC_DATA_XFER_DMA_DIS 0x0 /* disable data transfer with DMA function */
#define SQ_SDMMC_DATA_XFER_DMA_EN (0x1 << 10) /* enable data transfer with DMA function */

/* Data transfer cycle */
#define SQ_SDMMC_DATA_XFER_CYC_SINGLE 0x0 /* data transfer cycle is single */
#define SQ_SDMMC_DATA_XFER_CYC_MULTIPLE (0x1 << 9) /* data transfer cycle is multiple */

/* Data transfer error */
#define SQ_SDMMC_DATA_XFER_STAT_ERR (0x1 << 8) /* data transfer error status is error */
#define SQ_SDMMC_DATA_XFER_BUS_CONFLICT_ERR (0x1 << 7) /* data transfer conflict error is error */
#define SQ_SDMMC_DATA_XFER_TO_ERR (0x1 << 6) /* data transfer timeout error is error */
#define SQ_SDMMC_DATA_XFER_CRC_ERR (0x1 << 5) /* data transfer CRC error is error */
#define SQ_SDMMC_READ_DATA_XFER_STR_BIT_ERR (0x1 << 4) /* read data transfer start bit error is error */
#define SQ_SDMMC_READ_DATA_XFER_END_BIT_ERR (0x1 << 3) /* read data transfer end bit error is error */
#define SQ_SDMMC_WRITE_DATA_XFER_CRC_STAT_N_ERR 0x2 /* write data transfer CRC status is no error */
#define SQ_SDMMC_WRITE_DATA_XFER_CRC_STAT_CRC_ERR 0x5 /* write data transfer CRC status is CRC error */
#define SQ_SDMMC_WRITE_DATA_XFER_CRC_STAT_NO_RESP 0x7 /* write data transfer CRC status is no response */

#endif
