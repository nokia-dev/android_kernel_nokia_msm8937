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

#ifndef __SHTPS_FTS_H__
#define __SHTPS_FTS_H__
/* --------------------------------------------------------------------------- */
#include <linux/module.h>
#include <linux/wakelock.h>
#include <linux/slab.h>
#include <linux/pm_qos.h>
#include <linux/version.h>
#include <asm/uaccess.h>
#include <linux/interrupt.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#ifdef CONFIG_FB
#include <linux/notifier.h>
#include <linux/fb.h>
#endif

//#include <mach/cpuidle.h>

//#include <sharp/proximity.h>
#include <linux/input/shtps_dev.h>
//#include <misc/shub_driver.h>

#include "shtps_cfg.h"
#include "shtps_i2c.h"
#include "shtps_fwctl.h"
#include "shtps_filter.h"
#include "shtps_fts_debug.h"
#include "shtps_log.h"
#include "focaltech_core.h"

/* ===================================================================================
 * Debug
 */

/* ===================================================================================
 * Common
 */
#define SPI_ERR_CHECK(check, label) \
	if((check)) goto label

#define SHTPS_POSTYPE_X (0)
#define SHTPS_POSTYPE_Y (1)

#define SHTPS_TOUCH_CANCEL_COORDINATES_X (0)
#define SHTPS_TOUCH_CANCEL_COORDINATES_Y (9999)

#define SHTPS_ABS_CALC(A,B)	(((A)>(B)) ? ((A)-(B)) : ((B)-(A)))
#define SHTPS_ABS(A)		SHTPS_ABS_CALC(A,0)

/* ===================================================================================
 * [ STFLIB kernel event request ]
 */
#define SHTPS_DEF_STFLIB_KEVT_SIZE_MAX		0xFFFF
#define SHTPS_DEF_STFLIB_KEVT_DUMMY			0x0000
#define SHTPS_DEF_STFLIB_KEVT_SCANRATE_MODE	0x0001
#define SHTPS_DEF_STFLIB_KEVT_CHARGER_ARMOR	0x0002
#define SHTPS_DEF_STFLIB_KEVT_CLEAR_REQUEST	0x0003
#define SHTPS_DEF_STFLIB_KEVT_TIMEOUT		0x0004
#define SHTPS_DEF_STFLIB_KEVT_CHANGE_PARAM	0x0005
#define SHTPS_DEF_STFLIB_KEVT_CALIBRATION	0x0006

#define SHTPS_DEF_STFLIB_KEVT_SCANRATE_MODE_NORMAL	0		/* Normal */
#define SHTPS_DEF_STFLIB_KEVT_SCANRATE_MODE_LOW		1		/* Low */
#define SHTPS_DEF_STFLIB_KEVT_SCANRATE_MODE_HIGH	2		/* High */

#define SHTPS_DEF_STFLIB_KEVT_CLEAR_REQUEST_SLEEP	0		/* SLEEP */
#define SHTPS_DEF_STFLIB_KEVT_CLEAR_REQUEST_REZERO	1		/* REZERO */


/* ===================================================================================
 * Structure / enum
 */
struct shtps_irq_info{
	int							irq;
	u8							state;
	u8							wake;
};

struct shtps_state_info{
	int							state;
	int							mode;
	int							starterr;
	unsigned long				starttime;
	int							startup_min_time;
};

struct shtps_loader_info{
	int							ack;
	wait_queue_head_t			wait_ack;
};

struct shtps_diag_info{
	u8							pos_mode;
	u8							tm_mode;
	u8							tm_data[SHTPS_TM_TXNUM_MAX * SHTPS_TM_RXNUM_MAX * 2];
	int							event;
	int							tm_ack;
	int							tm_stop;
	wait_queue_head_t			wait;
	wait_queue_head_t			tm_wait_ack;
};

struct shtps_facetouch_info{
	int							mode;
	int							state;
	int							detect;
	int							palm_thresh;
	int							wake_sig;
	u8							palm_det;
	u8							touch_num;
	wait_queue_head_t			wait_off;
	
	u8							wakelock_state;
	struct wake_lock            wake_lock;
	struct pm_qos_request		qos_cpu_latency;
};

struct shtps_offset_info{
	int							enabled;
	u16							base[5];
	signed short				diff[12];
};

