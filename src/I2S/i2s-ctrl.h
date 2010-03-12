#ifndef __I2S_H_INCLUDED
#define __I2S_H_INCLUDED

#include <type.h>

extern int sq_i2s_play_pcm_normal(int autotest);
extern int sq_i2s_play_pcm_hwdma_panther7_hdma(int autotest);
extern int sq_i2s_capture_pcm_normal(int autotest);
extern int sq_i2s_capture_pcm_hwdma_panther7_hdma(int autotest);


#endif

