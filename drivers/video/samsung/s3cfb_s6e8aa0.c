/* linux/drivers/video/samsung/s3cfb_s6e8aa0.c
 *
 * MIPI-DSI based AMS529HA01 AMOLED lcd panel driver.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/ctype.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/workqueue.h>
#include <linux/backlight.h>
#include <linux/lcd.h>
#include <plat/gpio-cfg.h>
#include <plat/regs-dsim.h>
#include <mach/dsim.h>
#include <mach/mipi_ddi.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include <linux/platform_device.h>

#define SMART_DIMMING

#include "s5p-dsim.h"
#ifndef SMART_DIMMING
#include "s6e8aa0_gamma_j.h"
#include "s6e8aa0_gamma_k.h"
#include "s6e8aa0_gamma_l.h"
#endif
#ifdef SMART_DIMMING
#include "s6e8aa0_param.h"
#include "smart_dimming.h"
#endif

struct delayed_work hs_clk_re_try;
int count_dsim;

#define MIN_BRIGHTNESS		0
#define MAX_BRIGHTNESS		255
#define DEFAULT_GAMMA_LEVEL		12
#define DEFAULT_BRIGHTNESS		120

#define POWER_IS_ON(pwr)		((pwr) <= FB_BLANK_NORMAL)

#ifdef SMART_DIMMING
#define LDI_MTP_LENGTH		24
#define LDI_MTP_ADDR			0xd3

#define PANEL_ID_COMMAND		0xD1

#define ELVSS_MAX_LEVEL_BRIGHTNESS	300
#define ELVSS_3_LEVEL_BRIGHTNESS	200
#define ELVSS_2_LEVEL_BRIGHTNESS	160
#define ELVSS_1_LEVEL_BRIGHTNESS	100
#define ELVSS_MIN_LEVEL_BRIGHTNESS	0

#define ELVSS_OFFSET_MAX		0x00
#define ELVSS_OFFSET_2		0x08
#define ELVSS_OFFSET_1		0x0d
#define ELVSS_OFFSET_MIN		0x11

#define DYNAMIC_ELVSS_MIN_VALUE	0x81
#define DYNAMIC_ELVSS_MAX_VALUE	0x9F

#define ELVSS_MODE0_MIN_VOLTAGE	62
#define ELVSS_MODE1_MIN_VOLTAGE	52

#define RD_CNT				3

struct str_elvss {
	u8 reference;
	u8 limit;
};
#endif

struct lcd_info {
	unsigned int			bl;
	unsigned int			acl_enable;
	unsigned int			cur_acl;
	unsigned int			current_bl;

	unsigned int			ldi_enable;
	unsigned int			power;
	struct mutex			lock;

	struct device			*dev;
	struct lcd_device		*ld;
	struct backlight_device		*bd;
	struct lcd_platform_data	*lcd_pd;
	struct early_suspend		early_suspend;

	unsigned int			lcd_id;

	const unsigned char		**gamma_table;
#ifdef SMART_DIMMING
	unsigned int			support_elvss;
	struct str_smart_dim		smart;
	struct str_elvss		elvss;
	struct mutex			bl_lock;
#endif
	unsigned int			irq;
	unsigned int			connected;
};

struct lcd_info *lcd;

extern void s5p_dsim_frame_done_mask_enable(int state);
#ifdef SMART_DIMMING
static int s6e8ax0_adb_brightness_update(struct lcd_info *lcd, u32 br, u32 force);
static int s6e8aa0_update_brightness(struct lcd_info *lcd, u32 brightness);
#endif
static struct mipi_ddi_platform_data *ddi_pd;

static int s6e8ax0_write(struct lcd_info *lcd, const unsigned char *seq, int len)
{
	int size;
	const unsigned char *wbuf;

	if (!lcd->connected)
		return 0;

	mutex_lock(&lcd->lock);

	size = len;
	wbuf = seq;

	if (size == 1)
		ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_WR_NO_PARA, wbuf[0], 0);
	else if (size == 2)
		ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_WR_1_PARA, wbuf[0], wbuf[1]);
	else
		ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int)wbuf, size);

	mutex_unlock(&lcd->lock);

	return 0;
}

#ifdef SMART_DIMMING
static int s6e8ax0_read(struct lcd_info *lcd, const u8 addr, u16 count, u8 *buf)
{
	int ret = 0;

	if (!lcd->connected)
		return ret;

	mutex_lock(&lcd->lock);

	if (ddi_pd->cmd_read)
		ret = ddi_pd->cmd_read(ddi_pd->dsim_base, addr, count, buf);

	mutex_unlock(&lcd->lock);

	return ret;
}
#endif

static int s6e8ax0_set_link(void *pd, unsigned int dsim_base,
	unsigned char (*cmd_write) (unsigned int dsim_base, unsigned int data0,
	    unsigned int data1, unsigned int data2),
	int (*cmd_read) (u32 reg_base, u8 addr, u16 count, u8 *buf))
{
	struct mipi_ddi_platform_data *temp_pd = NULL;

	temp_pd = (struct mipi_ddi_platform_data *) pd;
	if (temp_pd == NULL) {
		printk(KERN_ERR "mipi_ddi_platform_data is null.\n");
		return -1;
	}

	ddi_pd = temp_pd;

	ddi_pd->dsim_base = dsim_base;

	if (cmd_write)
		ddi_pd->cmd_write = cmd_write;
	else
		printk(KERN_WARNING "cmd_write function is null.\n");

	if (cmd_read)
		ddi_pd->cmd_read = cmd_read;
	else
		printk(KERN_WARNING "cmd_read function is null.\n");

	return 0;
}

#ifndef SMART_DIMMING
static int s6e8ax0_gamma_ctl(struct lcd_info *lcd)
{
	s6e8ax0_write(lcd, lcd->gamma_table[lcd->bl], GAMMA_PARAM_SIZE);

	/* Gamma Set Update */
	s6e8ax0_write(lcd, SEQ_GAMMA_UPDATE, ARRAY_SIZE(SEQ_GAMMA_UPDATE));

	lcd->current_bl = lcd->bl;

	return 0;
}
#endif

