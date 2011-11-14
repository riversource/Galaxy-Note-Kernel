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

#ifdef CONFIG_VIDEO_M5MO
#include <media/m5mo_platform.h>
#endif
#ifdef CONFIG_VIDEO_S5K6AAFX
#include <media/s5k6aafx_platform.h>
#endif
#ifdef CONFIG_VIDEO_S5K5BAFX
#include <media/s5k5bafx_platform.h>
#endif
#ifdef CONFIG_VIDEO_S5K5BBGX
#include <media/s5k5bbgx_platform.h>
#endif
#ifdef CONFIG_VIDEO_SR130PC10
#include <media/sr130pc10_platform.h>
#endif

#ifdef CONFIG_VIDEO_FIMC

#define WRITEBACK_ENABLED
/*
 * External camera reset
 * Because the most of cameras take i2c bus signal, so that
 * you have to reset at the boot time for other i2c slave devices.
 * This function also called at fimc_init_camera()
 * Do optimization for cameras on your platform.
*/

#ifdef CONFIG_VIDEO_M5MO
#define CAM_CHECK_ERR_RET(x, msg)	\
	if (unlikely((x) < 0)) { \
		printk(KERN_ERR "\nfail to %s: err = %d\n", msg, x); \
		return x; \
	}
#define CAM_CHECK_ERR(x, msg)	\
		if (unlikely((x) < 0)) { \
			printk(KERN_ERR "\nfail to %s: err = %d\n", msg, x); \
		}

static int m5mo_get_i2c_busnum(void)
{
#ifdef CONFIG_VIDEO_M5MO_USE_SWI2C
	return 25;
#else
	return 0;
#endif
}

#ifdef CONFIG_VIDEO_SR130PC10
static int m5mo_power_on_sr130(void)
{
	struct regulator *regulator;
	int ret = 0;

	printk(KERN_DEBUG "%s: in\n", __func__);

	ret = gpio_request(GPIO_CAM_VGA_nSTBY, "GPL2");
	if (ret) {
		printk(KERN_ERR "faile to request gpio(GPIO_CAM_VGA_nSTBY)\n");
		return ret;
	}
	ret = gpio_request(GPIO_CAM_VGA_nRST, "GPL2");
	if (ret) {
		printk(KERN_ERR "faile to request gpio(GPIO_CAM_VGA_nRST)\n");
		return ret;
	}
	ret = gpio_request(GPIO_CAM_SENSOR_CORE, "GPE2");
	if (ret) {
		printk(KERN_ERR "fail to request gpio(CAM_SENSOR_CORE)\n");
		return ret;
	}
	ret = gpio_request(GPIO_CAM_IO_EN, "GPE2");
	if (ret) {
		printk(KERN_ERR "fail to request gpio(CAM_IO_EN)\n");
		return ret;
	}
	ret = gpio_request(GPIO_VT_CAM_15V, "GPE2");
	if (ret) {
		printk(KERN_ERR "faile to request gpio(GPIO_VT_CAM_15V)\n");
		return ret;
	}
	ret = gpio_request(GPIO_ISP_RESET, "GPY3");
	if (ret) {
		printk(KERN_ERR "faile to request gpio(ISP_RESET)\n");
		return ret;
	}
#ifdef CONFIG_8M_AF_EN
	ret = gpio_request(GPIO_8M_AF_EN, "GPK1");
	if (ret) {
		printk(KERN_ERR "fail to request gpio(8M_AF_EN)\n");
		return ret;
	}
#endif

	/* CAM_VT_nSTBY low */
	ret = gpio_direction_output(GPIO_CAM_VGA_nSTBY, 0);
	CAM_CHECK_ERR_RET(ret, "output VGA_nSTBY");

	/* CAM_VT_nRST	low */
	ret =gpio_direction_output(GPIO_CAM_VGA_nRST, 0);
	CAM_CHECK_ERR_RET(ret, "output VGA_nRST");
	udelay(10);

	/* CAM_ISP_CORE_1.2V */
	regulator = regulator_get(NULL, "cam_isp_core");
	if (IS_ERR(regulator))
		return -ENODEV;
	ret = regulator_enable(regulator);
	regulator_put(regulator);
	CAM_CHECK_ERR_RET(ret, "enable cam_isp_core");
	udelay(1);

	/* CAM_SENSOR_CORE_1.2V */
	ret = gpio_direction_output(GPIO_CAM_SENSOR_CORE, 1);
	CAM_CHECK_ERR_RET(ret, "output senser_core");
	udelay(1);

	/* CAM_AF_2.8V */
#ifdef CONFIG_8M_AF_EN
	/* 8M_AF_2.8V_EN */
	ret = gpio_direction_output(GPIO_8M_AF_EN, 1);
	CAM_CHECK_ERR(ret, "output AF");
#else
	regulator = regulator_get(NULL, "cam_af");
	if (IS_ERR(regulator))
		return -ENODEV;
	ret = regulator_enable(regulator);
	regulator_put(regulator);
	CAM_CHECK_ERR(ret, "output cam_af");
#endif
	mdelay(7);

	/* CAM_SENSOR_A2.8V */
	ret = gpio_direction_output(GPIO_CAM_IO_EN, 1);
	CAM_CHECK_ERR_RET(ret, "output IO_EN");
	/* it takes about 100us at least during level transition.*/
	udelay(1);

	/* VT_CORE_1.5V */
	ret = gpio_direction_output(GPIO_VT_CAM_15V, 1);
#ifdef CONFIG_TARGET_LOCALE_NA
	s3c_gpio_setpull(GPIO_VT_CAM_15V, S3C_GPIO_PULL_NONE);
#endif /* CONFIG_TARGET_LOCALE_NA */
	CAM_CHECK_ERR_RET(ret, "output VT_CAM_1.5V");
	udelay(1);

	/* VT_CAM_1.8V */
	regulator = regulator_get(NULL, "vt_cam_1.8v");
	if (IS_ERR(regulator))
		return -ENODEV;
	ret = regulator_enable(regulator);
	regulator_put(regulator);
	CAM_CHECK_ERR_RET(ret, "enable vt_1.8v");
	udelay(1);

	/* CAM_ISP_1.8V */
	regulator = regulator_get(NULL, "cam_isp");
	if (IS_ERR(regulator))
		return -ENODEV;
	ret = regulator_enable(regulator);
	regulator_put(regulator);
	CAM_CHECK_ERR_RET(ret, "enable cam_isp");
	udelay(1); /* at least */

	/* CAM_SENSOR_IO_1.8V */
	regulator = regulator_get(NULL, "cam_sensor_io");
	if (IS_ERR(regulator))
		return -ENODEV;
	ret = regulator_enable(regulator);
	regulator_put(regulator);
	CAM_CHECK_ERR_RET(ret, "enable sensor_io");
	udelay(300);

	/* CAM_VT_nSTBY high */
	ret = gpio_direction_output(GPIO_CAM_VGA_nSTBY, 1);
	CAM_CHECK_ERR_RET(ret, "output VGA_nSTBY");
	udelay(10);

	/* MCLK */
	ret = s3c_gpio_cfgpin(GPIO_CAM_MCLK, S3C_GPIO_SFN(2));
	CAM_CHECK_ERR_RET(ret, "cfg mclk");
	s3c_gpio_setpull(GPIO_CAM_MCLK, S3C_GPIO_PULL_NONE);
	mdelay(12);

	/* CAM_VT_nRST	high */
	ret =gpio_direction_output(GPIO_CAM_VGA_nRST, 1);
	CAM_CHECK_ERR_RET(ret, "output VGA_nRST");
	udelay(12);

	/* CAM_VT_nSTBY low */
	ret = gpio_direction_output(GPIO_CAM_VGA_nSTBY, 0);
	CAM_CHECK_ERR_RET(ret, "output VGA_nSTBY");
	udelay(10);

	/* ISP_RESET */
	ret = gpio_direction_output(GPIO_ISP_RESET, 1);
	CAM_CHECK_ERR_RET(ret, "output reset");
	mdelay(4);

	gpio_free(GPIO_CAM_VGA_nSTBY);
	gpio_free(GPIO_CAM_VGA_nRST);
	gpio_free(GPIO_CAM_SENSOR_CORE);
	gpio_free(GPIO_CAM_IO_EN);
	gpio_free(GPIO_VT_CAM_15V);
	gpio_free(GPIO_ISP_RESET);
#ifdef CONFIG_8M_AF_EN
	gpio_free(GPIO_8M_AF_EN);
#endif

	return ret;
}

