#include <platform.h>
#include <irqs.h>

#ifndef SQ_APB0_SPI0
#ifdef SQ_APB0_SPI
#define SQ_APB0_SPI0 SQ_APB0_SPI
#else
#error "SPI IP base is not defined"
#endif
#endif

#ifndef SQ_APB0_SPI1
#define SQ_APB0_SPI1 SQ_APB0_SPI0
#define SQ_SPI_IP_1_TEST 0
#else
#define SQ_SPI_IP_1_TEST 1
#endif

#ifndef SQ_INTC_SPI0
#ifdef SQ_INTC_SPI
#define SQ_INTC_SPI0 SQ_INTC_SPI
#else
#error "SPI IRQ is not defined"
#endif
#endif

#ifndef SQ_INTC_SPI1
#define SQ_INTC_SPI1 SQ_INTC_SPI0
#endif

//20080201 jsho modify, for menu control
#ifdef CONFIG_PANTHER7_HDMA
#if defined(CONFIG_CDK) || defined(CONFIG_PC9002) || defined(CONFIG_SCDK) || defined(CONFIG_MSMV)
#define SQ_SPI_HWDMA_PANTHER7_HDMA_TEST 0
#else
#define SQ_SPI_HWDMA_PANTHER7_HDMA_TEST 1
#endif
#else
#define SQ_SPI_HWDMA_PANTHER7_HDMA_TEST 0
#endif

//20080201 jsho modify, for menu control
#if defined(CONFIG_CDK) || defined(CONFIG_PDK) || defined(CONFIG_SCDK)
#define SQ_SPI_EEPROM_0_TEST 1 
#define SQ_SPI_EEPROM_1_TEST 0
#define SQ_SPI_MARVEL_WIFI_0_TEST 0
#define SQ_SPI_MARVEL_WIFI_1_TEST 0
#define SQ_SPI_TSC2000_0_TEST 0
#define SQ_SPI_TSC2000_1_TEST 1
#elif defined(CONFIG_PC9002) || defined(CONFIG_SQ8000)
#define SQ_SPI_EEPROM_0_TEST 0 
#define SQ_SPI_EEPROM_1_TEST 0
#define SQ_SPI_MARVEL_WIFI_0_TEST 1
#define SQ_SPI_MARVEL_WIFI_1_TEST 0
#define SQ_SPI_TSC2000_0_TEST 0
#define SQ_SPI_TSC2000_1_TEST 1
#elif defined(CONFIG_PC7210)
#define SQ_SPI_EEPROM_0_TEST 0
#define SQ_SPI_EEPROM_1_TEST 0
#define SQ_SPI_MARVEL_WIFI_0_TEST 1
#define SQ_SPI_MARVEL_WIFI_1_TEST 0
#define SQ_SPI_TSC2000_0_TEST 1
#define SQ_SPI_TSC2000_1_TEST 0
#elif defined(CONFIG_MSMV)
#define SQ_SPI_EEPROM_0_TEST 0
#define SQ_SPI_EEPROM_1_TEST 0
#define SQ_SPI_MARVEL_WIFI_0_TEST 0
#define SQ_SPI_MARVEL_WIFI_1_TEST 0
#define SQ_SPI_TSC2000_0_TEST 0
#define SQ_SPI_TSC2000_1_TEST 0
#else
#define SQ_SPI_EEPROM_0_TEST 1 
#define SQ_SPI_EEPROM_1_TEST 0
#define SQ_SPI_MARVEL_WIFI_0_TEST 0
#define SQ_SPI_MARVEL_WIFI_1_TEST 0
#define SQ_SPI_TSC2000_0_TEST 0
#define SQ_SPI_TSC2000_1_TEST 0
#endif

#ifdef CONFIG_SQ8000
#define SPI_SLAVE_TEST 1
#else
#define SPI_SLAVE_TEST 0
#endif