static int s6e8ax0_set_elvss(struct lcd_info *lcd)
{
	int ret = 0;

	switch (lcd->bl) {
	case 0 ... 7: /* 30cd ~ 100cd */
		ret = s6e8ax0_write(lcd, ELVSS_TABLE[0], ELVSS_PARAM_SIZE);
		break;
	case 8 ... 13: /* 110cd ~ 160cd */
		ret = s6e8ax0_write(lcd, ELVSS_TABLE[1], ELVSS_PARAM_SIZE);
		break;
	case 14 ... 17: /* 170cd ~ 200cd */
		ret = s6e8ax0_write(lcd, ELVSS_TABLE[2], ELVSS_PARAM_SIZE);
		break;
	case 18 ... 26: /* 210cd ~ 290cd */
		ret = s6e8ax0_write(lcd, ELVSS_TABLE[3], ELVSS_PARAM_SIZE);
		break;
	default:
		break;
	}

	dev_dbg(&lcd->ld->dev, "level  = %d\n", lcd->bl);

	if (ret) {
		ret = -EPERM;
		goto elvss_err;
	}

elvss_err:
	return ret;
}

static int s6e8ax0_set_acl(struct lcd_info *lcd)
{
	int ret = 0;

	if (lcd->acl_enable) {
		if (lcd->cur_acl == 0) {
			if (lcd->bl == 0 || lcd->bl == 1) {
				s6e8ax0_write(lcd, SEQ_ACL_OFF, ARRAY_SIZE(SEQ_ACL_OFF));
				dev_dbg(&lcd->ld->dev, "%s : cur_acl=%d\n", __func__, lcd->cur_acl);
			} else
				s6e8ax0_write(lcd, SEQ_ACL_ON, ARRAY_SIZE(SEQ_ACL_ON));
		}
		switch (lcd->bl) {
		case 0 ... 1: /* 30cd ~ 40cd */
			if (lcd->cur_acl != 0) {
				s6e8ax0_write(lcd, SEQ_ACL_OFF, ARRAY_SIZE(SEQ_ACL_OFF));
				lcd->cur_acl = 0;
				dev_dbg(&lcd->ld->dev, "%s : cur_acl=%d\n", __func__, lcd->cur_acl);
			}
			break;
		case 2: /* 50cd */
			if (lcd->cur_acl != 200) {
				s6e8ax0_write(lcd, ACL_CUTOFF_TABLE[ACL_STATUS_20P], ACL_PARAM_SIZE);
				lcd->cur_acl = 200;
				dev_dbg(&lcd->ld->dev, "%s : cur_acl=%d\n", __func__, lcd->cur_acl);
			}
			break;
		case 3: /* 60cd */
			if (lcd->cur_acl != 330) {
				s6e8ax0_write(lcd, ACL_CUTOFF_TABLE[ACL_STATUS_33P], ACL_PARAM_SIZE);
				lcd->cur_acl = 330;
				dev_dbg(&lcd->ld->dev, "%s : cur_acl=%d\n", __func__, lcd->cur_acl);
			}
			break;
		case 4: /* 70cd */
			if (lcd->cur_acl != 430) {
				s6e8ax0_write(lcd, ACL_CUTOFF_TABLE[ACL_STATUS_43P], ACL_PARAM_SIZE);
				lcd->cur_acl = 430;
				dev_dbg(&lcd->ld->dev, "%s : cur_acl=%d\n", __func__, lcd->cur_acl);
			}
			break;
		case 5: /* 80cd */
			if (lcd->cur_acl != 450) {
				s6e8ax0_write(lcd, ACL_CUTOFF_TABLE[ACL_STATUS_45P_80CD], ACL_PARAM_SIZE);
				lcd->cur_acl = 450;
				dev_dbg(&lcd->ld->dev, "%s : cur_acl=%d\n", __func__, lcd->cur_acl);
			}
			break;
		case 6 ... 15: /* 90cd ~ 180cd */
			if (lcd->cur_acl != 451) {
				s6e8ax0_write(lcd, ACL_CUTOFF_TABLE[ACL_STATUS_45P], ACL_PARAM_SIZE);
				lcd->cur_acl = 451;
				dev_dbg(&lcd->ld->dev, "%s : cur_acl=%d\n", __func__, lcd->cur_acl);
			}
			break;
		case 16: /* 190cd */
			if (lcd->cur_acl != 480) {
				s6e8ax0_write(lcd, ACL_CUTOFF_TABLE[ACL_STATUS_48P], ACL_PARAM_SIZE);
				lcd->cur_acl = 480;
				dev_dbg(&lcd->ld->dev, "%s : cur_acl=%d\n", __func__, lcd->cur_acl);
			}
			break;
		case 17: /* 200cd */
			if (lcd->cur_acl != 500) {
				s6e8ax0_write(lcd, ACL_CUTOFF_TABLE[ACL_STATUS_50P], ACL_PARAM_SIZE);
				lcd->cur_acl = 500;
				dev_dbg(&lcd->ld->dev, "%s : cur_acl=%d\n", __func__, lcd->cur_acl);
			}
			break;
		case 18: /* 210cd */
			if (lcd->cur_acl != 520) {
				s6e8ax0_write(lcd, ACL_CUTOFF_TABLE[ACL_STATUS_52P], ACL_PARAM_SIZE);
				lcd->cur_acl = 520;
				dev_dbg(&lcd->ld->dev, "%s : cur_acl=%d\n", __func__, lcd->cur_acl);
			}
			break;
		case 19: /* 220cd */
			if (lcd->cur_acl != 530) {
				s6e8ax0_write(lcd, ACL_CUTOFF_TABLE[ACL_STATUS_53P], ACL_PARAM_SIZE);
				lcd->cur_acl = 530;
				dev_dbg(&lcd->ld->dev, "%s : cur_acl=%d\n", __func__, lcd->cur_acl);
			}
			break;
		case 20: /* 230cd */
			if (lcd->cur_acl != 550) {
				s6e8ax0_write(lcd, ACL_CUTOFF_TABLE[ACL_STATUS_55P_230CD], ACL_PARAM_SIZE);
				lcd->cur_acl = 550;
				dev_dbg(&lcd->ld->dev, "%s : cur_acl=%d\n", __func__, lcd->cur_acl);
			}
			break;
		case 21 ... 22: /* 240cd ~ 250cd */
			if (lcd->cur_acl != 551) {
				s6e8ax0_write(lcd, ACL_CUTOFF_TABLE[ACL_STATUS_55P_240CD], ACL_PARAM_SIZE);
				lcd->cur_acl = 551;
				dev_dbg(&lcd->ld->dev, "%s : cur_acl=%d\n", __func__, lcd->cur_acl);
			}
			break;
		case 23 ... 25: /* 260cd ~ 280cd */
			if (lcd->cur_acl != 552) {
				s6e8ax0_write(lcd, ACL_CUTOFF_TABLE[ACL_STATUS_55P_260CD], ACL_PARAM_SIZE);
				lcd->cur_acl = 552;
				dev_dbg(&lcd->ld->dev, "%s : cur_acl=%d\n", __func__, lcd->cur_acl);
			}
			break;
		default: /* 290cd */
			if (lcd->cur_acl != 553) {
				s6e8ax0_write(lcd, ACL_CUTOFF_TABLE[ACL_STATUS_55P_290CD], ACL_PARAM_SIZE);
				lcd->cur_acl = 553;
				dev_dbg(&lcd->ld->dev, "%s : cur_acl=%d\n", __func__, lcd->cur_acl);
			}
			break;
		}
	} else {
		s6e8ax0_write(lcd, SEQ_ACL_OFF, ARRAY_SIZE(SEQ_ACL_OFF));
		lcd->cur_acl = 0;
		dev_dbg(&lcd->ld->dev, "%s : cur_acl=%d\n", __func__, lcd->cur_acl);
	}

	if (ret) {
		ret = -EPERM;
		goto acl_err;
	}

acl_err:
	return ret;
}

