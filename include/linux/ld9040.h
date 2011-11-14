/*inclue/linux/ld9040.h
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * Header file for Samsung Display Panel(AMOLED) driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#include <linux/types.h>

struct ld9040_panel_data {
	const unsigned short *seq_user_set;
	const unsigned short *seq_displayctl_set;
	const unsigned short *seq_gtcon_set;
	const unsigned short *seq_panelcondition_set;
	const unsigned short *seq_pwrctl_set;
	const unsigned short *display_on;
	const unsigned short *display_off;
	const unsigned short *sleep_in;
	const unsigned short *sleep_out;
	const unsigned short *acl_on;
	const unsigned short **acl_table;
	const unsigned short *elvss_on;
	const unsigned short **elvss_table;
	const unsigned short *default_gamma;
	const unsigned short **gamma19_table;
	const unsigned short **gamma22_table;
	const unsigned short lcdtype;
};

#define	LCDTYPE_M2			(1)
#define	LCDTYPE_SM2_A1		(0)
#define	LCDTYPE_SM2_A2		(2)
