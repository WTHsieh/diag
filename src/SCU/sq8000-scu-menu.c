#include <test_item.h>

extern int sq_sleep_mode_test(int autotest);
extern int sq_stop_mode_test(int autotest);

struct test_item sq_mode_main_test_items[] = {
	{
		"Sleep Mode Test",
		sq_sleep_mode_test,
		1,
		1
	},
	{
		"Stop Mode Test",
		sq_stop_mode_test,
		0,
		1
	}
};

struct test_item_container sq_mode_main_container = {
     .menu_name = "Sq Sleep and Stop Mode Test Main Menu",
     .shell_name = "Mode",
     .items = sq_mode_main_test_items,
     .test_item_size = sizeof(sq_mode_main_test_items)
};


extern int sq_sleep_with_irq_test(int autotest);
extern int sq_sleep_with_fiq_test(int autotest);

struct test_item sq_sleep_main_test_items[] = {
	{
		"Wake Up with IRQ",
		sq_sleep_with_irq_test,
		1,
		1
	},
	{
		"Wake Up with FIQ",
		sq_sleep_with_fiq_test,
		0,
		1
	}
};

struct test_item_container sq_sleep_main_container = {
     .menu_name = "Sq Sleep Mode Test Main Menu",
     .shell_name = "Sleep",
     .items = sq_sleep_main_test_items,
     .test_item_size = sizeof(sq_sleep_main_test_items)
};


extern int sq_stop_with_irq_test(int autotest);
extern int sq_stop_with_fiq_test(int autotest);

struct test_item sq_stop_main_test_items[] = {
	{
		"Wake Up with IRQ",
		sq_stop_with_irq_test,
		1,
		1
	},
	{
		"Wake Up with FIQ",
		sq_stop_with_fiq_test,
		0,
		1
	}
};

struct test_item_container sq_stop_main_container = {
     .menu_name = "Sq Stop Mode Test Main Menu",
     .shell_name = "Stop",
     .items = sq_stop_main_test_items,
     .test_item_size = sizeof(sq_stop_main_test_items)
};