#ifndef SMART_DIMMING
static int update_brightness(struct lcd_info *lcd)
{
	int ret;

	ret = s6e8ax0_gamma_ctl(lcd);
	if (ret)
		return -EPERM;

	ret = s6e8ax0_set_elvss(lcd);
	if (ret)
		return -EPERM;

	ret = s6e8ax0_set_acl(lcd);
	if (ret)
		return -EPERM;

	return 0;
}
#endif

static int s6e8ax0_ldi_init(struct lcd_info *lcd)
{
	int ret = 0;

	s6e8ax0_write(lcd, SEQ_APPLY_LEVEL_2_KEY, ARRAY_SIZE(SEQ_APPLY_LEVEL_2_KEY));
	msleep(20);
	s6e8ax0_write(lcd, SEQ_SLEEP_OUT, ARRAY_SIZE(SEQ_SLEEP_OUT));
	msleep(5);
	s6e8ax0_write(lcd, SEQ_PANEL_CONDITION_SET, ARRAY_SIZE(SEQ_PANEL_CONDITION_SET));
	s6e8ax0_write(lcd, SEQ_DISPLAY_CONDITION_SET, ARRAY_SIZE(SEQ_DISPLAY_CONDITION_SET));
#ifdef SMART_DIMMING
	s6e8aa0_update_brightness(lcd, 30);
#else
	s6e8ax0_write(lcd, lcd->gamma_table[DEFAULT_GAMMA_LEVEL], GAMMA_PARAM_SIZE);
	s6e8ax0_write(lcd, SEQ_GAMMA_UPDATE, ARRAY_SIZE(SEQ_GAMMA_UPDATE));
#endif
	s6e8ax0_write(lcd, SEQ_ETC_SOURCE_CONTROL, ARRAY_SIZE(SEQ_ETC_SOURCE_CONTROL));
	s6e8ax0_write(lcd, SEQ_ETC_PENTILE_CONTROL, ARRAY_SIZE(SEQ_ETC_PENTILE_CONTROL));
	s6e8ax0_write(lcd, SEQ_ETC_POWER_CONTROL, ARRAY_SIZE(SEQ_ETC_POWER_CONTROL));
	s6e8ax0_write(lcd, SEQ_ELVSS_NVM_SETTING, ARRAY_SIZE(SEQ_ELVSS_NVM_SETTING));
	s6e8ax0_write(lcd, SEQ_ELVSS_CONTROL, ARRAY_SIZE(SEQ_ELVSS_CONTROL));

	return ret;
}

static int s6e8ax0_ldi_enable(struct lcd_info *lcd)
{
	int ret = 0;

	s6e8ax0_write(lcd, SEQ_DISPLAY_ON, ARRAY_SIZE(SEQ_DISPLAY_ON));

	return ret;
}

static int s6e8ax0_ldi_disable(struct lcd_info *lcd)
{
	int ret = 0;

	s6e8ax0_write(lcd, SEQ_DISPLAY_OFF, ARRAY_SIZE(SEQ_DISPLAY_OFF));
	s6e8ax0_write(lcd, SEQ_STANDBY_ON, ARRAY_SIZE(SEQ_STANDBY_ON));

	return ret;
}

static int s6e8ax0_power_on(struct lcd_info *lcd)
{
	int ret = 0;
	struct lcd_platform_data *pd = NULL;
	pd = lcd->lcd_pd;

	/* dev_info(&lcd->ld->dev, "%s\n", __func__); */

	ret = s6e8ax0_ldi_init(lcd);

	if (ret) {
		dev_err(&lcd->ld->dev, "failed to initialize ldi.\n");
		goto err;
	}

	mdelay(130);

	ret = s6e8ax0_ldi_enable(lcd);
	if (ret) {
		dev_err(&lcd->ld->dev, "failed to enable ldi.\n");
		goto err;
	}

#ifdef SMART_DIMMING
	lcd->ldi_enable = 1;

	s6e8ax0_adb_brightness_update(lcd, lcd->bd->props.brightness, 1);
#else
	update_brightness(lcd);

	lcd->ldi_enable = 1;
#endif
err:
	return ret;
}

static int s6e8ax0_power_off(struct lcd_info *lcd)
{
	int ret = 0;

	dev_info(&lcd->ld->dev, "%s\n", __func__);

	lcd->ldi_enable = 0;

	ret = s6e8ax0_ldi_disable(lcd);

	mdelay(120);

	return ret;
}

