#include <platform.h>
#include <genlib.h>
#include <irqs.h>
#include <test_item.h>
#include <dma/dma.h>
#include "hdma-ctrl.h"
#include "../DMA/hdma-regs.h"

#define HDMA_TO_THRESHOLD_CNT (1024 * 1024)

static	u32_t hdma_init_src_addr = 0x00a00000;
static	u32_t hdma_init_dst_addr = 0x00a00800;
static  int hdma_init_count = 2048;

static int sq_hdma_complete_flag = 0;
static int sq_hdma_to_cnt = 0;
static	u32_t hdma_src_addr;
static	u32_t hdma_dst_addr;
static  int hdma_count;
static	int autorun;
static u32_t sq_hdma_channel_num, sq_hdma_burst_type, sq_hdma_data_size, sq_hdma_src_dir, sq_hdma_dst_dir,
			sq_hdma_fly;
//extern struct hdma_ip_base_irq  *hdma_ip;
//extern struct hdma_ip_base_irq  hdma_ip[];

//for nand test
u32_t hdma_pattern_addr = 0x02000000;
u32_t hdma_nand_buffer1 = 0x04000000;
u32_t hdma_nand_buffer2 = 0x06000000;
u32_t hdma_nand_writesize = 2048;


static struct sq_dma_notifier sq_hdma_notifier = {
	.complete = sq_hdma_notifier_complete,
};

extern struct test_item_container hdma_main_container;

extern int 
HDMATesting(int autotest)
{
	int ret=0;	

	autorun = autotest;
	hdma_src_addr = hdma_init_src_addr;
	hdma_dst_addr = hdma_init_dst_addr;
	hdma_count = hdma_init_count;	
	
	printf("\nHDMA initial setting :\n");
	printf("src addr : 0x%8x\n", hdma_src_addr);
	printf("dst addr : 0x%8x\n", hdma_dst_addr);
	printf("count : 0x%x\n", hdma_count);
	
	ret=test_item_ctrl(&hdma_main_container,autotest);
	return ret;
}

extern struct test_item_container hdma_para_setting_container;

extern int
hdma_set_parameter(int autotest)
{
	int ret=0;
	
	ret=test_item_ctrl(&hdma_para_setting_container,autotest);
	return ret;
}

extern int
hdma_set_address_size(int autotest)
{
		printf("set src addr : ");
		scanf("%x\n", &hdma_src_addr);
		printf("set dst addr : ");
		scanf("%x\n", &hdma_dst_addr);
		printf("set count : ");
		scanf("%x\n", &hdma_count);
		return 0;
}

extern int
hdma_test_nand(int autotest)
{
	int burst_type;
	int ret;
	
	sq_hdma_channel_num = SQ_HDMA_CH_0;    
	sq_request_dma(sq_hdma_channel_num, &sq_hdma_notifier);
	
		printf("set burst type : \n");
		printf("0.Single\n");
		printf("1.INCR4\n");
		printf("2.INCR8\n");
		printf("3.INCR16\n");
		printf("burst_type>");
		scanf("%d", &burst_type);

	 /* Initialize the control setting for HDMA */
     sq_hdma_src_dir = SQ_DMA_DIR_INCR;
     sq_hdma_dst_dir = SQ_DMA_DIR_INCR;
     sq_hdma_data_size = SQ_DMA_DATA_WORD;
     sq_hdma_fly = SQ_DMA_FLY_DIS;
		
	switch (burst_type) {
     case 0:
	  sq_hdma_burst_type = SQ_DMA_BURST_SINGLE;
	  break;
     case 1:
	  sq_hdma_burst_type = SQ_DMA_BURST_INCR4;
	  break;
     case 2:
	  sq_hdma_burst_type = SQ_DMA_BURST_INCR8;
	  break;
     case 3:
	  sq_hdma_burst_type = SQ_DMA_BURST_INCR16;
	  break;
     default:
	  printf("Unknown burst type\n");
	  sq_disable_dma(sq_hdma_channel_num);
	  sq_free_dma(sq_hdma_channel_num);
	  return - 1;
     }
	  /* Set HDMA */
	  hdma_src_addr = hdma_pattern_addr;
	  hdma_dst_addr = hdma_nand_buffer1;
	  hdma_count = hdma_nand_writesize;
	  
	  if(sq_hdma_software_dma_tranfer(0) != 0){
		  sq_disable_dma(sq_hdma_channel_num);
		  sq_free_dma(sq_hdma_channel_num);
	  	  return -1;
  	  }
	 	  /* Wait for dma to be complete */
	  
	 /***********	write from  nand_buffer1 to nand_buffer1	***********/
	    /* Set HDMA */ 
	  hdma_src_addr = hdma_nand_buffer1;
	  hdma_dst_addr = hdma_nand_buffer2;
	  hdma_count = hdma_nand_writesize;
	  
	  ret = sq_hdma_software_dma_tranfer(0);
	  sq_disable_dma(sq_hdma_channel_num);
 	  sq_free_dma(sq_hdma_channel_num);
	  
	  if(ret != 0)
	  	return -1;
	  printf(" Nand test success!! ");
	  	return 0;
}

	/*			main setting			*/