struct shtps_polling_info{
	int							stop_margin;
	int							stop_count;
	int							single_fingers_count;
	int							single_fingers_max;
	u8							single_fingers_enable;
};

struct shtps_lpwg_ctrl{		//**********	SHTPS_LPWG_MODE_ENABLE
	u8							lpwg_switch;
	u8							lpwg_state;
	u8							lpwg_sweep_on_req_state;
	u8							lpwg_double_tap_req_state;
	u8							lpwg_set_state;
	u8							is_notified;
	u8							notify_enable;

	struct wake_lock			wake_lock;
	struct pm_qos_request		pm_qos_lock_idle;
	signed long				pm_qos_idle_value;
	unsigned long				notify_time;
	struct delayed_work			notify_interval_delayed_work;
	u8							block_touchevent;
	unsigned long				wakeup_time;
	
	#if defined( SHTPS_LPWG_GRIP_SUPPORT_ENABLE )
		u8							grip_state;
	#endif /* SHTPS_LPWG_GRIP_SUPPORT_ENABLE */

	#if defined( SHTPS_HOST_LPWG_MODE_ENABLE )
		struct shtps_touch_info		pre_info;
		unsigned long				swipe_check_time;
	#endif /* SHTPS_HOST_LPWG_MODE_ENABLE */

};


struct shtps_touch_pos_info{
	unsigned short	x;
	unsigned short	y;
};

struct shtps_touch_hist_info{
	unsigned short	x;
	unsigned short	y;
	unsigned char	state;
	unsigned char	wx;
	unsigned char	wy;
	unsigned char	z;
};

enum{	//**********	SHTPS_GUARANTEE_I2C_ACCESS_IN_WAKE_ENABLE
	SHTPS_DETER_SUSPEND_I2C_PROC_IRQ = 0x00,
	SHTPS_DETER_SUSPEND_I2C_PROC_CHARGER_ARMOR,
	SHTPS_DETER_SUSPEND_I2C_PROC_SETSLEEP,
	SHTPS_DETER_SUSPEND_I2C_PROC_IOCTL_SETLPWG,
	SHTPS_DETER_SUSPEND_I2C_PROC_IOCTL_SETLPMODE,
	SHTPS_DETER_SUSPEND_I2C_PROC_IOCTL_SETCONLPMODE,
	SHTPS_DETER_SUSPEND_I2C_PROC_IOCTL_SETLCDBRIGHTLPMODE,
	SHTPS_DETER_SUSPEND_I2C_PROC_OPEN,
	SHTPS_DETER_SUSPEND_I2C_PROC_CLOSE,
	SHTPS_DETER_SUSPEND_I2C_PROC_ENABLE,
	SHTPS_DETER_SUSPEND_I2C_PROC_GRIP,
	SHTPS_DETER_SUSPEND_I2C_PROC_IOCTL_SETHOVER,
	SHTPS_DETER_SUSPEND_I2C_PROC_IOCTL_SETPEN,
	SHTPS_DETER_SUSPEND_I2C_PROC_COVER,
	SHTPS_DETER_SUSPEND_I2C_PROC_IOCTL_SETLPWG_DOUBLETAP,
	SHTPS_DETER_SUSPEND_I2C_PROC_IOCTL_SETGLOVE,
	SHTPS_DETER_SUSPEND_I2C_PROC_IOCTL_SET_HIGH_REPORT_MODE,

	SHTPS_DETER_SUSPEND_I2C_PROC_NUM,
};

struct shtps_deter_suspend_i2c{	//**********	SHTPS_GUARANTEE_I2C_ACCESS_IN_WAKE_ENABLE
	u8							suspend;
	struct work_struct			pending_proc_work;
	u8							wake_lock_state;
	struct wake_lock			wake_lock;
	struct pm_qos_request		pm_qos_lock_idle;
	
	#ifdef SHTPS_DEVELOP_MODE_ENABLE
		struct delayed_work			pending_proc_work_delay;
	#endif /* SHTPS_DEVELOP_MODE_ENABLE */
	
	struct shtps_deter_suspend_i2c_pending_info{
		u8						pending;
		u8						param;
	} pending_info[SHTPS_DETER_SUSPEND_I2C_PROC_NUM];