static int s6e8ax0_power(struct lcd_info *lcd, int power)
{
	int ret = 0;

	if (POWER_IS_ON(power) && !POWER_IS_ON(lcd->power))
		ret = s6e8ax0_power_on(lcd);
	else if (!POWER_IS_ON(power) && POWER_IS_ON(lcd->power))
		ret = s6e8ax0_power_off(lcd);

	if (!ret)
		lcd->power = power;

	return ret;
}

static int s6e8ax0_set_power(struct lcd_device *ld, int power)
{
	struct lcd_info *lcd = lcd_get_data(ld);

	if (power != FB_BLANK_UNBLANK && power != FB_BLANK_POWERDOWN &&
		power != FB_BLANK_NORMAL) {
		dev_err(&lcd->ld->dev, "power value should be 0, 1 or 4.\n");
		return -EINVAL;
	}

	return s6e8ax0_power(lcd, power);
}

static int s6e8ax0_get_power(struct lcd_device *ld)
{
	struct lcd_info *lcd = lcd_get_data(ld);

	return lcd->power;
}

static int get_backlight_level_from_brightness(int brightness)
{
	int backlightlevel;

	/* brightness setting from platform is from 0 to 255
	 * But in this driver, brightness is only supported from 0 to 24 */

	switch (brightness) {
	case 0:
		backlightlevel = 0;
		break;
	case 1 ... 29:
		backlightlevel = 0;
		break;
	case 30 ... 34:
		backlightlevel = 1;
		break;
	case 35 ... 39:
		backlightlevel = 2;
		break;
	case 40 ... 44:
		backlightlevel = 3;
		break;
	case 45 ... 49:
		backlightlevel = 4;
		break;
	case 50 ... 54:
		backlightlevel = 5;
		break;
	case 55 ... 64:
		backlightlevel = 6;
		break;
	case 65 ... 74:
		backlightlevel = 7;
		break;
	case 75 ... 83:
		backlightlevel = 8;
		break;
	case 84 ... 93:
		backlightlevel = 9;
		break;
	case 94 ... 103:
		backlightlevel = 10;
		break;
	case 104 ... 113:
		backlightlevel = 11;
		break;
	case 114 ... 122:
		backlightlevel = 12;
		break;
	case 123 ... 132:
		backlightlevel = 13;
		break;
	case 133 ... 142:
		backlightlevel = 14;
		break;
	case 143 ... 152:
		backlightlevel = 15;
		break;
	case 153 ... 162:
		backlightlevel = 16;
		break;
	case 163 ... 171:
		backlightlevel = 17;
		break;
	case 172 ... 181:
		backlightlevel = 18;
		break;
	case 182 ... 191:
		backlightlevel = 19;
		break;
	case 192 ... 201:
		backlightlevel = 20;
		break;
	case 202 ... 210:
		backlightlevel = 21;
		break;
	case 211 ... 220:
		backlightlevel = 22;
		break;
	case 221 ... 230:
		backlightlevel = 23;
		break;
	case 231 ... 240:
		backlightlevel = 24;
		break;
	case 241 ... 250:
		backlightlevel = 25;
		break;
	case 251 ... 255:
		backlightlevel = 26;
		break;
	default:
		backlightlevel = DEFAULT_GAMMA_LEVEL;
		break;
	}
	return backlightlevel;
}

#ifdef SMART_DIMMING
static int s6e8aa0_update_brightness(struct lcd_info *lcd, u32 brightness)
{
	int ret = 0;
	u8 gamma_regs[GAMMA_PARAM_SIZE] = {0,};
	u32 gamma;
#if 0
	int i;
#endif

	gamma_regs[0] = 0xFA;
	gamma_regs[1] = 0x01;

	gamma = brightness;

	calc_gamma_table(&lcd->smart, gamma, gamma_regs+2);

	s6e8ax0_write(lcd, gamma_regs, GAMMA_PARAM_SIZE);

#if 0
	printk("##### print gamma reg #####\n");
	for (i = 0; i < 26; i++)
		printk("[%02d] : %02x\n", i, gamma_regs[i]);
#endif

	s6e8ax0_write(lcd, SEQ_GAMMA_UPDATE, sizeof(SEQ_GAMMA_UPDATE));

	return ret;
}

u8 get_offset_brightness(u32 brightness)
{
	u8 offset = 0;

	switch (brightness) {
	case 0 ... 100:
		offset = ELVSS_OFFSET_MIN;
		break;
	case 101 ... 160:
		offset = ELVSS_OFFSET_1;
		break;
	case 161 ... 200:
		offset = ELVSS_OFFSET_2;
		break;
	case 201 ...  300:
		offset = ELVSS_OFFSET_MAX;
		break;
	default:
		offset = ELVSS_OFFSET_MAX;
		break;
	}
	return offset;
}

u8 get_elvss_voltage_value(u8 limit, u8 elvss)
{
	u8 volt;

	if (limit == 0x00)
		volt = ELVSS_MODE0_MIN_VOLTAGE - (elvss-1);
	else if (limit == 0x01)
		volt = ELVSS_MODE1_MIN_VOLTAGE - (elvss-1);
	else {
		printk("[ERROR:ELVSS]:%s undefined elvss limit value :%x\n", __func__, limit);
		return 0;
	}
	return volt;
}

u8 get_elvss_value(struct lcd_info *lcd, u32 brightness)
{
	u8 ref = 0;
	u8 offset;

	if (lcd->elvss.limit == 0x00)
		ref = (lcd->elvss.reference | 0x80);
	else if (lcd->elvss.limit == 0x01)
		ref = (lcd->elvss.reference + 0x40);
	else {
		printk("[ERROR:ELVSS]:%s undefined elvss limit value :%x\n", __func__, lcd->elvss.limit);
		return 0;
	}

	offset = get_offset_brightness(brightness);
	ref += offset;

	if (ref < DYNAMIC_ELVSS_MIN_VALUE)
		ref = DYNAMIC_ELVSS_MIN_VALUE;
	else if (ref > DYNAMIC_ELVSS_MAX_VALUE)
		ref = DYNAMIC_ELVSS_MAX_VALUE;

	return ref;
}

