#include <test_item.h>
#include "dependency.h"

extern int sq_lcm_eink_test(int autotest);
extern int sq_lcm_t6963c_test(int autotest);
extern int inr_pc7230_line_in_test(int autotest);
extern int inr_pc7230_ipod_ducking_test(int autotest);
extern int inr_pc7230_ipod_and_line_in_detection_test(int autotest);

struct test_item sq_lcm_main_test_items[] = {
	{
		"Eink Test",
		sq_lcm_eink_test,
		1,
		LCM_EINK_TEST
	},
	{
		"T6963C Test",
		sq_lcm_t6963c_test,
		1,
		LCM_T6963C_TEST
	},
#ifdef CONFIG_INR_PC7230
	{
		"INR-PC7230 Line-in Test",
		inr_pc7230_line_in_test,
		0,
		1
	},
	{
		"INR-PC7230 iPod Ducking Test",
		inr_pc7230_ipod_ducking_test,
		0,
		1
	},
	{
		"INR-PC7230 iPod and Line-in Detection Test",
		inr_pc7230_ipod_and_line_in_detection_test,
		0,
		1
	},
#endif
};

struct test_item_container sq_lcm_main_container = {
     .menu_name = "Sq LCM Test Main Menu",
     .shell_name = "LCM",
     .items = sq_lcm_main_test_items,
     .test_item_size = sizeof(sq_lcm_main_test_items)
};

