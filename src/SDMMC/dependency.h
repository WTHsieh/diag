#include <platform.h>
#include <irqs.h>

#ifndef SQ_APB0_SDMMC0
#ifdef SQ_APB0_SDMMC
#define SQ_APB0_SDMMC0 SQ_APB0_SDMMC
#else
#error "SD/MMC IP base address is not defined"
#endif
#endif

#ifndef SQ_INTC_SDMMC0
#ifdef SQ_INTC_SDMMC
#define SQ_INTC_SDMMC0 SQ_INTC_SDMMC
#else
#error "SD/MMC IRQ is not defined"
#endif
#endif

#ifdef CONFIG_A2ADMA
#define SQ_SDMMC_SWDMA_A2A_TEST 1
#else
#define SQ_SDMMC_SWDMA_A2A_TEST 0
#endif

#ifdef CONFIG_HDMA
#define SQ_SDMMC_SWDMA_HDMA_TEST 1
#else
#define SQ_SDMMC_SWDMA_HDMA_TEST 0
#endif

#if defined(CONFIG_PANTHER7_HDMA) && (defined(CONFIG_PDK) || defined(CONFIG_PC7210) || defined(CONFIG_PC9002) || defined(CONFIG_SCDK))
#define SQ_SDMMC_SWDMA_PANTHER7_HDMA_TEST 1
#else
#define SQ_SDMMC_SWDMA_PANTHER7_HDMA_TEST 0
#endif

#if defined(CONFIG_CDK) || defined(CONFIG_SQ8000) || defined(CONFIG_PC9002) || defined(CONFIG_SCDK)
#define SQ_SDMMC_MUTLIPLE_PARTIAL_READ_TEST 0
#else
#define SQ_SDMMC_MUTLIPLE_PARTIAL_READ_TEST 0 
#endif
