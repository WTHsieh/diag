#include <platform.h>
#include <irqs.h>
#include <io.h>
#include <type.h>
#include "vip-regs.h"
#include "vip.h"

static u32 int_sts;		
static u32 frame_sts;
static void vipOneFrameIsr(void *pparam);
static void vipTwoFrameIsr(void *pparam);

static inline void
sq_vip_write(u32 val, u32 reg)
{
	iowrite32(val, SQ_VIP_BASE+reg);
}

static inline u32
sq_vip_read(u32 reg)
{
	return ioread32(SQ_VIP_BASE+reg);
}

extern int
vipSetOutFormat(int format)
{
	if(format==FORMAT_NTSC)
		sq_vip_write(sq_vip_read(SQ_VIP_CTRL) & ~VIP_CTRL_FORMAT_PAL, SQ_VIP_CTRL);
	else if(format==FORMAT_PAL)
		sq_vip_write(sq_vip_read(SQ_VIP_CTRL) | VIP_CTRL_FORMAT_PAL, SQ_VIP_CTRL);
	else {
		printf("VIP : Error Output Format!!\n");
		return -1;
	}
	return 0;	
}

extern int
vipSet422Output(int en)
{
	if(en)
		sq_vip_write(sq_vip_read(SQ_VIP_CTRL) | VIP_CTRL_422_OUTPUT, SQ_VIP_CTRL);
	else if(en==0)
		sq_vip_write(sq_vip_read(SQ_VIP_CTRL) & ~VIP_CTRL_422_OUTPUT, SQ_VIP_CTRL);
	else {
		printf("VIP : Error Output Data Format!!\n");
		return -1;
	}
	return 0;
	
}

extern int
vipSetFrameSize(int framesize)
{
	switch(framesize)
  {
  	case FRAMESIZE_QCIF:
    	sq_vip_write(0x00B00090, SQ_VIP_FS);
    	break;
    case FRAMESIZE_CIF:
      sq_vip_write(0x01600120, SQ_VIP_FS);
      break;
    case FRAMESIZE_QVGA:
      sq_vip_write(0x014000F0, SQ_VIP_FS);
      break;
    case FRAMESIZE_VGA:
      sq_vip_write(0x028001E0, SQ_VIP_FS);
      break;
    case FRAMESIZE_D1:
      sq_vip_write(0x02D001E0, SQ_VIP_FS);
      break;
    default:
    	printf("VIP : Error Frame Size!!\n");
    	return -1;
	}
	return 0;
}

extern int 
vipSetFrameCaptureAddr(int frameNum, u32 Y_pt, u32 Cb_pt, u32 Cr_pt)
{
	if(frameNum==FRAME1) {
		sq_vip_write(Y_pt, SQ_VIP_CAPTURE_F1SA_Y);
		sq_vip_write(Cb_pt, SQ_VIP_CAPTURE_F1SA_Cb);
		sq_vip_write(Cr_pt, SQ_VIP_CAPTURE_F1SA_Cr);
	}
	else if(frameNum==FRAME2) {
		sq_vip_write(Y_pt, SQ_VIP_CAPTURE_F2SA_Y);
		sq_vip_write(Cb_pt, SQ_VIP_CAPTURE_F2SA_Cb);
		sq_vip_write(Cr_pt, SQ_VIP_CAPTURE_F2SA_Cr);
	}
	else {
		printf("VIP : Error Frame Number!!\n");
    return -1;
	}
	return 0;
}

extern int
vipSetFrameMode(int frame_mode)
{
	switch(frame_mode) {
		case ONE_FRAME:
			sq_vip_write(sq_vip_read(SQ_VIP_CTRL) | VIP_CTRL_ONE_FRAME_STOP, SQ_VIP_CTRL);
			request_irq(SQ_INTC_VIP,vipOneFrameIsr,NULL);
			break;
		case TWO_FRAME:
			sq_vip_write((sq_vip_read(SQ_VIP_CTRL) & ~VIP_CTRL_ONE_FRAME_STOP) | VIP_CTRL_PING_PONG_MODE, SQ_VIP_CTRL);
			request_irq(SQ_INTC_VIP,vipTwoFrameIsr,NULL);
			break;
		case CONTINUOUS:
			sq_vip_write(sq_vip_read(SQ_VIP_CTRL) & ~(VIP_CTRL_ONE_FRAME_STOP | VIP_CTRL_PING_PONG_MODE), SQ_VIP_CTRL);
			break;
		default:
    	printf("VIP : Error Frame Mode!!\n");
    	return -1;
	}
	return 0;
}

