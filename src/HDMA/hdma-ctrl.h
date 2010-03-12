#ifndef _hdmactrl_h_included_
#define _hdmactrl_h_included_

extern int		HDMATesting (int autotest);
static void		sq_hdma_make_test_pattern(u8 *mem, u32 cnt);
static void		sq_hdma_clear_memory(u8 *mem, u32 cnt);	
static int		sq_hdma_compare_memory(u8 *mem, u8 *cmpr_mem, u32 cnt, 
			     u32 data_size, u32 src_dir, u32 dst_dir);
extern int		sq_hdma_software_dma_tranfer(int autotest);	
extern int 		sq_hdma_show_setting(int autotest);

static void sq_hdma_notifier_complete(void *data);
				

#endif // _hdmactrl_h_included_