static int m5mo_power_down_sr130(void)
{
	struct regulator *regulator;
	int ret = 0;

	printk(KERN_DEBUG "%s: in\n", __func__);

#ifdef CONFIG_8M_AF_EN
	ret = gpio_request(GPIO_8M_AF_EN, "GPK1");
	if (ret) {
		printk(KERN_ERR "fail to request gpio(8M_AF_EN)\n");
		return ret;
	}
#endif
	ret = gpio_request(GPIO_ISP_RESET, "GPY3");
	if (ret) {
		printk(KERN_ERR "faile to request gpio(ISP_RESET)\n");
		return ret;
	}
	ret = gpio_request(GPIO_CAM_IO_EN, "GPE2");
	if (ret) {
		printk(KERN_ERR "fail to request gpio(GPIO_CAM_IO_EN)\n");
		return ret;
	}
	ret = gpio_request(GPIO_CAM_SENSOR_CORE, "GPE2");
	if (ret) {
		printk(KERN_ERR "fail to request gpio(CAM_SENSOR_COR)\n");
		return ret;
	}
	ret = gpio_request(GPIO_VT_CAM_15V, "GPE2");
	if (ret) {
		printk(KERN_ERR "faile to request gpio(GPIO_VT_CAM_15V)\n");
		return ret;
	}

	/* s3c_i2c0_force_stop(); */

	/* CAM_VT_nSTBY low */
	ret = gpio_direction_output(GPIO_CAM_VGA_nSTBY, 0);
	CAM_CHECK_ERR_RET(ret, "output VGA_nSTBY");

	mdelay(3);

	/* ISP_RESET */
	ret = gpio_direction_output(GPIO_ISP_RESET, 0);
	CAM_CHECK_ERR(ret, "output reset");
	udelay(1);

	/* CAM_VT_nRST	low */
	ret =gpio_direction_output(GPIO_CAM_VGA_nRST, 0);
	CAM_CHECK_ERR_RET(ret, "output VGA_nRST");
	mdelay(2);

	/* MCLK */
	ret = s3c_gpio_cfgpin(GPIO_CAM_MCLK, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_CAM_MCLK, S3C_GPIO_PULL_DOWN);
	CAM_CHECK_ERR(ret, "cfg mclk");
	udelay(1);

	/* CAM_AF_2.8V */
#ifdef CONFIG_8M_AF_EN
	/* 8M_AF_2.8V_EN */
	ret = gpio_direction_output(GPIO_8M_AF_EN, 0);
	CAM_CHECK_ERR(ret, "output AF");
#else
	regulator = regulator_get(NULL, "cam_af");
	if (IS_ERR(regulator))
		return -ENODEV;
	if (regulator_is_enabled(regulator))
		ret = regulator_force_disable(regulator);
	regulator_put(regulator);
	CAM_CHECK_ERR(ret, "disable cam_af");
#endif
	udelay(5);

	/* CAM_SENSOR_IO_1.8V */
	regulator = regulator_get(NULL, "cam_sensor_io");
	if (IS_ERR(regulator))
		return -ENODEV;
	if (regulator_is_enabled(regulator))
		ret = regulator_force_disable(regulator);
	regulator_put(regulator);
	CAM_CHECK_ERR(ret, "disable, sensor_io");
	udelay(1);

	/* CAM_ISP_1.8V */
	regulator = regulator_get(NULL, "cam_isp");
	if (IS_ERR(regulator))
		return -ENODEV;
	if (regulator_is_enabled(regulator))
		ret = regulator_force_disable(regulator);
	regulator_put(regulator);
	CAM_CHECK_ERR(ret, "disable cam_isp");
	udelay(1); /* 100us -> 500us */

	/* VT_CAM_1.8V */
	regulator = regulator_get(NULL, "vt_cam_1.8v");
	if (IS_ERR(regulator))
		return -ENODEV;
	if (regulator_is_enabled(regulator))
		ret = regulator_disable(regulator);
	regulator_put(regulator);
	CAM_CHECK_ERR(ret, "disable vt_1.8v");
	udelay(1); /* 10us -> 250us */

	/* VT_CORE_1.5V */
	ret = gpio_direction_output(GPIO_VT_CAM_15V, 0);
	CAM_CHECK_ERR(ret, "output VT_CAM_1.5V");
	udelay(1); /*10 -> 300 us */

	/* CAM_SENSOR_A2.8V */
	ret = gpio_direction_output(GPIO_CAM_IO_EN, 0);
	CAM_CHECK_ERR(ret, "output IO_EN");
	udelay(1);

	/* CAM_SENSOR_CORE_1.2V */
	ret = gpio_direction_output(GPIO_CAM_SENSOR_CORE, 0);
	CAM_CHECK_ERR(ret, "output SENSOR_CORE");
	udelay(5);

	/* CAM_ISP_CORE_1.2V */
	regulator = regulator_get(NULL, "cam_isp_core");
	if (IS_ERR(regulator))
		return -ENODEV;
	if (regulator_is_enabled(regulator))
		ret = regulator_force_disable(regulator);
	regulator_put(regulator);
	CAM_CHECK_ERR(ret, "disable isp_core");

	gpio_free(GPIO_CAM_VGA_nSTBY);
	gpio_free(GPIO_CAM_VGA_nRST);
#ifdef CONFIG_8M_AF_EN
	gpio_free(GPIO_8M_AF_EN);
#endif
	gpio_free(GPIO_ISP_RESET);
	gpio_free(GPIO_CAM_IO_EN);
	gpio_free(GPIO_CAM_SENSOR_CORE);
	gpio_free(GPIO_VT_CAM_15V);

	return ret;
}
#endif

static int m5mo_power_on(void)
{
	struct regulator *regulator;
	int ret = 0;

	printk(KERN_DEBUG "%s: in\n", __func__);

	ret = gpio_request(GPIO_CAM_VGA_nSTBY, "GPL2");
	if (ret) {
		printk(KERN_ERR "faile to request gpio(GPIO_CAM_VGA_nSTBY)\n");
		return ret;
	}
	ret = gpio_request(GPIO_CAM_VGA_nRST, "GPL2");
	if (ret) {
		printk(KERN_ERR "faile to request gpio(GPIO_CAM_VGA_nRST)\n");
		return ret;
	}
	ret = gpio_request(GPIO_CAM_SENSOR_CORE, "GPE2");
	if (ret) {
		printk(KERN_ERR "fail to request gpio(CAM_SENSOR_CORE)\n");
		return ret;
	}
	ret = gpio_request(GPIO_CAM_IO_EN, "GPE2");
	if (ret) {
		printk(KERN_ERR "fail to request gpio(CAM_IO_EN)\n");
		return ret;
	}
	ret = gpio_request(GPIO_VT_CAM_15V, "GPE2");
	if (ret) {
		printk(KERN_ERR "faile to request gpio(GPIO_VT_CAM_15V)\n");
		return ret;
	}
	ret = gpio_request(GPIO_ISP_RESET, "GPY3");
	if (ret) {
		printk(KERN_ERR "faile to request gpio(ISP_RESET)\n");
		return ret;
	}
#ifdef CONFIG_8M_AF_EN
	ret = gpio_request(GPIO_8M_AF_EN, "GPK1");
	if (ret) {
		printk(KERN_ERR "fail to request gpio(8M_AF_EN)\n");
		return ret;
	}
#endif

	/* CAM_VT_nSTBY low */
	ret = gpio_direction_output(GPIO_CAM_VGA_nSTBY, 0);
	CAM_CHECK_ERR_RET(ret, "output VGA_nSTBY");

	/* CAM_VT_nRST	low */
	gpio_direction_output(GPIO_CAM_VGA_nRST, 0);
	CAM_CHECK_ERR_RET(ret, "output VGA_nRST");
	udelay(10);

	/* CAM_ISP_CORE_1.2V */
	regulator = regulator_get(NULL, "cam_isp_core");
	if (IS_ERR(regulator))
		return -ENODEV;
	ret = regulator_enable(regulator);
	regulator_put(regulator);
	CAM_CHECK_ERR_RET(ret, "enable cam_isp_core");
	/* No delay */

	/* CAM_SENSOR_CORE_1.2V */
	ret = gpio_direction_output(GPIO_CAM_SENSOR_CORE, 1);
	CAM_CHECK_ERR_RET(ret, "output senser_core");
#if defined(CONFIG_MACH_Q1_REV02) || defined(CONFIG_MACH_Q1_REV00)
	udelay(120); //Fixed Power Sepuence for Q1 H/W
#else	
	udelay(10);
#endif
	/* CAM_SENSOR_A2.8V */
	ret = gpio_direction_output(GPIO_CAM_IO_EN, 1);
	CAM_CHECK_ERR_RET(ret, "output IO_EN");
	/* it takes about 100us at least during level transition.*/
	udelay(160); /* 130us -> 160us */

	/* VT_CORE_1.5V */
	ret = gpio_direction_output(GPIO_VT_CAM_15V, 1);
#ifdef CONFIG_TARGET_LOCALE_NA
	s3c_gpio_setpull(GPIO_VT_CAM_15V, S3C_GPIO_PULL_NONE);
#endif /* CONFIG_TARGET_LOCALE_NA */
	CAM_CHECK_ERR_RET(ret, "output VT_CAM_1.5V");
	udelay(20);

#if defined(CONFIG_MACH_Q1_REV02) || defined(CONFIG_MACH_Q1_REV00)
        udelay(120);
#endif

	/* CAM_AF_2.8V */
#ifdef CONFIG_8M_AF_EN
	/* 8M_AF_2.8V_EN */
	ret = gpio_direction_output(GPIO_8M_AF_EN, 1);
	CAM_CHECK_ERR(ret, "output AF");
#else
	regulator = regulator_get(NULL, "cam_af");
	if (IS_ERR(regulator))
		return -ENODEV;
	ret = regulator_enable(regulator);
	regulator_put(regulator);
	CAM_CHECK_ERR(ret, "output cam_af");
#endif
	mdelay(7);

	/* VT_CAM_1.8V */
	regulator = regulator_get(NULL, "vt_cam_1.8v");
	if (IS_ERR(regulator))
		return -ENODEV;
	ret = regulator_enable(regulator);
	regulator_put(regulator);
	CAM_CHECK_ERR_RET(ret, "enable vt_1.8v");
	udelay(10);

	/* CAM_ISP_1.8V */
	regulator = regulator_get(NULL, "cam_isp");
	if (IS_ERR(regulator))
		return -ENODEV;
	ret = regulator_enable(regulator);
	regulator_put(regulator);
	CAM_CHECK_ERR_RET(ret, "enable cam_isp");
	udelay(120); /* at least */

	/* CAM_SENSOR_IO_1.8V */
	regulator = regulator_get(NULL, "cam_sensor_io");
	if (IS_ERR(regulator))
		return -ENODEV;
	ret = regulator_enable(regulator);
	regulator_put(regulator);
	CAM_CHECK_ERR_RET(ret, "enable sensor_io");
	udelay(30);

	/* MCLK */
	ret = s3c_gpio_cfgpin(GPIO_CAM_MCLK, S3C_GPIO_SFN(2));
	CAM_CHECK_ERR_RET(ret, "cfg mclk");
	s3c_gpio_setpull(GPIO_CAM_MCLK, S3C_GPIO_PULL_NONE);
	udelay(70);

	/* ISP_RESET */
	ret = gpio_direction_output(GPIO_ISP_RESET, 1);
	CAM_CHECK_ERR_RET(ret, "output reset");
	mdelay(4);

	gpio_free(GPIO_CAM_VGA_nSTBY);
	gpio_free(GPIO_CAM_VGA_nRST);
	gpio_free(GPIO_CAM_SENSOR_CORE);
	gpio_free(GPIO_CAM_IO_EN);
	gpio_free(GPIO_VT_CAM_15V);
	gpio_free(GPIO_ISP_RESET);
#ifdef CONFIG_8M_AF_EN
	gpio_free(GPIO_8M_AF_EN);
#endif

	return ret;
}

