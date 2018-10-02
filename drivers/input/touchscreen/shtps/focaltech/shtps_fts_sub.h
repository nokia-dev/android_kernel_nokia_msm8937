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

#ifndef __SHTPS_SUB_H__
#define __SHTPS_SUB_H__
/* -------------------------------------------------------------------------- */
#include <linux/wakelock.h>
#include <linux/slab.h>
#include <linux/pm_qos.h>


/* -------------------------------------------------------------------------- */

#include "shtps_fts.h"
/* -------------------------------------------------------------------------- */
#ifdef SHTPS_SEND_SHTERM_EVENT_ENABLE
	#include <sharp/shterm_k.h>
	void shtps_send_shterm_event(int event_num);
#endif

/* -------------------------------------------------------------------------- */
#if defined(SHTPS_PROXIMITY_SUPPORT_ENABLE)
	int shtps_proximity_state_check(struct shtps_fts *ts);
	int shtps_proximity_check(struct shtps_fts *ts);
	void shtps_lpwg_proximity_check_delayed_work_function(struct work_struct *work);
	void shtps_lpwg_proximity_check_cancel(struct shtps_fts *ts);
	void shtps_lpwg_proximity_check_start(struct shtps_fts *ts);
#endif /* SHTPS_PROXIMITY_SUPPORT_ENABLE */

/* -------------------------------------------------------------------------- */
#define SHTPS_PERFORMANCE_CHECK_STATE_START	(0)
#define SHTPS_PERFORMANCE_CHECK_STATE_CONT	(1)
#define SHTPS_PERFORMANCE_CHECK_STATE_END	(2)
#if defined(SHTPS_PERFORMANCE_CHECK_ENABLE)
	void shtps_performance_check_init(void);
	void shtps_performance_check(int state);
#else
	#define	shtps_performance_check_init()
	#define	shtps_performance_check(A)
#endif	/* SHTPS_PERFORMANCE_CHECK_ENABLE */

/* -------------------------------------------------------------------------- */
#if defined(SHTPS_CPU_CLOCK_CONTROL_ENABLE)
	#include <linux/cpufreq.h>
	#include <linux/notifier.h>

	struct shtps_cpu_clock_ctrl_info{
		int							perf_lock_cluster0_state;
		int							perf_lock_cluster1_state;
		struct delayed_work			perf_lock_cluster0_disable_delayed_work;
		struct delayed_work			perf_lock_cluster1_disable_delayed_work;
		int							report_event;
		struct shtps_fts			*ts_p;
		struct fingers				fingers_hist[2][SHTPS_FINGER_MAX];
		struct pm_qos_request		qos_cpu_cluster0_latency;
		int							qos_cpu_cluster0_latency_state;
		struct pm_qos_request		qos_cpu_cluster1_latency;
		int							qos_cpu_cluster1_latency_state;
	};

	void shtps_perflock_register_notifier(void);
	void shtps_perflock_unregister_notifier(void);
	void shtps_perflock_enable_start(struct shtps_fts *ts, u8 event, struct shtps_touch_info *info);
	void shtps_perflock_sleep(struct shtps_fts *ts);
	void shtps_perflock_init(struct shtps_fts *ts);
	void shtps_perflock_deinit(struct shtps_fts *ts);
	void shtps_perflock_set_event(struct shtps_fts *ts, u8 event);
#else
	#define shtps_perflock_register_notifier()
	#define shtps_perflock_unregister_notifier()
	#define shtps_perflock_enable_start(A, B, C)
	#define shtps_perflock_sleep(A)
	#define shtps_perflock_init(A)
	#define shtps_perflock_deinit(A)
	#define shtps_perflock_set_event(A, B)
#endif /* SHTPS_CPU_CLOCK_CONTROL_ENABLE */

