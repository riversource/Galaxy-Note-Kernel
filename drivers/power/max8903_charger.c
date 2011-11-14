/*
 * max8903_charger.c - Maxim 8903 USB/Adapter Charger Driver
 *
 * Copyright (C) 2011 Samsung Electronics
 * MyungJoo Ham <myungjoo.ham@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/power_supply.h>
#include <linux/max8903_charger.h>

struct max8903_info{
	struct max8903_platform_data *pdata;
	struct device *dev;
	struct power_supply psy_bat;
	bool is_usb_cable;
	int irq_chg_ing;
};

static enum power_supply_property max8903_battery_props[] = {
	POWER_SUPPLY_PROP_STATUS, /* Charger status output */
	POWER_SUPPLY_PROP_ONLINE, /* External power source */
	//POWER_SUPPLY_PROP_HEALTH, /* Fault or OK */
};

static inline int max8903_is_charging(struct max8903_info *info)
{
	int ta_nconnected = gpio_get_value(info->pdata->gpio_ta_nconnected);
	int ta_nchg = gpio_get_value(info->pdata->gpio_ta_nchg);

	dev_info(info->dev, "%s: charging state = 0x%x\n", __func__,
		 (ta_nconnected << 1) | ta_nchg);

	/*return (ta_nconnected == 0 && ta_nchg == 0); */
	return (ta_nconnected << 1) | ta_nchg;
}


static int max8903_get_property(struct power_supply *psy,
				enum power_supply_property psp,
				union power_supply_propval *val)
{
	struct max8903_info *info =
	    container_of(psy, struct max8903_info, psy_bat);

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		switch (max8903_is_charging(info)) {
		case 0:
			val->intval = POWER_SUPPLY_STATUS_CHARGING;
			break;
		case 1:
			val->intval = POWER_SUPPLY_STATUS_NOT_CHARGING;
			break;
		default:
			val->intval = POWER_SUPPLY_STATUS_DISCHARGING;
			break;
		}
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		/* battery is always online */
		val->intval = 1;
		break;
	/*case POWER_SUPPLY_PROP_HEALTH:
		val->intval = POWER_SUPPLY_HEALTH_GOOD;
		if (data->fault)
			val->intval = POWER_SUPPLY_HEALTH_UNSPEC_FAILURE;
		break;*/
	default:
		return -EINVAL;
	}
	return 0;
}


static int max8903_enable_charging(struct max8903_info *info, bool enable)
{
	int gpio_curr_adj= info->pdata->gpio_curr_adj;
	int gpio_ta_en = info->pdata->gpio_ta_en;
	unsigned long flags;

	dev_info(info->dev, "%s: %s charging,%s\n", __func__,
		 enable ? "enable" : "disable",
		 info->is_usb_cable ? "USB" : "TA");

	if (enable) {
		gpio_direction_output(gpio_ta_en, GPIO_LEVEL_LOW);
		if (info->is_usb_cable) {
			/* Charging by USB cable */
			gpio_direction_output(gpio_curr_adj, GPIO_LEVEL_LOW);
		} else {
			/* Charging by TA cable */
			gpio_direction_output(gpio_curr_adj, GPIO_LEVEL_HIGH);
		}
	} else {
		gpio_direction_output(gpio_curr_adj, GPIO_LEVEL_LOW);
		gpio_direction_output(gpio_ta_en, GPIO_LEVEL_HIGH);
	}

	msleep(300);
	return max8903_is_charging(info);
}


