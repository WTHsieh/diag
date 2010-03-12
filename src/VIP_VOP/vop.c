#include <platform.h>
#include <irqs.h>
#include <io.h>
#include <type.h>
#include "vop-regs.h"
#include "vop.h"

static void vopOneFrameIsr(void *pparam);
static void vopTwoFrameIsr(void *pparam);
static u32 int_sts;	
static u32 frame_sts;
static inline void
sq_vop_write(u32 val, u32 reg)
{
	iowrite32(val, SQ_VOP_BASE+reg);
}

static inline u32
sq_vop_read(u32 reg)
{
	return ioread32(SQ_VOP_BASE+reg);
}


extern void
vopReset()
{
	sq_vop_write(VOP_CTRL_RESET | VOP_CTRL_DISPLAY_DIS, SQ_VOP_CTRL);
	MSDELAY(260);
	sq_vop_write(VOP_INT_DISABLE, SQ_VOP_INTE);
	sq_vop_write(VOP_CTRL_NO_RESET | VOP_CTRL_DISPLAY_DIS, SQ_VOP_CTRL);
}

extern void
vopStart()
{
	sq_vop_write(VOP_AHBR_CTRL_INCR16, SQ_VOP_AHBR_CTRL);	
	sq_vop_write(VOP_INTE_BUFFER_UNDERRUN | VOP_INTE_DISPLAY_COMPLETE, SQ_VOP_INTE);
	sq_vop_write(sq_vop_read(SQ_VOP_CTRL) | VOP_CTRL_DISPLAY_EN, SQ_VOP_CTRL);
}

void
vopStop ()
{  
  sq_vop_write(VOP_INT_DISABLE, SQ_VOP_INTE);
  sq_vop_write(VOP_CTRL_DISPLAY_DIS, SQ_VOP_CTRL);
  free_irq(SQ_INTC_VOP);
}

extern int
vopSetOutFormat(int format)
{
	if(format==FORMAT_NTSC)
		sq_vop_write(sq_vop_read(SQ_VOP_CTRL) & ~VOP_CTRL_FORMAT_PAL, SQ_VOP_CTRL);
	else if(format==FORMAT_PAL)
		sq_vop_write(sq_vop_read(SQ_VOP_CTRL) | VOP_CTRL_FORMAT_PAL, SQ_VOP_CTRL);
	else {
		printf("VOP : Error Output Format!!\n");
		return -1;
	}
	return 0;	
}

extern int
vopSetFrameSize(int framesize)
{
	u32 size;
	switch(framesize)
  {
  	case FRAMESIZE_QCIF:
  		size=(176<<10)+144;    
    	break;
    case FRAMESIZE_CIF:
    	size=(352<<10)+288;     
      break;
    case FRAMESIZE_QVGA:
    	size=(320<<10)+240;     
      break;
    case FRAMESIZE_VGA:
    	size=(640<<10)+480;
      break;
    case FRAMESIZE_D1:
    	size=(720<<10)+480;
      break;
    default:
    	printf("VOP : Error Frame Size!!\n");
    	return -1;
	}
	sq_vop_write(size, SQ_VOP_FSS);
	return 0;
}

extern int 
vopSetFrameDisplayAddr(int frameNum, u32 Y_pt, u32 Cb_pt, u32 Cr_pt)
{
	if(frameNum==FRAME1) {
		sq_vop_write(Y_pt, SQ_VOP_DRF1SAY);
		sq_vop_write(Cb_pt, SQ_VOP_DRF1SACB);
		sq_vop_write(Cr_pt, SQ_VOP_DRF1SACR);
		sq_vop_write(VOP_FBS_FRAME1_USE_BY_HW, SQ_VOP_FBS);
	}
	else if(frameNum==FRAME2) {
		sq_vop_write(Y_pt, SQ_VOP_DRF2SAY);
		sq_vop_write(Cb_pt, SQ_VOP_DRF2SACB);
		sq_vop_write(Cr_pt, SQ_VOP_DRF2SACR);
		sq_vop_write(VOP_FBS_FRAME2_USE_BY_HW, SQ_VOP_FBS);
	}
	else {
		printf("Error Frame Number!!\n");
    return -1;
	}
	return 0;
}


extern int
vopSetFrameMode(int frame_mode)
{
	if(frame_mode==ONE_FRAME)
		request_irq(SQ_INTC_VOP, vopOneFrameIsr, NULL);
	else if(frame_mode==TWO_FRAME)
		request_irq(SQ_INTC_VOP, vopTwoFrameIsr, NULL);
	else {
		printf("Error Frame Mode!!\n");
    return -1;
	}
	return 0;
}


static void
vopOneFrameIsr(void *pparam)
{
  int_sts = sq_vop_read(SQ_VOP_INT_STS);
  frame_sts = sq_vop_read(SQ_VOP_FBS);
  
	if (int_sts & VOP_INT_STS_DISPLAY_COMPLETE) {    
  	if ((frame_sts & VOP_FBS_FRAME1_USE_BY_HW) == 0) 
	  	sq_vop_write(VOP_FBS_FRAME1_USE_BY_HW, SQ_VOP_FBS);
  }
  
  if (int_sts & VOP_INT_STS_BUFFER_UNDERUN) {
  	int line_num;
    line_num = int_sts >> VOP_INT_STS_LINE_NUM_OF_UNDERUN_SHIFT;
		printf("VOP : Buffer Underflow at line 0x%x\n",line_num);
	}
}

static void
vopTwoFrameIsr(void *pparam)
{
  int_sts = sq_vop_read(SQ_VOP_INT_STS);
  frame_sts = sq_vop_read(SQ_VOP_FBS);
  
	if (int_sts & VOP_INT_STS_DISPLAY_COMPLETE) {    
  	if ((frame_sts & VOP_FBS_FRAME1_USE_BY_HW) == 0)
	  	sq_vop_write(VOP_FBS_FRAME1_USE_BY_HW, SQ_VOP_FBS);
  	if ((frame_sts & VOP_FBS_FRAME2_USE_BY_HW) == 0)
	  	sq_vop_write(VOP_FBS_FRAME2_USE_BY_HW, SQ_VOP_FBS);
  }
  
  if (int_sts & VOP_INT_STS_BUFFER_UNDERUN) {
  	int line_num;
    line_num = int_sts >> VOP_INT_STS_LINE_NUM_OF_UNDERUN_SHIFT;
		printf("VOP : Buffer Underflow at line 0x%x\n",line_num);
	}
}

