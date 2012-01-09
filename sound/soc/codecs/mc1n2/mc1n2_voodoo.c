/*
 * Author: François SIMOND aka supercurio
 * License: GPLv2
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/slab.h>

#include "mc1n2_voodoo.h"

static unsigned long vol_hpgain_addr;
module_param(vol_hpgain_addr, ulong, 0644);
static unsigned long vol_hpsp_addr;
module_param(vol_hpsp_addr, ulong, 0644);

static struct mc1n2_voodoo *mc1n2;

static ssize_t version_show(struct class *class, struct class_attribute *attr,
			    char *buf)
{
	return sprintf(buf, "%s\n", VERSION);
}

static CLASS_ATTR(version, 0444, version_show, NULL);

static ssize_t vol_show(volmap map, int length, char *buf)
{

	int i;
	ssize_t size = 0;

	for (i = 0; i < length; i++)
		size += sprintf(buf, "%s0x%x\n", buf, map[i]);

	return size;
}

static void vol_store(volmap map, int length, const char *buf)
{

	int i = 0;
	unsigned short val;
	int bytes_read = 0;
	unsigned short val_read[length];

	printk(NAME ": receive configuration from sysfs\n");

	while (sscanf(buf, "%hx%n", &val, &bytes_read) == 1) {
		buf += bytes_read;

		pr_debug(NAME ": read %d: 0x%x\n", i, val);
		val_read[i] = val;

		i++;
		if (i > length) {
			printk(NAME ": too many values (%d expected)\n",
			       length);
			return;
		}
	}

	if (i == length) {
		printk(NAME ": record %d values\n", i);
		for (i = 0; i < length; i++)
			map[i] = val_read[i];
	} else {
		printk(NAME ": not enough values: %d (%d expected)\n", i,
		       length);
		return;
	}
}

static ssize_t vol_hpgain_show(struct class *class,
			       struct class_attribute *attr, char *buf)
{
	volmap map;

	if (vol_hpgain_addr) {
		map = (volmap) vol_hpgain_addr;
		return vol_show(map, HPGAIN_SIZE, buf);
	} else
		return -ENODATA;
}

static ssize_t vol_hpgain_store(struct class *class,
				struct class_attribute *attr,
				const char *buf, size_t count)
{
	volmap map;

	if (vol_hpgain_addr) {
		map = (volmap) vol_hpgain_addr;
		vol_store(map, HPGAIN_SIZE, buf);
	}

	return count;
}

static CLASS_ATTR(vol_hpgain, 0666, vol_hpgain_show, vol_hpgain_store);

static ssize_t vol_hpsp_show(struct class *class,
			     struct class_attribute *attr, char *buf)
{
	volmap map;

	if (vol_hpsp_addr) {
		map = (volmap) vol_hpsp_addr;
		return vol_show(map, HPSP_SIZE, buf);
	} else
		return -ENODATA;
}

static ssize_t vol_hpsp_store(struct class *class,
			      struct class_attribute *attr,
			      const char *buf, size_t count)
{
	volmap map;

	if (vol_hpsp_addr) {
		map = (volmap) vol_hpsp_addr;
		vol_store(map, HPSP_SIZE, buf);
	}
	count = (size_t) buf;

	return count;
}

static CLASS_ATTR(vol_hpsp, 0666, vol_hpsp_show, vol_hpsp_store);

static int __init mc1n2_voodoo_init(void)
{
	int ret;

	printk(NAME ": init\n");

	mc1n2 = kzalloc(sizeof(*mc1n2), GFP_KERNEL);

	mc1n2->mc1n2_voodoo_class = class_create(THIS_MODULE, "mc1n2_voodoo");
	if (!mc1n2->mc1n2_voodoo_class)
		return -ENOMEM;

	ret = class_create_file(mc1n2->mc1n2_voodoo_class, &class_attr_version);
	ret = class_create_file(mc1n2->mc1n2_voodoo_class,
				&class_attr_vol_hpgain);
	ret = class_create_file(mc1n2->mc1n2_voodoo_class,
				&class_attr_vol_hpsp);

	return 0;
}

void __exit mc1n2_voodoo_exit(void)
{
	printk(NAME ": exit\n");

	class_remove_file(mc1n2->mc1n2_voodoo_class, &class_attr_version);
	class_remove_file(mc1n2->mc1n2_voodoo_class, &class_attr_vol_hpgain);
	class_remove_file(mc1n2->mc1n2_voodoo_class, &class_attr_vol_hpsp);
	class_destroy(mc1n2->mc1n2_voodoo_class);

	kfree(mc1n2);
}

MODULE_DESCRIPTION(NAME);
MODULE_AUTHOR("supercurio");
MODULE_LICENSE("GPL");
MODULE_VERSION(VERSION);

module_init(mc1n2_voodoo_init);
module_exit(mc1n2_voodoo_exit);