/* -------------------------------------------------------------------------- */
#if defined(SHTPS_CPU_IDLE_SLEEP_CONTROL_ENABLE)
	struct shtps_cpu_idle_sleep_ctrl_info{
		struct pm_qos_request		qos_cpu_latency;
		int							wake_lock_idle_state;
	};

	void shtps_wake_lock_idle(struct shtps_fts *ts);
	void shtps_wake_unlock_idle(struct shtps_fts *ts);
	void shtps_cpu_idle_sleep_wake_lock_init( struct shtps_fts *ts );
	void shtps_cpu_idle_sleep_wake_lock_deinit( struct shtps_fts *ts );
#else
	#define shtps_wake_lock_idle(A)
	#define shtps_wake_unlock_idle(A)
	#define shtps_cpu_idle_sleep_wake_lock_init(A)
	#define shtps_cpu_idle_sleep_wake_lock_deinit(A)
#endif /* SHTPS_CPU_IDLE_SLEEP_CONTROL_ENABLE */

/* -------------------------------------------------------------------------- */
#if defined(SHTPS_CPU_SLEEP_CONTROL_FOR_FWUPDATE_ENABLE)
	struct shtps_cpu_sleep_ctrl_fwupdate_info{
		struct wake_lock			wake_lock_for_fwupdate;
		struct pm_qos_request		qos_cpu_latency_for_fwupdate;
		int							wake_lock_for_fwupdate_state;
	};

	void shtps_wake_lock_for_fwupdate(struct shtps_fts *ts);
	void shtps_wake_unlock_for_fwupdate(struct shtps_fts *ts);
	void shtps_fwupdate_wake_lock_init( struct shtps_fts *ts );
	void shtps_fwupdate_wake_lock_deinit( struct shtps_fts *ts );
#else
	#define shtps_wake_lock_for_fwupdate(A)
	#define shtps_wake_unlock_for_fwupdate(A)
	#define shtps_fwupdate_wake_lock_init(A)
	#define shtps_fwupdate_wake_lock_deinit(A)
#endif /* SHTPS_CPU_SLEEP_CONTROL_FOR_FWUPDATE_ENABLE */

/* -------------------------------------------------------------------------- */
#if defined( SHTPS_TPIN_CHECK_ENABLE ) || defined( SHTPS_CHECK_CRC_ERROR_ENABLE )
	int shtps_tpin_enable_check(struct shtps_fts *ts);
#endif /* SHTPS_TPIN_CHECK_ENABLE || SHTPS_CHECK_CRC_ERROR_ENABLE*/

/* -------------------------------------------------------------------------- */
struct shtps_req_msg;

void shtps_func_request_async( struct shtps_fts *ts, int event);
int shtps_func_request_sync( struct shtps_fts *ts, int event);
int shtps_func_async_init( struct shtps_fts *ts);
void shtps_func_async_deinit( struct shtps_fts *ts);
/* -------------------------------------------------------------------------- */
#if defined(SHTPS_DEF_RECORD_LOG_FILE_ENABLE)
	#define SHTPS_DEF_RECORD_LOG_BUF_SIZE		4096

	struct shtps_record_log_file_info{
		struct shtps_fts		*ts_p;
		int							status;
		int							busy;
		char						log_str[SHTPS_DEF_RECORD_LOG_BUF_SIZE];
	};

	void shtps_record_log_file_init(struct shtps_fts *ts);
	void shtps_record_log_file_deinit(struct shtps_fts *ts);

        enum {
            SHTPS_RECORD_LOG_RESULT_SUCCESS,
            SHTPS_RECORD_LOG_RESULT_FAILURE
        };

        enum {
            SHTPS_RECORD_LOG_TYPE_LCD_ON,
            SHTPS_RECORD_LOG_TYPE_LCD_OFF,
            SHTPS_RECORD_LOG_TYPE_REZERO
        };
#else
	#define shtps_record_log_file_init(A)
	#define shtps_record_log_file_deinit(A)

#endif /* SHTPS_DEF_RECORD_LOG_FILE_ENABLE */

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
#endif	/* __SHTPS_SUB_H__ */