static int m5mo_power_down(void)
{
	struct regulator *regulator;
	int ret = 0;

	printk(KERN_DEBUG "%s: in\n", __func__);

#ifdef CONFIG_8M_AF_EN
	ret = gpio_request(GPIO_8M_AF_EN, "GPK1");
	if (ret) {
		printk(KERN_ERR "fail to request gpio(8M_AF_EN)\n");
		return ret;
	}
#endif
	ret = gpio_request(GPIO_ISP_RESET, "GPY3");
	if (ret) {
		printk(KERN_ERR "faile to request gpio(ISP_RESET)\n");
		return ret;
	}
	ret = gpio_request(GPIO_CAM_IO_EN, "GPE2");
	if (ret) {
		printk(KERN_ERR "fail to request gpio(GPIO_CAM_IO_EN)\n");
		return ret;
	}
	ret = gpio_request(GPIO_CAM_SENSOR_CORE, "GPE2");
	if (ret) {
		printk(KERN_ERR "fail to request gpio(CAM_SENSOR_COR)\n");
		return ret;
	}
	ret = gpio_request(GPIO_VT_CAM_15V, "GPE2");
	if (ret) {
		printk(KERN_ERR "faile to request gpio(GPIO_VT_CAM_15V)\n");
		return ret;
	}

	/* s3c_i2c0_force_stop(); */

	mdelay(3);

	/* ISP_RESET */
	ret = gpio_direction_output(GPIO_ISP_RESET, 0);
	CAM_CHECK_ERR(ret, "output reset");
	#ifdef CONFIG_TARGET_LOCALE_KOR
	mdelay(3); /* fix without seeing signal form for kor.*/
	#else
	mdelay(2);
	#endif

	/* MCLK */
	ret = s3c_gpio_cfgpin(GPIO_CAM_MCLK, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_CAM_MCLK, S3C_GPIO_PULL_DOWN);
	CAM_CHECK_ERR(ret, "cfg mclk");
	udelay(20);

	/* CAM_AF_2.8V */
#ifdef CONFIG_8M_AF_EN
	/* 8M_AF_2.8V_EN */
	ret = gpio_direction_output(GPIO_8M_AF_EN, 0);
	CAM_CHECK_ERR(ret, "output AF");
#else
	regulator = regulator_get(NULL, "cam_af");
	if (IS_ERR(regulator))
		return -ENODEV;
	if (regulator_is_enabled(regulator))
		ret = regulator_force_disable(regulator);
	regulator_put(regulator);
	CAM_CHECK_ERR(ret, "disable cam_af");
#endif

	/* CAM_SENSOR_IO_1.8V */
	regulator = regulator_get(NULL, "cam_sensor_io");
	if (IS_ERR(regulator))
		return -ENODEV;
	if (regulator_is_enabled(regulator))
		ret = regulator_force_disable(regulator);
	regulator_put(regulator);
	CAM_CHECK_ERR(ret, "disable, sensor_io");
	udelay(10);

	/* CAM_ISP_1.8V */
	regulator = regulator_get(NULL, "cam_isp");
	if (IS_ERR(regulator))
		return -ENODEV;
	if (regulator_is_enabled(regulator))
		ret = regulator_force_disable(regulator);
	regulator_put(regulator);
	CAM_CHECK_ERR(ret, "disable cam_isp");
	udelay(500); /* 100us -> 500us */

	/* VT_CAM_1.8V */
	regulator = regulator_get(NULL, "vt_cam_1.8v");
	if (IS_ERR(regulator))
		return -ENODEV;
	if (regulator_is_enabled(regulator))
		ret = regulator_disable(regulator);
	regulator_put(regulator);
	CAM_CHECK_ERR(ret, "disable vt_1.8v");
	udelay(250); /* 10us -> 250us */

	/* VT_CORE_1.5V */
	ret = gpio_direction_output(GPIO_VT_CAM_15V, 0);
	CAM_CHECK_ERR(ret, "output VT_CAM_1.5V");
	udelay(300); /*10 -> 300 us */

	/* CAM_SENSOR_A2.8V */
	ret = gpio_direction_output(GPIO_CAM_IO_EN, 0);
	CAM_CHECK_ERR(ret, "output IO_EN");
	udelay(800);

	/* CAM_SENSOR_CORE_1.2V */
	ret = gpio_direction_output(GPIO_CAM_SENSOR_CORE, 0);
	CAM_CHECK_ERR(ret, "output SENSOR_CORE");
	udelay(5);

	/* CAM_ISP_CORE_1.2V */
	regulator = regulator_get(NULL, "cam_isp_core");
	if (IS_ERR(regulator))
		return -ENODEV;
	if (regulator_is_enabled(regulator))
		ret = regulator_force_disable(regulator);
	regulator_put(regulator);
	CAM_CHECK_ERR(ret, "disable isp_core");

#if defined(CONFIG_MACH_Q1_REV02) || defined(CONFIG_MACH_Q1_REV00)
	mdelay(250);
#endif
	
#ifdef CONFIG_8M_AF_EN
	gpio_free(GPIO_8M_AF_EN);
#endif
	gpio_free(GPIO_ISP_RESET);
	gpio_free(GPIO_CAM_IO_EN);
	gpio_free(GPIO_CAM_SENSOR_CORE);
	gpio_free(GPIO_VT_CAM_15V);

	return ret;
}

int s3c_csis_power(int enable)
{
	struct regulator *regulator;
	int ret = 0;

	/* mipi_1.1v ,mipi_1.8v are always powered-on.
	* If they are off, we then power them on.
	*/
	if (enable) {
		/* VMIPI_1.1V */
		regulator = regulator_get(NULL, "vmipi_1.1v");
		if (IS_ERR(regulator))
			goto error_out;
		if (!regulator_is_enabled(regulator)) {
			printk(KERN_WARNING "%s: vmipi_1.1v is off. so ON\n",
				__func__);
			ret = regulator_enable(regulator);
			CAM_CHECK_ERR(ret, "enable vmipi_1.1v");
		}
		regulator_put(regulator);

		/* VMIPI_1.8V */
		regulator = regulator_get(NULL, "vmipi_1.8v");
		if (IS_ERR(regulator))
			goto error_out;
		if (!regulator_is_enabled(regulator)) {
			printk(KERN_WARNING "%s: vmipi_1.8v is off. so ON\n",
				__func__);
			ret = regulator_enable(regulator);
			CAM_CHECK_ERR(ret, "enable vmipi_1.8v");
		}
		regulator_put(regulator);
	}

	return 0;

error_out:
	printk(KERN_ERR "%s: ERROR: failed to check mipi-power\n", __func__);
	return 0;
}

#if defined(CONFIG_MACH_Q1_REV02)
static bool is_torch = 0;
#endif

static int m5mo_flash_power(int enable)
{
	struct regulator *flash = regulator_get(NULL, "led_flash");
	struct regulator *movie = regulator_get(NULL, "led_movie");

	if (enable) {

#if defined(CONFIG_MACH_Q1_REV02)		
		if(regulator_is_enabled(movie)) {
			printk(KERN_DEBUG "%s: m5mo_torch set~~~~",__func__);	
			is_torch = 1;
			goto torch_exit;
		}
		is_torch = 0;
#endif

		regulator_set_current_limit(flash, 490000, 530000);
		regulator_enable(flash);
		regulator_set_current_limit(movie, 90000, 110000);
		regulator_enable(movie);
	} else {
	
#if defined(CONFIG_MACH_Q1_REV02)			
		if(is_torch)
			goto torch_exit;
#endif		
		
		if (regulator_is_enabled(flash))
			regulator_disable(flash);
		if (regulator_is_enabled(movie))
			regulator_disable(movie);
	}

torch_exit:
	regulator_put(flash);
	regulator_put(movie);

	return 0;
}

static int m5mo_power(int enable)
{
	int ret = 0;

	printk("%s %s\n", __func__, enable ? "on" : "down");
	if (enable) {
#ifdef CONFIG_VIDEO_SR130PC10
		ret = m5mo_power_on_sr130();
#else
		ret = m5mo_power_on();
#endif
		if (unlikely(ret))
			goto error_out;
	} else
#ifdef CONFIG_VIDEO_SR130PC10
		ret = m5mo_power_down_sr130();
#else
		ret = m5mo_power_down();
#endif
	ret = s3c_csis_power(enable);
	m5mo_flash_power(enable);

error_out:
	return ret;
}

