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

#ifndef __SHTPS_CFG_FT8607_H__
#define __SHTPS_CFG_FT8607_H__

/* ===================================================================================
 * [ Debug ]
 */
#define SHTPS_DEVELOP_MODE_ENABLE

//#define SHTPS_RELEASE_FOR_MANUFACTURER

#ifdef	SHTPS_DEVELOP_MODE_ENABLE
	//#define SHTPS_PERFORMANCE_CHECK_ENABLE
	//#define SHTPS_LOG_SEQ_ENABLE
	//#define SHTPS_LOG_SPIACCESS_SEQ_ENABLE
	#define SHTPS_LOG_DEBUG_ENABLE
	#define	SHTPS_LOG_EVENT_ENABLE
	#define	SHTPS_MODULE_PARAM_ENABLE
	#define	SHTPS_DEBUG_VARIABLE_DEFINES
	#define SHTPS_CREATE_KOBJ_ENABLE
	#define SHTPS_DEVICE_ACCESS_LOG_ENABLE
#endif

#define	SHTPS_LOG_ERROR_ENABLE
#define SHTPS_DEF_RECORD_LOG_FILE_ENABLE

#ifdef SHTPS_LOG_EVENT_ENABLE
	#define SHTPS_LOG_OUTPUT_SWITCH_ENABLE
#endif /* #if defined( SHTPS_LOG_EVENT_ENABLE ) */

#if defined(SHTPS_PERFORMANCE_CHECK_ENABLE)
	#define SHTPS_PERFORMANCE_CHECK_PIN_ENABLE
	//#define SHTPS_PERFORMANCE_TIME_LOG_ENABLE
#endif /* SHTPS_PERFORMANCE_CHECK_ENABLE */

/* ===================================================================================
 * [ Diag ]
 */
#ifdef	SHTPS_FACTORY_MODE_ENABLE
	#undef	SHTPS_BOOT_FWUPDATE_ENABLE
	#undef	SHTPS_BOOT_FWUPDATE_FORCE_UPDATE
	#define	SHTPS_FMODE_GESTURE_ENABLE
	#define SHTPS_TPIN_CHECK_ENABLE
#else
	#define	SHTPS_BOOT_FWUPDATE_ENABLE
	#undef	SHTPS_BOOT_FWUPDATE_FORCE_UPDATE
	#undef	SHTPS_FMODE_GESTURE_ENABLE
	#undef	SHTPS_TPIN_CHECK_ENABLE
	
	#if defined(SHTPS_BOOT_FWUPDATE_ENABLE)
		#define	SHTPS_BOOT_FWUPDATE_SKIP_ES0_ENABLE
	#endif
	
#endif

#define SHTPS_CHECK_CRC_ERROR_ENABLE
#define	SHTPS_SMEM_BASELINE_ENABLE

#define SHTPS_DIAGPOLL_TIME							100


/* ===================================================================================
 * [ Firmware update ]
 */
//#define SHTPS_MULTI_FW_ENABLE
//#define SHTPS_CHECK_HWID_ENABLE
#define SHTPS_BOOT_FWUPDATE_ONLY_ON_HANDSET
#define SHTPS_SPI_FWBLOCKWRITE_ENABLE
#define	SHTPS_FWUPDATE_BUILTIN_ENABLE
//#define SHTPS_PANEL_POW_CTL_FOR_FWUPDATE_ENABLE

#define SHTPS_GET_HW_VERSION_RET_ES_0		0
#define SHTPS_GET_HW_VERSION_RET_ES_1		1
#define SHTPS_GET_HW_VERSION_RET_ES_1_5		2
#define SHTPS_GET_HW_VERSION_RET_PP_1		3
#define SHTPS_GET_HW_VERSION_RET_PP_1_5		4
#define SHTPS_GET_HW_VERSION_RET_PP_2		5
#define SHTPS_GET_HW_VERSION_RET_PP_2_5		6
#define SHTPS_GET_HW_VERSION_RET_MP			7
#define SHTPS_GET_HW_VERSION_RET_UNKNOWN	0xFF

