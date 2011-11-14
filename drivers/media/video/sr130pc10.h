/*
 * Driver Register File for SR130PC10(1.3MP Camera) from SILICONFILE
 *
 * Copyright (C) 2011,
 * DongSeong Lim<dongseong.lim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __SR130PC10_H
#define __SR130PC10_H

#include <linux/types.h>

#define SR130PC10_DRIVER_NAME	"SR130PC10"

typedef enum {
	STREAM_STOP,
	STREAM_START,
} stream_cmd_t;

struct sr130pc10_framesize {
	u32 width;
	u32 height;
};

struct sr130pc10_exif {
	u32 shutter_speed;
	u16 iso;
};


/*
 * Driver information
 */
struct sr130pc10_state {
	struct v4l2_subdev sd;
	/*
	 * req_fmt is the requested format from the application.
	 * set_fmt is the output format of the camera. Finally FIMC
	 * converts the camera output(set_fmt) to the requested format
	 * with hardware scaler.
	 */
	struct v4l2_pix_format req_fmt;
	struct sr130pc10_framesize preview_frmsizes;
	struct sr130pc10_framesize capture_frmsizes;
	struct sr130pc10_exif exif;

	enum v4l2_sensor_mode sensor_mode;
	s32 vt_mode;
	s32 check_dataline;
	u32 req_fps;
	u32 set_fps;
	u32 initialized;
};

static inline struct sr130pc10_state *to_state(struct v4l2_subdev *sd) {
	return container_of(sd, struct sr130pc10_state, sd);
}

/* #define CONFIG_CAM_DEBUG */
#define cam_warn(fmt, ...)	\
	do { \
		printk(KERN_WARNING "%s: " fmt, __func__, ##__VA_ARGS__); \
	} while (0)

#define cam_err(fmt, ...)	\
	do { \
		printk(KERN_ERR "%s: " fmt, __func__, ##__VA_ARGS__); \
	} while (0)

#define cam_info(fmt, ...)	\
	do { \
		printk(KERN_INFO "%s: " fmt, __func__, ##__VA_ARGS__); \
	} while (0)

#ifdef CONFIG_CAM_DEBUG
#define cam_dbg(fmt, ...)	\
	do { \
		printk(KERN_DEBUG "%s: " fmt, __func__, ##__VA_ARGS__); \
	} while (0)
#else
#define cam_dbg(fmt, ...)
#endif /* CONFIG_CAM_DEBUG */


/************ driver feature ************/
#define SR130PC10_USLEEP
/* #define CONFIG_LOAD_FILE */

#include  "sr130pc10_setfile.h"

#endif /* __SR130PC10_H */