int s6e8aa0_update_elvss(struct lcd_info *lcd, u32 brightness)
{
	u8 elvss_cmd[3];
	u8 elvss;

	elvss = get_elvss_value(lcd, brightness);
	if (!elvss) {
		printk("[ERROR:LCD]:%s:get_elvss_value() failed\n", __func__);
		return -1;
	}

	elvss_cmd[0] = 0xb1;
	elvss_cmd[1] = 0x04;
	elvss_cmd[2] = elvss;

	//printk("elvss reg : %02x, Volt : %d", elvss_cmd[2], get_elvss_voltage_value(lcd->elvss.limit, elvss));
	s6e8ax0_write(lcd, elvss_cmd, sizeof(elvss_cmd));

	return 0;
}


u32 transform_gamma(u32 brightness)
{
	u32 gamma;

	switch (brightness) {
	case 0:
		gamma = 30;
		break;
	case 1 ... 29:
		gamma = 30;
		break;
	case 30 ... 34:
		gamma = 40;
		break;
	case 35 ... 39:
		gamma = 50;
		break;
	case 40 ... 44:
		gamma = 60;
		break;
	case 45 ... 49:
		gamma = 70;
		break;
	case 50 ... 54:
		gamma = 80;
		break;
	case 55 ... 64:
		gamma = 90;
		break;
	case 65 ... 74:
		gamma = 100;
		break;
	case 75 ... 83:
		gamma = 110;
		break;
	case 84 ... 93:
		gamma = 120;
		break;
	case 94 ... 103:
		gamma = 130;
		break;
	case 104 ... 113:
		gamma = 140;
		break;
	case 114 ... 122:
		gamma = 150;
		break;
	case 123 ... 132:
		gamma = 160;
		break;
	case 133 ... 142:
		gamma = 170;
		break;
	case 143 ... 152:
		gamma = 180;
		break;
	case 153 ... 162:
		gamma = 190;
		break;
	case 163 ... 171:
		gamma = 200;
		break;
	case 172 ... 181:
		gamma = 210;
		break;
	case 182 ... 191:
		gamma = 220;
		break;
	case 192 ... 201:
		gamma = 230;
		break;
	case 202 ... 210:
		gamma = 240;
		break;
	case 211 ... 220:
		gamma = 250;
		break;
	case 221 ... 230:
		gamma = 260;
		break;
	case 231 ... 240:
		gamma = 270;
		break;
	case 241 ... 250:
		gamma = 280;
		break;
	case 251 ... 255:
		gamma = 290;
		break;
	default:
		gamma = 150;
		break;
	}
	return gamma - 1;
}

static int s6e8ax0_adb_brightness_update(struct lcd_info *lcd, u32 br, u32 force)
{
	u32 gamma;
	int ret = 0;

	mutex_lock(&lcd->bl_lock);

	lcd->bl = get_backlight_level_from_brightness(br);

	if ((force) || ((lcd->ldi_enable) && (lcd->current_bl != lcd->bl))) {
		gamma = transform_gamma(br);
		dev_info(&lcd->ld->dev, "brightness=%d, gamma=%d\n", br, gamma);

		ret = s6e8aa0_update_brightness(lcd, gamma);

		ret = s6e8ax0_set_acl(lcd);

		if (lcd->support_elvss)
			ret = s6e8aa0_update_elvss(lcd, gamma);
		else
			ret = s6e8ax0_set_elvss(lcd);

		lcd->current_bl = lcd->bl;
	}
	mutex_unlock(&lcd->bl_lock);
	return ret;
}
#endif

#ifdef SMART_DIMMING
static int s6e8ax0_set_brightness(struct backlight_device *bd)
{
	int ret = 0;
	u32 brightness = (u32)bd->props.brightness;

	struct lcd_info *lcd = bl_get_data(bd);

	/* global_brightness = brightness;

	if (brightness == lcd->bl)
		return 0;

	lcd->bl = brightness; */

	if (brightness < MIN_BRIGHTNESS ||
		brightness > bd->props.max_brightness) {
		dev_err(&bd->dev, "lcd brightness should be %d to %d. now %d\n",
		MIN_BRIGHTNESS, MAX_BRIGHTNESS, brightness);
		return -EINVAL;
	}

	/* dev_info(&lcd->ld->dev, "brightness=%d\n", brightness); */

	ret = s6e8ax0_adb_brightness_update(lcd, brightness, 0);

	return ret;
}
#else
static int s6e8ax0_set_brightness(struct backlight_device *bd)
{
	int ret = 0;
	int brightness = bd->props.brightness;
	struct lcd_info *lcd = bl_get_data(bd);

	if (brightness < MIN_BRIGHTNESS ||
		brightness > bd->props.max_brightness) {
		dev_err(&bd->dev, "lcd brightness should be %d to %d. now %d\n",
			MIN_BRIGHTNESS, MAX_BRIGHTNESS, brightness);
		return -EINVAL;
	}

	lcd->bl = get_backlight_level_from_brightness(brightness);

	if ((lcd->ldi_enable) && (lcd->current_bl != lcd->bl)) {
		ret = update_brightness(lcd);
		dev_info(&lcd->ld->dev, "id=%d, brightness=%d, bl=%d\n", lcd->lcd_id, bd->props.brightness, lcd->bl);
		if (ret < 0) {
			dev_err(lcd->dev, "err in %s\n", __func__);
			return -EINVAL;
		}
	}

	return ret;
}
#endif

static int s6e8ax0_get_brightness(struct backlight_device *bd)
{
	return bd->props.brightness;
}

static struct lcd_ops s6e8ax0_lcd_ops = {
	.set_power = s6e8ax0_set_power,
	.get_power = s6e8ax0_get_power,
};

static const struct backlight_ops s6e8ax0_backlight_ops  = {
	.get_brightness = s6e8ax0_get_brightness,
	.update_status = s6e8ax0_set_brightness,
};