extern struct test_item_container hdma_ch_container;

extern int
hdma_channel_0(int autotest)
{
    int ret=0;
	
    sq_hdma_channel_num = SQ_HDMA_CH_0;    
	sq_request_dma(sq_hdma_channel_num, &sq_hdma_notifier);
	ret = test_item_ctrl(&hdma_ch_container, autotest);
	sq_disable_dma(sq_hdma_channel_num);
	sq_free_dma(sq_hdma_channel_num);
	return ret;    
}
extern int
hdma_channel_1(int autotest)
{
    int ret=0;
	
    sq_hdma_channel_num = SQ_HDMA_CH_1;    
	sq_request_dma(sq_hdma_channel_num, &sq_hdma_notifier);
	ret = test_item_ctrl(&hdma_ch_container, autotest);
	sq_disable_dma(sq_hdma_channel_num);
	sq_free_dma(sq_hdma_channel_num);
	return ret;  
}
extern int
hdma_channel_2(int autotest)
{
    int ret=0;
	
    sq_hdma_channel_num = SQ_HDMA_CH_2;    
	sq_request_dma(sq_hdma_channel_num, &sq_hdma_notifier);
	ret = test_item_ctrl(&hdma_ch_container, autotest);
	sq_disable_dma(sq_hdma_channel_num);
	sq_free_dma(sq_hdma_channel_num);
	return ret;  
}
extern int
hdma_channel_3(int autotest)
{
    int ret=0;
	
    sq_hdma_channel_num = SQ_HDMA_CH_3;    
	sq_request_dma(sq_hdma_channel_num, &sq_hdma_notifier);
	ret = test_item_ctrl(&hdma_ch_container, autotest);
	sq_disable_dma(sq_hdma_channel_num);
	sq_free_dma(sq_hdma_channel_num);
	return ret;  
}

	/*			On the fly			*/
extern struct test_item_container hdma_fly_container;

extern int
hdma_fly_disable(int autotest)
{
    int ret=0;
	
    sq_hdma_fly = SQ_DMA_FLY_DIS;
	ret=test_item_ctrl(&hdma_fly_container,autotest);
	return ret;
}
extern int
hdma_fly_enable(int autotest)
{
    int ret=0;
	
    sq_hdma_fly = SQ_DMA_FLY_WRITE;
	ret=test_item_ctrl(&hdma_fly_container,autotest);
	return ret;
}

	/*			Auto Reload and Auto Start			*/
extern struct test_item_container hdma_autoreload_container;