	u8							suspend_irq_state;
	u8							suspend_irq_wake_state;
	u8							suspend_irq_detect;
};

/* -------------------------------------------------------------------------- */
struct shtps_fts {
	struct input_dev*			input;
	int							rst_pin;
	int							tpin;
	#ifdef CONFIG_OF
		struct pinctrl				*pinctrl;
		struct pinctrl_state		*tpin_state_active;
		struct pinctrl_state		*tpin_state_suspend;
	#endif /* CONFIG_OF */
	struct shtps_irq_info		irq_mgr;
#ifdef CONFIG_FB
	struct notifier_block		fb_notif;
#endif

	struct shtps_touch_info		fw_report_info;
	struct shtps_touch_info		fw_report_info_store;
	struct shtps_touch_info		report_info;

	struct shtps_state_info		state_mgr;
	struct shtps_loader_info	loader;
	struct shtps_diag_info		diag;
	struct shtps_facetouch_info	facetouch;
	struct shtps_polling_info	poll_info;
	struct shtps_touch_state	touch_state;
	wait_queue_head_t			wait_start;
	struct delayed_work 		tmo_check;
	unsigned char				finger_state[3];	/* SHTPS_FINGER_MAX/4+1 */
	u16							bt_ver;
	char						phys[32];
	struct kobject				*kobj;
	u8							is_lcd_on;
	#ifdef SHTPS_DEVELOP_MODE_ENABLE
		char					stflib_param_str[100];
	#endif /* SHTPS_DEVELOP_MODE_ENABLE */

	#if defined( SHTPS_BOOT_FWUPDATE_ENABLE )
		int							boot_fw_update_checked;
	#endif /* SHTPS_BOOT_FWUPDATE_ENABLE */

	struct shtps_offset_info	offset;

	#if defined( SHTPS_LOW_POWER_MODE_ENABLE )
		u8							lpmode_req_state;
		u8							lpmode_continuous_req_state;
	#endif /* #if defined( SHTPS_LOW_POWER_MODE_ENABLE ) */

	#if defined(SHTPS_LPWG_MODE_ENABLE)
		struct input_dev*			input_key;
		u16							keycodes[2];
		u8							key_state;
	#endif /* #if defined(SHTPS_LPWG_MODE_ENABLE) */

	#if defined( SHTPS_PIXEL_TOUCH_THRESHOLD_ENABLE )
	#endif /* SHTPS_PIXEL_TOUCH_THRESHOLD_ENABLE */

	#if defined(SHTPS_LPWG_MODE_ENABLE)
		struct shtps_lpwg_ctrl		lpwg;
		u8							lpwg_hover_enable_state_sotre;

		#if defined(SHTPS_PROXIMITY_SUPPORT_ENABLE)
			int							lpwg_proximity_get_data;
			struct wake_lock            wake_lock_proximity;
			struct pm_qos_request		pm_qos_lock_idle_proximity;
			struct delayed_work			proximity_check_delayed_work;
		#endif /* SHTPS_PROXIMITY_SUPPORT_ENABLE */
	#endif /* SHTPS_LPWG_MODE_ENABLE */

	#if defined(SHTPS_MULTI_FW_ENABLE)
		u8							multi_fw_type;
	#endif /* SHTPS_MULTI_FW_ENABLE */

	#if defined(SHTPS_GUARANTEE_I2C_ACCESS_IN_WAKE_ENABLE)
		struct shtps_deter_suspend_i2c		deter_suspend_i2c;
	#endif /* SHTPS_GUARANTEE_I2C_ACCESS_IN_WAKE_ENABLE */

	#if defined(SHTPS_FINGER_KEY_EXCLUDE_ENABLE)
		u8							exclude_touch_disable_check_state;
		u8							exclude_key_disable_check_state;
		u16							exclude_touch_disable_finger;
		unsigned long				exclude_touch_disable_check_time;
		unsigned long				exclude_key_disable_check_time;
	#endif /* SHTPS_FINGER_KEY_EXCLUDE_ENABLE */

	#if defined(SHTPS_LOW_REPORTRATE_MODE)
		u8							low_report_rate_mode_state;
	#endif /* SHTPS_LOW_REPORTRATE_MODE */

	#if defined(SHTPS_COVER_ENABLE)
		u8							cover_state;
	#endif /* SHTPS_COVER_ENABLE */

