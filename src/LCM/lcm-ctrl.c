#include <global.h>
#include <test_item.h>


extern struct test_item_container sq_lcm_main_container;

extern int
lcm_test(int autotest)
{
	int ret = 0;

	ret = test_item_ctrl(&sq_lcm_main_container, autotest);
	
	return ret;
}