extern int
hdma_autoreload_disable(int autotest)
{
    int ret=0;
	
    ret=test_item_ctrl(&hdma_autoreload_container,autotest);
	return ret;
}
extern int
hdma_autoreload_enable(int autotest)
{
    int ret=0;
	
    ret=test_item_ctrl(&hdma_autoreload_container,autotest);
	return ret;
}

	/*			Source Direction			*/
extern struct test_item_container hdma_src_dir_container;

extern int
hdma_source_direction_fixed(int autotest)
{
    int ret=0;
	
    sq_hdma_src_dir = SQ_DMA_DIR_FIXED;
	ret=test_item_ctrl(&hdma_src_dir_container,autotest);
	return ret;
}
extern int
hdma_source_direction_increment(int autotest)
{
    int ret=0;
	
    sq_hdma_src_dir = SQ_DMA_DIR_INCR;
	ret=test_item_ctrl(&hdma_src_dir_container,autotest);
	return ret;
}

	/*			Dstination Direction			*/
extern struct test_item_container hdma_dst_dir_container;

extern int
hdma_destination_direction_fixed(int autotest)
{
    int ret=0;
	
    sq_hdma_dst_dir = SQ_DMA_DIR_FIXED;
	ret=test_item_ctrl(&hdma_dst_dir_container,autotest);
	return ret;
}
extern int
hdma_destination_direction_increment(int autotest)
{
    int ret=0;
	
    sq_hdma_dst_dir = SQ_DMA_DIR_INCR;
	ret=test_item_ctrl(&hdma_dst_dir_container,autotest);
	return ret;
}

	/*			Burst Type			*/
extern struct test_item_container hdma_burst_type_container;

extern int
hdma_burst_type_single(int autotest)
{
    int ret=0;
	
    sq_hdma_burst_type = SQ_DMA_BURST_SINGLE;
	ret=test_item_ctrl(&hdma_burst_type_container,autotest);
	return ret;
}
extern int
hdma_burst_type_incr4(int autotest)
{
    int ret=0;
	
    sq_hdma_burst_type = SQ_DMA_BURST_INCR4;
	ret=test_item_ctrl(&hdma_burst_type_container,autotest);
	return ret;
}
extern int
hdma_burst_type_incr8(int autotest)
{
    int ret=0;
	
    sq_hdma_burst_type = SQ_DMA_BURST_INCR8;
	ret=test_item_ctrl(&hdma_burst_type_container,autotest);
	return ret;
}
extern int
hdma_burst_type_incr16(int autotest)
{
    int ret=0;
	
    sq_hdma_burst_type = SQ_DMA_BURST_INCR16;
	ret=test_item_ctrl(&hdma_burst_type_container,autotest);
	return ret;
}

	/*			Data Size			*/
extern struct test_item_container hdma_data_size_container;

extern int
hdma_data_size_byte(int autotest)
{
    int ret=0;
	
    sq_hdma_data_size = SQ_DMA_DATA_BYTE;
	ret=test_item_ctrl(&hdma_data_size_container,autotest);
	return ret;
}
extern int
hdma_data_size_halfword(int autotest)
{
    int ret=0;
	
    sq_hdma_data_size = SQ_DMA_DATA_HALFWORD;
	ret=test_item_ctrl(&hdma_data_size_container,autotest);
	return ret;
}
extern int
hdma_data_size_word(int autotest)
{
    int ret=0;
	
    sq_hdma_data_size = SQ_DMA_DATA_WORD;
	ret=test_item_ctrl(&hdma_data_size_container,autotest);
	return ret;
}