	#if defined(SHTPS_CTRL_FW_REPORT_RATE)
		u8							fw_report_rate_cur;
		u8							fw_report_rate_req_state;
	#endif /* SHTPS_CTRL_FW_REPORT_RATE */

	#if defined(SHTPS_GLOVE_DETECT_ENABLE)
		u8							glove_enable_state;
	#endif /* SHTPS_GLOVE_DETECT_ENABLE */

	/* ------------------------------------------------------------------------ */
	/* acync */
	struct workqueue_struct		*workqueue_p;
	struct work_struct			work_data;
	struct list_head			queue;
	spinlock_t					queue_lock;
	struct shtps_req_msg		*cur_msg_p;
	u8							work_wake_lock_state;
	struct wake_lock			work_wake_lock;
	struct pm_qos_request		work_pm_qos_lock_idle;

	#if defined(SHTPS_CPU_CLOCK_CONTROL_ENABLE)
		struct shtps_cpu_clock_ctrl_info					*cpu_clock_ctrl_p;
	#endif /* SHTPS_CPU_CLOCK_CONTROL_ENABLE */

	#if defined(SHTPS_CPU_IDLE_SLEEP_CONTROL_ENABLE)
		struct shtps_cpu_idle_sleep_ctrl_info				*cpu_idle_sleep_ctrl_p;
	#endif /* SHTPS_CPU_IDLE_SLEEP_CONTROL_ENABLE */

	#if defined(SHTPS_CPU_SLEEP_CONTROL_FOR_FWUPDATE_ENABLE)
		struct shtps_cpu_sleep_ctrl_fwupdate_info			*cpu_sleep_ctrl_fwupdate_p;
	#endif /* SHTPS_CPU_SLEEP_CONTROL_FOR_FWUPDATE_ENABLE */

	#if defined(SHTPS_DEF_RECORD_LOG_FILE_ENABLE)
		struct shtps_record_log_file_info					*record_log_file_info_p;
	#endif /* SHTPS_DEF_RECORD_LOG_FILE_ENABLE */

	/* ------------------------------------------------------------------------ */
	struct device				*ctrl_dev_p;
	struct shtps_fwctl_info		*fwctl_p;
};

/* ----------------------------------------------------------------------------
*/
enum{
	SHTPS_FWTESTMODE_V01 = 0x00,
	SHTPS_FWTESTMODE_V02,
	SHTPS_FWTESTMODE_V03,
};

enum{
	SHTPS_EVENT_TU,
	SHTPS_EVENT_TD,
	SHTPS_EVENT_DRAG,
	SHTPS_EVENT_MTDU,
};

enum{
	SHTPS_TOUCH_STATE_NO_TOUCH		= 0x00,
	SHTPS_TOUCH_STATE_FINGER		= 0x01,
	SHTPS_TOUCH_STATE_PEN			= 0x02,
	SHTPS_TOUCH_STATE_PALM			= 0x03,
	SHTPS_TOUCH_STATE_UNKNOWN		= 0x04,
	SHTPS_TOUCH_STATE_HOVER			= 0x05,
	SHTPS_TOUCH_STATE_GLOVE			= 0x06,
	SHTPS_TOUCH_STATE_NARROW_OBJECT	= 0x07,
	SHTPS_TOUCH_STATE_HAND_EDGE		= 0x08,

	SHTPS_TOUCH_STATE_COVER			= 0x0A,
	SHTPS_TOUCH_STATE_STYLUS		= 0x0B,
	SHTPS_TOUCH_STATE_ERASER		= 0x0C,
	SHTPS_TOUCH_STATE_SMALL_OBJECT	= 0x0D,
};

enum{
	SHTPS_STARTUP_SUCCESS,
	SHTPS_STARTUP_FAILED
};

enum{
	SHTPS_IRQ_WAKE_DISABLE,
	SHTPS_IRQ_WAKE_ENABLE,
};

enum{
	SHTPS_IRQ_STATE_DISABLE,
	SHTPS_IRQ_STATE_ENABLE,
};

enum{
	SHTPS_MODE_NORMAL,
	SHTPS_MODE_LOADER,
};

