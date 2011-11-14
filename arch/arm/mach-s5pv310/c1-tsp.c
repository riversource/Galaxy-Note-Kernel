#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/serial_core.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/fixed.h>
#include <linux/i2c.h>
#include <linux/i2c-gpio.h>
#include <plat/devs.h>
#include <plat/gpio-cfg.h>
#include <plat/regs-serial.h>
#include <plat/csis.h>
#include <plat/fimc.h>
#include <mach/gpio.h>
#include "c1.h"

#if defined(CONFIG_TOUCHSCREEN_MXT540E)
#include <linux/i2c/mxt540e.h>
#else
#include <linux/i2c/mxt224_u1.h>
#endif

#if defined(CONFIG_TOUCHSCREEN_QT602240) || defined(CONFIG_TOUCHSCREEN_MXT768E)
static void mxt224_power_on(void)
{
	s3c_gpio_cfgpin(GPIO_TSP_LDO_ON, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_TSP_LDO_ON, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_TSP_LDO_ON, GPIO_LEVEL_HIGH);
	mdelay(70);
	s3c_gpio_setpull(GPIO_TSP_INT, S3C_GPIO_PULL_NONE);
	s3c_gpio_cfgpin(GPIO_TSP_INT, S3C_GPIO_SFN(0xf));
	/*mdelay(40); */
	/* printk("mxt224_power_on is finished\n"); */
}

static void mxt224_power_off(void)
{
	s3c_gpio_cfgpin(GPIO_TSP_INT, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_TSP_INT, S3C_GPIO_PULL_DOWN);

	s3c_gpio_cfgpin(GPIO_TSP_LDO_ON, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_TSP_LDO_ON, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_TSP_LDO_ON, GPIO_LEVEL_LOW);
	/* printk("mxt224_power_off is finished\n"); */
}

#if defined(CONFIG_MACH_C1Q1_REV02) || defined(CONFIG_MACH_Q1_REV00) || defined(CONFIG_MACH_Q1_REV02)
#ifdef CONFIG_TOUCHSCREEN_MXT768E
static u8 t7_config[] = {GEN_POWERCONFIG_T7,
				64, 255, 20};
static u8 t8_config[] = {GEN_ACQUISITIONCONFIG_T8,
				64, 0, 20, 20, 0, 0, 20, 0, 50, 25};
static u8 t9_config[] = {TOUCH_MULTITOUCHSCREEN_T9,
				139, 0, 0, 16, 26, 0, 208, 50, 2, 6, 0, 5, 1,
				0, MXT224_MAX_MT_FINGERS, 10, 10, 5, 255, 3,
				255, 3, 0, 0, 0, 0, 136, 60, 136, 40, 40, 15, 0, 0};

static u8 t15_config[] = {TOUCH_KEYARRAY_T15,
				1, 16, 26, 1, 6, 0, 64, 255, 3, 0, 0};

static u8 t18_config[] = {SPT_COMCONFIG_T18,
				0, 0};

static u8 t40_config[] = {PROCI_GRIPSUPPRESSION_T40,
				0, 0, 0, 0, 0};

static u8 t42_config[] = {PROCI_TOUCHSUPPRESSION_T42,
				0, 0, 0, 0, 0, 0, 0, 0};

static u8 t43_config[] = {SPT_DIGITIZER_T43,
				0, 0, 0, 0};

static u8 t48_config[] = {PROCG_NOISESUPPRESSION_T48,
				1, 0, 65, 0, 12, 24, 36, 48, 8, 16, 11, 40, 0, 0,
				0, 0, 0};


static u8 t46_config[] = {SPT_CTECONFIG_T46,
				0, 0, 8, 32, 0, 0, 0, 0};
static u8 end_config[] = {RESERVED_T255};

static const u8 *mxt224_config[] = {
	t7_config,
	t8_config,
	t9_config,
	t15_config,
	t18_config,
	t40_config,
	t42_config,
	t43_config,
	t46_config,
	t48_config,
	end_config,
};
#else
static u8 t7_config[] = {GEN_POWERCONFIG_T7,
				64, 255, 20};
