#include <test_item.h>
#include "dependency.h"

extern int sq_nand_flsh_parity_8bytes(int autotest);
extern int sq_nand_flsh_parity_15bytes(int autotest);
extern int sq_nand_flsh_parity_switch(int autotest);

struct test_item sq_nand_flsh_main_test_items[] = {
	{"ECC Parity length 8 bytes", 
	 sq_nand_flsh_parity_8bytes,
	 1,
	 SQ_NAND_PARITY_8B},
	{"ECC Parity length 15 bytes",
	 sq_nand_flsh_parity_15bytes,
	 1,
	 SQ_NAND_PARITY_15B},
	 {"ECC Parity length switch test",
	 sq_nand_flsh_parity_switch,
	 1,
	 SQ_NAND_PARITY_SWITCH}
};

struct test_item_container sq_nand_flsh_main_test_container = {
	.menu_name = "NAND Test Main Menu",
	.shell_name = "nand flash",
	.items = sq_nand_flsh_main_test_items,
	.test_item_size = sizeof(sq_nand_flsh_main_test_items)
};

extern int sq_nand_flsh_transfer_test(int autotest);
extern int sq_nand_flsh_ecc_test(int autotest);
extern int sq_nand_flsh_autoread_test(int autotest);
extern int sq_nand_flsh_check_syndrome_bit(int autotest);
extern int sq_nand_flsh_a2a_burst_type_test(int autotest);
extern int sq_nand_flsh_hdma_burst_type_test(int autotest);
extern int sq_nand_flsh_panther7_hdma_burst_type_test(int autotest);
extern int sq_nand_flsh_transfer_cache(int autotest);
extern int sq_nand_flsh_ecc_correct_test(int autotest);

struct test_item sq_nand_flsh_parity_test_items[] = {
	{"Data Access Test", 
	 sq_nand_flsh_transfer_test,
	 1,
	 1},
	{"ECC Function Test in ECC Test Mode",
	 sq_nand_flsh_ecc_test,
	 0,
	 1},
	{"Auto Read Function Test",
	 sq_nand_flsh_autoread_test,
	 0,
	 1},
	{"Syndrome Bit Indication Test",
	 sq_nand_flsh_check_syndrome_bit,
	 0,
	 0},
	{"Data Access Test with A2A",
	 sq_nand_flsh_a2a_burst_type_test,
	 1,
	 SQ_NAND_SWDMA_A2A_TEST},
	{"Data Access Test with HDMA",
	 sq_nand_flsh_hdma_burst_type_test,
	 1,
	 SQ_NAND_SWDMA_HDMA_TEST},
	{"Data Access Test with Panther7 HDMA",
	 sq_nand_flsh_panther7_hdma_burst_type_test,
	 1,
	 SQ_NAND_SWDMA_PANTHER7_HDMA_TEST},
#ifdef CONFIG_CACHE
	{"Data Access Test with Cache Controller",
	 sq_nand_flsh_transfer_cache,
	 1,
	 1
	},
#endif
	{"ECC correct test",
	 sq_nand_flsh_ecc_correct_test,
	 1,
	 1
	}
};

struct test_item_container sq_nand_flsh_parity_test_container = {
	.menu_name = "NAND Test ECC Parity Menu",
	.shell_name = "nand flash",
	.items = sq_nand_flsh_parity_test_items,
	.test_item_size = sizeof(sq_nand_flsh_parity_test_items)
};
	
extern int sq_nand_flsh_transfer_ecc(int autotest);
extern int sq_nand_flsh_transfer_ecc_cached(int autotest);
extern int sq_nand_flsh_transfer_raw(int autotest);
extern int sq_nand_flsh_transfer_copyback(int autotest);

struct test_item sq_nand_flsh_transfer_test_items[] = {
	{"Data Access with ECC Code",
	 sq_nand_flsh_transfer_ecc,
	 1,
	 1},
	{"Data Access with ECC Code using Cache",
	 sq_nand_flsh_transfer_ecc_cached,
	 0,
	 SQ_NAND_TX_ECC_CACHED_TEST},
	{"Raw Data Access without ECC Code",
	 sq_nand_flsh_transfer_raw,
	 1,
	 1},
	{"Data Access using Copy-Back",
	 sq_nand_flsh_transfer_copyback,
	 0,
	 SQ_NAND_TX_COPYBACK},
};

struct test_item_container sq_nand_flsh_transfer_test_container = {
	.menu_name = "NAND Read/Write Test Menu",
	.shell_name = "nand",
	.items = sq_nand_flsh_transfer_test_items,
	.test_item_size = sizeof(sq_nand_flsh_transfer_test_items)
};

extern int sq_nand_flsh_ecc_error_0(int autotest);
extern int sq_nand_flsh_ecc_error_1(int autotest);
extern int sq_nand_flsh_ecc_error_2(int autotest);
extern int sq_nand_flsh_ecc_error_3(int autotest);
extern int sq_nand_flsh_ecc_error_4(int autotest);
extern int sq_nand_flsh_ecc_error_5(int autotest);
extern int sq_nand_flsh_ecc_error_6(int autotest);
extern int sq_nand_flsh_ecc_error_7(int autotest);

struct test_item sq_nand_flsh_ecc8_test_items[] = {
	{"0 error",
	 sq_nand_flsh_ecc_error_0,
	 0,
	 1},
	{"1 error",
	 sq_nand_flsh_ecc_error_1,
	 0,
	 1},
	{"2 errors",
	 sq_nand_flsh_ecc_error_2,
	 0,
	 1},
	{"3 errors",
	 sq_nand_flsh_ecc_error_3,
	 0,
	 1},
	{"4 errors",
	 sq_nand_flsh_ecc_error_4,
	 0,
	 1}
};