enum{
	SHTPS_EVENT_START,
	SHTPS_EVENT_STOP,
	SHTPS_EVENT_SLEEP,
	SHTPS_EVENT_WAKEUP,
	SHTPS_EVENT_STARTLOADER,
	SHTPS_EVENT_STARTTM,
	SHTPS_EVENT_STOPTM,
	SHTPS_EVENT_FACETOUCHMODE_ON,
	SHTPS_EVENT_FACETOUCHMODE_OFF,
	SHTPS_EVENT_INTERRUPT,
	SHTPS_EVENT_TIMEOUT,
};

enum{
	SHTPS_STATE_IDLE,
	SHTPS_STATE_ACTIVE,
	SHTPS_STATE_BOOTLOADER,
	SHTPS_STATE_FACETOUCH,
	SHTPS_STATE_FWTESTMODE,
	SHTPS_STATE_SLEEP,
	SHTPS_STATE_SLEEP_FACETOUCH,
};

enum{
	SHTPS_PHYSICAL_KEY_DOWN = 0,
	SHTPS_PHYSICAL_KEY_UP,
	SHTPS_PHYSICAL_KEY_NUM,
};

enum{
	SHTPS_IRQ_FLASH		= 0x01,
	SHTPS_IRQ_STATE		= 0x02,
	SHTPS_IRQ_ABS 		= 0x04,
	SHTPS_IRQ_ANALOG 	= 0x08,
	SHTPS_IRQ_BUTTON 	= 0x10,
	SHTPS_IRQ_SENSOR 	= 0x20,
	SHTPS_IRQ_ALL		= (  SHTPS_IRQ_FLASH
							| SHTPS_IRQ_STATE
							| SHTPS_IRQ_ABS
							| SHTPS_IRQ_ANALOG
							| SHTPS_IRQ_BUTTON
							| SHTPS_IRQ_SENSOR),
};

enum{
	SHTPS_LPMODE_TYPE_NON_CONTINUOUS = 0,
	SHTPS_LPMODE_TYPE_CONTINUOUS,
};

enum{
	SHTPS_LPMODE_REQ_NONE		= 0x00,
	SHTPS_LPMODE_REQ_COMMON		= 0x01,
	SHTPS_LPMODE_REQ_ECO		= 0x02,
	SHTPS_LPMODE_REQ_HOVER_OFF	= 0x04,	
	SHTPS_LPMODE_REQ_LCD_BRIGHT	= 0x08,
};

enum{
	SHTPS_FUNC_REQ_EVEMT_OPEN = 0,
	SHTPS_FUNC_REQ_EVEMT_CLOSE,
	SHTPS_FUNC_REQ_EVEMT_ENABLE,
	SHTPS_FUNC_REQ_EVEMT_DISABLE,
	SHTPS_FUNC_REQ_EVEMT_CHECK_CRC_ERROR,
	SHTPS_FUNC_REQ_EVEMT_PROXIMITY_CHECK,
	SHTPS_FUNC_REQ_EVEMT_LCD_ON,
	SHTPS_FUNC_REQ_EVEMT_LCD_OFF,
	SHTPS_FUNC_REQ_EVEMT_GRIP_ON,
	SHTPS_FUNC_REQ_EVEMT_GRIP_OFF,
	SHTPS_FUNC_REQ_EVEMT_COVER_ON,
	SHTPS_FUNC_REQ_EVEMT_COVER_OFF,
	SHTPS_FUNC_REQ_BOOT_FW_UPDATE,
};

enum{
	SHTPS_DRAG_DIR_NONE = 0,
	SHTPS_DRAG_DIR_PLUS,
	SHTPS_DRAG_DIR_MINUS,
};

enum{
	SHTPS_LPWG_DETECT_GESTURE_TYPE_NONE			= 0x00,
	SHTPS_LPWG_DETECT_GESTURE_TYPE_DOUBLE_TAP	= 0x01,
	SHTPS_LPWG_DETECT_GESTURE_TYPE_SWIPE		= 0x02,
};