static u8 t8_config[] = {GEN_ACQUISITIONCONFIG_T8,
				36, 0, 20, 20, 0, 0, 10, 10, 50, 25};
static u8 t9_config[] = {TOUCH_MULTITOUCHSCREEN_T9,
				139, 0, 0, 18, 11, 0, 16, MXT224_THRESHOLD, 2, 1, 0, 3, 1,
				0, MXT224_MAX_MT_FINGERS, 10, 10, 10, 31, 3,
				223, 1, 0, 0, 0, 0, 0, 0, 0, 0, 10, 5, 5, 5};

static u8 t15_config[] = {TOUCH_KEYARRAY_T15,
				131, 16, 11, 2, 1, 0, 0, 40, 3, 0, 0};

static u8 t18_config[] = {SPT_COMCONFIG_T18,
				0, 0};

static u8 t48_config[] = {PROCG_NOISESUPPRESSION_T48,
				3, 0, 2, 10, 6, 12, 18, 24, 20, 30, 0, 0, 0, 0,
				0, 0, 0};


static u8 t46_config[] = {SPT_CTECONFIG_T46,
				0, 2, 0, 0, 0, 0, 0};
static u8 end_config[] = {RESERVED_T255};

static const u8 *mxt224_config[] = {
	t7_config,
	t8_config,
	t9_config,
	t15_config,
	t18_config,
	t46_config,
	t48_config,
	end_config,
};
#endif
#else
/*
	Configuration for MXT224
*/
static u8 t7_config[] = {GEN_POWERCONFIG_T7,
				48,		/* IDLEACQINT */
				255,	/* ACTVACQINT */
				25		/* ACTV2IDLETO: 25 * 200ms = 5s */};
static u8 t8_config[] = {GEN_ACQUISITIONCONFIG_T8,
				10, 0, 5, 1, 0, 0, 9, 30};/*byte 3: 0*/
static u8 t9_config[] = {TOUCH_MULTITOUCHSCREEN_T9,
				131, 0, 0, 19, 11, 0, 32, MXT224_THRESHOLD, 2, 1,
				0,
				15,		/* MOVHYSTI */
				1, 11, MXT224_MAX_MT_FINGERS, 5, 40, 10, 31, 3,
				223, 1, 0, 0, 0, 0, 143, 55, 143, 90, 18};

static u8 t18_config[] = {SPT_COMCONFIG_T18,
				0, 1};
static u8 t20_config[] = {PROCI_GRIPFACESUPPRESSION_T20,
				7, 0, 0, 0, 0, 0, 0, 30, 20, 4, 15, 10};
static u8 t22_config[] = {PROCG_NOISESUPPRESSION_T22,
				143, 0, 0, 0, 0, 0, 0, 3, 30, 0, 0,  29, 34, 39,
				49, 58, 3};
static u8 t28_config[] = {SPT_CTECONFIG_T28,
				0, 0, 3, 16, 19, 60};
static u8 end_config[] = {RESERVED_T255};

static const u8 *mxt224_config[] = {
	t7_config,
	t8_config,
	t9_config,
	t18_config,
	t20_config,
	t22_config,
	t28_config,
	end_config,
};

/*
	Configuration for MXT224-E
*/

#if defined(CONFIG_TARGET_LOCALE_NAATT)
static u8 t7_config_e[] = {GEN_POWERCONFIG_T7,
				48, 255, 25};
static u8 t8_config_e[] = {GEN_ACQUISITIONCONFIG_T8,
				27, 0, 5, 1, 0, 0, 8, 8, 8, 180};

/* MXT224E_0V5_CONFIG */
/* NEXTTCHDI added */
static u8 t9_config_e[] = {TOUCH_MULTITOUCHSCREEN_T9,
				139, 0, 0, 19, 11, 0, 32, 50, 2, 1,
				10, 3, 1, 11, MXT224_MAX_MT_FINGERS, 5, 40, 10, 31, 3,
				223, 1, 10, 10, 10, 10, 143, 40, 143, 80,
				18, 15, 50, 50, 2};