#if defined(SHTPS_MULTI_FW_ENABLE)
	#include "ft8607/shtps_fw_ft8607_es1.h"
	#include "ft8607/shtps_fw_ft8607.h"
	
	typedef struct {
		unsigned char		hwrev;
		const unsigned char	*data;
		unsigned short		size;
		unsigned short		ver;
		char				*name;
	} shtps_multi_fw_info_t;
	
	static const shtps_multi_fw_info_t SHTPS_MULTI_FW_INFO_TBL[] = {
		{ SHTPS_GET_HW_VERSION_RET_ES_0,   tps_fw_data_es1, SHTPS_FWSIZE_NEWER_ES1, SHTPS_FWVER_NEWER_ES1, "ES1"    },
		{ SHTPS_GET_HW_VERSION_RET_ES_1,   tps_fw_data_es1, SHTPS_FWSIZE_NEWER_ES1, SHTPS_FWVER_NEWER_ES1, "ES1"    },
		{ SHTPS_GET_HW_VERSION_RET_ES_1_5, tps_fw_data_es1, SHTPS_FWSIZE_NEWER_ES1, SHTPS_FWVER_NEWER_ES1, "ES1"    },
		{ SHTPS_GET_HW_VERSION_RET_PP_1,   tps_fw_data,     SHTPS_FWSIZE_NEWER,     SHTPS_FWVER_NEWER    , "NORMAL" },
		{ SHTPS_GET_HW_VERSION_RET_PP_1_5, tps_fw_data,     SHTPS_FWSIZE_NEWER,     SHTPS_FWVER_NEWER    , "NORMAL" },
		{ SHTPS_GET_HW_VERSION_RET_PP_2,   tps_fw_data,     SHTPS_FWSIZE_NEWER,     SHTPS_FWVER_NEWER    , "NORMAL" },
		{ SHTPS_GET_HW_VERSION_RET_PP_2_5, tps_fw_data,     SHTPS_FWSIZE_NEWER,     SHTPS_FWVER_NEWER    , "NORMAL" },
		{ SHTPS_GET_HW_VERSION_RET_MP,     tps_fw_data,     SHTPS_FWSIZE_NEWER,     SHTPS_FWVER_NEWER    , "NORMAL" },
		{ SHTPS_GET_HW_VERSION_RET_UNKNOWN,tps_fw_data,     SHTPS_FWSIZE_NEWER,     SHTPS_FWVER_NEWER    , "NORMAL" },
	};
	static const int SHTPS_MULTI_FW_INFO_SIZE = sizeof(SHTPS_MULTI_FW_INFO_TBL) / sizeof(shtps_multi_fw_info_t);
#else
	#include "ft8607/shtps_fw_ft8607.h"
#endif /* SHTPS_MULTI_FW_ENABLE */


/* ===================================================================================
 * [ Model specifications ]
 */
#define CONFIG_SHTPS_FOCALTECH_LCD_SIZE_X		720
#define CONFIG_SHTPS_FOCALTECH_LCD_SIZE_Y		1280
//#define CONFIG_SHTPS_FOCALTECH_FACETOUCH_DETECT
//#define CONFIG_SHTPS_FOCALTECH_FACETOUCH_OFF_DETECT
//#define CONFIG_SHTPS_FOCALTECH_POSITION_OFFSET
#define CONFIG_SHTPS_FOCALTECH_ALWAYS_ACTIVEMODE

//#define SHTPS_IRQ_LEVEL_ENABLE
//#define SHTPS_SYSTEM_HOT_STANDBY_ENABLE
#define SHTPS_IRQ_LOADER_CHECK_INT_STATUS_ENABLE

//#define SHTPS_GLOVE_DETECT_ENABLE
#define SHTPS_LPWG_MODE_ENABLE
#define SHTPS_COVER_ENABLE

#if defined( SHTPS_LPWG_MODE_ENABLE )
	#define SHTPS_HOST_LPWG_MODE_ENABLE
#endif /* SHTPS_LPWG_MODE_ENABLE */

#if defined( CONFIG_SHTPS_FOCALTECH_ALWAYS_ACTIVEMODE )
	#define SHTPS_LOW_POWER_MODE_ENABLE
#endif /* #if defined( CONFIG_SHTPS_FOCALTECH_ALWAYS_ACTIVEMODE ) */

#define SHTPS_QOS_LATENCY_DEF_VALUE	 			34

#define SHTPS_NOTIFY_TOUCH_MINOR_ENABLE

#define SHTPS_VAL_FINGER_WIDTH_MAXSIZE (15)

//#define SHTPS_POWER_ON_CONTROL_ENABLE

/* ===================================================================================
 * [ Firmware control ]
 */
#define SHTPS_FWDATA_BLOCK_SIZE_MAX					0x10000

#define SHTPS_GET_PANEL_SIZE_X(ts)					(shtps_fwctl_get_maxXPosition(ts) + 1)
#define SHTPS_GET_PANEL_SIZE_Y(ts)					(shtps_fwctl_get_maxYPosition(ts) + 1)

#define SHTPS_POS_SCALE_X(ts)	(((CONFIG_SHTPS_FOCALTECH_LCD_SIZE_X) * 10000) / shtps_fwctl_get_maxXPosition(ts))
#define SHTPS_POS_SCALE_Y(ts)	(((CONFIG_SHTPS_FOCALTECH_LCD_SIZE_Y) * 10000) / shtps_fwctl_get_maxYPosition(ts))

