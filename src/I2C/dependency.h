#include <platform.h>
#include <irqs.h>

#ifndef SQ_APB0_I2C0
#ifdef SQ_APB0_I2C
#define SQ_APB0_I2C0 SQ_APB0_I2C
#else
#error "I2C IP base address is not defined"
#endif
#endif

#ifndef SQ_APB0_I2C1
#define SQ_APB0_I2C1 SQ_APB0_I2C0
#define SQ_I2C1_TEST 0
#else
#define SQ_I2C1_TEST 1
#endif

#ifndef SQ_APB0_I2C2
#define SQ_APB0_I2C2 SQ_APB0_I2C0
#define SQ_I2C2_TEST 0
#else
#define SQ_I2C2_TEST 1
#endif

#ifndef SQ_INTC_I2C0
#ifdef SQ_INTC_I2C
#define SQ_INTC_I2C0 SQ_INTC_I2C
#else
#error "I2C IRQ is not defined"
#endif
#endif

#ifndef SQ_INTC_I2C1
#define SQ_INTC_I2C1 SQ_INTC_I2C0
#endif

#ifndef SQ_INTC_I2C2
#define SQ_INTC_I2C2 SQ_INTC_I2C0
#endif

#if defined(CONFIG_PC7210) || defined(CONFIG_PC9002)
#define SQ_I2C_AT24C02B_EEPROM_TEST 0
#else
#define SQ_I2C_AT24C02B_EEPROM_TEST 1
#endif

#if defined(CONFIG_MSMV)
#define SQ_I2C_TPS62353_TEST 1

#define TPS62353_IP0	1
#define TPS62353_IP1	1
#define TPS62353_IP2	1

#define TPS62353_TYPE0	0		//0:VSEL PIN LOW (Change valtage from register VSEL0)	: default valtage 1.0V
#define TPS62353_TYPE1	0
#define TPS62353_TYPE2	1		//1:VSEL PIN HIGH (Change valtage from register VSEL1)	: default valtage 1.2V
#else
#define SQ_I2C_TPS62353_TEST 0

#define TPS62353_IP0	0	
#define TPS62353_IP1	0
#define TPS62353_IP2	0

#define TPS62353_TYPE0	0
#define TPS62353_TYPE1	0
#define TPS62353_TYPE2	0
#endif