static int m5mo_config_isp_irq(void)
{
	s3c_gpio_cfgpin(GPIO_ISP_INT, S3C_GPIO_SFN(0xF));
	s3c_gpio_setpull(GPIO_ISP_INT, S3C_GPIO_PULL_NONE);
	return 0;
}

#if defined(CONFIG_TARGET_LOCALE_NAATT)
static bool isRecording;
static int m5mo_set_recording_state(bool on)
{
	isRecording = on;
}
#endif

static struct m5mo_platform_data m5mo_plat = {
	.default_width = 640, /* 1920 */
	.default_height = 480, /* 1080 */
	.pixelformat = V4L2_PIX_FMT_UYVY,
	.freq = 24000000,
	.is_mipi = 1,
	.config_isp_irq = m5mo_config_isp_irq,
#if defined(CONFIG_TARGET_LOCALE_NAATT)
	.set_recording_state = m5mo_set_recording_state,
#endif
};

static struct i2c_board_info m5mo_i2c_info = {
	I2C_BOARD_INFO("M5MO", 0x1F),
	.platform_data = &m5mo_plat,
	.irq = IRQ_EINT(13),
};

static struct s3c_platform_camera m5mo = {
	.id		= CAMERA_CSI_C,
	.clk_name	= "sclk_cam0",
	.get_i2c_busnum	= m5mo_get_i2c_busnum,
	.cam_power	= m5mo_power, /*smdkv310_mipi_cam0_reset,*/
	.type		= CAM_TYPE_MIPI,
	.fmt		= ITU_601_YCBCR422_8BIT, /*MIPI_CSI_YCBCR422_8BIT*/
	.order422	= CAM_ORDER422_8BIT_CBYCRY,
	.info		= &m5mo_i2c_info,
	.pixelformat	= V4L2_PIX_FMT_UYVY,
	.srclk_name	= "xusbxti", /* "mout_mpll" */
	.clk_rate	= 24000000, /* 48000000 */
	.line_length	= 1920,
	.width		= 640,
	.height		= 480,
	.window		= {
		.left	= 0,
		.top	= 0,
		.width	= 640,
		.height	= 480,
	},

	.mipi_lanes	= 2,
	.mipi_settle	= 12,
	.mipi_align	= 32,

	/* Polarity */
	.inv_pclk	= 0,
	.inv_vsync	= 1,
	.inv_href	= 0,
	.inv_hsync	= 0,
	.reset_camera	= 0,
	.initialized	= 0,
};
#endif /* #ifdef CONFIG_VIDEO_M5MO */

#ifdef CONFIG_VIDEO_S5K6AAFX
static int s5k6aafx_get_i2c_busnum(void)
{
#if !defined(CONFIG_MACH_C1_REV00)
	if (system_rev >= 3)
		return 12;
	else
		return 6;
#else
	return 12;
#endif
}

static int s5k6aafx_power_on(void)
{
#if !defined (CONFIG_MACH_C1_REV00)
	struct regulator *regulator;
#endif
	int err = 0;

#if defined (CONFIG_VIDEO_M5MO)
	err = m5mo_power(1);
	if (err) {
		printk(KERN_ERR "%s ERROR: fail to power on\n", __func__);
		return err;
	}
#endif
	err = gpio_request(GPIO_CAM_IO_EN, "GPE2");
	if (err) {
		printk(KERN_ERR "faile to request gpio(GPIO_CAM_IO_EN)\n");
		return err;
	}

#if defined(CONFIG_MACH_C1_REV00)
	err = gpio_request(GPIO_VT_CAM_15V, "GPE4");
#else
	err = gpio_request(GPIO_VT_CAM_15V, "GPE2");
#endif
	if (err) {
		printk(KERN_ERR "faile to request gpio(GPIO_VT_CAM_15V)\n");
		return err;
	}
#if defined (CONFIG_MACH_C1_REV00)
	err = gpio_request(GPIO_VT_CAM_18V, "GPE4");
	if (err) {
		printk(KERN_ERR "faile to request gpio(GPIO_VT_CAM_18V)\n");
		return err;
	}
#endif

	err = gpio_request(GPIO_CAM_VGA_nSTBY, "GPL2");
	if (err) {
		printk(KERN_ERR "faile to request gpio(GPIO_CAM_VGA_nSTBY)\n");
		return err;
	}

	err = gpio_request(GPIO_CAM_VGA_nRST, "GPL2");
	if (err) {
		printk(KERN_ERR "faile to request gpio(GPIO_CAM_VGA_nRST)\n");
		return err;
	}

	/* CAM_SENSOR_A2.8V, CAM_IO_EN */
	gpio_direction_output(GPIO_CAM_IO_EN, 1);
	mdelay(1);

	/* VT_CAM_1.5V */
	gpio_direction_output(GPIO_VT_CAM_15V, 1);
	mdelay(1);

	/* VT_CAM_1.8V */
#if defined (CONFIG_MACH_C1_REV00)
	gpio_direction_output(GPIO_VT_CAM_18V, 1);
#else
	regulator = regulator_get(NULL, "vt_cam_1.8v");
	if (IS_ERR(regulator))
		return -ENODEV;
	regulator_enable(regulator);
	regulator_put(regulator);
#endif
	mdelay(1);

	/* CAM_VGA_nSTBY */
	gpio_direction_output(GPIO_CAM_VGA_nSTBY, 1);
	udelay(500);

	/* Mclk */
	s3c_gpio_cfgpin(GPIO_CAM_MCLK, S3C_GPIO_SFN(2));
	s3c_gpio_setpull(GPIO_CAM_MCLK, S3C_GPIO_PULL_NONE);
	udelay(500);

	/* CAM_VGA_nRST	 */
	gpio_direction_output(GPIO_CAM_VGA_nRST, 1);
	mdelay(2);

	gpio_free(GPIO_CAM_IO_EN);
	gpio_free(GPIO_VT_CAM_15V);
#if defined (CONFIG_MACH_C1_REV00)
	gpio_free(GPIO_VT_CAM_18V);
#endif
	gpio_free(GPIO_CAM_VGA_nSTBY);
	gpio_free(GPIO_CAM_VGA_nRST);

	return 0;
}

static int s5k6aafx_power_off(void)
{
#if !defined (CONFIG_MACH_C1_REV00)
	struct regulator *regulator;
#endif
	int err = 0;

#if defined (CONFIG_VIDEO_M5MO)
	err = m5mo_power(0);
	if (err) {
		printk(KERN_ERR "%s ERROR: fail to power down\n", __func__);
		return err;
	}
#endif

	err = gpio_request(GPIO_CAM_IO_EN, "GPE2");
	if (err) {
		printk(KERN_ERR "faile to request gpio(GPIO_CAM_IO_EN)\n");
		return err;
	}

#if defined(CONFIG_MACH_C1_REV00)
	err = gpio_request(GPIO_VT_CAM_15V, "GPE4");
#else
	err = gpio_request(GPIO_VT_CAM_15V, "GPE2");
#endif
	if (err) {
		printk(KERN_ERR "faile to request gpio(GPIO_VT_CAM_15V)\n");
		return err;
	}

#if defined (CONFIG_MACH_C1_REV00)
	err = gpio_request(GPIO_VT_CAM_18V, "GPE4");
	if (err) {
		printk(KERN_ERR "faile to request gpio(GPIO_VT_CAM_18V)\n");
		return err;
	}
#endif

	err = gpio_request(GPIO_CAM_VGA_nSTBY, "GPL2");
	if (err) {
		printk(KERN_ERR "faile to request gpio(GPIO_CAM_VGA_nSTBY)\n");
		return err;
	}

	err = gpio_request(GPIO_CAM_VGA_nRST, "GPL2");
	if (err) {
		printk(KERN_ERR "faile to request gpio(GPIO_CAM_VGA_nRST)\n");
		return err;
	}

	/* CAM_VGA_nRST	 */
	gpio_direction_output(GPIO_CAM_VGA_nRST, 0);
	udelay(200);

	/* Mclk */
	s3c_gpio_cfgpin(GPIO_CAM_MCLK, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_CAM_MCLK, S3C_GPIO_PULL_DOWN);
	udelay(200);

	/* CAM_VGA_nSTBY */
	gpio_direction_output(GPIO_CAM_VGA_nSTBY, 0);
	udelay(200);

	/* VT_CAM_1.8V */
#if defined (CONFIG_MACH_C1_REV00)
	gpio_direction_output(GPIO_VT_CAM_18V, 0);
#else
	regulator = regulator_get(NULL, "vt_cam_1.8v");
	if (IS_ERR(regulator))
		return -ENODEV;
	if (regulator_is_enabled(regulator))
		regulator_force_disable(regulator);
	regulator_put(regulator);
#endif
	mdelay(1);

	/* VT_CAM_1.5V */
	gpio_direction_output(GPIO_VT_CAM_15V, 0);
	mdelay(1);

	/* CAM_SENSOR_A2.8V, CAM_IO_EN */
	gpio_direction_output(GPIO_CAM_IO_EN, 0);

	gpio_free(GPIO_CAM_IO_EN);
	gpio_free(GPIO_VT_CAM_15V);
#if defined (CONFIG_MACH_C1_REV00)
	gpio_free(GPIO_VT_CAM_18V);
#endif
	gpio_free(GPIO_CAM_VGA_nSTBY);
	gpio_free(GPIO_CAM_VGA_nRST);

	return 0;
}

static int s5k6aafx_power(int onoff)
{
	int ret = 0;

	printk("%s(): %s \n", __func__, onoff ? "on" : "down");

	if (onoff)
		ret = s5k6aafx_power_on();
	else {
		ret = s5k6aafx_power_off();
		/* s3c_i2c0_force_stop();*/ /* DSLIM. Should be implemented */
	}

	return ret;
}

