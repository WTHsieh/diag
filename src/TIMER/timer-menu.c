#include <test_item.h>
#include "dependency.h"

extern int sq_timer_mode_0_test(int autotest);
extern int sq_timer_mode_1(int autotest);
extern int sq_timer_mode_2(int autotest);

struct test_item sq_timer_main_test_items[] = {
	{"Mode 0",
	 sq_timer_mode_0_test,
	 1,
	 1},
	{"Mode 1",
	 sq_timer_mode_1,
	 1,
	 SQ_TIMER_CHAIN_MODE_1_TEST},
	{"Mode 2",
	 sq_timer_mode_2,
	 1,
	 SQ_TIMER_CHAIN_MODE_2_TEST},
};

struct test_item_container sq_timer_main_test_container = {
	.menu_name = "SQ Timer Test Main Menu",
	.shell_name = "sq_timer",
	.items = sq_timer_main_test_items,
	.test_item_size = sizeof(sq_timer_main_test_items),
};

extern int sq_timer_mode_0_timer_0_test(int autotest);
extern int sq_timer_mode_0_timer_1_test(int autotest);
extern int sq_timer_mode_0_timer_2_test(int autotest);

struct test_item sq_timer_mode_0_timer_test_items[] = {
	{"Timer 0",
	 sq_timer_mode_0_timer_0_test,
	 1,
	 1},
	{"Timer 1",
	 sq_timer_mode_0_timer_1_test,
	 1,
	 1},
	{"Timer 2",
	 sq_timer_mode_0_timer_2_test,
	 1,
	 1},
};

struct test_item_container sq_timer_mode_0_timer_test_container = {
	.menu_name = "SQ Timer Mode 0 Timer Test Menu",
	.shell_name = "sq_timer",
	.items = sq_timer_mode_0_timer_test_items,
	.test_item_size = sizeof(sq_timer_mode_0_timer_test_items),
};

extern int sq_timer_cnt_free_running(int autotest);
extern int sq_timer_cnt_periodical(int autotest);

struct test_item sq_timer_cnt_mode_test_items[] = {
	{"Free-Running",
	 sq_timer_cnt_free_running,
	 1,
	 1},
	{"Periodical",
	 sq_timer_cnt_periodical,
	 1,
	 1},
};

struct test_item_container sq_timer_cnt_mode_test_container = {
	.menu_name = "SQ Timer Counting Mode Test Menu",
	.shell_name = "sq_timer",
	.items = sq_timer_cnt_mode_test_items,
	.test_item_size = sizeof(sq_timer_cnt_mode_test_items),
};