static ssize_t lcd_power_store(struct device *dev,
				       struct device_attribute *attr,
				       const char *buf, size_t len)
{
	int rc;
	int lcd_enable;
	struct lcd_info *lcd = dev_get_drvdata(dev);

	dev_info(dev, "%s\n", __func__);

	rc = strict_strtoul(buf, 0, (unsigned long *)&lcd_enable);
	if (rc < 0)
		return rc;

	if (lcd_enable)
		s6e8ax0_power(lcd, FB_BLANK_UNBLANK);
	else
		s6e8ax0_power(lcd, FB_BLANK_POWERDOWN);

	return len;
}

static DEVICE_ATTR(lcd_power, 0220, NULL, lcd_power_store);

#ifdef SMART_DIMMING
const u8 enable_mtp_register[] = {
	0xf1,
	0x5a,
	0x5a,
};

const u8 disable_mtp_register[] = {
	0xf1,
	0xa5,
	0xa5,
};

#if 0
static s16 s9_to_s16(s16 v)
{
	return (s16)(v << 7) >> 7;
}
#endif

int s6e8aa0_read_id(struct lcd_info *lcd)
{
	int ret, i;
	char rd_buf[RD_CNT] = {0,};
	int rdcnt = RD_CNT;

	//struct lcd_info *lcd = dev_get_drvdata(dev);

	ret = s6e8ax0_read(lcd, 0xd1, rdcnt, rd_buf);
	if (!ret) {
		printk("[ERROR:LCD] : %s :s6e8ax0_read failed\n", __func__);
		return -1;
	}

	printk("MIPI Read Done Read Byte : %d\n"
		"Read Data : ", ret);

	for (i = 0; i < rdcnt; i++)
		printk(" %x ", rd_buf[i]);
	printk("\n");

	lcd->elvss.limit = rd_buf[2] & 0xc0;
	lcd->elvss.reference = rd_buf[2] & 0x3f;

	printk("Dynamic ELVSS Information\n");
	printk("limit    : %02x\n", lcd->elvss.limit);
	printk("Refrence : %02x Volt : %d\n", lcd->elvss.reference,
	get_elvss_voltage_value(lcd->elvss.limit, lcd->elvss.reference));

	return 0;
}

int s6e8ax0_read_mtp(struct lcd_info *lcd, u8 *mtp_data)
{
	int ret;

	s6e8ax0_write(lcd, enable_mtp_register, ARRAY_SIZE(enable_mtp_register));
	ret = s6e8ax0_read(lcd, LDI_MTP_ADDR, LDI_MTP_LENGTH, mtp_data);
	if (!ret) {
		printk("ERROR:MTP read failed\n");
		return 0;
	}
	s6e8ax0_write(lcd, disable_mtp_register, ARRAY_SIZE(disable_mtp_register));
	return ret;
}
#endif

static ssize_t lcdtype_show(struct device *dev, struct
	device_attribute *attr, char *buf)
{
	char temp[15];
	sprintf(temp, "SMD_AMS529HA01\n");
	strcat(buf, temp);
	return strlen(buf);
}

static DEVICE_ATTR(lcdtype, 0444, lcdtype_show, NULL);

static ssize_t acl_enable_show(struct device *dev, struct
	device_attribute *attr, char *buf)
{
	struct lcd_info *lcd = dev_get_drvdata(dev);
	char temp[3];

	sprintf(temp, "%d\n", lcd->acl_enable);
	strcpy(buf, temp);

	return strlen(buf);
}
static ssize_t acl_enable_store(struct device *dev, struct
	device_attribute *attr, const char *buf, size_t size)
{
	struct lcd_info *lcd = dev_get_drvdata(dev);
	int value;
	int rc;

	rc = strict_strtoul(buf, (unsigned int)0, (unsigned long *)&value);
	if (rc < 0)
		return rc;
	else {
		if (lcd->acl_enable != value) {
			dev_info(dev, "%s - %d, %d\n", __func__, lcd->acl_enable, value);
			lcd->acl_enable = value;
			if (lcd->ldi_enable)
				s6e8ax0_set_acl(lcd);
		}
		return 0;
	}
}

static DEVICE_ATTR(acl_set, 0664, acl_enable_show, acl_enable_store);

static ssize_t gamma_table_show(struct device *dev, struct
	device_attribute *attr, char *buf)
{
	struct lcd_info *lcd = dev_get_drvdata(dev);
	int i, j;

	for (i = 0; i < GAMMA_2_2_MAX; i++) {
		for (j = 0; j < GAMMA_PARAM_SIZE; j++)
			printk("0x%02x, ", lcd->gamma_table[i][j]);
		printk("\n");
	}

	return strlen(buf);
}
static DEVICE_ATTR(gamma_table, 0444, gamma_table_show, NULL);

#ifdef SMART_DIMMING
static ssize_t mtp_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int i, j;
	unsigned int cnt;
	struct lcd_info *lcd = dev_get_drvdata(dev);
	const char *ivstr[IV_MAX] = {
		"V1",
		"V15",
		"V35",
		"V59",
		"V87",
		"V171",
		"V255"
	};

	cnt = sprintf(buf, "============ MTP VALUE ============\n");
	for (i = IV_1; i < IV_MAX; i++) {
		cnt += sprintf(buf+cnt, "[%5s] : ", ivstr[i]);
		for (j = CI_RED; j < CI_MAX; j++)
			cnt += sprintf(buf+cnt, "%4d ", lcd->smart.mtp[j][i]);
		cnt += sprintf(buf+cnt, "\n");
	}
	return cnt;
}


static DEVICE_ATTR(mtp, 0444, mtp_show, NULL);

static ssize_t id_show(struct device *dev, struct
	device_attribute *attr, char *buf)
{
	int cnt, ret;
	char rd_buf[RD_CNT] = {0,};
	int rdcnt = RD_CNT;
	struct lcd_info *lcd = dev_get_drvdata(dev);

	ret = s6e8ax0_read(lcd, 0xd1, rdcnt, rd_buf);
	if (!ret) {
		printk("[ERROR:LCD] : %s :s6e8ax0_read failed\n", __func__);
		return -1;
	}
	cnt = sprintf(buf, "READ ID  : 0x%02x 0x%02x 0x%02x\n", rd_buf[0], rd_buf[1], rd_buf[2]);

	return cnt;
}

static DEVICE_ATTR(id, 0444, id_show, NULL);

