#include <test_item.h>
#include "dependency.h"


extern int sq_voltage_test(int autotest);
extern int sq_other_test(int autotest);

struct test_item sq_tps_main_items[] = {
	{"Voltage Test",
	 sq_voltage_test,
	 1,
	 1},
	{"Other Test",
	 sq_other_test,
	 1,
	 1},
};

struct test_item_container sq_tps62353_main_container = {
	.menu_name = "TPS Main Test ",
	.shell_name = "TPS",
	.items = sq_tps_main_items,
	.test_item_size = sizeof(sq_tps_main_items)
};


extern int sq_voltage_increase (int autotest);
extern int sq_voltage_decrease (int autotest);
extern int sq_voltage_set (int autotest);
extern int sq_voltage_show (int autotest);

struct test_item sq_tps_voltage_items[] = {
	{"Voltage Increase (+4)",
	 sq_voltage_increase,
	 1,
	 1},
	{"Voltage Decrease (-4)",
	 sq_voltage_decrease,
	 1,
	 1},
	{"Voltage set",
	 sq_voltage_set,
	 0,
	 1},
	{"Voltage show",
	 sq_voltage_show,
	 0,
	 1},
};

struct test_item_container sq_tps62353_voltage_container = {
	.menu_name = "TPS Voltage Test ",
	.shell_name = "Voltage",
	.items = sq_tps_voltage_items,
	.test_item_size = sizeof(sq_tps_voltage_items)
};

extern int sq_regs_vsel0_set (int autotest);
extern int sq_regs_vsel1_set (int autotest);
extern int sq_regs_ctrl1_set (int autotest);
extern int sq_regs_ctrl2_set (int autotest);

struct test_item sq_tps_regs_items[] = {
	{"VSEL0",
	 sq_regs_vsel0_set,
	 0,
	 1},
	{"VSEL1",
	 sq_regs_vsel1_set,
	 0,
	 1},
	{"CONTROL1",
	 sq_regs_ctrl1_set,
	 0,
	 1},
	{"CONTROL2",
	 sq_regs_ctrl2_set,
	 0,
	 1},
};

struct test_item_container sq_tps62353_regs_container = {
	.menu_name = "TPS Regs Test ",
	.shell_name = "Regs",
	.items = sq_tps_regs_items,
	.test_item_size = sizeof(sq_tps_regs_items)
};