extern void
vipReset()
{
#ifdef CONFIG_SQ8000
	sq_vip_write(VIP_RESET, SQ_VIP_RESET);
#else
	sq_vip_write(VIP_CTRL_RESET, SQ_VIP_CTRL);
#endif
	MSDELAY(260);
	sq_vip_write(VIP_INT_MASK_DISABLE, SQ_VIP_INT_MASK);
	sq_vip_write(VIP_CTRL_CAPTURE_DIS, SQ_VIP_CTRL);
}

extern void
vipStart()
{
	sq_vip_write(VIP_AHBR_CTRL_INCR16, SQ_VIP_AHBR_CTRL);
	sq_vip_write(sq_vip_read(SQ_VIP_FB_SR) & ~VIP_FBS_FRAME1_DATA_READY, SQ_VIP_FB_SR);
	sq_vip_write(sq_vip_read(SQ_VIP_FB_SR) & ~VIP_FBS_FRAME2_DATA_READY, SQ_VIP_FB_SR);
	sq_vip_write(VIP_INT_MASK_CAPTURE_COMPLETE | VIP_INT_MASK_CAPTURE_FRAME_LOSS, SQ_VIP_INT_MASK);
	sq_vip_write(sq_vip_read(SQ_VIP_CTRL) | VIP_CTRL_CAPTURE_EN, SQ_VIP_CTRL);
}

extern void
vipStop ()
{  
  sq_vip_write(VIP_INT_MASK_DISABLE, SQ_VIP_INT_MASK);
  sq_vip_write(VIP_CTRL_CAPTURE_DIS, SQ_VIP_CTRL);
  free_irq(SQ_INTC_VIP);
}

static void
vipOneFrameIsr(void *pparam)
{
	int_sts = sq_vip_read(SQ_VIP_INT_STS);  
  frame_sts = sq_vip_read(SQ_VIP_FB_SR);
  
  if (int_sts & VIP_INT_STS_CAPTURE_COMPLETE) {    
  	if (frame_sts & VIP_FBS_FRAME1_DATA_READY)
  		sq_vip_write(frame_sts & ~VIP_FBS_FRAME1_DATA_READY, SQ_VIP_FB_SR);
  } 
  //if (int_sts & VIP_INT_STS_CAPTURE_FRAME_LOSS)
	//	printf("VIP : Capture frame loss!!\n");
}


static void
vipTwoFrameIsr(void *pparam)
{
  int_sts = sq_vip_read(SQ_VIP_INT_STS); 
 
  frame_sts = sq_vip_read(SQ_VIP_FB_SR);
  
  if (int_sts & VIP_INT_STS_CAPTURE_COMPLETE) {    
  	if (frame_sts & VIP_FBS_FRAME1_DATA_READY) {
  		sq_vip_write(sq_vip_read(SQ_VIP_CAPTURE_F1SA_Y),SQ_VIP_CAPTURE_F1SA_Y);
  		sq_vip_write(sq_vip_read(SQ_VIP_CAPTURE_F1SA_Cb),SQ_VIP_CAPTURE_F1SA_Cb);
  		sq_vip_write(sq_vip_read(SQ_VIP_CAPTURE_F1SA_Cr),SQ_VIP_CAPTURE_F1SA_Cr);
  		sq_vip_write(frame_sts & ~VIP_FBS_FRAME1_DATA_READY, SQ_VIP_FB_SR);
  	}
  	if (frame_sts & VIP_FBS_FRAME2_DATA_READY) {
  		sq_vip_write(sq_vip_read(SQ_VIP_CAPTURE_F2SA_Y),SQ_VIP_CAPTURE_F2SA_Y);
  		sq_vip_write(sq_vip_read(SQ_VIP_CAPTURE_F2SA_Cb),SQ_VIP_CAPTURE_F2SA_Cb);
  		sq_vip_write(sq_vip_read(SQ_VIP_CAPTURE_F2SA_Cr),SQ_VIP_CAPTURE_F2SA_Cr);
  		sq_vip_write(frame_sts & ~VIP_FBS_FRAME2_DATA_READY, SQ_VIP_FB_SR);
  	}
  }
  //if (int_sts & VIP_INT_STS_CAPTURE_FRAME_LOSS)
	//	printf("VIP : Capture frame loss!!\n");
}

