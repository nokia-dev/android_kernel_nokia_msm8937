/*
 * FocalTech ft8607 TouchScreen driver.
 *
 * Copyright (c) 2016  Focal tech Ltd.
 * Copyright (c) 2016, Sharp. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __SHTPS_FWCTL_FOCALTECH_H__
#define __SHTPS_FWCTL_FOCALTECH_H__
/* -------------------------------------------------------------------------- */
#include "shtps_param_extern.h"

/* -------------------------------------------------------------------------- */
#define FTS_MAX_POINTS			10

#define FTS_META_REGS			3
#define FTS_ONE_TCH_LEN			6
#define FTS_TCH_LEN(x)			(FTS_META_REGS + (FTS_ONE_TCH_LEN * (x)))

#define FTS_PRESS				0x7F
#define FTS_MAX_ID				0x0F
#define FTS_TOUCH_X_H_POS		0
#define FTS_TOUCH_X_L_POS		1
#define FTS_TOUCH_Y_H_POS		2
#define FTS_TOUCH_Y_L_POS		3
#define FTS_TOUCH_WEIGHT		4
#define FTS_TOUCH_AREA			5
#define FTS_TOUCH_POINT_NUM		2
#define FTS_TOUCH_EVENT_POS		0
#define FTS_TOUCH_ID_POS		2

#define FTS_TOUCH_DOWN			0
#define FTS_TOUCH_UP			1
#define FTS_TOUCH_CONTACT		2

#define FTS_POINT_READ_BUFSIZE	(FTS_META_REGS + (FTS_ONE_TCH_LEN * FTS_MAX_POINTS))

#define FTS_GESTURE_LEFT		0x20
#define FTS_GESTURE_RIGHT		0x21
#define FTS_GESTURE_UP			0x22
#define FTS_GESTURE_DOWN		0x23
#define FTS_GESTURE_DOUBLECLICK	0x24
#define FTS_GESTURE_O			0x30
#define FTS_GESTURE_W			0x31
#define FTS_GESTURE_M			0x32
#define FTS_GESTURE_E			0x33
#define FTS_GESTURE_L			0x44
#define FTS_GESTURE_S			0x46
#define FTS_GESTURE_V			0x54
#define FTS_GESTURE_Z			0x41

#define FTS_DEVIDE_MODE_ADDR				0x00
#define FTS_REG_LINE_NUM					0x01
#define FTS_REG_SCAN_MODE_ADDR				0x06
#define FTS_REG_RawBuf0						0x6A
#define FTS_REG_CbBuf0						0x6E
#define FTS_REG_CbAddrH						0x18
#define FTS_REG_CbAddrL						0x19
#define FTS_REG_FW_VER						0xA6
#define FTS_REG_MODEL_VER					0xA8
#define FTS_REG_PMODE						0xA5
#define FTS_REG_STATUS						0x01

/* power register bits*/
#define FTS_PMODE_ACTIVE					0x00
#define FTS_PMODE_MONITOR					0x01
#define FTS_PMODE_STANDBY					0x02
#define FTS_PMODE_HIBERNATE					0x03

#define FTS_REGVAL_LINE_NUM_CHANNEL_AREA	0xAD
#define FTS_REGVAL_LINE_NUM_KEY_AREA		0xAE

#define FTS_REGVAL_SCAN_MODE_RAW			0x00
#define FTS_REGVAL_SCAN_MODE_DIFFER			0x01

/*-----------------------------------------------------------
Error Code for Comm
-----------------------------------------------------------*/
#define ERROR_CODE_OK						0
#define ERROR_CODE_CHECKSUM_ERROR			-1
#define ERROR_CODE_INVALID_COMMAND			-2
#define ERROR_CODE_INVALID_PARAM			-3
#define ERROR_CODE_IIC_WRITE_ERROR			-4
#define ERROR_CODE_IIC_READ_ERROR			-5
#define ERROR_CODE_WRITE_USB_ERROR			-6
#define ERROR_CODE_WAIT_RESPONSE_TIMEOUT	-7
#define ERROR_CODE_PACKET_RE_ERROR			-8
#define ERROR_CODE_NO_DEVICE				-9
#define ERROR_CODE_WAIT_WRITE_TIMEOUT		-10
#define ERROR_CODE_READ_USB_ERROR			-11
#define ERROR_CODE_COMM_ERROR				-12
#define ERROR_CODE_ALLOCATE_BUFFER_ERROR	-13
#define ERROR_CODE_DEVICE_OPENED			-14
#define ERROR_CODE_DEVICE_CLOSED			-15

/*-----------------------------------------------------------
FW Upgrade
-----------------------------------------------------------*/
#define FTS_PACKET_LENGTH					120
#define FTS_UPGRADE_AA						0xAA
#define FTS_UPGRADE_55						0x55
#define FTS_RST_DELAY_AA_MS					2

#define FTS_UPGRADE_LOOP					30

#define FTS_LEN_FLASH_ECC_MAX				0xFFFE

#endif/* __SHTPS_FWCTL_FOCALTECH_H__ */