struct test_item_container sq_nand_flsh_ecc8_test_container = {
	.menu_name = "NAND Ecc 8 Test Menu",
	.shell_name = "nand",
	.items = sq_nand_flsh_ecc8_test_items,
	.test_item_size = sizeof(sq_nand_flsh_ecc8_test_items)
};

struct test_item sq_nand_flsh_ecc15_test_items[] = {
	{"0 error",
	 sq_nand_flsh_ecc_error_0,
	 0,
	 1},
	{"1 error",
	 sq_nand_flsh_ecc_error_1,
	 0,
	 1},
	{"2 errors",
	 sq_nand_flsh_ecc_error_2,
	 0,
	 1},
	{"3 errors",
	 sq_nand_flsh_ecc_error_3,
	 0,
	 1},
	{"4 errors",
	 sq_nand_flsh_ecc_error_4,
	 0,
	 1},
	{"5 errors",
	 sq_nand_flsh_ecc_error_5,
	 0,
	 1},
	{"6 errors",
	 sq_nand_flsh_ecc_error_6,
	 0,
	 1},
	{"7 errors",
	 sq_nand_flsh_ecc_error_7,
	 0,
	 1},
};

struct test_item_container sq_nand_flsh_ecc15_test_container = {
	.menu_name = "NAND Ecc 15 Test Menu",
	.shell_name = "nand",
	.items = sq_nand_flsh_ecc15_test_items,
	.test_item_size = sizeof(sq_nand_flsh_ecc15_test_items)
};

extern int sq_nand_flsh_autoread_0(int autotest);
extern int sq_nand_flsh_autoread_2048(int autotest);
extern int sq_nand_flsh_autoread_4096(int autotest);
extern int sq_nand_flsh_autoread_6144(int autotest);
extern int sq_nand_flsh_autoread_erase(int autotest);
extern int sq_nand_flsh_autoread_compare(int autotest);

struct test_item sq_nand_flsh_autoread_test_items[] = {
	{"Program 2K bytes data at 0",
	 sq_nand_flsh_autoread_0,
	 0,
	 1},
	{"Program 2K bytes data at 2048",
	 sq_nand_flsh_autoread_2048,
	 0,
	 1},
	{"Program 2K bytes data at 4096",
	 sq_nand_flsh_autoread_4096,
	 0,
	 1},
	{"Program 2K bytes data at 6144",
	 sq_nand_flsh_autoread_6144,
	 0,
	 1},
	{"Erase the 0th block",
	 sq_nand_flsh_autoread_erase,
	 0,
	 1},
	{"Compare the data written last time",
	 sq_nand_flsh_autoread_compare,
	 0,
	 1},
};

struct test_item_container sq_nand_flsh_autoread_test_container = {
	.menu_name = "NAND Auto-read Test Menu",
	.shell_name = "nand",
	.items = sq_nand_flsh_autoread_test_items,
	.test_item_size = sizeof(sq_nand_flsh_autoread_test_items)
};

extern int sq_nand_flsh_swdma_burst_type_single(int autotest);
extern int sq_nand_flsh_swdma_burst_type_incr4(int autotest);
extern int sq_nand_flsh_swdma_burst_type_incr8(int autotest);
extern int sq_nand_flsh_swdma_burst_type_incr16(int autotest);

struct test_item sq_nand_flsh_swdma_burst_type_test_items[] = {
	{"Single",
	 sq_nand_flsh_swdma_burst_type_single,
	 1,
	 1},
	{"INCR4",
	 sq_nand_flsh_swdma_burst_type_incr4,
	 1,
	 1},
	{"INCR8",
	 sq_nand_flsh_swdma_burst_type_incr8,
	 1,
	 1},
	{"INCR16",
	 sq_nand_flsh_swdma_burst_type_incr16,
	 1,
	 1}
};

struct test_item_container sq_nand_flsh_swdma_burst_type_test_container = {
	.menu_name = "NAND Panther7 Software DMA Burst Type Test Menu",
	.shell_name = "nand",
	.items = sq_nand_flsh_swdma_burst_type_test_items,
	.test_item_size = sizeof(sq_nand_flsh_swdma_burst_type_test_items)
};

extern int sq_nand_flsh_parity_switch_8to8(int autotest);
extern int sq_nand_flsh_parity_switch_8to15(int autotest);
extern int sq_nand_flsh_parity_switch_15to8(int autotest);
extern int sq_nand_flsh_parity_switch_15to15(int autotest);

struct test_item sq_nand_flsh_parity_switch_test_items[] = {
	{"Parity Length 8 to 8",
	 sq_nand_flsh_parity_switch_8to8,
	 1,
	 1},
	{"Parity Length 8 to 15",
	 sq_nand_flsh_parity_switch_8to15,
	 1,
	 1},
	{"Parity Length 15 to 8",
	 sq_nand_flsh_parity_switch_15to8,
	 1,
	 1},
	{"Parity Length 15 to 15",
	 sq_nand_flsh_parity_switch_15to15,
	 1,
	 1}
};

struct test_item_container sq_nand_flsh_parity_switch_container = {
	.menu_name = "Parity Switch Test Menu",
	.shell_name = "Parity Switch",
	.items = sq_nand_flsh_parity_switch_test_items,
	.test_item_size = sizeof(sq_nand_flsh_parity_switch_test_items)
};
