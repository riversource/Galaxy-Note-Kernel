/**
 * wimax_i2c.h
 *
 * EEPROM access functions
 */
#ifndef __WIMAX_I2C_H__
#define __WIMAX_I2C_H__

/* Write WiMAX boot image to EEPROM */
int eeprom_write_boot(void);

/* Write HW rev to EEPROM */
int eeprom_write_rev(void);

/* Erase WiMAX certification */
void eeprom_erase_cert(void);

/* Check WiMAX cerification exist or not */
int eeprom_check_cert(void);

/* Check WiMAX calibration done or not */
int eeprom_check_cal(void);

/* Read WiMAX boot image */
void eeprom_read_boot(void);

/* Read entire EEPROM data */
void eeprom_read_all(void);

/* Erase entire EEPROM data */
void eeprom_erase_all(void);

extern struct image_data g_wimax_image;

void wimax_i2c_reset(void);

#ifndef DRIVER_BIT_BANG

/* Initialize i2c-gpio driver for eeprom*/
int wmxeeprom_init(void);

/* Remove the eeprom i2c-gpio driver */
void wmxeeprom_exit(void);

#endif

#endif	/* __WIMAX_I2C_H__ */

