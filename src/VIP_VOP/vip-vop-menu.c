#include <test_item.h>

#ifdef CONFIG_LDK3V21
#define HAVE_VIP 1
#else
#define HAVE_VIP 0
#endif

extern int sq_vip_to_vop_test(int autotest);
extern int sq_vop_to_vip_test(int autotest);
extern int sq_vip_to_lcd_test(int autotest);
extern int sq_vop_color_display_test(int autotest);

struct test_item sq_vip_vop_main_test_items[] = {
	{
		"VIP -> VOP",
		sq_vip_to_vop_test,
		1,
		HAVE_VIP
	},
	{
		"VOP -> VIP",
		sq_vop_to_vip_test,
		0,
		0
	},
	{
		"VIP -> LCD",
		sq_vip_to_lcd_test,
		1,
		HAVE_VIP
	},
	{
		"VOP Color Display",
		sq_vop_color_display_test,
		1,
		1
	}
};

struct test_item_container sq_vip_vop_main_container = {
     .menu_name = "VIP VOP Test Main Menu",
     .shell_name = "VIP_VOP",
     .items = sq_vip_vop_main_test_items,
     .test_item_size = sizeof(sq_vip_vop_main_test_items)
};

extern int sq_viop_ntsc_test(int autotest);
extern int sq_viop_pal_test(int autotest);

struct test_item sq_viop_format_test_items[] = {
	{
		"NTSC",
		sq_viop_ntsc_test,
		1,
		1
	},
	{
		"PAL",
		sq_viop_pal_test,
		1,
		1
	}
};

struct test_item_container sq_viop_format_container = {
     .menu_name = "VIP VOP Test Format Menu",
     .shell_name = "VIP_VOP",
     .items = sq_viop_format_test_items,
     .test_item_size = sizeof(sq_viop_format_test_items)
};

extern int sq_viop_cif_test(int autotest);
extern int sq_viop_qvga_test(int autotest);
extern int sq_viop_vga_test(int autotest);
extern int sq_viop_d1_test(int autotest);

struct test_item sq_viop_size_test_items[] = {
	{
		"CIF (352x288)",
		sq_viop_cif_test,
		1,
		1
	},
	{
		"QVGA (320x240)",
		sq_viop_qvga_test,
		1,
		1
	},
	{
		"VGA (640x480)",
		sq_viop_vga_test,
		1,
		1
	},
	{
		"D1 (720x480)",
		sq_viop_d1_test,
		1,
		1
	}
};

struct test_item_container sq_viop_size_container = {
     .menu_name = "VIP VOP Test Size Menu",
     .shell_name = "VIP_VOP",
     .items = sq_viop_size_test_items,
     .test_item_size = sizeof(sq_viop_size_test_items)
};
extern int sq_vip_422_test(int autotest);
extern int sq_vip_420_test(int autotest);

struct test_item sq_vip_42x_test_items[] = {
	{
		"4:2:0",
		sq_vip_420_test,
		1,
		1
	},
	{
		"4:2:2",
		sq_vip_422_test,
		1,
		1
	}
};

struct test_item_container sq_vip_42x_container = {
     .menu_name = "VIP VOP Test 42x Menu",
     .shell_name = "VIP_VOP",
     .items = sq_vip_42x_test_items,
     .test_item_size = sizeof(sq_vip_42x_test_items)
};
