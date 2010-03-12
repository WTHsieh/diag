#include <platform.h>
#include <irqs.h>


#if defined(CONFIG_CDK) || defined(CONFIG_PC9002) || defined(CONFIG_SQ8000) || defined(CONFIG_SCDK) || defined(CONFIG_MSMV)
#define SQ_PWM_NUM			2
#elif defined(CONFIG_PDK) || defined(CONFIG_PC7210)
#define SQ_PWM_NUM			4
#else
#define SQ_PWM_NUM			1
#endif

#if defined(CONFIG_PDK) || defined(CONFIG_PC7210)
#define SUPT_CH			6
#elif defined(CONFIG_PC9002)
#define SUPT_CH			2
#elif defined(CONFIG_SQ8000)
#define SUPT_CH			5
#else
#define SUPT_CH			8
#endif


// use pwm number to test adc
#if defined(CONFIG_CDK) || defined(CONFIG_PC9002) || defined(CONFIG_SQ8000) || defined(CONFIG_SCDK) || defined(CONFIG_MSMV)
#define USE_PWM_NUM			1
#elif defined(CONFIG_PDK) || defined(CONFIG_PC7210)
#define USE_PWM_NUM			1
#else
#define USE_PWM_NUM			0
#endif

#define DEFAULT_PRE_SCL		0x0		// default Prescale Factor
#define DEFAULT_HRC			0x64	// default HRC
#define DEFAULT_LRC			0x64	// default LRC


// use pwm number to test capture mode
#define USE_PWM_NUM_0		0
#define USE_PWM_NUM_1		1


#ifdef SQ_INTC_PWM0
#define SQ_PWMT_0_ENABLE		1
#else
#define SQ_PWMT_0_ENABLE		0
#endif

#ifdef SQ_INTC_PWM1
#define SQ_PWMT_1_ENABLE		1
#else
#define SQ_PWMT_1_ENABLE		0
#endif

#ifdef SQ_INTC_PWM2
#define SQ_PWMT_2_ENABLE		1
#else
#define SQ_PWMT_2_ENABLE		0
#endif

#ifdef SQ_INTC_PWM3
#define SQ_PWMT_3_ENABLE		1
#else
#define SQ_PWMT_3_ENABLE		0
#endif

#ifdef	CONFIG_MSMV
#define	SQ_ADC_TEST	0
#define	SQ_APB0_ADC	0
#define SQ_INTC_ADC	0
#define SQ_PWMT_CURRENT_SENSE_TEST 1
#define	SQ_PWMT_CAPTURE_LOOP	0
#else
#define SQ_ADC_TEST	1
#define SQ_PWMT_CURRENT_SENSE_TEST 0
#define SQ_PWMT_CAPTURE_LOOP 1
#endif