/* ===================================================================================
 * [ Hardware specifications ]
 */
#define	SHTPS_I2C_BLOCKREAD_BUFSIZE			(SHTPS_TM_TXNUM_MAX * 4)
#define SHTPS_I2C_BLOCKWRITE_BUFSIZE		0x10

#define SHTPS_I2C_RETRY_COUNT					5
#define SHTPS_I2C_RETRY_WAIT					5

#define SHTPS_STARTUP_MIN_TIME					300
#define SHTPS_HWRESET_TIME_MS					5
#define SHTPS_HWRESET_AFTER_TIME_MS				300
#define SHTPS_HWRESET_WAIT_MS					290
#define SHTPS_SWRESET_WAIT_MS					300

#define SHTPS_POWER_VDDH_WAIT_US				5
#define SHTPS_POWER_VBUS_WAIT_US				5

#define SHTPS_READ_SERIAL_NUMBER_SIZE			16

//#define SHTPS_PROXIMITY_SUPPORT_ENABLE

/* ===================================================================================
 * [ Performance ]
 */
//#define SHTPS_CPU_CLOCK_CONTROL_ENABLE

#define SHTPS_CPU_IDLE_SLEEP_CONTROL_ENABLE
#define SHTPS_CPU_SLEEP_CONTROL_FOR_FWUPDATE_ENABLE


/* ===================================================================================
 * [ Standard ]
 */
#define SHTPS_IRQ_LINKED_WITH_IRQWAKE_ENABLE

#define SHTPS_INPUT_POWER_MODE_CHANGE_ENABLE
#define SHTPS_GUARANTEE_I2C_ACCESS_IN_WAKE_ENABLE
#define SHTPS_TOUCHCANCEL_BEFORE_FORCE_TOUCHUP_ENABLE


/* ===================================================================================
 * [ Host functions ]
 */
#if defined(SHTPS_RELEASE_FOR_MANUFACTURER)

#else

#define SHTPS_ACTIVE_SLEEP_WAIT_ALWAYS_ENABLE
//#define SHTPS_REPORT_TOOL_TYPE_LOCK_ENABLE
//#define SHTPS_CTRL_FW_REPORT_RATE
//#define SHTPS_DEF_DISALLOW_Z_ZERO_TOUCH_EVENT_ENABLE

#ifdef CONFIG_SHUB_ML630Q790
	#define SHTPS_MOTION_PEDO_STOP_REQ_ENABLE
#endif

#ifdef CONFIG_SHTERM
	#define SHTPS_SEND_SHTERM_EVENT_ENABLE
#endif

#if defined(SHTPS_BOOT_FWUPDATE_FORCE_UPDATE)
	#undef SHTPS_BOOT_FWUPDATE_ONLY_ON_HANDSET
#endif /* SHTPS_BOOT_FWUPDATE_FORCE_UPDATE */

#if defined(SHTPS_LPWG_MODE_ENABLE)
//	#define SHTPS_LPWG_DOUBLE_TAP_ENABLE
	#define SHTPS_LPWG_CHANGE_SWIPE_DISTANCE_ENABLE
	#define SHTPS_LPWG_GRIP_SUPPORT_ENABLE
		#if defined(SHTPS_LPWG_GRIP_SUPPORT_ENABLE)
			#define SHTPS_DEF_LPWG_GRIP_PROC_ASYNC_ENABLE
		#endif /* SHTPS_LPWG_GRIP_SUPPORT_ENABLE */
	#define SHTPS_LPWG_ALLOWED_SWIPES_ENABLE
#endif /* SHTPS_LPWG_MODE_ENABLE */

#if defined(SHTPS_CTRL_FW_REPORT_RATE) && defined(SHTPS_LOW_POWER_MODE_ENABLE)
//	#define SHTPS_DEF_CTRL_FW_REPORT_RATE_LINKED_LCD_BRIGHT_ENABLE
#endif /* SHTPS_CTRL_FW_REPORT_RATE && SHTPS_LOW_POWER_MODE_ENABLE */

#if defined(SHTPS_CPU_CLOCK_CONTROL_ENABLE) && defined(SHTPS_LOW_POWER_MODE_ENABLE)
	#define SHTPS_DEF_CTRL_CPU_CLOCK_LINKED_LCD_BRIGHT_ENABLE
	#define SHTPS_DEF_CTRL_CPU_CLOCK_LINKED_ECO_ENABLE
#endif /* SHTPS_CPU_CLOCK_CONTROL_ENABLE && SHTPS_LOW_POWER_MODE_ENABLE */

#endif /*SHTPS_RELEASE_FOR_MANUFACTURER*/

/* -----------------------------------------------------------------------------------
 */
#endif /* __SHTPS_CFG_FT8607_H__ */