enum{
	SHTPS_GESTURE_TYPE_NONE						= 0x00,
	SHTPS_GESTURE_TYPE_ONE_FINGER_SINGLE_TAP	= 0x01,
	SHTPS_GESTURE_TYPE_ONE_FINGER_TAP_AND_HOLD	= 0x02,
	SHTPS_GESTURE_TYPE_ONE_FINGER_DOUBLE_TAP	= 0x03,
	SHTPS_GESTURE_TYPE_ONE_FINGER_EARLY_TAP		= 0x04,
	SHTPS_GESTURE_TYPE_ONE_FINGER_FLICK			= 0x05,
	SHTPS_GESTURE_TYPE_ONE_FINGER_PRESS			= 0x06,
	SHTPS_GESTURE_TYPE_ONE_FINGER_SWIPE			= 0x07,
	SHTPS_GESTURE_TYPE_ONE_FINGER_CIRCLE		= 0x08,
	SHTPS_GESTURE_TYPE_ONE_FINGER_TRIANGLE		= 0x09,
	SHTPS_GESTURE_TYPE_ONE_FINGER_VEE			= 0x0A,

	SHTPS_GESTURE_TYPE_TRIPLE_TAP				= 0x0C,
	SHTPS_GESTURE_TYPE_CLICK					= 0x0D,

	SHTPS_GESTURE_TYPE_PINCH					= 0x80,
	SHTPS_GESTURE_TYPE_ROTATE					= 0x81,
};

enum{
	SHTPS_LPWG_SWEEP_ON_REQ_OFF			= 0x00,
	SHTPS_LPWG_SWEEP_ON_REQ_ON			= 0x01,
	SHTPS_LPWG_SWEEP_ON_REQ_GRIP_ONLY	= 0x02,
};

enum{
	SHTPS_LPWG_DOUBLE_TAP_REQ_OFF	= 0x00,
	SHTPS_LPWG_DOUBLE_TAP_REQ_ON	= 0x01,
};

enum{
	SHTPS_LPWG_REQ_NONE			= 0x00,
	SHTPS_LPWG_REQ_SWEEP_ON		= 0x01,
	SHTPS_LPWG_REQ_DOUBLE_TAP	= 0x02,
};

enum{
	SHTPS_LPWG_SET_STATE_OFF		= SHTPS_LPWG_REQ_NONE,
	SHTPS_LPWG_SET_STATE_SWEEP_ON	= SHTPS_LPWG_REQ_SWEEP_ON,
	SHTPS_LPWG_SET_STATE_DOUBLE_TAP	= SHTPS_LPWG_REQ_DOUBLE_TAP,
};

enum{
	SHTPS_HW_TYPE_BOARD = 0,
	SHTPS_HW_TYPE_HANDSET,
};

enum{
	SHTPS_HW_REV_ES_0 = 0,
	SHTPS_HW_REV_ES_1,
	SHTPS_HW_REV_PP_1,
	SHTPS_HW_REV_PP_2,
	SHTPS_HW_REV_PMP,
	SHTPS_HW_REV_MP,
};

#if defined(SHTPS_PROXIMITY_SUPPORT_ENABLE)
enum{
	SHTPS_PROXIMITY_ENABLE = SH_PROXIMITY_ENABLE,
	SHTPS_PROXIMITY_DISABLE = SH_PROXIMITY_DISABLE,
};

enum{
	SHTPS_PROXIMITY_NEAR = SH_PROXIMITY_NEAR,
	SHTPS_PROXIMITY_FAR = SH_PROXIMITY_FAR,
};
#endif

enum{
	SHTPS_DEV_STATE_SLEEP = 0,
	SHTPS_DEV_STATE_DOZE,
	SHTPS_DEV_STATE_ACTIVE,
	SHTPS_DEV_STATE_LPWG,
	SHTPS_DEV_STATE_LOADER,
	SHTPS_DEV_STATE_TESTMODE,
};

/* ----------------------------------------------------------------------------
*/
extern struct shtps_fts*	gShtps_fts;

void shtps_mutex_lock_ctrl(void);
void shtps_mutex_unlock_ctrl(void);
void shtps_mutex_lock_loader(void);
void shtps_mutex_unlock_loader(void);
void shtps_mutex_lock_proc(void);
void shtps_mutex_unlock_proc(void);
void shtps_mutex_lock_facetouch_qos_ctrl(void);
void shtps_mutex_unlock_facetouch_qos_ctrl(void);
#if defined(SHTPS_CHECK_HWID_ENABLE)
	int shtps_system_get_hw_revision(void);
	int shtps_system_get_hw_type(void);
#endif /* SHTPS_CHECK_HWID_ENABLE */