extern int 
sq_hdma_software_dma_tranfer(int autotest)
{
	sq_disable_dma(sq_hdma_channel_num);
	sq_set_dma_mode(sq_hdma_channel_num, SQ_DMA_MODE_SW);
	sq_set_dma_source_address(sq_hdma_channel_num, hdma_src_addr);
	sq_set_dma_destination_address(sq_hdma_channel_num, hdma_dst_addr);
	sq_set_dma_transfer_count(sq_hdma_channel_num, hdma_count);
	sq_set_dma_source_direction(sq_hdma_channel_num, sq_hdma_src_dir);
	sq_set_dma_destination_direction(sq_hdma_channel_num, sq_hdma_dst_dir);
	sq_set_dma_burst_type(sq_hdma_channel_num, sq_hdma_burst_type);
	sq_set_dma_data_size(sq_hdma_channel_num, sq_hdma_data_size);
	sq_set_dma_fly_operation(sq_hdma_channel_num, sq_hdma_fly);
	
	sq_hdma_clear_memory((u8 *)hdma_src_addr, hdma_count);
	sq_hdma_clear_memory((u8 *)hdma_dst_addr, hdma_count);	
	sq_hdma_make_test_pattern((u8 *)hdma_src_addr, hdma_count);
	
	sq_hdma_complete_flag = 0;
	sq_hdma_to_cnt = 0;
	
	sq_enable_dma(sq_hdma_channel_num);
		
	while (!sq_hdma_complete_flag) {
		if (sq_hdma_to_cnt > HDMA_TO_THRESHOLD_CNT) {
			printf("Timeout\n");
			return -1;
		}
		sq_hdma_to_cnt++;
	}
	if(sq_hdma_compare_memory((u8 *)hdma_src_addr, (u8 *)hdma_dst_addr, hdma_count, 
					    sq_hdma_data_size, sq_hdma_src_dir, sq_hdma_dst_dir) == -1){
		printf("error occur on the setting!!\n");
		sq_hdma_show_setting(1);
		if(!autorun){
				printf("input enter to continue\n");
				getchar();
		}
		return -1;
	}
	return 0;
}

static void 
sq_hdma_make_test_pattern (u8 *mem, u32 cnt)	
{
	int i;
	
	for(i = 0; i < cnt; i++)
		mem[i] = i;
}

static void 
sq_hdma_clear_memory (u8 *mem, u32 cnt)	
{
	int i;
	u32 *mem_32 = (u32 *)mem;
	
	for(i = 0 ; i < (cnt >> 2) ;i++)
		mem_32[i] = 0;
}