static ssize_t gamma_show(struct device *dev, struct
	device_attribute *attr, char *buf)
{
	//this function show default gamma value for 300cd
	int i, j;
	unsigned int cnt;
	struct lcd_info *lcd = dev_get_drvdata(dev);

	cnt = sprintf(buf, "=============== default gamma ===============\n");
	for (i = 0; i < 4; i++) {
		for (j = 0; j < 6; j++)
			cnt += sprintf(buf + cnt, "0x%02x, ", lcd->smart.default_gamma[i*6+j]);
		cnt += sprintf(buf+cnt, "\n");
	}
	return cnt;
}

static ssize_t gamma_store(struct device *dev, struct
	device_attribute *attr, const char *buf, size_t size)
{
	// this function can set brightness
	struct lcd_info *lcd = dev_get_drvdata(dev);
	int value;

	sscanf(buf, "%d", &value);
	printk("%s : %d\n", __func__, value);
	s6e8ax0_adb_brightness_update(lcd, value, 1);

	return size;
}

static DEVICE_ATTR(gamma, 0444, gamma_show, gamma_store);
#endif

static void hs_clk_re_try_work(struct work_struct *work)
{
	int read_oled_det;

	read_oled_det = gpio_get_value(GPIO_OLED_DET);

	printk(KERN_INFO "%s, %d, %d\n", __func__, count_dsim, read_oled_det);

	if (read_oled_det == 0) {
		if (count_dsim < 10) {
			schedule_delayed_work(&hs_clk_re_try, HZ/8);
			count_dsim++;
			s5p_dsim_frame_done_mask_enable(1);
		} else
			s5p_dsim_frame_done_mask_enable(0);
	} else
		s5p_dsim_frame_done_mask_enable(0);
}

