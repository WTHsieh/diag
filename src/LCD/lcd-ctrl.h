#ifndef _lcd_ctrl_h_included_
#define _lcd_ctrl_h_included_

int Sq_Color_16BPP_FSFill (int color , u32 color_base);
int Sq_LUT_Fill(int index , u32 color_base);
int Sq_ColorFSFill (u32 color , u32 color_base);
extern void Sq_vip_lcd_initial(int output422);
extern void Sq_vip_lcd_stop(void);
#endif // _lcd_ctrl_h_included_