void shtps_system_set_sleep(struct shtps_fts *ts);
void shtps_system_set_wakeup(struct shtps_fts *ts);
#if defined(SHTPS_GUARANTEE_I2C_ACCESS_IN_WAKE_ENABLE)
	void shtps_charger_armor_proc(struct shtps_fts *ts, u8 charger);
	void shtps_ioctl_setlpwg_proc(struct shtps_fts *ts, u8 on);
	void shtps_ioctl_setlpmode_proc(struct shtps_fts *ts, u8 on);
	void shtps_ioctl_setconlpmode_proc(struct shtps_fts *ts, u8 on);
	void shtps_ioctl_setlcdbrightlpmode_proc(struct shtps_fts *ts, u8 on);
	void shtps_ioctl_setpen_proc(struct shtps_fts *ts, u8 on);
	int shtps_check_suspend_state(struct shtps_fts *ts, int proc, u8 param);
	void shtps_set_suspend_state(struct shtps_fts *ts);
	void shtps_clr_suspend_state(struct shtps_fts *ts);
	void shtps_ioctl_sethover_proc(struct shtps_fts *ts, u8 on);
	void shtps_ioctl_setlpwg_doubletap_proc(struct shtps_fts *ts, u8 on);
	void shtps_ioctl_setglove_proc(struct shtps_fts *ts, u8 on);
	void shtps_ioctl_set_high_report_mode_proc(struct shtps_fts *ts, u8 on);
#endif /* SHTPS_GUARANTEE_I2C_ACCESS_IN_WAKE_ENABLE */
int shtps_start(struct shtps_fts *ts);
void shtps_shutdown(struct shtps_fts *ts);

void shtps_reset(struct shtps_fts *ts);



int shtps_fwdate(struct shtps_fts *ts, u8 *year, u8 *month);
int shtps_get_serial_number(struct shtps_fts *ts, u8 *buf);
u16 shtps_fwver(struct shtps_fts *ts);
u16 shtps_fwver_builtin(struct shtps_fts *ts);
int shtps_fwsize_builtin(struct shtps_fts *ts);
unsigned char* shtps_fwdata_builtin(struct shtps_fts *ts);

int shtps_wait_startup(struct shtps_fts *ts);
int shtps_get_fingermax(struct shtps_fts *ts);
int shtps_get_diff(unsigned short pos1, unsigned short pos2, unsigned long factor);
int shtps_get_fingerwidth(struct shtps_fts *ts, int num, struct shtps_touch_info *info);
void shtps_set_eventtype(u8 *event, u8 type);
void shtps_report_touch_on(struct shtps_fts *ts, int finger, int x, int y, int w, int wx, int wy, int z);
void shtps_report_touch_off(struct shtps_fts *ts, int finger, int x, int y, int w, int wx, int wy, int z);

void shtps_event_report(struct shtps_fts *ts, struct shtps_touch_info *info, u8 event);

#if defined(SHTPS_LPWG_MODE_ENABLE)
	void shtps_set_lpwg_mode_on(struct shtps_fts *ts);
	void shtps_set_lpwg_mode_off(struct shtps_fts *ts);
	int shtps_is_lpwg_active(struct shtps_fts *ts);
	#if defined(SHTPS_PROXIMITY_SUPPORT_ENABLE)
		void shtps_notify_cancel_wakeup_event(struct shtps_fts *ts);
	#endif /* SHTPS_PROXIMITY_SUPPORT_ENABLE */
	#if defined(SHTPS_LPWG_GRIP_SUPPORT_ENABLE)
		void shtps_lpwg_grip_set_state(struct shtps_fts *ts, u8 request);
	#endif /* SHTPS_LPWG_GRIP_SUPPORT_ENABLE */
	int shtps_set_lpwg_sleep_check(struct shtps_fts *ts);
	int shtps_set_lpwg_wakeup_check(struct shtps_fts *ts);
#endif /* SHTPS_LPWG_MODE_ENABLE */
int shtps_get_tm_rxsize(struct shtps_fts *ts);
int shtps_get_tm_txsize(struct shtps_fts *ts);
int shtps_baseline_offset_disable(struct shtps_fts *ts);
void shtps_read_tmdata(struct shtps_fts *ts, u8 mode);

