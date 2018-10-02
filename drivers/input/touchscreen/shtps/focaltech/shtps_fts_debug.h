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

#ifndef __SHTPS_FTS_DEBUG_H__
#define __SHTPS_FTS_DEBUG_H__

/* -----------------------------------------------------------------------------------
 */
#ifdef	SHTPS_DEVELOP_MODE_ENABLE
	#define SHTPS_TOUCH_EMURATOR_ENABLE
	#define SHTPS_TOUCH_EVENTLOG_ENABLE
	#define SHTPS_ACCUMULATE_FW_REPORT_INFO_ENABLE
#endif

/* -----------------------------------------------------------------------------------
 */
struct shtps_debug_init_param{
	struct kobject		*shtps_root_kobj;
};

#define SHTPS_DEBUG_PARAM_NAME_ONOFF	"enable"
#define SHTPS_DEBUG_PARAM_NAME_LOG		"log"

struct shtps_debug_param_regist_info{
	const char *parent_name;
	const char *param_name;
	int *param_p;
	ssize_t (*show)(struct kobject *kobj, struct kobj_attribute *attr, char *buf);
	ssize_t (*store)(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count);
};

int shtps_debug_param_add(struct shtps_debug_param_regist_info *info_p);


/* -----------------------------------------------------------------------------------
 */
int shtps_debug_init(struct shtps_debug_init_param *param);
void shtps_debug_deinit(void);
void shtps_debug_sleep_enter(void);
#if defined ( SHTPS_TOUCH_EMURATOR_ENABLE )
	int shtps_touch_emu_is_running(void);
	int shtps_touch_emu_set_finger_info(u8 *buf, int bufsize);
	int shtps_touch_emu_is_recording(void);
	int shtps_touch_emu_rec_finger_info(u8 *buf);
	void shtps_touch_emu_stop(void);
#endif /* #if defined ( SHTPS_TOUCH_EMURATOR_ENABLE ) */
#if defined ( SHTPS_TOUCH_EVENTLOG_ENABLE )
	int shtps_touch_eventlog_is_recording(void);
	int shtps_touch_eventlog_rec_event_info(int state, int finger, int x, int y, int w, int wx, int wy, int z);
#endif /* #if defined ( SHTPS_TOUCH_EVENTLOG_ENABLE ) */

#if defined ( SHTPS_ACCUMULATE_FW_REPORT_INFO_ENABLE )
	void shtps_accumulate_fw_report_info_add(struct shtps_touch_info *touch_info);
#endif /* #if defined ( SHTPS_ACCUMULATE_FW_REPORT_INFO_ENABLE ) */

#endif /* __SHTPS_FTS_DEBUG_H__ */
