#include <platform.h>
#include <irqs.h>

#ifndef SQ_AHB0_NAND
#define SQ_AHB0_NAND 0x1e6e0000//0x19CA0000//0x1E6E0000
#endif

#ifndef SQ_INTC_NAND
#define SQ_INTC_NAND 11//19//12
#endif

#ifndef SQ_AHB0_NAND0
#ifdef SQ_AHB0_NAND
#define SQ_AHB0_NAND0 SQ_AHB0_NAND
#else
#error "NAND IP base address is not defined"
#endif
#endif

#ifndef SQ_INTC_NAND0
#ifdef SQ_INTC_NAND
#define SQ_INTC_NAND0 SQ_INTC_NAND
#else
#error "NAND IRQ is not defined"
#endif
#endif

/*	only support 15B (older) or support 15B/8B function	*/ 
#if defined(CONFIG_SQ8000)
#define CONFIG_NAND_TWO_ECC_MODEL
#endif

#ifdef CONFIG_NAND_TWO_ECC_MODEL
#define SQ_NAND_PARITY_8B		1
#define SQ_NAND_PARITY_15B		1
#define SQ_NAND_PARITY_SWITCH	1
#else
#define SQ_NAND_PARITY_8B		0
#define SQ_NAND_PARITY_15B		1
#define SQ_NAND_PARITY_SWITCH	0
#endif

/* Configure the test menu */
#ifdef CONFIG_A2ADMA
#define SQ_NAND_SWDMA_A2A_TEST 1
#else
#define SQ_NAND_SWDMA_A2A_TEST 0
#endif

#ifdef CONFIG_HDMA
#define SQ_NAND_SWDMA_HDMA_TEST 1
#else
#define SQ_NAND_SWDMA_HDMA_TEST 0
#endif

#if defined(CONFIG_PANTHER7_HDMA) && (defined(CONFIG_PDK) || defined(CONFIG_PC7210) || defined(CONFIG_SQ8000))
#define SQ_NAND_SWDMA_PANTHER7_HDMA_TEST 1
#else
#define SQ_NAND_SWDMA_PANTHER7_HDMA_TEST 0
#endif

#define SQ_NAND_TX_ECC_CACHED_TEST 1
#define SQ_NAND_TX_COPYBACK 1