static struct s5k6aafx_platform_data s5k6aafx_plat = {
	.default_width = 640,
	.default_height = 480,
	.pixelformat = V4L2_PIX_FMT_UYVY,
#if defined (CONFIG_VIDEO_S5K6AAFX_MIPI)
	.freq = 24000000,
	.is_mipi = 1,
#endif
};

static struct i2c_board_info  s5k6aafx_i2c_info = {
	I2C_BOARD_INFO("S5K6AAFX", 0x78 >> 1),
	.platform_data = &s5k6aafx_plat,
};

static struct s3c_platform_camera s5k6aafx = {
#if defined (CONFIG_VIDEO_S5K6AAFX_MIPI)
	.id		= CAMERA_CSI_D,
	.type		= CAM_TYPE_MIPI,
	.fmt		= ITU_601_YCBCR422_8BIT,
	.order422	= CAM_ORDER422_8BIT_CBYCRY,

	.mipi_lanes	= 1,
	.mipi_settle	= 6,
	.mipi_align	= 32,
#else
	.id		= CAMERA_PAR_A,
	.type		= CAM_TYPE_ITU,
	.fmt		= ITU_601_YCBCR422_8BIT,
	.order422	= CAM_ORDER422_8BIT_YCBYCR,
#endif
	.get_i2c_busnum	= s5k6aafx_get_i2c_busnum,
	.info		= &s5k6aafx_i2c_info,
	.pixelformat	= V4L2_PIX_FMT_UYVY,
	.srclk_name	= "xusbxti",
	.clk_name	= "sclk_cam0",
	.clk_rate	= 24000000,
	.line_length	= 640,
	.width		= 640,
	.height		= 480,
	.window		= {
		.left	= 0,
		.top	= 0,
		.width	= 640,
		.height	= 480,
	},

	/* Polarity */
	.inv_pclk	= 0,
	.inv_vsync	= 1,
	.inv_href	= 0,
	.inv_hsync	= 0,
	.reset_camera	= 0,
	.initialized	= 0,
	.cam_power	= s5k6aafx_power,
};
#endif

#ifdef CONFIG_VIDEO_S5K5BAFX
static int s5k5bafx_get_i2c_busnum(void)
{

#if defined(CONFIG_MACH_Q1_REV02) || defined(CONFIG_MACH_Q1_REV00)
	return 12;
#else

	if (system_rev >= 3)
		return 12;
	else
		return 6;
#endif

}

static int s5k5bafx_power_on(void)
{
	struct regulator *regulator;
	int ret = 0;

	/* printk("%s: in\n", __func__); */

	ret = gpio_request(GPIO_ISP_RESET, "GPY3");
	if (ret) {
		printk(KERN_ERR "faile to request gpio(ISP_RESET)\n");
		return ret;
	}
	ret = gpio_request(GPIO_CAM_IO_EN, "GPE2");
	if (ret) {
		printk(KERN_ERR "faile to request gpio(GPIO_CAM_IO_EN)\n");
		return ret;
	}
	ret = gpio_request(GPIO_VT_CAM_15V, "GPE2");
	if (ret) {
		printk(KERN_ERR "faile to request gpio(GPIO_VT_CAM_15V)\n");
		return ret;
	}
	ret = gpio_request(GPIO_CAM_VGA_nSTBY, "GPL2");
	if (ret) {
		printk(KERN_ERR "faile to request gpio(GPIO_CAM_VGA_nSTBY)\n");
		return ret;
	}
	ret = gpio_request(GPIO_CAM_VGA_nRST, "GPL2");
	if (ret) {
		printk(KERN_ERR "faile to request gpio(GPIO_CAM_VGA_nRST)\n");
		return ret;
	}

	if (system_rev >= 9) {
		s3c_gpio_setpull(VT_CAM_SDA_18V, S3C_GPIO_PULL_NONE);
		s3c_gpio_setpull(VT_CAM_SCL_18V, S3C_GPIO_PULL_NONE);
	}

	/* ISP_RESET low*/
	ret = gpio_direction_output(GPIO_ISP_RESET, 0);
	CAM_CHECK_ERR_RET(ret, "output reset");
	udelay(100);

	/* CAM_ISP_CORE_1.2V */
	regulator = regulator_get(NULL, "cam_isp_core");
	if (IS_ERR(regulator))
		return -ENODEV;
	ret = regulator_enable(regulator);
	regulator_put(regulator);
	CAM_CHECK_ERR_RET(ret, "enable isp_core");
	udelay(10);

	/* CAM_SENSOR_A2.8V */
	ret = gpio_direction_output(GPIO_CAM_IO_EN, 1);
	CAM_CHECK_ERR_RET(ret, "output io_en");
	udelay(300); /* don't change me */

	/* VT_CORE_1.5V */
	ret = gpio_direction_output(GPIO_VT_CAM_15V, 1);
	CAM_CHECK_ERR_RET(ret, "output vt_15v");
	udelay(100);

	/* CAM_ISP_1.8V */
	regulator = regulator_get(NULL, "cam_isp");
	if (IS_ERR(regulator))
		return -ENODEV;
	ret = regulator_enable(regulator);
	regulator_put(regulator);
	CAM_CHECK_ERR_RET(ret, "enable cam_isp");
	udelay(10);

	/* VT_CAM_1.8V */
	regulator = regulator_get(NULL, "vt_cam_1.8v");
	if (IS_ERR(regulator))
		return -ENODEV;
	ret = regulator_enable(regulator);
	regulator_put(regulator);
	CAM_CHECK_ERR_RET(ret, "enable vt_1.8v");
	udelay(10);

	/* CAM_VGA_nSTBY */
	ret = gpio_direction_output(GPIO_CAM_VGA_nSTBY, 1);
	CAM_CHECK_ERR_RET(ret, "output VGA_nSTBY");
	udelay(50);

	/* Mclk */
	ret = s3c_gpio_cfgpin(GPIO_CAM_MCLK, S3C_GPIO_SFN(2));
	s3c_gpio_setpull(GPIO_CAM_MCLK, S3C_GPIO_PULL_NONE);
	CAM_CHECK_ERR_RET(ret, "cfg mclk");
	udelay(100);

	/* CAM_VGA_nRST	 */
	ret = gpio_direction_output(GPIO_CAM_VGA_nRST, 1);
	CAM_CHECK_ERR_RET(ret, "output VGA_nRST");
	mdelay(2);

	gpio_free(GPIO_ISP_RESET);
	gpio_free(GPIO_CAM_IO_EN);
	gpio_free(GPIO_VT_CAM_15V);
	gpio_free(GPIO_CAM_VGA_nSTBY);
	gpio_free(GPIO_CAM_VGA_nRST);

	return 0;
}

static int s5k5bafx_power_off(void)
{
	struct regulator *regulator;
	int ret = 0;

	/* printk("n%s: in\n", __func__); */

	ret = gpio_request(GPIO_CAM_VGA_nRST, "GPL2");
	if (ret) {
		printk(KERN_ERR "faile to request gpio(GPIO_CAM_VGA_nRST)\n");
		return ret;
	}
	ret = gpio_request(GPIO_CAM_VGA_nSTBY, "GPL2");
	if (ret) {
		printk(KERN_ERR "faile to request gpio(GPIO_CAM_VGA_nSTBY)\n");
		return ret;
	}
	ret = gpio_request(GPIO_VT_CAM_15V, "GPE2");
	if (ret) {
		printk(KERN_ERR "faile to request gpio(GPIO_VT_CAM_15V)\n");
		return ret;
	}
	ret = gpio_request(GPIO_CAM_IO_EN, "GPE2");
	if (ret) {
		printk(KERN_ERR "faile to request gpio(GPIO_CAM_IO_EN)\n");
		return ret;
	}

	/* CAM_VGA_nRST	 */
	ret = gpio_direction_output(GPIO_CAM_VGA_nRST, 0);
	CAM_CHECK_ERR(ret, "output VGA_nRST");
	udelay(100);

	/* Mclk */
	ret = s3c_gpio_cfgpin(GPIO_CAM_MCLK, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_CAM_MCLK, S3C_GPIO_PULL_DOWN);
	CAM_CHECK_ERR(ret, "cfg mclk");
	udelay(20);

	/* CAM_VGA_nSTBY */
	ret = gpio_direction_output(GPIO_CAM_VGA_nSTBY, 0);
	CAM_CHECK_ERR(ret, "output VGA_nSTBY");
	udelay(20);

	/* VT_CAM_1.8V */
	regulator = regulator_get(NULL, "vt_cam_1.8v");
	if (IS_ERR(regulator))
		return -ENODEV;
	if (regulator_is_enabled(regulator))
		ret = regulator_disable(regulator);
	regulator_put(regulator);
	CAM_CHECK_ERR(ret, "disable vt_1.8v");
	udelay(10);

	/* CAM_ISP_1.8V */
	regulator = regulator_get(NULL, "cam_isp");
	if (IS_ERR(regulator))
		return -ENODEV;
	if (regulator_is_enabled(regulator))
		ret = regulator_force_disable(regulator);
	regulator_put(regulator);
	CAM_CHECK_ERR(ret, "disable cam_isp");
	udelay(10);

	/* VT_CORE_1.5V */
	ret = gpio_direction_output(GPIO_VT_CAM_15V, 0);
	CAM_CHECK_ERR(ret, "output vt_1.5v");
	udelay(10);

	/* CAM_SENSOR_A2.8V */
	ret = gpio_direction_output(GPIO_CAM_IO_EN, 0);
	CAM_CHECK_ERR(ret, "output io_en");
	udelay(10);

	/* CAM_ISP_CORE_1.2V */
	regulator = regulator_get(NULL, "cam_isp_core");
	if (IS_ERR(regulator))
		return -ENODEV;
	if (regulator_is_enabled(regulator))
		ret = regulator_force_disable(regulator);
	regulator_put(regulator);
	CAM_CHECK_ERR(ret, "disable isp_core");

	if (system_rev >= 9) {
		gpio_direction_input(VT_CAM_SDA_18V);
		s3c_gpio_setpull(VT_CAM_SDA_18V, S3C_GPIO_PULL_DOWN);
		gpio_direction_input(VT_CAM_SCL_18V);
		s3c_gpio_setpull(VT_CAM_SCL_18V, S3C_GPIO_PULL_DOWN);
	}

#if defined(CONFIG_MACH_Q1_REV02) || defined(CONFIG_MACH_Q1_REV00)
	mdelay(350);
#endif

	gpio_free(GPIO_CAM_VGA_nRST);
	gpio_free(GPIO_CAM_VGA_nSTBY);
	gpio_free(GPIO_VT_CAM_15V);
	gpio_free(GPIO_CAM_IO_EN);

	return 0;
}

