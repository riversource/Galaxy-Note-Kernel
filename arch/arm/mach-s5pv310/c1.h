/*
 * arch/arm/mach-s5pv210/c1.h
 */

#ifndef __C1_H__
#define __C1_H__

extern struct ld9040_panel_data c1_panel_data;
extern struct ld9040_panel_data c1_panel_data_a2;
extern struct ld9040_panel_data c1_panel_data_m2;
extern struct s6e63m0_panel_data epic2_panel_data;

#ifdef CONFIG_WIMAX_CMC
extern struct platform_device s3c_device_cmc732;
#endif

extern void c1_config_gpio_table(void);
extern void c1_config_sleep_gpio_table(void);
extern void s3c_setup_uart_cfg_gpio(struct platform_device *pdev);

extern void c1_fimc_set_platdata(void);

extern void tsp_register_callback(void *function);
extern void tsp_read_ta_status(bool *ta_status);

extern struct mxt224_platform_data mxt224_data;
extern struct mxt540e_platform_data mxt540e_data;

#endif
