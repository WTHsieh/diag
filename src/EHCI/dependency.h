#include <platform.h>
#include <irqs.h>


#ifndef SQ_AHB0_UHC0
#ifdef SQ_AHB0_UHC
#define SQ_AHB0_UHC0 SQ_AHB0_UHC
#else
#error "EHCI IP base address is not defined"
#endif
#endif

#ifndef SQ_AHB0_UHC1
#define SQ_AHB0_UHC1 SQ_AHB0_UHC0
#define SQ_EHCI1_TEST 0
#else
#define SQ_EHCI1_TEST 1
#endif


#ifndef SQ_INTC_UHC0
#ifdef SQ_INTC_UHC
#define SQ_INTC_UHC0 SQ_INTC_UHC
#else
#error "EHCI IRQ is not defined"
#endif
#endif

#ifndef SQ_INTC_UHC1
#define SQ_INTC_UHC1 SQ_INTC_UHC0
#endif