static int max8903_set_property(struct power_supply *psy,
				enum power_supply_property psp,
				const union power_supply_propval *val)
{
	struct max8903_info *info =
		container_of(psy, struct max8903_info, psy_bat);
	bool enable;

	switch (psp) {
	case POWER_SUPPLY_PROP_CURRENT_NOW:	/* Set charging current */
		info->is_usb_cable = (val->intval <= 450);
		break;
	case POWER_SUPPLY_PROP_STATUS:	/* Enable/Disable charging */
		enable = (val->intval == POWER_SUPPLY_STATUS_CHARGING);
		max8903_enable_charging(info, enable);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static irqreturn_t max8903_chg_ing_irq(int irq, void *data)
{
	struct max8903_info *info = data;
	int ret = 0;

	dev_info(info->dev, "chg_ing IRQ occurred!\n");

	if (gpio_get_value(info->pdata->gpio_ta_nconnected))
		return IRQ_HANDLED;

	if (info->pdata->topoff_cb)
		ret = info->pdata->topoff_cb();

	if (ret) {
		dev_err(info->dev, "%s: error from topoff_cb(%d)\n", __func__,
			ret);
		return IRQ_HANDLED;
	}

	return IRQ_HANDLED;
}


static __devinit int max8903_probe(struct platform_device *pdev)
{
	struct max8903_platform_data *pdata = dev_get_platdata(&pdev->dev);
	struct max8903_info *info;
	int ret;

	dev_info(&pdev->dev, "%s : MAX8903 Charger Driver Loading\n", __func__);

	info = kzalloc(sizeof(*info), GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	platform_set_drvdata(pdev, info);

	info->dev = &pdev->dev;
	info->pdata = pdata;

	info->psy_bat.name = "max8903-charger",
		info->psy_bat.type = POWER_SUPPLY_TYPE_BATTERY,
		info->psy_bat.properties = max8903_battery_props,
		info->psy_bat.num_properties = ARRAY_SIZE(max8903_battery_props),
		info->psy_bat.get_property = max8903_get_property,
		info->psy_bat.set_property = max8903_set_property,
		ret = power_supply_register(&pdev->dev, &info->psy_bat);
	if (ret) {
		dev_err(info->dev, "Failed to register psy_bat\n");
		goto err_kfree;
	}

	if (pdata->cfg_gpio) {
		ret = pdata->cfg_gpio();
		if (ret) {
			dev_err(info->dev, "failed to configure GPIO\n");
			goto err_kfree;
		}
	}

	if (gpio_is_valid(pdata->gpio_curr_adj)) {
		if (!pdata->gpio_curr_adj) {
			dev_err(info->dev, "gpio_curr_adj defined as 0\n");
			WARN_ON(!pdata->gpio_curr_adj);
			ret = -EIO;
			goto err_kfree;
		}
		gpio_request(pdata->gpio_curr_adj, "MAX8903 gpio_curr_adj");
	}

	if (gpio_is_valid(pdata->gpio_ta_en)) {
		if (!pdata->gpio_ta_en) {
			dev_err(info->dev, "gpio_ta_en defined as 0\n");
			WARN_ON(!pdata->gpio_ta_en);
			ret = -EIO;
			goto err_kfree;
		}
		gpio_request(pdata->gpio_ta_en, "MAX8903 gpio_ta_en");
	}

	if (gpio_is_valid(pdata->gpio_ta_nconnected)) {
		if (!pdata->gpio_ta_nconnected) {
			dev_err(info->dev, "gpio_ta_nconnected defined as 0\n");
			WARN_ON(!pdata->gpio_ta_nconnected);
			ret = -EIO;
			goto err_kfree;
		}
		gpio_request(pdata->gpio_ta_nconnected,
			"MAX8903 TA_nCONNECTED");
	}

	if (gpio_is_valid(pdata->gpio_ta_nchg)) {
		if (!pdata->gpio_ta_nchg) {
			dev_err(info->dev, "gpio_ta_nchg defined as 0\n");
			WARN_ON(!pdata->gpio_ta_nchg);
			ret = -EIO;
			goto err_kfree;
		}
		gpio_request(pdata->gpio_ta_nchg,
			"MAX8903 gpio_ta_nchg");
	}
#if 0
	info->irq_chg_ing = gpio_to_irq(pdata->gpio_chg_ing);

	ret = request_threaded_irq(info->irq_chg_ing, NULL,
				max8922_chg_ing_irq,
				IRQF_TRIGGER_RISING | IRQF_ONESHOT,
				"chg_ing", info);
	if (ret)
		dev_err(&pdev->dev, "%s: fail to request chg_ing IRQ:"
			" %d: %d\n", __func__, info->irq_chg_ing, ret);
#endif

	return 0;
err_kfree:
	power_supply_unregister(&info->psy_bat);
	platform_set_drvdata(pdev, NULL);
	kfree(info);
	return ret;
}

static int __devexit max8903_remove(struct platform_device *pdev)
{
	struct max8903_info *info = platform_get_drvdata(pdev);

	power_supply_unregister(&info->psy_bat);

	gpio_free(info->pdata->gpio_curr_adj);
	gpio_free(info->pdata->gpio_ta_en);
	gpio_free(info->pdata->gpio_ta_nconnected);
	gpio_free(info->pdata->gpio_ta_nchg);

	kfree(info);

	return 0;
}

#ifdef CONFIG_PM
static int max8903_suspend(struct device *dev)
{
	struct max8903_info *info = dev_get_drvdata(dev);

	if (info && info->irq_chg_ing)
		disable_irq(info->irq_chg_ing);

	return 0;
}

static int max8903_resume(struct device *dev)
{
	struct max8903_info *info = dev_get_drvdata(dev);

	if (info && info->irq_chg_ing)
		enable_irq(info->irq_chg_ing);

	return 0;
}
#else
#define max8903_charger_suspend	NULL
#define max8903_charger_resume	NULL
#endif

static const struct dev_pm_ops max8903_pm_ops = {
	.suspend = max8903_suspend,
	.resume = max8903_resume,
};

static struct platform_driver max8903_driver = {
	.driver = {
		.name = "max8903-charger",
		.owner = THIS_MODULE,
		.pm = &max8903_pm_ops,
		},
	.probe = max8903_probe,
	.remove = __devexit_p(max8903_remove),
};

static int __init max8903_init(void)
{
	return platform_driver_register(&max8903_driver);
}
module_init(max8903_init);

static void __exit max8903_exit(void)
{
	platform_driver_unregister(&max8903_driver);
}
module_exit(max8903_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MAX8903 Charger Driver");
MODULE_AUTHOR("MyungJoo Ham <myungjoo.ham@samsung.com>");
MODULE_ALIAS("max8903-charger");