static irqreturn_t oled_det_int(int irq, void *dev_id)
{
	printk(KERN_INFO "[DSIM] %s\n", __func__);

	schedule_delayed_work(&hs_clk_re_try, HZ/16);

	count_dsim = 0;

	return IRQ_HANDLED;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
void s6e8ax0_early_suspend(void)
{
	printk("+%s\n", __func__);
	disable_irq(lcd->irq);
	s6e8ax0_power(lcd, FB_BLANK_POWERDOWN);
	printk("-%s\n", __func__);

	return ;
}

void s6e8ax0_late_resume(void)
{
	printk("+%s\n", __func__);
	s6e8ax0_power(lcd, FB_BLANK_UNBLANK);
	enable_irq(lcd->irq);
	printk("-%s\n", __func__);

	return ;
}
#endif

static int s6e8ax0_probe(struct device *dev)
{
	int ret = 0;
#ifndef SMART_DIMMING
	struct platform_device *pdev = to_platform_device(dev);
	struct s5p_platform_dsim *pd = (struct s5p_platform_dsim *)pdev->dev.platform_data;
#endif
#ifdef SMART_DIMMING
	u8 mtp_data[LDI_MTP_LENGTH] = {0,};
	u32 i;
	u8 id_buf[3] = {0,};
#endif

	lcd = kzalloc(sizeof(struct lcd_info), GFP_KERNEL);
	if (!lcd) {
		pr_err("failed to allocate for lcd\n");
		ret = -ENOMEM;
		goto err_alloc;
	}

	lcd->ld = lcd_device_register("lcd", dev,
		lcd, &s6e8ax0_lcd_ops);
	if (IS_ERR(lcd->ld)) {
		pr_err("failed to register lcd device\n");
		ret = PTR_ERR(lcd->ld);
		goto out_free_lcd;
	}

	lcd->bd = backlight_device_register("pwm-backlight", dev,
		lcd, &s6e8ax0_backlight_ops, NULL);
	if (IS_ERR(lcd->bd)) {
		pr_err("failed to register backlight device\n");
		ret = PTR_ERR(lcd->bd);
		goto out_free_backlight;
	}

	lcd->dev = dev;
	lcd->bd->props.max_brightness = MAX_BRIGHTNESS;
	lcd->bd->props.brightness = DEFAULT_BRIGHTNESS;
	lcd->bl = DEFAULT_GAMMA_LEVEL;
	lcd->current_bl = lcd->bl;

	lcd->acl_enable = 0;
	lcd->cur_acl = 0;

	lcd->power = FB_BLANK_UNBLANK;
	lcd->ldi_enable = 1;
	lcd->connected = 1;

	ret = device_create_file(lcd->dev, &dev_attr_acl_set);
	if (ret < 0)
		dev_err(&lcd->ld->dev, "failed to add sysfs entries, %d\n", __LINE__);

	ret = device_create_file(lcd->dev, &dev_attr_lcdtype);
	if (ret < 0)
		dev_err(&lcd->ld->dev, "failed to add sysfs entries, %d\n", __LINE__);

	ret = device_create_file(lcd->dev, &dev_attr_lcd_power);
	if (ret < 0)
		dev_err(&lcd->ld->dev, "failed to add sysfs entries, %d\n", __LINE__);

	ret = device_create_file(&lcd->ld->dev, &dev_attr_gamma_table);
	if (ret < 0)
		dev_err(&lcd->ld->dev, "failed to add sysfs entries, %d\n", __LINE__);

#ifdef SMART_DIMMING
	ret = device_create_file(lcd->dev, &dev_attr_mtp);
	if (ret < 0)
		dev_err(&lcd->ld->dev, "failed to add sysfs entries, %d\n", __LINE__);

	ret = device_create_file(lcd->dev, &dev_attr_id);
	if (ret < 0)
		dev_err(&lcd->ld->dev, "failed to add sysfs entries, %d\n", __LINE__);

	ret = device_create_file(lcd->dev, &dev_attr_gamma);
	if (ret < 0)
		dev_err(&lcd->ld->dev, "failed to add sysfs entries, %d\n", __LINE__);
#endif
	dev_set_drvdata(dev, lcd);
#if 0
#ifdef CONFIG_HAS_EARLYSUSPEND
	lcd->early_suspend.suspend = s6e8ax0_early_suspend;
	lcd->early_suspend.resume = s6e8ax0_late_resume;
	lcd->early_suspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB - 2;
	register_early_suspend(&lcd->early_suspend);
#endif
#endif
#ifndef SMART_DIMMING
	lcd->lcd_id = pd->dsim_lcd_info->lcd_id;

	if (lcd->lcd_id == 1) {
		memcpy(&SEQ_PANEL_CONDITION_SET, &SEQ_PANEL_CONDITION_SET_REVJ, sizeof(SEQ_PANEL_CONDITION_SET_REVJ));
		lcd->gamma_table = gamma22_table_j;
		dev_info(&lcd->ld->dev, "s6e8aa0 lcd panel gamma data has been switched for M3 1-3\n");
	} else if (lcd->lcd_id == 2) {
		lcd->gamma_table = gamma22_table_k;
		dev_info(&lcd->ld->dev, "s6e8aa0 lcd panel gamma data has been switched for M3 1-4\n");
	} else if (lcd->lcd_id == 3) {
		lcd->gamma_table = gamma22_table_l;
		dev_info(&lcd->ld->dev, "s6e8aa0 lcd panel gamma data has been switched for M3 1-5\n");
	} else {
		lcd->gamma_table = gamma22_table_l;
		dev_info(&lcd->ld->dev, "unknown lcd id : s6e8aa0 lcd panel gamma data has been switched for M3 1-3\n");
	}
#endif

	mutex_init(&lcd->lock);

	dev_info(&lcd->ld->dev, "s6e8aa0 lcd panel driver has been probed.\n");

#ifdef SMART_DIMMING
	mutex_init(&lcd->bl_lock);

	//read mpt
	ret = s6e8ax0_read(lcd, PANEL_ID_COMMAND, 3, id_buf);
	if (!ret) {
		printk("[LCD:ERROR] : %s read id failed\n", __func__);
		//return -1;
	}

	printk("Read ID : %x, %x, %x\n", id_buf[0], id_buf[1], id_buf[2]);

	if (id_buf[2] == 0x33) {
		lcd->support_elvss = 0;
		printk("ID-3 is 0xff does not support dynamic elvss\n");
	} else {
		lcd->support_elvss = 1;
		lcd->elvss.limit = (id_buf[2] & 0xc0) >> 6;
		lcd->elvss.reference = id_buf[2] & 0x3f;
		printk("ID-3 is 0x%x support dynamic elvss\n", id_buf[2]);
		printk("Dynamic ELVSS Information\n");
		printk("limit    : %02x\n", lcd->elvss.limit);
		printk("Refrence : %02x Volt : %d\n", lcd->elvss.reference,
		get_elvss_voltage_value(lcd->elvss.limit, lcd->elvss.reference));
	}

	for (i = 0; i < PANEL_ID_MAX; i++)
		lcd->smart.panelid[i] = id_buf[i];

	init_table_info(&lcd->smart);

	ret = s6e8ax0_read_mtp(lcd, mtp_data);
	if (!ret) {
		printk("[LCD:ERROR] : %s read mtp failed\n", __func__);
		//return -1;
	}

	calc_voltage_table(&lcd->smart, mtp_data);

	s6e8ax0_adb_brightness_update(lcd, lcd->bd->props.brightness, 1);
#endif

	if (id_buf[0] == 0xa2) {
		lcd->connected = 1;
		INIT_DELAYED_WORK(&hs_clk_re_try, hs_clk_re_try_work);

		lcd->irq = gpio_to_irq(GPIO_OLED_DET);

		s3c_gpio_cfgpin(GPIO_OLED_DET, S3C_GPIO_SFN(0xf));
		s3c_gpio_setpull(GPIO_OLED_DET, S3C_GPIO_PULL_NONE);
		set_irq_type(lcd->irq, IRQ_TYPE_EDGE_FALLING);

		if (request_irq(lcd->irq, oled_det_int, IRQF_TRIGGER_FALLING, "vgh_toggle", 0)) {
			pr_err("failed to reqeust irq. %d\n", lcd->irq);
			ret = -EINVAL;
			goto out_free_backlight;
		}
	} else {
		lcd->connected = 0;
		dev_info(&lcd->ld->dev, "panel is not connected\n");
	}

	return 0;

out_free_backlight:
	lcd_device_unregister(lcd->ld);
	kfree(lcd);
	return ret;

out_free_lcd:
	kfree(lcd);
	return ret;

err_alloc:
	return ret;
}


static int __devexit s6e8ax0_remove(struct device *dev)
{
	struct lcd_info *lcd = dev_get_drvdata(dev);

	s6e8ax0_power(lcd, FB_BLANK_POWERDOWN);
	lcd_device_unregister(lcd->ld);
	backlight_device_unregister(lcd->bd);
	kfree(lcd);

	return 0;
}

/* Power down all displays on reboot, poweroff or halt. */
static void s6e8ax0_shutdown(struct device *dev)
{
	struct lcd_info *lcd = dev_get_drvdata(dev);

	s6e8ax0_power(lcd, FB_BLANK_POWERDOWN);
}

static struct mipi_lcd_driver s6e8ax0_mipi_driver = {
	.name = "s6e8aa0",
/*	.init			= s6e8ax0_panel_init, */
/*	.display_on		= s6e8ax0_display_on, */
/*	.display_off		= s6e8ax0_display_off, */
	.set_link		= s6e8ax0_set_link,
	.probe			= s6e8ax0_probe,
	.remove			= __devexit_p(s6e8ax0_remove),
	.shutdown		= s6e8ax0_shutdown,

#if 0
	/* add lcd partial and dim mode functionalies */
	.partial_mode_status = s6e8ax0_partial_mode_status,
	.partial_mode_on	= s6e8ax0_partial_mode_on,
	.partial_mode_off	= s6e8ax0_partial_mode_off,
#endif
};

static int s6e8ax0_init(void)
{
	return s5p_dsim_register_lcd_driver(&s6e8ax0_mipi_driver);
}

static void s6e8ax0_exit(void)
{
	return;
}

module_init(s6e8ax0_init);
module_exit(s6e8ax0_exit);

MODULE_DESCRIPTION("MIPI-DSI S6E8AA0:AMS529HA01 (800x1280) Panel Driver");
MODULE_LICENSE("GPL");
