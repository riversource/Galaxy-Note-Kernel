/*
 *  wacom_i2c_coord_table.h - Wacom G5 Digitizer Controller (I2C bus)
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

// Portrait Right
short CodTblX_PRight_48[] = {
#include "wacom_i2c_coordX_PRight.h"
};
short CodTblY_PRight_48[] = {
#include "wacom_i2c_coordY_PRight.h"
};

// Portrait Left
short CodTblX_PLeft_48[] = {
#include "wacom_i2c_coordX_PLeft.h"
};
short CodTblY_PLeft_48[] = {
#include "wacom_i2c_coordY_PLeft.h"
};



/* Landscape 1 */

// Landscape 90 Right is same with Portrait Left
#define CodTblX_LRight_48 CodTblX_PLeft_48
#define CodTblY_LRight_48 CodTblY_PLeft_48


// Landscape 90 Left
short CodTblX_LLeft_48[] = {
#include "wacom_i2c_coordX_LLeft.h"
};
short CodTblY_LLeft_48[] = {
#include "wacom_i2c_coordY_LLeft.h"
};



/* Landscape 2 */

// Landscape 270 Right
#define CodTblX_LRight2_48 CodTblX_PRight_48
#define CodTblY_LRight2_48 CodTblY_PRight_48


// Landscape 270 Left
#define CodTblX_LLeft2_48 CodTblX_PRight_48
#define CodTblY_LLeft2_48 CodTblY_PRight_48






const short CodTblX_CCW_LLeft_44[] = {
#include "wacom_i2c_coordX_CCW_LLeft_44.h"
};
const short CodTblY_CCW_LLeft_44[] = {
#include "wacom_i2c_coordY_CCW_LLeft_44.h"
};

const short CodTblX_CW_LRight_44[] = {
#include "wacom_i2c_coordX_CW_LRight_44.h"
};
const short CodTblY_CW_LRight_44[] = {
#include "wacom_i2c_coordY_CW_LRight_44.h"
};

const short CodTblX_PLeft_44[] = {
#include "wacom_i2c_coordX_PLeft_44.h"
};
const short CodTblY_PLeft_44[] = {
#include "wacom_i2c_coordY_PLeft_44.h"
};

const short CodTblX_PRight_44[] = {
#include "wacom_i2c_coordX_PRight_44.h"
};
const short CodTblY_PRight_44[] = {
#include "wacom_i2c_coordY_PRight_44.h"
};
/* Distance Offset Table */
short *tableX[MAX_HAND][MAX_ROTATION]= {{CodTblX_PLeft_44, CodTblX_CCW_LLeft_44, CodTblX_PRight_44, CodTblX_CW_LRight_44},
										{CodTblX_PRight_44, CodTblX_PLeft_44, CodTblX_CW_LRight_44, CodTblX_CCW_LLeft_44}};

short *tableY[MAX_HAND][MAX_ROTATION]= {{CodTblY_PLeft_44, CodTblY_CCW_LLeft_44, CodTblY_PRight_44, CodTblY_CW_LRight_44},
										{CodTblY_PRight_44, CodTblY_PLeft_44, CodTblY_CW_LRight_44, CodTblY_CCW_LLeft_44}};

short *tableX_48[MAX_HAND][MAX_ROTATION]= {{CodTblX_PLeft_48, CodTblX_LLeft_48, CodTblX_LLeft2_48, 0},
										{CodTblX_PRight_48, CodTblX_LRight_48, CodTblX_LRight2_48, 0}};
short *tableY_48[MAX_HAND][MAX_ROTATION]={{CodTblY_PLeft_48, CodTblY_LLeft_48, CodTblY_LLeft2_48, 0},
										{CodTblY_PRight_48, CodTblY_LRight_48, CodTblY_LRight2_48, 0}};

/* Tilt offset */
/* 0: Left, 1: Right */
/* 0: Portrait 0, 1: Landscape 90, 2: Landscape 270 */
short tilt_offsetX[MAX_HAND][MAX_ROTATION]= {{120, 110, -110, -100},{-120, 120, -130, 100}};
short tilt_offsetY[MAX_HAND][MAX_ROTATION]={{-110, 110, -150, 100},{-130, -110, 70, 100}};

short tilt_offsetX_48[MAX_HAND][MAX_ROTATION]= {{60, 80, 0, 0},{-60, 80, 0, 0}};
short tilt_offsetY_48[MAX_HAND][MAX_ROTATION]={{-130, 130, 0, 0},{-130, -130, 0, 0}};


/* Origin Shift */
short origin_offset[]={600, 620};
short origin_offset_48[]={720, 760};