static int s5k5bafx_power(int onoff)
{
	int ret = 0;

	printk(KERN_INFO "%s(): %s\n", __func__, onoff ? "on" : "down");
	if (onoff) {
		ret = s5k5bafx_power_on();
		if (unlikely(ret))
			goto error_out;
	} else {
		ret = s5k5bafx_power_off();
		/* s3c_i2c0_force_stop();*/ /* DSLIM. Should be implemented */
	}

	ret = s3c_csis_power(onoff);

error_out:
	return ret;
}

static struct s5k5bafx_platform_data s5k5bafx_plat = {
	.default_width = 640,
	.default_height = 480,
	.pixelformat = V4L2_PIX_FMT_UYVY,
	.freq = 24000000,
	.is_mipi = 1,
};

static struct i2c_board_info  s5k5bafx_i2c_info = {
	I2C_BOARD_INFO("S5K5BAFX", 0x5A >> 1),
	.platform_data = &s5k5bafx_plat,
};

static struct s3c_platform_camera s5k5bafx = {
	.id		= CAMERA_CSI_D,
	.type		= CAM_TYPE_MIPI,
	.fmt		= ITU_601_YCBCR422_8BIT,
	.order422	= CAM_ORDER422_8BIT_CBYCRY,
	.mipi_lanes	= 1,
	.mipi_settle	= 6,
	.mipi_align	= 32,

	.get_i2c_busnum	= s5k5bafx_get_i2c_busnum,
	.info		= &s5k5bafx_i2c_info,
	.pixelformat	= V4L2_PIX_FMT_UYVY,
	.srclk_name	= "xusbxti",
	.clk_name	= "sclk_cam0",
	.clk_rate	= 24000000,
	.line_length	= 640,
	.width		= 640,
	.height		= 480,
	.window		= {
		.left	= 0,
		.top	= 0,
		.width	= 640,
		.height	= 480,
	},

	/* Polarity */
	.inv_pclk	= 0,
	.inv_vsync	= 1,
	.inv_href	= 0,
	.inv_hsync	= 0,
	.reset_camera	= 0,
	.initialized	= 0,
	.cam_power	= s5k5bafx_power,
};
#endif

#ifdef CONFIG_VIDEO_S5K5BBGX
static int s5k5bbgx_get_i2c_busnum(void)
{
	return 12;
}