static int 
sq_hdma_compare_memory(u8 *mem, u8 *cmpr_mem, u32 cnt, 
			     u32 data_size, u32 src_dir, u32 dst_dir)
{
	u32 i;
	u8 *mem_8 = mem;
	u8 *cmpr_mem_8 = cmpr_mem;
	u16 *mem_16 = (u16 *)mem;
	u16 *cmpr_mem_16 = (u16 *)cmpr_mem;
	u32 *mem_32 = (u32 *)mem;
	u32 *cmpr_mem_32 = (u32 *)cmpr_mem;
	int err_flag = 0;
		
	switch (data_size) {
	case SQ_DMA_DATA_BYTE:
		if ((SQ_DMA_DIR_INCR == src_dir) &&
		    (SQ_DMA_DIR_INCR == dst_dir)) {
			for (i = 0; i < cnt; i++) {
				if (mem_8[i] != cmpr_mem_8[i]) {
					err_flag |= -1;
					printf("\nByte %d, 0x%02x (0x%08x) != 0x%02x (0x%08x)", i, mem_8[i], &mem_8[i], 
					       cmpr_mem_8[i], &cmpr_mem_8[i]);
				}
			}
		} else if ((SQ_DMA_DIR_INCR == src_dir) &&
			   (SQ_DMA_DIR_FIXED == dst_dir)) {
			if (mem_8[cnt-4] != cmpr_mem_8[0]) {
				err_flag |= -1;
				printf("\nByte %d, 0x%02x (0x%08x) != 0x%02x (0x%08x)", cnt-1, mem_8[cnt-1], &mem_8[cnt-1], 
				       cmpr_mem_8[0], &cmpr_mem_8[0]);
			}

		} else if ((SQ_DMA_DIR_FIXED == src_dir) &&
			   (SQ_DMA_DIR_INCR == dst_dir)) {
			for (i = 0; i < cnt; i++) {
				if (mem_8[i&3] != cmpr_mem_8[i]) {
					err_flag |= -1;
					printf("\nByte %d, 0x%02x (0x%08x) != 0x%02x (0x%08x)", i, mem_8[0], &mem_8[0], 
					       cmpr_mem_8[i], &cmpr_mem_8[i]);
				}
			}
		} else if ((SQ_DMA_DIR_FIXED == src_dir) &&
			   (SQ_DMA_DIR_FIXED == dst_dir)) {
			if (mem_8[0] != cmpr_mem_8[0]) {
				err_flag |= -1;
				printf("\nByte 0, 0x%02x (0x%08x) != 0x%02x (0x%08x)", mem_8[0], &mem_8[0], 
				       cmpr_mem_8[0], &cmpr_mem_8[0]);
			}
		} else {
			err_flag |= -1;
			printf("SQ HDMA: unknown direction for source and destination\n");
		}
		break;
	case SQ_DMA_DATA_HALFWORD:
		if ((SQ_DMA_DIR_INCR == src_dir) &&
		    (SQ_DMA_DIR_INCR == dst_dir)) {
			for (i = 0; i < (cnt >> 1); i++) {
				if (mem_16[i] != cmpr_mem_16[i]) {
					err_flag |= -1;
					printf("\nHalfword %d, 0x%04x (0x%08x) != 0x%04x (0x%08x)", i, mem_16[i], &mem_16[i], 
					       cmpr_mem_16[i], &cmpr_mem_16[i]);
				}
			}
		} else if ((SQ_DMA_DIR_INCR == src_dir) &&
			   (SQ_DMA_DIR_FIXED == dst_dir)) {
			if (mem_16[(cnt>>1)-2] != cmpr_mem_16[0]) {
				err_flag |= -1;
				printf("\nHalfword %d, 0x%04x (0x%08x) != 0x%04x (0x%08x)", (cnt>>1)-1, mem_16[(cnt>>1)-1], &mem_16[(cnt>>1)-1], 
				       cmpr_mem_16[0], &cmpr_mem_16[0]);
			}

		} else if ((SQ_DMA_DIR_FIXED == src_dir) &&
			   (SQ_DMA_DIR_INCR == dst_dir)) {
			for (i = 0; i < (cnt >> 1); i++) {
				if (mem_16[i&1] != cmpr_mem_16[i]) {
					err_flag |= -1;
					printf("\nHalfword %d, 0x%04x (0x%08x) != 0x%04x (0x%08x)", i, mem_16[0], &mem_16[0], 
					       cmpr_mem_16[i], &cmpr_mem_16[i]);
				}
			}
		} else if ((SQ_DMA_DIR_FIXED == src_dir) &&
			   (SQ_DMA_DIR_FIXED == dst_dir)) {
			if (mem_16[0] != cmpr_mem_16[0]) {
				err_flag |= -1;
				printf("\nHalfword 0, 0x%04x (0x%08x) != 0x%04x (0x%08x)", mem_16[0], &mem_16[0], 
				       cmpr_mem_16[0], &cmpr_mem_16[0]);
			}
		} else {
			err_flag |= -1;
			printf("HDMA: unknown direction for source and destination\n");
		}
		break;
	case SQ_DMA_DATA_WORD:
		if ((SQ_DMA_DIR_INCR == src_dir) &&
		    (SQ_DMA_DIR_INCR == dst_dir)) {
			for (i = 0; i < (cnt >> 2); i++) {
				if (mem_32[i] != cmpr_mem_32[i]) {
					err_flag |= -1;
					printf("\nWord %d, 0x%08x (0x%08x) != 0x%08x (0x%08x)", i, mem_32[i], &mem_32[i], 
					       cmpr_mem_32[i], &cmpr_mem_32[i]);
				}
			}
		} else if ((SQ_DMA_DIR_INCR == src_dir) &&
			   (SQ_DMA_DIR_FIXED == dst_dir)) {
			if (mem_32[(cnt>>2)-1] != cmpr_mem_32[0]) {
				err_flag |= -1;
				printf("\nWord %d, 0x%08x (0x%08x) != 0x%08x (0x%08x)", (cnt>>2)-1, mem_32[(cnt>>2)-1], &mem_32[(cnt>>2)-1], 
				       cmpr_mem_32[0], &cmpr_mem_32[0]);
			}

		} else if ((SQ_DMA_DIR_FIXED == src_dir) &&
			   (SQ_DMA_DIR_INCR == dst_dir)) {
			for (i = 0; i < (cnt >> 2); i++) {
				if (mem_32[0] != cmpr_mem_32[i]) {
					err_flag |= -1;
					printf("\nWord %d, 0x%08x (0x%08x) != 0x%08x (0x%08x)", i, mem_32[0], &mem_32[0], 
					       cmpr_mem_32[i], &cmpr_mem_32[i]);
				}
			}
		} else if ((SQ_DMA_DIR_FIXED == src_dir) &&
			   (SQ_DMA_DIR_FIXED == dst_dir)) {
			if (mem_32[0] != cmpr_mem_32[0]) {
				err_flag |= -1;
				printf("\nWord 0, 0x%08x (0x%08x) != 0x%08x (0x%08x)", mem_32[0], &mem_32[0], 
				       cmpr_mem_32[0], &cmpr_mem_32[0]);
			}
		} else {
			err_flag |= -1;
			printf("HDMA: unknown direction for source and destination\n");
		}
		break;
	default:
		err_flag |= -1;
		printf("HDMA: unknown data size\n");
	}
	return err_flag;
}