static u8 t15_config_e[] = {TOUCH_KEYARRAY_T15,
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static u8 t18_config_e[] = {SPT_COMCONFIG_T18,
				0, 0};

static u8 t23_config_e[] = {TOUCH_PROXIMITY_T23,
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static u8 t25_config_e[] = {SPT_SELFTEST_T25,
				0, 0, 188, 52, 124, 21, 188, 52, 124, 21, 0, 0, 0, 0};

static u8 t40_config_e[] = {PROCI_GRIPSUPPRESSION_T40,
				0, 0, 0, 0, 0};

static u8 t42_config_e[] = {PROCI_TOUCHSUPPRESSION_T42,
				0, 32, 120, 100, 0, 0, 0, 0};

static u8 t46_config_e[] = {SPT_CTECONFIG_T46,
				0, 3, 16, 35, 0, 0, 1, 0};

static u8 t47_config_e[] = {PROCI_STYLUS_T47,
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

/*MXT224E_0V5_CONFIG */
static u8 t48_config_e[] = {PROCG_NOISESUPPRESSION_T48,
	3, 4, 72, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	6, 6, 0, 0, 100, 4, 64, 10, 0, 20, 5, 0, 38, 0, 5,
	0, 0, 0, 0, 0, 0, 32, 50, 2, 3, 1, 11, 10, 5, 40, 10, 10,
	10, 10, 143, 40, 143, 80, 18, 15, 2 };

static u8 t48_config_e_ta[] = {PROCG_NOISESUPPRESSION_T48,
	1, 4, 88, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	6, 6, 0, 0, 100, 4, 64, 10, 0, 20, 5, 0, 38, 0, 20,
	0, 0, 0, 0, 0, 0, 16, 70, 2, 5, 2, 46, 10, 5, 40, 10, 0,
	10, 10, 143, 40, 143, 80, 18, 15, 2 };

#elif defined(CONFIG_MACH_C1_NA_SPR_EPIC2_REV00)
static u8 t7_config_e[] = {GEN_POWERCONFIG_T7,
				48, 255, 15};

static u8 t8_config_e[] = {GEN_ACQUISITIONCONFIG_T8,
				27, 0, 5, 1, 0, 0, 4, 35, 40, 55};

static u8 t9_config_e[] = {TOUCH_MULTITOUCHSCREEN_T9,
				131, 0, 0, 19, 11, 0, 32, 50, 2, 7,
				10, 3, 1, 46, MXT224_MAX_MT_FINGERS, 5, 40, 10, 31, 3,
				223, 1, 10, 10, 10, 10, 143, 40, 143, 80,
				18, 15, 50, 50, 2};

static u8 t15_config_e[] = {TOUCH_KEYARRAY_T15,
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static u8 t18_config_e[] = {SPT_COMCONFIG_T18,
				0, 0};

static u8 t23_config_e[] = {TOUCH_PROXIMITY_T23,
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static u8 t25_config_e[] = {SPT_SELFTEST_T25,
				0, 0, 0, 0, 0, 0, 0, 0};

static u8 t40_config_e[] = {PROCI_GRIPSUPPRESSION_T40,
				0, 0, 0, 0, 0};

static u8 t42_config_e[] = {PROCI_TOUCHSUPPRESSION_T42,
				0, 32, 120, 100, 0, 0, 0, 0};

static u8 t46_config_e[] = {SPT_CTECONFIG_T46,
				0, 3, 16, 48, 0, 0, 1, 0, 0};

static u8 t47_config_e[] = {PROCI_STYLUS_T47,
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static u8 t48_config_e[] = {PROCG_NOISESUPPRESSION_T48,
				3, 4, 64, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 6, 6, 0, 0, 100, 4, 64,
				10, 0, 20, 5, 0, 38, 0, 5, 0, 0,
				0, 0, 0, 0, 32, 50, 2, 3, 1, 46,
				10, 5, 40, 10, 10, 10, 10, 143, 40, 143,
				80, 18, 15, 2};

static u8 t48_config_e_ta[] = {PROCG_NOISESUPPRESSION_T48,
				1, 4, 80, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 6, 6, 0, 0, 100, 4, 64,
				10, 0, 20, 5, 0, 38, 0, 20, 0, 0,
				0, 0, 0, 0, 16, 70, 2, 5, 2, 46,
				10, 5, 40, 10, 0, 10, 10, 143, 40, 143,
				80, 18, 15, 2};
#else

static u8 t7_config_e[] = {GEN_POWERCONFIG_T7,
				48,		/* IDLEACQINT */
				255,	/* ACTVACQINT */
				25		/* ACTV2IDLETO: 25 * 200ms = 5s */};
#ifdef CONFIG_TARGET_LOCALE_NA
static u8 t8_config_e[] = {GEN_ACQUISITIONCONFIG_T8,
				27, 0, 5, 1, 0, 0, 4, 35, 40, 55};
#else
static u8 t8_config_e[] = {GEN_ACQUISITIONCONFIG_T8,
				27, 0, 5, 1, 0, 0, 8, 8, 0, 0};
#endif
#if 1 /* MXT224E_0V5_CONFIG */
/* NEXTTCHDI added */
#ifdef CONFIG_TARGET_LOCALE_NA
static u8 t9_config_e[] = {TOUCH_MULTITOUCHSCREEN_T9,
				139, 0, 0, 19, 11, 0, 32, 50, 2, 1,
				10,
				10,		/* MOVHYSTI */
				1, 46, MXT224_MAX_MT_FINGERS, 5, 40, 10, 31, 3,
				223, 1, 10, 10, 10, 10, 143, 40, 143, 80,
				18, 15, 50, 50, 2};

#else
static u8 t9_config_e[] = {TOUCH_MULTITOUCHSCREEN_T9,
				131, 0, 0, 19, 11, 0, 32, 50, 2, 1,
				10,
				15,		/* MOVHYSTI */
				1, 11, MXT224_MAX_MT_FINGERS, 5, 40, 10, 31, 3,
				223, 1, 10, 10, 10, 10, 143, 40, 143, 80,
				18, 15, 50, 50, 2};
#endif
#else
static u8 t9_config_e[] = {TOUCH_MULTITOUCHSCREEN_T9,
				139, 0, 0, 19, 11, 0, 16, MXT224_THRESHOLD, 2, 1, 10, 3, 1,
				0, MXT224_MAX_MT_FINGERS, 10, 40, 10, 31, 3,
				223, 1, 10, 10, 10, 10, 143, 40, 143, 80, 18, 15, 50, 50};
#endif

static u8 t15_config_e[] = {TOUCH_KEYARRAY_T15,
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static u8 t18_config_e[] = {SPT_COMCONFIG_T18,
				0, 0};

static u8 t23_config_e[] = {TOUCH_PROXIMITY_T23,
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static u8 t25_config_e[] = {SPT_SELFTEST_T25,
				0, 0, 0, 0, 0, 0, 0, 0};

static u8 t40_config_e[] = {PROCI_GRIPSUPPRESSION_T40,
				0, 0, 0, 0, 0};

static u8 t42_config_e[] = {PROCI_TOUCHSUPPRESSION_T42,
				0, 0, 0, 0, 0, 0, 0, 0};

#ifdef CONFIG_TARGET_LOCALE_NA
static u8 t46_config_e[] = {SPT_CTECONFIG_T46,
				0, 3, 16, 45, 0, 0, 1, 0, 0};
#else
static u8 t46_config_e[] = {SPT_CTECONFIG_T46,
				0, 3, 16, 48, 0, 0, 1, 0, 0};
#endif
static u8 t47_config_e[] = {PROCI_STYLUS_T47,
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

#if 1 /*MXT224E_0V5_CONFIG */
#ifdef CONFIG_TARGET_LOCALE_NA
static u8 t48_config_e_ta[] = {PROCG_NOISESUPPRESSION_T48,
				1, 4, 0x50, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 6, 6, 0, 0, 100, 4, 64,
				10, 0, 20, 5, 0, 38, 0, 20, 0, 0,
				0, 0, 0, 0, 0, 40, 2,/*blen=0,threshold=50*/
				10,		/* MOVHYSTI */
				1, 15,
				10, 5, 40, 240, 245, 10, 10, 148, 50, 143,
				80, 18, 10, 2};
static u8 t48_config_e[] = {PROCG_NOISESUPPRESSION_T48,
				1, 4, 0x40, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 6, 6, 0, 0, 100, 4, 64,
				10, 0, 20, 5, 0, 38, 0, 5, 0, 0,  /*byte 27 original value 20*/
				0, 0, 0, 0, 32, 50, 2,
				10,
				1, 46,
				MXT224_MAX_MT_FINGERS, 5, 40, 10, 0, 10, 10, 143, 40, 143,
				80, 18, 15, 2};
#else
static u8 t48_config_e_ta[] = {PROCG_NOISESUPPRESSION_T48,
				1, 4, 0x50, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 6, 6, 0, 0, 100, 4, 64,
				10, 0, 20, 5, 0, 38, 0, 20, 0, 0,
				0, 0, 0, 0, 16, 70, 2,/*blen=0,threshold=50*/
				15,		/* MOVHYSTI */
				1, 46,
				10, 5, 40, 10, 0, 10, 10, 143, 40, 143,
				80, 18, 15, 2};

static u8 t48_config_e[] = {PROCG_NOISESUPPRESSION_T48,
				1, 4, 0x40, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 6, 6, 0, 0, 100, 4, 64,
				10, 0, 20, 5, 0, 38, 0, 5, 0, 0,  /*byte 27 original value 20*/
				0, 0, 0, 0, 32, MXT224E_THRESHOLD, 2,
				15,
				1, 11,
				MXT224_MAX_MT_FINGERS, 5, 40, 10, 10, 10, 10, 143, 40, 143,
				80, 18, 15, 2};
#endif
#else
/*static u8 t48_config_e[] = {PROCG_NOISESUPPRESSION_T48,
				1, 0, 40, 0, 0, 0, 0, 0, 0, 0, 0, 60, 31, 6,
				50, 64, 100};*/
#endif
#endif

static u8 end_config_e[] = {RESERVED_T255};

static const u8 *mxt224e_config[] = {
	t7_config_e,
	t8_config_e,
	t9_config_e,
	t15_config_e,
	t18_config_e,
	t23_config_e,
	t25_config_e,
	t40_config_e,
	t42_config_e,
	t46_config_e,
	t47_config_e,
	t48_config_e,
	end_config_e,
};

#endif

struct mxt224_platform_data mxt224_data = {
	.max_finger_touches = MXT224_MAX_MT_FINGERS,
	.gpio_read_done = GPIO_TSP_INT,
#if defined(CONFIG_MACH_C1Q1_REV02) || defined(CONFIG_MACH_Q1_REV00) || defined(CONFIG_MACH_Q1_REV02)
	.config = mxt224_config,
#else
	.config = mxt224_config,
	.config_e = mxt224e_config,
	.t48_ta_cfg = t48_config_e_ta,
#endif
	.min_x = 0,
#ifdef CONFIG_TOUCHSCREEN_MXT768E
	.max_x = 1023,
#else
	.max_x = 480,
#endif
	.min_y = 0,
#ifdef CONFIG_TOUCHSCREEN_MXT768E
	.max_y = 1023,
#else
	.max_y = 800,
#endif
	.min_z = 0,
	.max_z = 255,
	.min_w = 0,
	.max_w = 30,
	.power_on = mxt224_power_on,
	.power_off = mxt224_power_off,
	.register_cb = tsp_register_callback,
	.read_ta_status = tsp_read_ta_status,
};

/*
static struct qt602240_platform_data qt602240_platform_data = {
	.x_line		  = 19,
	.y_line		  = 11,
	.x_size		  = 800,
	.y_size		  = 480,
	.blen			  = 0x11,
	.threshold		  = 0x28,
	.voltage		  = 2800000,
	.orient		  = QT602240_DIAGONAL,
};
*/
#endif

#if defined(CONFIG_TOUCHSCREEN_MXT540E)
static void mxt540e_power_on(void)
{
	s3c_gpio_cfgpin(GPIO_TSP_LDO_ON, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_TSP_LDO_ON, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_TSP_LDO_ON, GPIO_LEVEL_HIGH);
	msleep(MXT540E_HW_RESET_TIME);
	s3c_gpio_setpull(GPIO_TSP_INT, S3C_GPIO_PULL_NONE);
	s3c_gpio_cfgpin(GPIO_TSP_INT, S3C_GPIO_SFN(0xf));
}

static void mxt540e_power_off(void)
{
	s3c_gpio_cfgpin(GPIO_TSP_INT, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_TSP_INT, S3C_GPIO_PULL_DOWN);

	s3c_gpio_cfgpin(GPIO_TSP_LDO_ON, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_TSP_LDO_ON, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_TSP_LDO_ON, GPIO_LEVEL_LOW);
}

/*
	Configuration for MXT540E
*/
#define MXT540E_MAX_MT_FINGERS		10
#define MXT540E_CHRGTIME_BATT		68
#define MXT540E_CHRGTIME_CHRG		48
#define MXT540E_THRESHOLD_BATT		50
#define MXT540E_THRESHOLD_CHRG		40
#define MXT540E_ACTVSYNCSPERX_BATT		20
#define MXT540E_ACTVSYNCSPERX_CHRG		32
#define MXT540E_CALCFG_BATT		98
#define MXT540E_CALCFG_CHRG		114
#define MXT540E_ATCHFRCCALTHR_WAKEUP		8
#define MXT540E_ATCHFRCCALRATIO_WAKEUP		180
#define MXT540E_ATCHFRCCALTHR_NORMAL		40
#define MXT540E_ATCHFRCCALRATIO_NORMAL		55

static u8 t7_config_e[] = {GEN_POWERCONFIG_T7,
		48, 255, 50};

static u8 t8_config_e[] = {GEN_ACQUISITIONCONFIG_T8,
		MXT540E_CHRGTIME_BATT, 0, 5, 1, 0, 0, 4, 30, MXT540E_ATCHFRCCALTHR_WAKEUP, MXT540E_ATCHFRCCALRATIO_WAKEUP};

static u8 t9_config_e[] = {TOUCH_MULTITOUCHSCREEN_T9,
		131, 0, 0, 16, 26, 0, 176, MXT540E_THRESHOLD_BATT, 2, 6,
		10, 10, 1, 46, MXT540E_MAX_MT_FINGERS, 20, 40, 20, 31, 3,
		255, 4, 253, 3, 254, 2, 136, 60, 136, 40,
		18, 12, 0, 0, 0};

static u8 t15_config_e[] = {TOUCH_KEYARRAY_T15,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static u8 t18_config_e[] = {SPT_COMCONFIG_T18,
		0, 0};

static u8 t19_config_e[] = {SPT_GPIOPWM_T19,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static u8 t25_config_e[] = {SPT_SELFTEST_T25,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

unsigned char t38_config_e[] = {SPT_USERDATA_T38,
		0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0};

static u8 t40_config_e[] = {PROCI_GRIPSUPPRESSION_T40,
		0, 0, 0, 0, 0};

static u8 t42_config_e[] = {PROCI_TOUCHSUPPRESSION_T42,
		0, 0, 0, 0, 0, 0, 0, 0};

static u8 t43_config_e[] = {SPT_DIGITIZER_T43,
		0, 0, 0, 0, 0, 0, 0};

static u8 t46_config_e[] = {SPT_CTECONFIG_T46,
		0, 0, 16, MXT540E_ACTVSYNCSPERX_BATT, 0, 0, 1, 0};

static u8 t47_config_e[] = {PROCI_STYLUS_T47,
		0, 30, 60, 15, 2, 20, 20, 150, 0, 32};

static u8 t48_config_e[] = {PROCG_NOISESUPPRESSION_T48,
		3, 132, MXT540E_CALCFG_BATT, 0, 0, 0, 0, 0, 0, 35,
		0, 0, 0, 6, 6, 0, 0, 64, 4, 64,
		10, 0, 25, 6, 0, 46, 0, 20, 0, 0,
		0, 0, 0, 0, 160, MXT540E_THRESHOLD_BATT, 2, 10, 1, 46,
		MXT540E_MAX_MT_FINGERS, 5, 20, 253, 3, 254, 2, 136, 60, 136,
		40, 18, 5, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0};

static u8 t48_config_chrg_e[] = {PROCG_NOISESUPPRESSION_T48,
		3, 132, MXT540E_CALCFG_CHRG, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 6, 6, 0, 0, 64, 4, 64,
		10, 0, 20, 6, 0, 46, 0, 0, 0, 0,
		0, 0, 0, 0, 112, MXT540E_THRESHOLD_CHRG, 2, 10, 1, 47,
		MXT540E_MAX_MT_FINGERS, 5, 20, 250, 250, 10, 10, 138, 70, 132,
		0, 18, 5, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0};

static u8 t52_config_e[] = {TOUCH_PROXKEY_T52,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static u8 t55_config_e[] = {ADAPTIVE_T55,
		0, 70, 20, 10, 16, 1};

static u8 t57_config_e[] = {SPT_GENERICDATA_T57,
		131, 25, 0};

static u8 end_config_e[] = {RESERVED_T255};

static const u8 *mxt540e_config[] = {
	t7_config_e,
	t8_config_e,
	t9_config_e,
	t15_config_e,
	t18_config_e,
	t19_config_e,
	t25_config_e,
	t38_config_e,
	t40_config_e,
	t42_config_e,
	t43_config_e,
	t46_config_e,
	t47_config_e,
	t48_config_e,
	t52_config_e,
	t55_config_e,
	t57_config_e,
	end_config_e,
};

struct mxt540e_platform_data mxt540e_data = {
	.max_finger_touches = MXT540E_MAX_MT_FINGERS,
	.gpio_read_done = GPIO_TSP_INT,
	.config_e = mxt540e_config,
	.min_x = 0,
	.max_x = 799,
	.min_y = 0,
	.max_y = 1279,
	.min_z = 0,
	.max_z = 255,
	.min_w = 0,
	.max_w = 30,
	.chrgtime_batt = MXT540E_CHRGTIME_BATT,
	.chrgtime_charging = MXT540E_CHRGTIME_CHRG,
	.tchthr_batt = MXT540E_THRESHOLD_BATT,
	.tchthr_charging = MXT540E_THRESHOLD_CHRG,
	.actvsyncsperx_batt = MXT540E_ACTVSYNCSPERX_BATT,
	.actvsyncsperx_charging = MXT540E_ACTVSYNCSPERX_CHRG,
	.calcfg_batt_e = MXT540E_CALCFG_BATT,
	.calcfg_charging_e = MXT540E_CALCFG_CHRG,
	.atchfrccalthr_e = MXT540E_ATCHFRCCALTHR_NORMAL,
	.atchfrccalratio_e = MXT540E_ATCHFRCCALRATIO_NORMAL,
	.t48_config_batt_e = t48_config_e,
	.t48_config_chrg_e = t48_config_chrg_e,
	.power_on = mxt540e_power_on,
	.power_off = mxt540e_power_off,
	.register_cb = tsp_register_callback,
	.read_ta_status = tsp_read_ta_status,
};
#endif