void shtps_irq_disable(struct shtps_fts *ts);
void shtps_irq_enable(struct shtps_fts *ts);

void shtps_read_touchevent(struct shtps_fts *ts, int state);
#if defined( SHTPS_BOOT_FWUPDATE_ENABLE )
	int shtps_boot_fwupdate_enable_check(struct shtps_fts *ts);
#endif /* #if defined( SHTPS_BOOT_FWUPDATE_ENABLE ) */

int shtps_enter_bootloader(struct shtps_fts *ts);
int shtps_lockdown_bootloader(struct shtps_fts *ts, u8* fwdata);
int shtps_flash_erase(struct shtps_fts *ts);
int shtps_flash_writeImage(struct shtps_fts *ts, u8 *fwdata);
int shtps_flash_writeConfig(struct shtps_fts *ts, u8 *fwdata);
int shtps_set_veilview_state(struct shtps_fts *ts, unsigned long arg);
int shtps_get_veilview_pattern(struct shtps_fts *ts);
int request_event(struct shtps_fts *ts, int event, int param);

void shtps_event_force_touchup(struct shtps_fts *ts);
void shtps_report_stflib_kevt(struct shtps_fts *ts, int event, int value);

int shtps_fw_update(struct shtps_fts *ts, const unsigned char *fw_data, int fw_size);
int shtps_get_logflag(void);
#if defined( SHTPS_DEVELOP_MODE_ENABLE )
int shtps_read_touchevent_from_outside(void);
#endif /* #if defined( SHTPS_DEVELOP_MODE_ENABLE ) */

#if defined( SHTPS_LOW_POWER_MODE_ENABLE )
	void shtps_set_lpmode(struct shtps_fts *ts, int type, int req, int on);
	int shtps_check_set_doze_enable(void);
#endif	/* SHTPS_LOW_POWER_MODE_ENABLE */

#if defined(SHTPS_GLOVE_DETECT_ENABLE)
	void shtps_report_touch_glove_on(struct shtps_fts *ts, int finger, int x, int y, int w, int wx, int wy, int z);
	void shtps_report_touch_glove_off(struct shtps_fts *ts, int finger, int x, int y, int w, int wx, int wy, int z);
#endif /* SHTPS_GLOVE_DETECT_ENABLE */

#if defined( CONFIG_SHTPS_FOCALTECH_FACETOUCH_OFF_DETECT )
	void shtps_facetouch_wakelock(struct shtps_fts *ts, u8 on);
#endif /* CONFIG_SHTPS_FOCALTECH_FACETOUCH_OFF_DETECT */

#if defined(SHTPS_HOST_LPWG_MODE_ENABLE)
	int shtps_check_host_lpwg_enable(void);
#endif /* SHTPS_HOST_LPWG_MODE_ENABLE */

void shtps_sleep(struct shtps_fts *ts, int on);

#if defined(SHTPS_CTRL_FW_REPORT_RATE)
	int shtps_set_fw_report_rate_init(struct shtps_fts *ts);
	void shtps_set_fw_report_rate(struct shtps_fts *ts);
#endif /* SHTPS_CTRL_FW_REPORT_RATE */

void shtps_set_startup_min_time(struct shtps_fts *ts, int time);

#if defined(SHTPS_GLOVE_DETECT_ENABLE)
	int shtps_set_glove_detect_enable(struct shtps_fts *ts);
	int shtps_set_glove_detect_disable(struct shtps_fts *ts);
#endif /* SHTPS_GLOVE_DETECT_ENABLE */

#if defined(SHTPS_COVER_ENABLE)
	void shtps_cover_set_state(struct shtps_fts *ts, u8 request);
#endif /* SHTPS_COVER_ENABLE */

#ifdef CONFIG_FB
int shtps_fb_notifier_callback(struct notifier_block *self, unsigned long event, void *data);
#endif

extern int shtps_fts_create_apk_debug_channel(struct shtps_fts *ts);
extern void shtps_fts_release_apk_debug_channel(struct shtps_fts *ts);

extern int shtps_fts_create_sysfs(struct shtps_fts *ts);
extern int shtps_fts_remove_sysfs(struct shtps_fts *ts);

int shtpsif_init(void);
void shtpsif_exit(void);
#endif /* __SHTPS_FTS_H__ */