extern int
sq_hdma_show_setting(int autotest)
{
	printf("channel : %d\n", sq_hdma_channel_num);
	printf("src addr : 0x%8x\n", hdma_src_addr);
	printf("dst addr : 0x%8x\n", hdma_dst_addr);
	
	switch(sq_hdma_burst_type){
		case SQ_DMA_BURST_SINGLE :
			printf("Burst Type : Single\n");
			break;
		case SQ_DMA_BURST_INCR4 :
			printf("Burst Type : INCR4\n");
			break;
		case SQ_DMA_BURST_INCR8 :
			printf("Burst Type : INCR8\n");
			break;
		case SQ_DMA_BURST_INCR16 :
			printf("Burst Type : INCR16\n");
			break;
	}
	switch(sq_hdma_data_size){
		case SQ_DMA_DATA_BYTE :
			printf("Data Size : Byte\n");
			break;
		case SQ_DMA_DATA_HALFWORD :
			printf("Data Size : Halfword\n");
			break;
		case SQ_DMA_DATA_WORD :
			printf("Data Size : Word\n");
			break;			
	}
	switch(sq_hdma_src_dir){
		case SQ_DMA_DIR_INCR :
			printf("Src Dir : Increment\n");
			break;
		case SQ_DMA_DIR_FIXED :
			printf("Src Dir : Fixed\n");
			break;
	}
	switch(sq_hdma_dst_dir){
		case SQ_DMA_DIR_INCR :
			printf("Dst Dir : Increment\n");
			break;
		case SQ_DMA_DIR_FIXED :
			printf("Dst Dir : Fixed\n");
			break;
	}
	switch(sq_hdma_fly){
		case SQ_DMA_FLY_WRITE :
			printf("On the fly : Enable\n");
			break;
		case SQ_DMA_FLY_DIS :
			printf("On the fly : Disable\n");
			break;
	}	
	return 0;
}

static void 
sq_hdma_notifier_complete(void *data)
{
	sq_hdma_complete_flag = 1;
}