static int s5k5bbgx_power_on(void)
{
	struct regulator *regulator;
	int ret = 0;

	/* printk("%s: in\n", __func__); */

	ret = gpio_request(GPIO_ISP_RESET, "GPY3");
	if (ret) {
		printk(KERN_ERR "faile to request gpio(ISP_RESET)\n");
		return ret;
	}
	ret = gpio_request(GPIO_CAM_IO_EN, "GPE2");
	if (ret) {
		printk(KERN_ERR "faile to request gpio(GPIO_CAM_IO_EN)\n");
		return ret;
	}
	ret = gpio_request(GPIO_VT_CAM_15V, "GPE2");
	if (ret) {
		printk(KERN_ERR "faile to request gpio(GPIO_VT_CAM_15V)\n");
		return ret;
	}
	ret = gpio_request(GPIO_CAM_VGA_nSTBY, "GPL2");
	if (ret) {
		printk(KERN_ERR "faile to request gpio(GPIO_CAM_VGA_nSTBY)\n");
		return ret;
	}
	ret = gpio_request(GPIO_CAM_VGA_nRST, "GPL2");
	if (ret) {
		printk(KERN_ERR "faile to request gpio(GPIO_CAM_VGA_nRST)\n");
		return ret;
	}

	s3c_gpio_setpull(VT_CAM_SDA_18V, S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(VT_CAM_SCL_18V, S3C_GPIO_PULL_NONE);

	/* ISP_RESET low*/
	ret = gpio_direction_output(GPIO_ISP_RESET, 0);
	CAM_CHECK_ERR_RET(ret, "output reset");
	udelay(100);

	/* CAM_ISP_CORE_1.2V */
	regulator = regulator_get(NULL, "cam_isp_core");
	if (IS_ERR(regulator))
		return -ENODEV;
	ret = regulator_enable(regulator);
	regulator_put(regulator);
	CAM_CHECK_ERR_RET(ret, "enable isp_core");
	udelay(10);

	/* CAM_SENSOR_A2.8V */
	ret = gpio_direction_output(GPIO_CAM_IO_EN, 1);
	CAM_CHECK_ERR_RET(ret, "output io_en");
	udelay(300); /* don't change me */

	/* VT_CORE_1.5V */
	ret = gpio_direction_output(GPIO_VT_CAM_15V, 1);
	CAM_CHECK_ERR_RET(ret, "output vt_15v");
	udelay(100);

	/* CAM_ISP_1.8V */
	regulator = regulator_get(NULL, "cam_isp");
	if (IS_ERR(regulator))
		return -ENODEV;
	ret = regulator_enable(regulator);
	regulator_put(regulator);
	CAM_CHECK_ERR_RET(ret, "enable cam_isp");
	udelay(10);

	/* VT_CAM_1.8V */
	regulator = regulator_get(NULL, "vt_cam_1.8v");
	if (IS_ERR(regulator))
		return -ENODEV;
	ret = regulator_enable(regulator);
	regulator_put(regulator);
	CAM_CHECK_ERR_RET(ret, "enable vt_1.8v");
	udelay(10);

	/* Mclk */
	ret = s3c_gpio_cfgpin(GPIO_CAM_MCLK, S3C_GPIO_SFN(2));
	s3c_gpio_setpull(GPIO_CAM_MCLK, S3C_GPIO_PULL_NONE);
	CAM_CHECK_ERR_RET(ret, "cfg mclk");
	udelay(10);

	/* CAM_VGA_nSTBY */
	ret = gpio_direction_output(GPIO_CAM_VGA_nSTBY, 1);
	CAM_CHECK_ERR_RET(ret, "output VGA_nSTBY");
	udelay(50);

	/* CAM_VGA_nRST	 */
	ret = gpio_direction_output(GPIO_CAM_VGA_nRST, 1);
	CAM_CHECK_ERR_RET(ret, "output VGA_nRST");
	udelay(100);

	gpio_free(GPIO_ISP_RESET);
	gpio_free(GPIO_CAM_IO_EN);
	gpio_free(GPIO_VT_CAM_15V);
	gpio_free(GPIO_CAM_VGA_nSTBY);
	gpio_free(GPIO_CAM_VGA_nRST);

	return 0;
}

static int s5k5bbgx_power_off(void)
{
	struct regulator *regulator;
	int ret = 0;

	/* printk("n%s: in\n", __func__); */

	ret = gpio_request(GPIO_CAM_VGA_nRST, "GPL2");
	if (ret) {
		printk(KERN_ERR "faile to request gpio(GPIO_CAM_VGA_nRST)\n");
		return ret;
	}
	ret = gpio_request(GPIO_CAM_VGA_nSTBY, "GPL2");
	if (ret) {
		printk(KERN_ERR "faile to request gpio(GPIO_CAM_VGA_nSTBY)\n");
		return ret;
	}
	ret = gpio_request(GPIO_VT_CAM_15V, "GPE2");
	if (ret) {
		printk(KERN_ERR "faile to request gpio(GPIO_VT_CAM_15V)\n");
		return ret;
	}
	ret = gpio_request(GPIO_CAM_IO_EN, "GPE2");
	if (ret) {
		printk(KERN_ERR "faile to request gpio(GPIO_CAM_IO_EN)\n");
		return ret;
	}

	/* CAM_VGA_nRST	 */
	ret = gpio_direction_output(GPIO_CAM_VGA_nRST, 0);
	CAM_CHECK_ERR(ret, "output VGA_nRST");
	udelay(100);

	/* CAM_VGA_nSTBY */
	ret = gpio_direction_output(GPIO_CAM_VGA_nSTBY, 0);
	CAM_CHECK_ERR(ret, "output VGA_nSTBY");
	udelay(20);

	/* Mclk */
	ret = s3c_gpio_cfgpin(GPIO_CAM_MCLK, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_CAM_MCLK, S3C_GPIO_PULL_DOWN);
	CAM_CHECK_ERR(ret, "cfg mclk");
	udelay(20);

	/* VT_CAM_1.8V */
	regulator = regulator_get(NULL, "vt_cam_1.8v");
	if (IS_ERR(regulator))
		return -ENODEV;
	if (regulator_is_enabled(regulator))
		ret = regulator_disable(regulator);
	regulator_put(regulator);
	CAM_CHECK_ERR(ret, "disable vt_1.8v");
	udelay(10);

	/* CAM_ISP_1.8V */
	regulator = regulator_get(NULL, "cam_isp");
	if (IS_ERR(regulator))
		return -ENODEV;
	if (regulator_is_enabled(regulator))
		ret = regulator_force_disable(regulator);
	regulator_put(regulator);
	CAM_CHECK_ERR(ret, "disable cam_isp");
	udelay(10);

	/* VT_CORE_1.5V */
	ret = gpio_direction_output(GPIO_VT_CAM_15V, 0);
	CAM_CHECK_ERR(ret, "output vt_1.5v");
	udelay(10);

	/* CAM_SENSOR_A2.8V */
	ret = gpio_direction_output(GPIO_CAM_IO_EN, 0);
	CAM_CHECK_ERR(ret, "output io_en");
	udelay(10);

	/* CAM_ISP_CORE_1.2V */
	regulator = regulator_get(NULL, "cam_isp_core");
	if (IS_ERR(regulator))
		return -ENODEV;
	if (regulator_is_enabled(regulator))
		ret = regulator_force_disable(regulator);
	regulator_put(regulator);
	CAM_CHECK_ERR(ret, "disable isp_core");

	gpio_direction_input(VT_CAM_SDA_18V);
	s3c_gpio_setpull(VT_CAM_SDA_18V, S3C_GPIO_PULL_DOWN);
	gpio_direction_input(VT_CAM_SCL_18V);
	s3c_gpio_setpull(VT_CAM_SCL_18V, S3C_GPIO_PULL_DOWN);

	gpio_free(GPIO_CAM_VGA_nRST);
	gpio_free(GPIO_CAM_VGA_nSTBY);
	gpio_free(GPIO_VT_CAM_15V);
	gpio_free(GPIO_CAM_IO_EN);

	return 0;
}

static int s5k5bbgx_power(int onoff)
{
	int ret = 0;

	printk(KERN_INFO "%s(): %s\n", __func__, onoff ? "on" : "down");
	if (onoff) {
		ret = s5k5bbgx_power_on();
		if (unlikely(ret))
			goto error_out;
	} else {
		ret = s5k5bbgx_power_off();
		/* s3c_i2c0_force_stop();*/ /* DSLIM. Should be implemented */
	}

	/* ret = s3c_csis_power(onoff); */

error_out:
	return ret;
}

static struct s5k5bbgx_platform_data s5k5bbgx_plat = {
	.default_width = 640,
	.default_height = 480,
	.pixelformat = V4L2_PIX_FMT_UYVY,
	.freq = 24000000,
	.is_mipi = 0,
};

static struct i2c_board_info  s5k5bbgx_i2c_info = {
	I2C_BOARD_INFO("S5K5BBGX", 0x5A >> 1),
	.platform_data = &s5k5bbgx_plat,
};

static struct s3c_platform_camera s5k5bbgx = {
#if defined (CONFIG_VIDEO_S5K5BBGX_MIPI)
	.id		= CAMERA_CSI_D,
	.type		= CAM_TYPE_MIPI,
	.fmt		= ITU_601_YCBCR422_8BIT,
	.order422	= CAM_ORDER422_8BIT_CBYCRY,

	.mipi_lanes	= 1,
	.mipi_settle	= 6,
	.mipi_align	= 32,
#else
	.id		= CAMERA_PAR_A,
	.type		= CAM_TYPE_ITU,
	.fmt		= ITU_601_YCBCR422_8BIT,
	.order422	= CAM_ORDER422_8BIT_YCBYCR,
#endif
	.get_i2c_busnum	= s5k5bbgx_get_i2c_busnum,
	.info		= &s5k5bbgx_i2c_info,
	.pixelformat	= V4L2_PIX_FMT_UYVY,
	.srclk_name	= "xusbxti",
	.clk_name	= "sclk_cam0",
	.clk_rate	= 24000000,
	.line_length	= 640,
	.width		= 640,
	.height		= 480,
	.window		= {
		.left	= 0,
		.top	= 0,
		.width	= 640,
		.height	= 480,
	},

	/* Polarity */
	.inv_pclk	= 0,
	.inv_vsync	= 1,
	.inv_href	= 0,
	.inv_hsync	= 0,
	.reset_camera	= 0,
	.initialized	= 0,
	.cam_power	= s5k5bbgx_power,
};
#endif

#ifdef CONFIG_VIDEO_SR130PC10
static int sr130pc10_get_i2c_busnum(void)
{
	return 12;
}

static int sr130pc10_power_on(void)
{
	struct regulator *regulator;
	int ret = 0;

	/* printk("%s: in\n", __func__); */

	ret = gpio_request(GPIO_ISP_RESET, "GPY3");
	if (ret) {
		printk(KERN_ERR "faile to request gpio(ISP_RESET)\n");
		return ret;
	}
	ret = gpio_request(GPIO_CAM_IO_EN, "GPE2");
	if (ret) {
		printk(KERN_ERR "faile to request gpio(GPIO_CAM_IO_EN)\n");
		return ret;
	}
	ret = gpio_request(GPIO_VT_CAM_15V, "GPE2");
	if (ret) {
		printk(KERN_ERR "faile to request gpio(GPIO_VT_CAM_15V)\n");
		return ret;
	}
	ret = gpio_request(GPIO_CAM_VGA_nSTBY, "GPL2");
	if (ret) {
		printk(KERN_ERR "faile to request gpio(GPIO_CAM_VGA_nSTBY)\n");
		return ret;
	}
	ret = gpio_request(GPIO_CAM_VGA_nRST, "GPL2");
	if (ret) {
		printk(KERN_ERR "faile to request gpio(GPIO_CAM_VGA_nRST)\n");
		return ret;
	}

	s3c_gpio_setpull(VT_CAM_SDA_18V, S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(VT_CAM_SCL_18V, S3C_GPIO_PULL_NONE);

	/* ISP_RESET low*/
	ret = gpio_direction_output(GPIO_ISP_RESET, 0);
	CAM_CHECK_ERR_RET(ret, "output reset");
	udelay(100);

	/* CAM_ISP_CORE_1.2V */
	regulator = regulator_get(NULL, "cam_isp_core");
	if (IS_ERR(regulator))
		return -ENODEV;
	ret = regulator_enable(regulator);
	regulator_put(regulator);
	CAM_CHECK_ERR_RET(ret, "enable isp_core");
	udelay(10);

	/* CAM_SENSOR_A2.8V */
	ret = gpio_direction_output(GPIO_CAM_IO_EN, 1);
	CAM_CHECK_ERR_RET(ret, "output io_en");
	udelay(300); /* don't change me */

	/* VT_CORE_1.5V */
	ret = gpio_direction_output(GPIO_VT_CAM_15V, 1);
	CAM_CHECK_ERR_RET(ret, "output vt_15v");
	udelay(100);

	/* CAM_ISP_1.8V */
	regulator = regulator_get(NULL, "cam_isp");
	if (IS_ERR(regulator))
		return -ENODEV;
	ret = regulator_enable(regulator);
	regulator_put(regulator);
	CAM_CHECK_ERR_RET(ret, "enable cam_isp");
	udelay(10);

	/* VT_CAM_1.8V */
	regulator = regulator_get(NULL, "vt_cam_1.8v");
	if (IS_ERR(regulator))
		return -ENODEV;
	ret = regulator_enable(regulator);
	regulator_put(regulator);
	CAM_CHECK_ERR_RET(ret, "enable vt_1.8v");
	udelay(10);

	/* CAM_VGA_nSTBY */
	ret = gpio_direction_output(GPIO_CAM_VGA_nSTBY, 1);
	CAM_CHECK_ERR_RET(ret, "output VGA_nSTBY");
	udelay(50);

	/* Mclk */
	ret = s3c_gpio_cfgpin(GPIO_CAM_MCLK, S3C_GPIO_SFN(2));
	s3c_gpio_setpull(GPIO_CAM_MCLK, S3C_GPIO_PULL_NONE);
	CAM_CHECK_ERR_RET(ret, "cfg mclk");
	mdelay(12);

	/* CAM_VGA_nRST	 */
	ret = gpio_direction_output(GPIO_CAM_VGA_nRST, 1);
	CAM_CHECK_ERR_RET(ret, "output VGA_nRST");
	mdelay(1);

	gpio_free(GPIO_ISP_RESET);
	gpio_free(GPIO_CAM_IO_EN);
	gpio_free(GPIO_VT_CAM_15V);
	gpio_free(GPIO_CAM_VGA_nSTBY);
	gpio_free(GPIO_CAM_VGA_nRST);

	return 0;
}

static int sr130pc10_power_off(void)
{
	struct regulator *regulator;
	int ret = 0;

	/* printk("n%s: in\n", __func__); */

	ret = gpio_request(GPIO_CAM_VGA_nRST, "GPL2");
	if (ret) {
		printk(KERN_ERR "faile to request gpio(GPIO_CAM_VGA_nRST)\n");
		return ret;
	}
	ret = gpio_request(GPIO_CAM_VGA_nSTBY, "GPL2");
	if (ret) {
		printk(KERN_ERR "faile to request gpio(GPIO_CAM_VGA_nSTBY)\n");
		return ret;
	}
	ret = gpio_request(GPIO_VT_CAM_15V, "GPE2");
	if (ret) {
		printk(KERN_ERR "faile to request gpio(GPIO_VT_CAM_15V)\n");
		return ret;
	}
	ret = gpio_request(GPIO_CAM_IO_EN, "GPE2");
	if (ret) {
		printk(KERN_ERR "faile to request gpio(GPIO_CAM_IO_EN)\n");
		return ret;
	}

	mdelay(2);

	/* CAM_VGA_nRST	 */
	ret = gpio_direction_output(GPIO_CAM_VGA_nRST, 0);
	CAM_CHECK_ERR(ret, "output VGA_nRST");
	mdelay(2);

	/* Mclk */
	ret = s3c_gpio_cfgpin(GPIO_CAM_MCLK, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_CAM_MCLK, S3C_GPIO_PULL_DOWN);
	CAM_CHECK_ERR(ret, "cfg mclk");
	udelay(20);

	/* CAM_VGA_nSTBY */
	ret = gpio_direction_output(GPIO_CAM_VGA_nSTBY, 0);
	CAM_CHECK_ERR(ret, "output VGA_nSTBY");
	udelay(20);

	/* VT_CAM_1.8V */
	regulator = regulator_get(NULL, "vt_cam_1.8v");
	if (IS_ERR(regulator))
		return -ENODEV;
	if (regulator_is_enabled(regulator))
		ret = regulator_disable(regulator);
	regulator_put(regulator);
	CAM_CHECK_ERR(ret, "disable vt_1.8v");
	udelay(10);

	/* CAM_ISP_1.8V */
	regulator = regulator_get(NULL, "cam_isp");
	if (IS_ERR(regulator))
		return -ENODEV;
	if (regulator_is_enabled(regulator))
		ret = regulator_force_disable(regulator);
	regulator_put(regulator);
	CAM_CHECK_ERR(ret, "disable cam_isp");
	udelay(10);

	/* VT_CORE_1.5V */
	ret = gpio_direction_output(GPIO_VT_CAM_15V, 0);
	CAM_CHECK_ERR(ret, "output vt_1.5v");
	udelay(10);

	/* CAM_SENSOR_A2.8V */
	ret = gpio_direction_output(GPIO_CAM_IO_EN, 0);
	CAM_CHECK_ERR(ret, "output io_en");
	udelay(10);

	/* CAM_ISP_CORE_1.2V */
	regulator = regulator_get(NULL, "cam_isp_core");
	if (IS_ERR(regulator))
		return -ENODEV;
	if (regulator_is_enabled(regulator))
		ret = regulator_force_disable(regulator);
	regulator_put(regulator);
	CAM_CHECK_ERR(ret, "disable isp_core");

	gpio_direction_input(VT_CAM_SDA_18V);
	s3c_gpio_setpull(VT_CAM_SDA_18V, S3C_GPIO_PULL_DOWN);
	gpio_direction_input(VT_CAM_SCL_18V);
	s3c_gpio_setpull(VT_CAM_SCL_18V, S3C_GPIO_PULL_DOWN);

	gpio_free(GPIO_CAM_VGA_nRST);
	gpio_free(GPIO_CAM_VGA_nSTBY);
	gpio_free(GPIO_VT_CAM_15V);
	gpio_free(GPIO_CAM_IO_EN);

	return 0;
}

static int sr130pc10_power(int onoff)
{
	int ret = 0;

	printk(KERN_INFO "%s(): %s\n", __func__, onoff ? "on" : "down");
	if (onoff) {
		ret = sr130pc10_power_on();
		if (unlikely(ret))
			goto error_out;
	} else {
		ret = sr130pc10_power_off();
		/* s3c_i2c0_force_stop();*/ /* DSLIM. Should be implemented */
	}

	/* ret = s3c_csis_power(onoff); */

error_out:
	return ret;
}

static struct sr130pc10_platform_data sr130pc10_plat = {
	.default_width = 640,
	.default_height = 480,
	.pixelformat = V4L2_PIX_FMT_YUYV,
	.freq = 24000000,
	.is_mipi = 0,
};

static struct i2c_board_info  sr130pc10_i2c_info = {
	I2C_BOARD_INFO("SR130PC10", 0x40 >> 1),
	.platform_data = &sr130pc10_plat,
};

static struct s3c_platform_camera sr130pc10 = {
#if defined (CONFIG_VIDEO_SR130PC10_MIPI)
	.id		= CAMERA_CSI_D,
	.type		= CAM_TYPE_MIPI,
	.fmt		= ITU_601_YCBCR422_8BIT,
	.order422	= CAM_ORDER422_8BIT_CBYCRY,

	.mipi_lanes	= 1,
	.mipi_settle	= 6,
	.mipi_align	= 32,
#else
	.id		= CAMERA_PAR_A,
	.type		= CAM_TYPE_ITU,
	.fmt		= ITU_601_YCBCR422_8BIT,
	.order422	= CAM_ORDER422_8BIT_CBYCRY,
#endif
	.get_i2c_busnum	= sr130pc10_get_i2c_busnum,
	.info		= &sr130pc10_i2c_info,
	.pixelformat	= V4L2_PIX_FMT_YUYV,
	.srclk_name	= "xusbxti",
	.clk_name	= "sclk_cam0",
	.clk_rate	= 24000000,
	.line_length	= 480,
	.width		= 640,
	.height		= 480,
	.window		= {
		.left	= 0,
		.top	= 0,
		.width	= 640,
		.height	= 480,
	},

	/* Polarity */
	.inv_pclk	= 0,
	.inv_vsync	= 1,
	.inv_href	= 0,
	.inv_hsync	= 0,
	.reset_camera	= 0,
	.initialized	= 0,
	.cam_power	= sr130pc10_power,
};
#endif

#ifdef WRITEBACK_ENABLED
static int get_i2c_busnum_writeback(void)
{
	return 0;
}

static struct i2c_board_info  writeback_i2c_info = {
	I2C_BOARD_INFO("WriteBack", 0x0),
};

static struct s3c_platform_camera writeback = {
	.id		= CAMERA_WB,
	.fmt		= ITU_601_YCBCR422_8BIT,
	.order422	= CAM_ORDER422_8BIT_CBYCRY,
	.get_i2c_busnum	= get_i2c_busnum_writeback,
	.info		= &writeback_i2c_info,
	.pixelformat	= V4L2_PIX_FMT_YUV444,
	.line_length	= 800,
	.width		= 480,
	.height		= 800,
	.window		= {
		.left	= 0,
		.top	= 0,
		.width	= 480,
		.height	= 800,
	},

	.initialized	= 0,
};
#endif

void cam_cfg_gpio(struct platform_device *pdev)
{
	int ret = 0;
	printk(KERN_INFO "\n\n\n%s: pdev->id=%d\n", __func__, pdev->id);

	if (pdev->id != 0)
		return;

#ifdef CONFIG_VIDEO_S5K5BAFX
	if (system_rev >= 9) {/* Rev0.9 */
		ret = gpio_direction_input(VT_CAM_SDA_18V);
		CAM_CHECK_ERR(ret, "VT_CAM_SDA_18V");
		s3c_gpio_setpull(VT_CAM_SDA_18V, S3C_GPIO_PULL_DOWN);

		ret = gpio_direction_input(VT_CAM_SCL_18V);
		CAM_CHECK_ERR(ret, "VT_CAM_SCL_18V");
		s3c_gpio_setpull(VT_CAM_SCL_18V, S3C_GPIO_PULL_DOWN);
	}
#endif
}

/* Interface setting */
static struct s3c_platform_fimc fimc_plat = {
#ifdef CONFIG_ITU_A
	.default_cam	= CAMERA_PAR_A,
#endif
#ifdef CONFIG_ITU_B
	.default_cam	= CAMERA_PAR_B,
#endif
#ifdef CONFIG_CSI_C
	.default_cam	= CAMERA_CSI_C,
#endif
#ifdef CONFIG_CSI_D
	.default_cam	= CAMERA_CSI_D,
#endif
	.camera		= {
#ifdef CONFIG_VIDEO_M5MO
		&m5mo,
#endif
#ifdef CONFIG_VIDEO_S5K5BBGX
		&s5k5bbgx,
#endif
#ifdef CONFIG_VIDEO_S5K5BAFX
		&s5k5bafx,
#endif
#ifdef CONFIG_VIDEO_S5K6AAFX
		&s5k6aafx,
#endif
#ifdef CONFIG_VIDEO_SR130PC10
		&sr130pc10,
#endif
#ifdef WRITEBACK_ENABLED
		&writeback,
#endif
	},
	.hw_ver		= 0x51,
#if defined(CONFIG_VIDEO_S5K5BBGX) || defined(CONFIG_VIDEO_SR130PC10)
	.cfg_gpio	=s3c_fimc0_cfg_gpio,
#else
	.cfg_gpio	= cam_cfg_gpio,
#endif
};
#endif /* CONFIG_VIDEO_FIMC */

#ifdef CONFIG_VIDEO_M5MO_USE_SWI2C
static struct i2c_gpio_platform_data i2c25_platdata = {
	.sda_pin		= S5PV310_GPD1(0),
	.scl_pin		= S5PV310_GPD1(1),
	.udelay			= 5,	/* 250KHz */
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only	= 0,
};

static struct platform_device s3c_device_i2c25 = {
	.name = "i2c-gpio",
	.id = 25,
	.dev.platform_data = &i2c25_platdata,
};

/* I2C25 */
static struct i2c_board_info i2c_devs25_emul[] __initdata = {
	/* need to work here */
};
#endif

void c1_fimc_set_platdata(void)
{
#ifdef CONFIG_VIDEO_FIMC
	/* fimc */
	s3c_fimc0_set_platdata(&fimc_plat);
	s3c_fimc1_set_platdata(&fimc_plat);
	s3c_fimc2_set_platdata(&fimc_plat);
	s3c_fimc3_set_platdata(&fimc_plat);
#ifdef CONFIG_ITU_A
#endif
#ifdef CONFIG_ITU_B
	smdkv310_cam1_reset(1);
#endif
#ifdef CONFIG_VIDEO_FIMC_MIPI
	s3c_csis0_set_platdata(NULL);
	s3c_csis1_set_platdata(NULL);
#endif
#endif
}
