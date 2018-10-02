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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/input.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/poll.h>
#include <linux/cdev.h>
#include <linux/jiffies.h>
#include <linux/irq.h>
#include <linux/of_irq.h>
#include <linux/wakelock.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/hrtimer.h>
#include <linux/input/mt.h>

#include <linux/pm_qos.h>

#include <linux/init.h>
#include <asm/processor.h>
#include <asm/uaccess.h>
#include <linux/pinctrl/pinconf-generic.h>

#if defined(SHTPS_CPU_CLOCK_CONTROL_ENABLE)
#include <linux/cpufreq.h>
#include <linux/notifier.h>
#endif /* defined(SHTPS_CPU_CLOCK_CONTROL_ENABLE) */

#include <linux/input/shtps_dev.h>

#include "shtps_fts.h"
#include "shtps_param_extern.h"
#include "shtps_fts_sub.h"
#include "shtps_fwctl.h"
#include "shtps_log.h"

#if defined( SHTPS_DEF_RECORD_LOG_FILE_ENABLE )
#include <linux/path.h>
#include <linux/namei.h>
#endif /* SHTPS_DEF_RECORD_LOG_FILE_ENABLE */

/* -----------------------------------------------------------------------------------
 */
#if defined(SHTPS_CPU_CLOCK_CONTROL_ENABLE)
	static DEFINE_MUTEX(shtps_cpu_clock_ctrl_lock);
#endif /* SHTPS_CPU_CLOCK_CONTROL_ENABLE */

#if defined(SHTPS_CPU_IDLE_SLEEP_CONTROL_ENABLE)
	static DEFINE_MUTEX(shtps_cpu_idle_sleep_ctrl_lock);
#endif /* SHTPS_CPU_IDLE_SLEEP_CONTROL_ENABLE */

#if defined(SHTPS_CPU_SLEEP_CONTROL_FOR_FWUPDATE_ENABLE)
	static DEFINE_MUTEX(shtps_cpu_sleep_ctrl_for_fwupdate_lock);
#endif /* SHTPS_CPU_SLEEP_CONTROL_FOR_FWUPDATE_ENABLE */

#if defined(SHTPS_DEF_RECORD_LOG_FILE_ENABLE)
	static DEFINE_MUTEX(shtps_record_log_file_lock);
#endif /* SHTPS_DEF_RECORD_LOG_FILE_ENABLE */

#define	TPS_MUTEX_LOG_LOCK(VALUE)		SHTPS_LOG_DBG_PRINT("mutex_lock:   -> (%s)\n",VALUE)
#define	TPS_MUTEX_LOG_UNLOCK(VALUE)		SHTPS_LOG_DBG_PRINT("mutex_unlock: <- (%s)\n",VALUE)

/* -----------------------------------------------------------------------------------
 */
#if defined( SHTPS_DEF_RECORD_LOG_FILE_ENABLE )
	#define SHTPS_RECORD_LOG_FILE			("/durable/touchpanel/touchpanel.log")
	#define SHTPS_RECORD_LOG_FILE_CHECK_SIZE	5000
	#define SHTPS_RECORD_LOG_EVENT_STORE_NUM	100
	#define SHTPS_RECORD_LOG_DATE_STR_LENGTH	34
	#define SHTPS_RECORD_LOG_EVENT_STR_LENGTH	16
	#define SHTPS_RECORD_LOG_BUF_SPACING_SIZE	(((SHTPS_RECORD_LOG_EVENT_STORE_NUM * (SHTPS_RECORD_LOG_DATE_STR_LENGTH + SHTPS_RECORD_LOG_EVENT_STR_LENGTH))*2))

	struct shtps_record_log_event_store_log {
		char	event_log_str[SHTPS_RECORD_LOG_DATE_STR_LENGTH + SHTPS_RECORD_LOG_EVENT_STR_LENGTH];
		int	event_log_length;
	};

	struct shtps_record_log_info {
		int	write_length;
		int	read_flag;
		int	record_log_file_out_flag;
		int	event_store_tbl_array;
		char	date_str_buf[SHTPS_RECORD_LOG_DATE_STR_LENGTH];
		char	write_buf[SHTPS_RECORD_LOG_FILE_CHECK_SIZE];
		struct shtps_record_log_event_store_log 	event_store_tbl[SHTPS_RECORD_LOG_EVENT_STORE_NUM];
		struct shtps_record_log_event_store_log 	event_init_read_tbl[SHTPS_RECORD_LOG_EVENT_STORE_NUM];
	};
	static struct shtps_record_log_info *shtps_dur_info_p = 0;

	const static char *WeekOfDay[] = {
	    "Sun",
	    "Mon",
	    "Tue",
	    "Wed",
	    "Thu",
	    "Fri",
	    "Sat"
	};

	int shtps_record_log_api_add_log(int type);
	void shtps_record_log_api_memory_init(void);
	void shtps_record_log_api_memory_deinit(void);
	static ssize_t shtps_record_log_kernel_write(struct file *fp, const char *buf, size_t size);
	static ssize_t shtps_record_log_kernel_read(struct file *fp, char *buf, size_t size , unsigned int offset);
	static int shtps_record_log_file_size_check(char *buf, size_t check_size);
	static int shtps_record_log_file_init_read(void);
	static int shtps_record_log_file(void);
	static int shtps_record_log_summary_init_file(void);
	static void record_log_get_date_str(char *str_buf);
	static int shtps_record_log_add_event_log_memory(char *logstr);
	static int shtps_record_log_add_event_log_store(char *logstr, struct shtps_record_log_event_store_log *store_tbl_p, int *array);
	static int shtps_record_log_event_log_buf(void);
#endif /* SHTPS_DEF_RECORD_LOG_FILE_ENABLE */

/* -----------------------------------------------------------------------------------
 */
#if defined(SHTPS_PERFORMANCE_CHECK_PIN_ENABLE)
	#define SHTPS_PERFORMANCE_CHECK_PIN		(32)
	static int shtps_performance_check_pin_state = 0;
#endif /* SHTPS_PERFORMANCE_CHECK_PIN_ENABLE */

#if defined( SHTPS_PERFORMANCE_TIME_LOG_ENABLE )
	static int shtps_performace_log_point = 0;
	static struct timeval shtps_performance_log_tv;
#endif /* SHTPS_PERFORMANCE_TIME_LOG_ENABLE */

#if defined(SHTPS_CPU_CLOCK_CONTROL_ENABLE)
	static void shtps_mutex_lock_cpu_clock_ctrl(void);
	static void shtps_mutex_unlock_cpu_clock_ctrl(void);
	static void shtps_perflock_cluster0_enable(struct shtps_fts *ts);
	static void shtps_perflock_cluster0_disable(struct shtps_fts *ts);
	static void shtps_perflock_cluster0_enable_check(struct shtps_fts *ts, u8 event, struct shtps_touch_info *info);
	static int shtps_perflock_cluster0_disable_timer_start(struct shtps_fts *ts, unsigned long delay_ms);
	static void shtps_perflock_cluster0_disable_delayed_work_function(struct work_struct *work);

	static void shtps_perflock_cluster1_enable(struct shtps_fts *ts);
	static void shtps_perflock_cluster1_disable(struct shtps_fts *ts);
	static void shtps_perflock_cluster1_enable_check(struct shtps_fts *ts, u8 event, struct shtps_touch_info *info);
	static int shtps_perflock_cluster1_disable_timer_start(struct shtps_fts *ts, unsigned long delay_ms);
	static void shtps_perflock_cluster1_disable_delayed_work_function(struct work_struct *work);
#endif /* SHTPS_CPU_CLOCK_CONTROL_ENABLE */

#if defined(SHTPS_CPU_IDLE_SLEEP_CONTROL_ENABLE)
	static void shtps_mutex_lock_cpu_idle_sleep_ctrl(void);
	static void shtps_mutex_unlock_cpu_idle_sleep_ctrl(void);
	static void shtps_wake_lock_idle_l(struct shtps_fts *ts);
	static void shtps_wake_unlock_idle_l(struct shtps_fts *ts);
#endif /* SHTPS_CPU_IDLE_SLEEP_CONTROL_ENABLE */

#if defined(SHTPS_CPU_SLEEP_CONTROL_FOR_FWUPDATE_ENABLE)
	static void shtps_mutex_lock_cpu_sleep_ctrl(void);
	static void shtps_mutex_unlock_cpu_sleep_ctrl(void);
#endif /* SHTPS_CPU_SLEEP_CONTROL_FOR_FWUPDATE_ENABLE */

	static void shtps_func_open(struct shtps_fts *ts);
	static void shtps_func_close(struct shtps_fts *ts);
	static int shtps_func_enable(struct shtps_fts *ts);
	static void shtps_func_disable(struct shtps_fts *ts);

	static void shtps_func_request_async_complete(void *arg_p);
	static void shtps_func_request_sync_complete(void *arg_p);
	static void shtps_func_workq( struct work_struct *work_p );

/* -------------------------------------------------------------------------- */
struct shtps_req_msg {	//**********	SHTPS_ASYNC_OPEN_ENABLE
	struct list_head queue;
	void	(*complete)(void *context);
	void	*context;
	int		status;
	int		event;
	void	*param_p;
};

/* -------------------------------------------------------------------------- */
#if defined(SHTPS_PERFORMANCE_CHECK_ENABLE)
void shtps_performance_check_init(void)
{
	#if defined( SHTPS_PERFORMANCE_CHECK_PIN_ENABLE )
		int result;
		SHTPS_LOG_FUNC_CALL();
		result = gpio_request(SHTPS_PERFORMANCE_CHECK_PIN, "tps_test");
		if(result < 0){
			DBG_PRINTK("test pin gpio_request() error : %d\n", result);
		}

		result = gpio_tlmm_config(GPIO_CFG(SHTPS_PERFORMANCE_CHECK_PIN, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		if(result < 0){
			DBG_PRINTK("test pin gpio_tlmm_config() error : %d\n", result);
		}

		shtps_performance_check_pin_state = 0;
		gpio_set_value(SHTPS_PERFORMANCE_CHECK_PIN, shtps_performance_check_pin_state);
	#endif /* SHTPS_PERFORMANCE_CHECK_PIN_ENABLE */

	#if defined( SHTPS_PERFORMANCE_TIME_LOG_ENABLE )
		shtps_performace_log_point = 0;
	#endif /* SHTPS_PERFORMANCE_TIME_LOG_ENABLE */
}

void shtps_performance_check(int state)
{
	#if defined( SHTPS_PERFORMANCE_CHECK_PIN_ENABLE ) || defined( SHTPS_PERFORMANCE_TIME_LOG_ENABLE )
	//	SHTPS_LOG_FUNC_CALL();
	#endif

	#if defined( SHTPS_PERFORMANCE_CHECK_PIN_ENABLE )
		if(state == SHTPS_PERFORMANCE_CHECK_STATE_START){
			shtps_performance_check_pin_state = 1;
		}else{
			shtps_performance_check_pin_state = 
				(shtps_performance_check_pin_state == 0)? 1 : 0;
		}

		gpio_set_value(SHTPS_PERFORMANCE_CHECK_PIN, shtps_performance_check_pin_state);

		if(state == SHTPS_PERFORMANCE_CHECK_STATE_END){
			shtps_performance_check_pin_state = 0;
			gpio_set_value(SHTPS_PERFORMANCE_CHECK_PIN, shtps_performance_check_pin_state);
		}
	#endif /* SHTPS_PERFORMANCE_CHECK_PIN_ENABLE */

	#if defined( SHTPS_PERFORMANCE_TIME_LOG_ENABLE )
		if(state == SHTPS_PERFORMANCE_CHECK_STATE_START){
			shtps_performace_log_point = 1;
		}else{
			static struct timeval tv;
			do_gettimeofday(&tv);

			DBG_PRINTK("[performace] pt:%02d time:%ldus\n",
				shtps_performace_log_point++,
				(tv.tv_sec * 1000000 + tv.tv_usec) - 
					(shtps_performance_log_tv.tv_sec * 1000000 + shtps_performance_log_tv.tv_usec));
		}
		do_gettimeofday(&shtps_performance_log_tv);
	#endif /* SHTPS_PERFORMANCE_TIME_LOG_ENABLE */
}
#endif	/* SHTPS_PERFORMANCE_CHECK_ENABLE */

/* -----------------------------------------------------------------------------------
 */
#ifdef SHTPS_SEND_SHTERM_EVENT_ENABLE
void shtps_send_shterm_event(int event_num)
{
	shbattlog_info_t info;
	SHTPS_LOG_FUNC_CALL();
	memset(&info, 0x00, sizeof(info));
	info.event_num = event_num;
	shterm_k_set_event(&info);
}
#endif  /* SHTPS_SEND_SHTERM_EVENT_ENABLE */

/* -----------------------------------------------------------------------------------
 */
#if defined(SHTPS_PROXIMITY_SUPPORT_ENABLE)
int shtps_proximity_state_check(struct shtps_fts *ts)
{
	int state = -1;
	int data = -1;

	SHTPS_LOG_FUNC_CALL();

	SHTPS_LOG_DBG_PRINT("[proximity] check state start\n");
	PROX_stateread_func(&state, &data);
	SHTPS_LOG_DBG_PRINT("[proximity] check state end(state:%d, data:%d)\n",state, data);

	if (state == SHTPS_PROXIMITY_ENABLE &&
		data == SHTPS_PROXIMITY_NEAR) {
		return 1;
	} else {
		return 0;
	}
}

int shtps_proximity_check(struct shtps_fts *ts)
{
	int data = -1;

	SHTPS_LOG_FUNC_CALL();

	SHTPS_LOG_DBG_PRINT("[proximity] check start\n");
	PROX_dataread_func(&data);
	SHTPS_LOG_DBG_PRINT("[proximity] check end(data:%d)\n",data);

	return data;
}

void shtps_lpwg_proximity_check_delayed_work_function(struct work_struct *work)
{
	struct delayed_work *dw = container_of(work, struct delayed_work, work);
	struct shtps_fts *ts = container_of(dw, struct shtps_fts, proximity_check_delayed_work);

	SHTPS_LOG_FUNC_CALL();
	shtps_func_request_async(ts, SHTPS_FUNC_REQ_EVEMT_PROXIMITY_CHECK);
}

void shtps_lpwg_proximity_check_cancel(struct shtps_fts *ts)
{
	SHTPS_LOG_FUNC_CALL();

	cancel_delayed_work(&ts->proximity_check_delayed_work);
}

void shtps_lpwg_proximity_check_start(struct shtps_fts *ts)
{
	SHTPS_LOG_FUNC_CALL();

	shtps_lpwg_proximity_check_cancel(ts);
	schedule_delayed_work(&ts->proximity_check_delayed_work, msecs_to_jiffies(SHTPS_LPWG_PROXIMITY_CHECK_PREWAIT));

	SHTPS_LOG_DBG_PRINT("[LPWG] notify interval start\n");
}

static void shtps_func_proximity_check(struct shtps_fts *ts)
{
	int proximity_data = -1;

	wake_lock(&ts->wake_lock_proximity);
    pm_qos_update_request(&ts->pm_qos_lock_idle_proximity, SHTPS_QOS_LATENCY_DEF_VALUE);

	proximity_data = shtps_proximity_check(ts);

	#if defined(SHTPS_LPWG_MODE_ENABLE)
	{
		unsigned long time = 0;

		ts->lpwg_proximity_get_data = proximity_data;
		if(ts->lpwg_proximity_get_data == SHTPS_PROXIMITY_NEAR){
			SHTPS_LOG_DBG_PRINT("[LPWG][Func] proximity near");

			shtps_mutex_lock_ctrl();
			if(ts->lpwg.is_notified != 0){
				if(time_after(jiffies, ts->lpwg.notify_time + msecs_to_jiffies(SHTPS_LPWG_MIN_NOTIFY_CANCEL_INTERVAL))){
					time = 0;
				}else{
					time = jiffies_to_msecs(((ts->lpwg.notify_time + msecs_to_jiffies(SHTPS_LPWG_MIN_NOTIFY_CANCEL_INTERVAL)) - jiffies)
												% SHTPS_LPWG_MIN_NOTIFY_CANCEL_INTERVAL);
				}
			}
			shtps_mutex_unlock_ctrl();

			if(time > 0){
				SHTPS_LOG_DBG_PRINT("[LPWG] cancel notify wait <%lums>\n", time);
				msleep(time);
			}

			shtps_mutex_lock_ctrl();
			if(ts->lpwg.is_notified != 0){
				shtps_notify_cancel_wakeup_event(ts);
				ts->lpwg.is_notified = 0;
			}
			shtps_mutex_unlock_ctrl();
		}
	}
	#endif /* SHTPS_LPWG_MODE_ENABLE */

    pm_qos_update_request(&ts->pm_qos_lock_idle_proximity, PM_QOS_DEFAULT_VALUE);
	wake_unlock(&ts->wake_lock_proximity);
}
#endif /* SHTPS_PROXIMITY_SUPPORT_ENABLE */

/* -----------------------------------------------------------------------------------
 */
#if defined(SHTPS_CPU_CLOCK_CONTROL_ENABLE)
static DEFINE_PER_CPU(unsigned int, perflock_freq) = 0;


static void shtps_mutex_lock_cpu_clock_ctrl(void)
{
	TPS_MUTEX_LOG_LOCK("shtps_cpu_clock_ctrl_lock");
	mutex_lock(&shtps_cpu_clock_ctrl_lock);
}

static void shtps_mutex_unlock_cpu_clock_ctrl(void)
{
	TPS_MUTEX_LOG_UNLOCK("shtps_cpu_clock_ctrl_lock");
	mutex_unlock(&shtps_cpu_clock_ctrl_lock);
}

static void shtps_perflock_cluster0_enable(struct shtps_fts *ts)
{
	int cpu;
	int req = 1;
	int enable_cpu_core_num = 0;

	shtps_mutex_lock_cpu_clock_ctrl();

	#if defined(SHTPS_DEF_CTRL_CPU_CLOCK_LINKED_LCD_BRIGHT_ENABLE)
		if(SHTPS_CTRL_CPU_CLOCK_FOR_LOW_BRIGHTNESS_ENABLE == 0){
			if( ((ts->lpmode_req_state | ts->lpmode_continuous_req_state) & SHTPS_LPMODE_REQ_LCD_BRIGHT) != 0 ){
				SHTPS_PERF_LOCK_PRINT("perf_lock <cluster0> not start because lcd bright is low.\n");
				req = 0;
			}
		}
	#endif /* SHTPS_DEF_CTRL_CPU_CLOCK_LINKED_LCD_BRIGHT_ENABLE */

	#if defined(SHTPS_DEF_CTRL_CPU_CLOCK_LINKED_ECO_ENABLE)
		if(SHTPS_CTRL_CPU_CLOCK_FOR_ECO_ENABLE == 0){
			if( ((ts->lpmode_req_state | ts->lpmode_continuous_req_state) & SHTPS_LPMODE_REQ_ECO) != 0 ){
				SHTPS_PERF_LOCK_PRINT("perf_lock <cluster0> not start because eco mode is on.\n");
				req = 0;
			}
		}
	#endif /* SHTPS_DEF_CTRL_CPU_CLOCK_LINKED_ECO_ENABLE */

	if(req == 1){
		ts->cpu_clock_ctrl_p->perf_lock_cluster0_state = 1;
		for_each_possible_cpu(cpu) {
			if(enable_cpu_core_num < SHTPS_PERF_LOCK_CLUSTER0_ENABLE_CORE_NUM){
				if (cpu < SHTPS_PERF_LOCK_CLUSTER1_TOP_CPU_ID) {
					#if defined(SHTPS_DEF_CTRL_CPU_CLOCK_LINKED_LCD_BRIGHT_ENABLE)
						if( ((ts->lpmode_req_state | ts->lpmode_continuous_req_state) & SHTPS_LPMODE_REQ_LCD_BRIGHT) == 0 ){
							per_cpu(perflock_freq, cpu) = SHTPS_PERF_LOCK_CLUSTER0_CLOCK_FREQUENCY;
							SHTPS_PERF_LOCK_PRINT("perf_lock <cluster0> set [cpu=%d, freq=%d]\n",
													cpu, SHTPS_PERF_LOCK_CLUSTER0_CLOCK_FREQUENCY);
						}else{
							per_cpu(perflock_freq, cpu) = SHTPS_PERF_LOCK_CLOCK_FREQUENCY_LCD_LPM;
							SHTPS_PERF_LOCK_PRINT("perf_lock <cluster0> set [cpu=%d, freq=%d]\n",
													cpu, SHTPS_PERF_LOCK_CLOCK_FREQUENCY_LCD_LPM);
						}
					#else
						per_cpu(perflock_freq, cpu) = SHTPS_PERF_LOCK_CLUSTER0_CLOCK_FREQUENCY;
						SHTPS_PERF_LOCK_PRINT("perf_lock <cluster0> set [cpu=%d, freq=%d]\n",
													cpu, SHTPS_PERF_LOCK_CLUSTER0_CLOCK_FREQUENCY);
					#endif /* SHTPS_DEF_CTRL_CPU_CLOCK_LINKED_LCD_BRIGHT_ENABLE */

					enable_cpu_core_num++;
				}
			}
		}
		sh_cpufreq_update_policy_try();
		SHTPS_PERF_LOCK_PRINT("perf_lock <cluster0> start\n");
	}

	if(SHTPS_PERF_LOCK_QOS_LATENCY_ENABLE != 0){
		if(ts->cpu_clock_ctrl_p->qos_cpu_cluster0_latency_state == 0){
			pm_qos_update_request(&ts->cpu_clock_ctrl_p->qos_cpu_cluster0_latency, SHTPS_PERF_LOCK_QOS_LATENCY_DEF_VALUE);
			ts->cpu_clock_ctrl_p->qos_cpu_cluster0_latency_state = 1;
			SHTPS_PERF_LOCK_PRINT("<cluster0> prohibit Standalone Powercollapse start\n");
		}
	}

	shtps_mutex_unlock_cpu_clock_ctrl();
}

static void shtps_perflock_cluster0_disable(struct shtps_fts *ts)
{
	int cpu;
	shtps_mutex_lock_cpu_clock_ctrl();

	if(ts->cpu_clock_ctrl_p->perf_lock_cluster0_state != 0){
		for_each_possible_cpu(cpu) {
			if (cpu < SHTPS_PERF_LOCK_CLUSTER1_TOP_CPU_ID) {
				per_cpu(perflock_freq, cpu) = 0;
			}
		}
		sh_cpufreq_update_policy_try();
		ts->cpu_clock_ctrl_p->perf_lock_cluster0_state = 0;
		SHTPS_PERF_LOCK_PRINT("perf_lock <cluster0> end\n");
	}

	if(ts->cpu_clock_ctrl_p->qos_cpu_cluster0_latency_state == 1){
		pm_qos_update_request(&ts->cpu_clock_ctrl_p->qos_cpu_cluster0_latency, PM_QOS_DEFAULT_VALUE);
		ts->cpu_clock_ctrl_p->qos_cpu_cluster0_latency_state = 0;
		SHTPS_PERF_LOCK_PRINT("<cluster0> prohibit Standalone Powercollapse end\n");
	}

	shtps_mutex_unlock_cpu_clock_ctrl();
}

static void shtps_perflock_cluster1_enable(struct shtps_fts *ts)
{
	int cpu;
	int req = 1;
	int enable_cpu_core_num = 0;

	shtps_mutex_lock_cpu_clock_ctrl();

	#if defined(SHTPS_DEF_CTRL_CPU_CLOCK_LINKED_LCD_BRIGHT_ENABLE)
		if(SHTPS_CTRL_CPU_CLOCK_FOR_LOW_BRIGHTNESS_ENABLE == 0){
			if( ((ts->lpmode_req_state | ts->lpmode_continuous_req_state) & SHTPS_LPMODE_REQ_LCD_BRIGHT) != 0 ){
				SHTPS_PERF_LOCK_PRINT("perf_lock <cluster1> not start because lcd bright is low.\n");
				req = 0;
			}
		}
	#endif /* SHTPS_DEF_CTRL_CPU_CLOCK_LINKED_LCD_BRIGHT_ENABLE */

	#if defined(SHTPS_DEF_CTRL_CPU_CLOCK_LINKED_ECO_ENABLE)
		if(SHTPS_CTRL_CPU_CLOCK_FOR_ECO_ENABLE == 0){
			if( ((ts->lpmode_req_state | ts->lpmode_continuous_req_state) & SHTPS_LPMODE_REQ_ECO) != 0 ){
				SHTPS_PERF_LOCK_PRINT("perf_lock <cluster1> not start because eco mode is on.\n");
				req = 0;
			}
		}
	#endif /* SHTPS_DEF_CTRL_CPU_CLOCK_LINKED_ECO_ENABLE */

	if(req == 1){
		ts->cpu_clock_ctrl_p->perf_lock_cluster1_state = 1;
		for_each_possible_cpu(cpu) {
			if(enable_cpu_core_num < SHTPS_PERF_LOCK_CLUSTER1_ENABLE_CORE_NUM){
				if (cpu >= SHTPS_PERF_LOCK_CLUSTER1_TOP_CPU_ID) {
					#if defined(SHTPS_DEF_CTRL_CPU_CLOCK_LINKED_LCD_BRIGHT_ENABLE)
						if( ((ts->lpmode_req_state | ts->lpmode_continuous_req_state) & SHTPS_LPMODE_REQ_LCD_BRIGHT) == 0 ){
							per_cpu(perflock_freq, cpu) = SHTPS_PERF_LOCK_CLUSTER1_CLOCK_FREQUENCY;
							SHTPS_PERF_LOCK_PRINT("perf_lock <cluster1> set [cpu=%d, freq=%d]\n",
													cpu, SHTPS_PERF_LOCK_CLUSTER1_CLOCK_FREQUENCY);
						}else{
							per_cpu(perflock_freq, cpu) = SHTPS_PERF_LOCK_CLOCK_FREQUENCY_LCD_LPM;
							SHTPS_PERF_LOCK_PRINT("perf_lock <cluster1> set [cpu=%d, freq=%d]\n",
													cpu, SHTPS_PERF_LOCK_CLOCK_FREQUENCY_LCD_LPM);
						}
					#else
						per_cpu(perflock_freq, cpu) = SHTPS_PERF_LOCK_CLUSTER1_CLOCK_FREQUENCY;
						SHTPS_PERF_LOCK_PRINT("perf_lock <cluster1> set [cpu=%d, freq=%d]\n",
													cpu, SHTPS_PERF_LOCK_CLUSTER1_CLOCK_FREQUENCY);
					#endif /* SHTPS_DEF_CTRL_CPU_CLOCK_LINKED_LCD_BRIGHT_ENABLE */

					enable_cpu_core_num++;
				}
			}
		}
		sh_cpufreq_update_policy_try();
		SHTPS_PERF_LOCK_PRINT("perf_lock <cluster1> start\n");
	}

	if(SHTPS_PERF_LOCK_QOS_LATENCY_ENABLE != 0){
		if(ts->cpu_clock_ctrl_p->qos_cpu_cluster1_latency_state == 0){
			pm_qos_update_request(&ts->cpu_clock_ctrl_p->qos_cpu_cluster1_latency, SHTPS_PERF_LOCK_QOS_LATENCY_DEF_VALUE);
			ts->cpu_clock_ctrl_p->qos_cpu_cluster1_latency_state = 1;
			SHTPS_PERF_LOCK_PRINT("<cluster1> prohibit Standalone Powercollapse start\n");
		}
	}

	shtps_mutex_unlock_cpu_clock_ctrl();
}

static void shtps_perflock_cluster1_disable(struct shtps_fts *ts)
{
	int cpu;
	shtps_mutex_lock_cpu_clock_ctrl();

	if(ts->cpu_clock_ctrl_p->perf_lock_cluster1_state != 0){
		for_each_possible_cpu(cpu) {
			if (cpu >= SHTPS_PERF_LOCK_CLUSTER1_TOP_CPU_ID) {
				per_cpu(perflock_freq, cpu) = 0;
			}
		}
		sh_cpufreq_update_policy_try();
		ts->cpu_clock_ctrl_p->perf_lock_cluster1_state = 0;
		SHTPS_PERF_LOCK_PRINT("perf_lock <cluster1> end\n");
	}

	if(ts->cpu_clock_ctrl_p->qos_cpu_cluster1_latency_state == 1){
		pm_qos_update_request(&ts->cpu_clock_ctrl_p->qos_cpu_cluster1_latency, PM_QOS_DEFAULT_VALUE);
		ts->cpu_clock_ctrl_p->qos_cpu_cluster1_latency_state = 0;
		SHTPS_PERF_LOCK_PRINT("<cluster1> prohibit Standalone Powercollapse end\n");
	}

	shtps_mutex_unlock_cpu_clock_ctrl();
}

static int shtps_perflock_ctrl_callback(struct notifier_block *nb, unsigned long event, void *data)
{
	struct cpufreq_policy *policy = data;
	switch (event) {
	case CPUFREQ_ADJUST:
//		SHTPS_PERF_LOCK_PRINT("set perflock [%d]\n", perflock_freq);
		cpufreq_verify_within_limits(policy, per_cpu(perflock_freq, policy->cpu), UINT_MAX);
		break;
	default:
		break;
	}
	return NOTIFY_OK;
}

static int shtps_perflock_cluster0_disable_timer_start(struct shtps_fts *ts, unsigned long delay_ms)
{
	cancel_delayed_work(&ts->cpu_clock_ctrl_p->perf_lock_cluster0_disable_delayed_work);
	schedule_delayed_work(&ts->cpu_clock_ctrl_p->perf_lock_cluster0_disable_delayed_work, msecs_to_jiffies(delay_ms));

	return 0;
}

static void shtps_perflock_cluster0_disable_delayed_work_function(struct work_struct *work)
{
	struct delayed_work *dw = container_of(work, struct delayed_work, work);
	struct shtps_cpu_clock_ctrl_info *cpuctrl = container_of(dw, struct shtps_cpu_clock_ctrl_info, perf_lock_cluster0_disable_delayed_work);
	//struct shtps_fts *ts = container_of(cpuctrl, struct shtps_fts, cpu_clock_ctrl_p);
	struct shtps_fts *ts = cpuctrl->ts_p;

	SHTPS_LOG_FUNC_CALL();

	shtps_perflock_cluster0_disable(ts);
	SHTPS_PERF_LOCK_PRINT("shtps_perflock_disable_timer end by Timer\n");

	return;
}

static int shtps_perflock_cluster1_disable_timer_start(struct shtps_fts *ts, unsigned long delay_ms)
{
	cancel_delayed_work(&ts->cpu_clock_ctrl_p->perf_lock_cluster1_disable_delayed_work);
	schedule_delayed_work(&ts->cpu_clock_ctrl_p->perf_lock_cluster1_disable_delayed_work, msecs_to_jiffies(delay_ms));

	return 0;
}

static void shtps_perflock_cluster1_disable_delayed_work_function(struct work_struct *work)
{
	struct delayed_work *dw = container_of(work, struct delayed_work, work);
	struct shtps_cpu_clock_ctrl_info *cpuctrl = container_of(dw, struct shtps_cpu_clock_ctrl_info, perf_lock_cluster1_disable_delayed_work);
	//struct shtps_fts *ts = container_of(cpuctrl, struct shtps_fts, cpu_clock_ctrl_p);
	struct shtps_fts *ts = cpuctrl->ts_p;

	SHTPS_LOG_FUNC_CALL();

	shtps_perflock_cluster1_disable(ts);
	SHTPS_PERF_LOCK_PRINT("shtps_perflock_disable_timer end by Timer\n");

	return;
}

static struct notifier_block shtps_perflock_ctrl_notifier = { 
	.notifier_call = shtps_perflock_ctrl_callback,
};

void shtps_perflock_register_notifier(void)
{
	if( cpufreq_register_notifier(&shtps_perflock_ctrl_notifier, CPUFREQ_POLICY_NOTIFIER) ){
		pr_err("%s: cannot register cpufreq notifier\n", __func__);
	}
}

void shtps_perflock_unregister_notifier(void)
{
	if( cpufreq_unregister_notifier(&shtps_perflock_ctrl_notifier, CPUFREQ_POLICY_NOTIFIER) ){
		pr_err("%s: cannot unregister cpufreq notifier\n", __func__);
	}
}

static void shtps_perflock_cluster0_enable_check(struct shtps_fts *ts, u8 event, struct shtps_touch_info *info)
{
	int fingerMax = shtps_get_fingermax(ts);
	int i;
	int perf_lock_enable_time;
	int x_diff;
	int y_diff;
	int xy_diff;
	int max_diff;

	if(SHTPS_PERF_LOCK_CLUSTER0_ENABLE == 0){
		return;
	}

	if(SHTPS_PERF_LOCK_ENABLE_ALLEVENTS == 1
		#if defined(SHTPS_DEF_CTRL_CPU_CLOCK_LINKED_LCD_BRIGHT_ENABLE)
			&& ( (SHTPS_CTRL_CPU_CLOCK_FOR_LOW_BRIGHTNESS_ENABLE == 0) ||
				 (((ts->lpmode_req_state | ts->lpmode_continuous_req_state) & SHTPS_LPMODE_REQ_LCD_BRIGHT) == 0) )
		#endif /* SHTPS_DEF_CTRL_CPU_CLOCK_LINKED_LCD_BRIGHT_ENABLE */
		){

		perf_lock_enable_time = SHTPS_PERF_LOCK_CLUSTER0_ENABLE_TIME_MS;

		if(event == SHTPS_EVENT_TU){
			max_diff = 0;

			for(i = 0; i < fingerMax; i++){
				if(ts->cpu_clock_ctrl_p->fingers_hist[0][i].state != SHTPS_TOUCH_STATE_NO_TOUCH){
					if(ts->cpu_clock_ctrl_p->fingers_hist[1][i].state == SHTPS_TOUCH_STATE_NO_TOUCH){
						x_diff = 0;
						y_diff = 0;
					}
					else{
						x_diff = SHTPS_ABS_CALC(ts->cpu_clock_ctrl_p->fingers_hist[1][i].x, ts->cpu_clock_ctrl_p->fingers_hist[0][i].x);
						y_diff = SHTPS_ABS_CALC(ts->cpu_clock_ctrl_p->fingers_hist[1][i].y, ts->cpu_clock_ctrl_p->fingers_hist[0][i].y);
					}
					xy_diff = (x_diff > y_diff ? x_diff : y_diff);
					if(xy_diff > max_diff){
						max_diff = xy_diff;
					}
				}
			}

			if(max_diff >= SHTPS_PERF_LOCK_ENABLE_FLICK_DIST){
				perf_lock_enable_time = SHTPS_PERF_LOCK_CLUSTER0_ENABLE_TIME_MS_FOR_FLICK;
			}
		}

		if( shtps_fwctl_get_dev_state(ts) == SHTPS_DEV_STATE_ACTIVE
		#if !defined(SHTPS_PERFLOCK_DOZE_DISABLE)
			|| shtps_fwctl_get_dev_state(ts) == SHTPS_DEV_STATE_DOZE
		#endif /*!defined(SHTPS_PERFLOCK_DOZE_DISABLE)*/
		 ){
			shtps_perflock_cluster0_enable(ts);
			shtps_perflock_cluster0_disable_timer_start(ts, perf_lock_enable_time);
			SHTPS_PERF_LOCK_PRINT("shtps_perflock_disable_timer start (%dms) by %s\n",
								perf_lock_enable_time,
								( (event == SHTPS_EVENT_TD) ? "TouchDown" :
								  (event == SHTPS_EVENT_DRAG) ? "Drag" :
								  (event == SHTPS_EVENT_TU) ? "TouchUp" :
								  (event == SHTPS_EVENT_MTDU) ? "MultiTouchUpDown" : "unknown" )
								);
		}
	}
	else{
		if(event == SHTPS_EVENT_TD){
			
			if( shtps_fwctl_get_dev_state(ts) == SHTPS_DEV_STATE_ACTIVE
			#if !defined(SHTPS_PERFLOCK_DOZE_DISABLE)
				|| shtps_fwctl_get_dev_state(ts) == SHTPS_DEV_STATE_DOZE
			#endif /*!defined(SHTPS_PERFLOCK_DOZE_DISABLE)*/
			 ){
				shtps_perflock_cluster0_enable(ts);
				shtps_perflock_cluster0_disable_timer_start(ts, SHTPS_PERF_LOCK_CLUSTER0_ENABLE_TIME_MS);
				SHTPS_PERF_LOCK_PRINT("shtps_perflock_disable_timer start by TouchDown\n");
			}

		}else if(event == SHTPS_EVENT_DRAG){
			if(ts->cpu_clock_ctrl_p->report_event == SHTPS_EVENT_TD){
				
				if( shtps_fwctl_get_dev_state(ts) == SHTPS_DEV_STATE_ACTIVE
				#if !defined(SHTPS_PERFLOCK_DOZE_DISABLE)
					|| shtps_fwctl_get_dev_state(ts) == SHTPS_DEV_STATE_DOZE
				#endif /*!defined(SHTPS_PERFLOCK_DOZE_DISABLE)*/
				 ){
					shtps_perflock_cluster0_enable(ts);
					shtps_perflock_cluster0_disable_timer_start(ts, SHTPS_PERF_LOCK_CLUSTER0_ENABLE_TIME_MS);
					SHTPS_PERF_LOCK_PRINT("shtps_perflock_disable_timer start by Drag\n");
				}
			}
		}
		#if !defined( SHTPS_PERFLOCK_TU_RELEASE_DISABLE )
		else if(event == SHTPS_EVENT_TU){
			shtps_perflock_cluster0_disable(ts);
			SHTPS_PERF_LOCK_PRINT("perf_lock end by TouchUp\n");
		}
		#endif /* SHTPS_PERFLOCK_TU_RELEASE_DISABLE */
	}
}

static void shtps_perflock_cluster1_enable_check(struct shtps_fts *ts, u8 event, struct shtps_touch_info *info)
{
	int fingerMax = shtps_get_fingermax(ts);
	int i;
	int perf_lock_enable_time;
	int x_diff;
	int y_diff;
	int xy_diff;
	int max_diff;

	if(SHTPS_PERF_LOCK_CLUSTER1_ENABLE == 0){
		return;
	}

	if(SHTPS_PERF_LOCK_ENABLE_ALLEVENTS == 1
		#if defined(SHTPS_DEF_CTRL_CPU_CLOCK_LINKED_LCD_BRIGHT_ENABLE)
			&& ( (SHTPS_CTRL_CPU_CLOCK_FOR_LOW_BRIGHTNESS_ENABLE == 0) ||
				 (((ts->lpmode_req_state | ts->lpmode_continuous_req_state) & SHTPS_LPMODE_REQ_LCD_BRIGHT) == 0) )
		#endif /* SHTPS_DEF_CTRL_CPU_CLOCK_LINKED_LCD_BRIGHT_ENABLE */
		){

		perf_lock_enable_time = SHTPS_PERF_LOCK_CLUSTER1_ENABLE_TIME_MS;

		if(event == SHTPS_EVENT_TU){
			max_diff = 0;

			for(i = 0; i < fingerMax; i++){
				if(ts->cpu_clock_ctrl_p->fingers_hist[0][i].state != SHTPS_TOUCH_STATE_NO_TOUCH){
					if(ts->cpu_clock_ctrl_p->fingers_hist[1][i].state == SHTPS_TOUCH_STATE_NO_TOUCH){
						x_diff = 0;
						y_diff = 0;
					}
					else{
						x_diff = SHTPS_ABS_CALC(ts->cpu_clock_ctrl_p->fingers_hist[1][i].x, ts->cpu_clock_ctrl_p->fingers_hist[0][i].x);
						y_diff = SHTPS_ABS_CALC(ts->cpu_clock_ctrl_p->fingers_hist[1][i].y, ts->cpu_clock_ctrl_p->fingers_hist[0][i].y);
					}
					xy_diff = (x_diff > y_diff ? x_diff : y_diff);
					if(xy_diff > max_diff){
						max_diff = xy_diff;
					}
				}
			}

			if(max_diff >= SHTPS_PERF_LOCK_ENABLE_FLICK_DIST){
				perf_lock_enable_time = SHTPS_PERF_LOCK_CLUSTER1_ENABLE_TIME_MS_FOR_FLICK;
			}
		}

		if( shtps_fwctl_get_dev_state(ts) == SHTPS_DEV_STATE_ACTIVE
		#if !defined(SHTPS_PERFLOCK_DOZE_DISABLE)
			|| shtps_fwctl_get_dev_state(ts) == SHTPS_DEV_STATE_DOZE
		#endif /*!defined(SHTPS_PERFLOCK_DOZE_DISABLE)*/
		 ){
			shtps_perflock_cluster1_enable(ts);
			shtps_perflock_cluster1_disable_timer_start(ts, perf_lock_enable_time);
			SHTPS_PERF_LOCK_PRINT("shtps_perflock_disable_timer start (%dms) by %s\n",
								perf_lock_enable_time,
								( (event == SHTPS_EVENT_TD) ? "TouchDown" :
								  (event == SHTPS_EVENT_DRAG) ? "Drag" :
								  (event == SHTPS_EVENT_TU) ? "TouchUp" :
								  (event == SHTPS_EVENT_MTDU) ? "MultiTouchUpDown" : "unknown" )
								);
		}
	}
	else{
		if(event == SHTPS_EVENT_TD){
			
			if( shtps_fwctl_get_dev_state(ts) == SHTPS_DEV_STATE_ACTIVE
			#if !defined(SHTPS_PERFLOCK_DOZE_DISABLE)
				|| shtps_fwctl_get_dev_state(ts) == SHTPS_DEV_STATE_DOZE
			#endif /*!defined(SHTPS_PERFLOCK_DOZE_DISABLE)*/
			 ){
				shtps_perflock_cluster1_enable(ts);
				shtps_perflock_cluster1_disable_timer_start(ts, SHTPS_PERF_LOCK_CLUSTER1_ENABLE_TIME_MS);
				SHTPS_PERF_LOCK_PRINT("shtps_perflock_disable_timer start by TouchDown\n");
			}

		}else if(event == SHTPS_EVENT_DRAG){
			if(ts->cpu_clock_ctrl_p->report_event == SHTPS_EVENT_TD){
				
				if( shtps_fwctl_get_dev_state(ts) == SHTPS_DEV_STATE_ACTIVE
				#if !defined(SHTPS_PERFLOCK_DOZE_DISABLE)
					|| shtps_fwctl_get_dev_state(ts) == SHTPS_DEV_STATE_DOZE
				#endif /*!defined(SHTPS_PERFLOCK_DOZE_DISABLE)*/
				 ){
					shtps_perflock_cluster1_enable(ts);
					shtps_perflock_cluster1_disable_timer_start(ts, SHTPS_PERF_LOCK_CLUSTER1_ENABLE_TIME_MS);
					SHTPS_PERF_LOCK_PRINT("shtps_perflock_disable_timer start by Drag\n");
				}
			}
		}
		#if !defined( SHTPS_PERFLOCK_TU_RELEASE_DISABLE )
		else if(event == SHTPS_EVENT_TU){
			shtps_perflock_cluster1_disable(ts);
			SHTPS_PERF_LOCK_PRINT("perf_lock end by TouchUp\n");
		}
		#endif /* SHTPS_PERFLOCK_TU_RELEASE_DISABLE */
	}
}

void shtps_perflock_enable_start(struct shtps_fts *ts, u8 event, struct shtps_touch_info *info)
{
	shtps_perflock_cluster0_enable_check(ts, event, info);
	shtps_perflock_cluster1_enable_check(ts, event, info);

	if(event == SHTPS_EVENT_TU){
		memset(ts->cpu_clock_ctrl_p->fingers_hist, 0, sizeof(ts->cpu_clock_ctrl_p->fingers_hist));
	}else{
		memcpy(ts->cpu_clock_ctrl_p->fingers_hist[1], ts->cpu_clock_ctrl_p->fingers_hist[0], sizeof(ts->cpu_clock_ctrl_p->fingers_hist[0]));
		memcpy(ts->cpu_clock_ctrl_p->fingers_hist[0], info->fingers, sizeof(info->fingers));
	}
}

void shtps_perflock_sleep(struct shtps_fts *ts)
{
	shtps_perflock_set_event(ts, SHTPS_EVENT_TU);
	shtps_perflock_cluster0_disable(ts);
	shtps_perflock_cluster1_disable(ts);
	SHTPS_PERF_LOCK_PRINT("perf_lock end by ForceTouchUp\n");

	memset(ts->cpu_clock_ctrl_p->fingers_hist, 0, sizeof(ts->cpu_clock_ctrl_p->fingers_hist));
}

void shtps_perflock_init(struct shtps_fts *ts)
{
	ts->cpu_clock_ctrl_p = kzalloc(sizeof(struct shtps_cpu_clock_ctrl_info), GFP_KERNEL);
	if(ts->cpu_clock_ctrl_p == NULL){
		PR_ERROR("memory allocation error:%s()\n", __func__);
		return;
	}

	shtps_perflock_set_event(ts, SHTPS_EVENT_TU);
	INIT_DELAYED_WORK(&ts->cpu_clock_ctrl_p->perf_lock_cluster0_disable_delayed_work, shtps_perflock_cluster0_disable_delayed_work_function);
	INIT_DELAYED_WORK(&ts->cpu_clock_ctrl_p->perf_lock_cluster1_disable_delayed_work, shtps_perflock_cluster1_disable_delayed_work_function);
	ts->cpu_clock_ctrl_p->qos_cpu_cluster0_latency.type = PM_QOS_REQ_ALL_CORES;
	ts->cpu_clock_ctrl_p->qos_cpu_cluster1_latency.type = PM_QOS_REQ_ALL_CORES;
	pm_qos_add_request(&ts->cpu_clock_ctrl_p->qos_cpu_cluster0_latency, PM_QOS_CPU_DMA_LATENCY, PM_QOS_DEFAULT_VALUE);
	pm_qos_add_request(&ts->cpu_clock_ctrl_p->qos_cpu_cluster1_latency, PM_QOS_CPU_DMA_LATENCY, PM_QOS_DEFAULT_VALUE);
	ts->cpu_clock_ctrl_p->ts_p = ts;
}

void shtps_perflock_deinit(struct shtps_fts *ts)
{
	pm_qos_remove_request(&ts->cpu_clock_ctrl_p->qos_cpu_cluster0_latency);
	pm_qos_remove_request(&ts->cpu_clock_ctrl_p->qos_cpu_cluster1_latency);

	if(ts->cpu_clock_ctrl_p)	kfree(ts->cpu_clock_ctrl_p);
	ts->cpu_clock_ctrl_p = NULL;

}

void shtps_perflock_set_event(struct shtps_fts *ts, u8 event)
{
	ts->cpu_clock_ctrl_p->report_event = event;
}
#endif /* SHTPS_CPU_CLOCK_CONTROL_ENABLE */

/* -----------------------------------------------------------------------------------
 */
#if defined(SHTPS_CPU_IDLE_SLEEP_CONTROL_ENABLE)
static void shtps_mutex_lock_cpu_idle_sleep_ctrl(void)
{
	TPS_MUTEX_LOG_LOCK("shtps_cpu_idle_sleep_ctrl_lock");
	mutex_lock(&shtps_cpu_idle_sleep_ctrl_lock);
}

static void shtps_mutex_unlock_cpu_idle_sleep_ctrl(void)
{
	TPS_MUTEX_LOG_UNLOCK("shtps_cpu_idle_sleep_ctrl_lock");
	mutex_unlock(&shtps_cpu_idle_sleep_ctrl_lock);
}

static void shtps_wake_lock_idle_l(struct shtps_fts *ts)
{
	SHTPS_LOG_FUNC_CALL();

	shtps_mutex_lock_cpu_idle_sleep_ctrl();
	if(ts->cpu_idle_sleep_ctrl_p->wake_lock_idle_state == 0){
		SHTPS_LOG_DBG_PRINT("wake_lock_idle on\n");
		pm_qos_update_request(&ts->cpu_idle_sleep_ctrl_p->qos_cpu_latency, SHTPS_QOS_LATENCY_DEF_VALUE);
		ts->cpu_idle_sleep_ctrl_p->wake_lock_idle_state = 1;
	}
	shtps_mutex_unlock_cpu_idle_sleep_ctrl();
}

static void shtps_wake_unlock_idle_l(struct shtps_fts *ts)
{
	SHTPS_LOG_FUNC_CALL();

	shtps_mutex_lock_cpu_idle_sleep_ctrl();
	if(ts->cpu_idle_sleep_ctrl_p->wake_lock_idle_state != 0){
		SHTPS_LOG_DBG_PRINT("wake_lock_idle off\n");
		pm_qos_update_request(&ts->cpu_idle_sleep_ctrl_p->qos_cpu_latency, PM_QOS_DEFAULT_VALUE);
		ts->cpu_idle_sleep_ctrl_p->wake_lock_idle_state = 0;
	}
	shtps_mutex_unlock_cpu_idle_sleep_ctrl();
}

void shtps_wake_lock_idle(struct shtps_fts *ts)
{
	if(SHTPS_STATE_ACTIVE == ts->state_mgr.state){
		shtps_wake_lock_idle_l(ts);
	}
}

void shtps_wake_unlock_idle(struct shtps_fts *ts)
{
	shtps_wake_unlock_idle_l(ts);
}

void shtps_cpu_idle_sleep_wake_lock_init( struct shtps_fts *ts )
{
	ts->cpu_idle_sleep_ctrl_p = kzalloc(sizeof(struct shtps_cpu_idle_sleep_ctrl_info), GFP_KERNEL);
	if(ts->cpu_idle_sleep_ctrl_p == NULL){
		PR_ERROR("memory allocation error:%s()\n", __func__);
		return;
	}
	// ts->cpu_idle_sleep_ctrl_p->wake_lock_idle_state = 0;	// no need
	ts->cpu_idle_sleep_ctrl_p->qos_cpu_latency.type = PM_QOS_REQ_ALL_CORES;
	pm_qos_add_request(&ts->cpu_idle_sleep_ctrl_p->qos_cpu_latency, PM_QOS_CPU_DMA_LATENCY, PM_QOS_DEFAULT_VALUE);
}

void shtps_cpu_idle_sleep_wake_lock_deinit( struct shtps_fts *ts )
{
	pm_qos_remove_request(&ts->cpu_idle_sleep_ctrl_p->qos_cpu_latency);

	if(ts->cpu_idle_sleep_ctrl_p)	kfree(ts->cpu_idle_sleep_ctrl_p);
	ts->cpu_idle_sleep_ctrl_p = NULL;
}

#endif /* SHTPS_CPU_IDLE_SLEEP_CONTROL_ENABLE */

/* -----------------------------------------------------------------------------------
 */
#if defined(SHTPS_CPU_SLEEP_CONTROL_FOR_FWUPDATE_ENABLE)
static void shtps_mutex_lock_cpu_sleep_ctrl(void)
{
	TPS_MUTEX_LOG_LOCK("shtps_cpu_sleep_ctrl_for_fwupdate_lock");
	mutex_lock(&shtps_cpu_sleep_ctrl_for_fwupdate_lock);
}

static void shtps_mutex_unlock_cpu_sleep_ctrl(void)
{
	TPS_MUTEX_LOG_UNLOCK("shtps_cpu_sleep_ctrl_for_fwupdate_lock");
	mutex_unlock(&shtps_cpu_sleep_ctrl_for_fwupdate_lock);
}

void shtps_wake_lock_for_fwupdate(struct shtps_fts *ts)
{
	SHTPS_LOG_FUNC_CALL();

	shtps_mutex_lock_cpu_sleep_ctrl();
	if(ts->cpu_sleep_ctrl_fwupdate_p->wake_lock_for_fwupdate_state == 0){
		SHTPS_LOG_DBG_PRINT("wake_lock_for_fwupdate on\n");
		wake_lock(&ts->cpu_sleep_ctrl_fwupdate_p->wake_lock_for_fwupdate);
		pm_qos_update_request(&ts->cpu_sleep_ctrl_fwupdate_p->qos_cpu_latency_for_fwupdate, SHTPS_QOS_LATENCY_DEF_VALUE);
		ts->cpu_sleep_ctrl_fwupdate_p->wake_lock_for_fwupdate_state = 1;
	}
	shtps_mutex_unlock_cpu_sleep_ctrl();
}

void shtps_wake_unlock_for_fwupdate(struct shtps_fts *ts)
{
	SHTPS_LOG_FUNC_CALL();

	shtps_mutex_lock_cpu_sleep_ctrl();
	if(ts->cpu_sleep_ctrl_fwupdate_p->wake_lock_for_fwupdate_state != 0){
		SHTPS_LOG_DBG_PRINT("wake_lock_for_fwupdate off\n");
		wake_unlock(&ts->cpu_sleep_ctrl_fwupdate_p->wake_lock_for_fwupdate);
		pm_qos_update_request(&ts->cpu_sleep_ctrl_fwupdate_p->qos_cpu_latency_for_fwupdate, PM_QOS_DEFAULT_VALUE);
		ts->cpu_sleep_ctrl_fwupdate_p->wake_lock_for_fwupdate_state = 0;
	}
	shtps_mutex_unlock_cpu_sleep_ctrl();
}

void shtps_fwupdate_wake_lock_init( struct shtps_fts *ts )
{
	ts->cpu_sleep_ctrl_fwupdate_p = kzalloc(sizeof(struct shtps_cpu_sleep_ctrl_fwupdate_info), GFP_KERNEL);
	if(ts->cpu_sleep_ctrl_fwupdate_p == NULL){
		PR_ERROR("memory allocation error:%s()\n", __func__);
		return;
	}

	//ts->cpu_sleep_ctrl_fwupdate_p->wake_lock_for_fwupdate_state = 0;	// no need
	wake_lock_init(&ts->cpu_sleep_ctrl_fwupdate_p->wake_lock_for_fwupdate, WAKE_LOCK_SUSPEND, "shtps_wake_lock_for_fwupdate");
	ts->cpu_sleep_ctrl_fwupdate_p->qos_cpu_latency_for_fwupdate.type = PM_QOS_REQ_ALL_CORES;
	pm_qos_add_request(&ts->cpu_sleep_ctrl_fwupdate_p->qos_cpu_latency_for_fwupdate, PM_QOS_CPU_DMA_LATENCY, PM_QOS_DEFAULT_VALUE);
}

void shtps_fwupdate_wake_lock_deinit( struct shtps_fts *ts )
{
	pm_qos_remove_request(&ts->cpu_sleep_ctrl_fwupdate_p->qos_cpu_latency_for_fwupdate);
	wake_lock_destroy(&ts->cpu_sleep_ctrl_fwupdate_p->wake_lock_for_fwupdate);

	if(ts->cpu_sleep_ctrl_fwupdate_p)	kfree(ts->cpu_sleep_ctrl_fwupdate_p);
	ts->cpu_sleep_ctrl_fwupdate_p = NULL;
}

#endif /* SHTPS_CPU_SLEEP_CONTROL_FOR_FWUPDATE_ENABLE */

/* -----------------------------------------------------------------------------------
 */
#if defined( SHTPS_CHECK_CRC_ERROR_ENABLE )
void shtps_func_check_crc_error(struct shtps_fts *ts)
{
	u8 buf;
	const unsigned char* fw_data = NULL;
	u8 update = 0;
	#if defined(SHTPS_PANEL_POW_CTL_FOR_FWUPDATE_ENABLE)
		int ret;
	#endif /* SHTPS_PANEL_POW_CTL_FOR_FWUPDATE_ENABLE */

	SHTPS_LOG_FUNC_CALL();
	if(shtps_tpin_enable_check(ts) != 0){
		return;
	}

	#if defined(SHTPS_PANEL_POW_CTL_FOR_FWUPDATE_ENABLE)
		ret = shdisp_api_panel_pow_ctl(SHDISP_PANEL_POWER_ON);
		if(ret != SHDISP_RESULT_SUCCESS){
			SHTPS_LOG_ERR_PRINT("%s() panel power on error.\n", __func__);
		}
	#endif /* SHTPS_PANEL_POW_CTL_FOR_FWUPDATE_ENABLE */

	if(shtps_start(ts) == 0){
		shtps_wait_startup(ts);
	}

	shtps_fwctl_get_device_status(ts, &buf);

	if(buf == SHTPS_REGVAL_STATUS_CRC_ERROR){
		SHTPS_LOG_ERR_PRINT("Touch panel CRC error detect\n");
		update = 1;
	}

	if(update != 0){
		fw_data = shtps_fwdata_builtin(ts);
		if(fw_data){
			int ret;
			int retry = 5;
			do{
				ret = shtps_fw_update(ts, fw_data, shtps_fwsize_builtin(ts));
				if (ret != 0) {
					shtps_reset(ts);
				}
			}while(ret != 0 && (retry-- > 0));
		}
	}

	if(ts->is_lcd_on == 0){
		shtps_shutdown(ts);
	}

	#if defined(SHTPS_PANEL_POW_CTL_FOR_FWUPDATE_ENABLE)
		ret = shdisp_api_panel_pow_ctl(SHDISP_PANEL_POWER_OFF);
		if(ret != SHDISP_RESULT_SUCCESS){
			SHTPS_LOG_ERR_PRINT("%s() panel power off error.\n", __func__);
		}
	#endif /* SHTPS_PANEL_POW_CTL_FOR_FWUPDATE_ENABLE */
}
#endif /* if defined( SHTPS_CHECK_CRC_ERROR_ENABLE ) */

/* -----------------------------------------------------------------------------------
 */
static void shtps_func_lcd_on(struct shtps_fts *ts)
{
	request_event(ts, SHTPS_EVENT_WAKEUP, 0);
}

static void shtps_func_lcd_off(struct shtps_fts *ts)
{
	request_event(ts, SHTPS_EVENT_SLEEP, 0);
}

/* -----------------------------------------------------------------------------------
 */
#if defined(SHTPS_DEF_LPWG_GRIP_PROC_ASYNC_ENABLE)
static void shtps_func_grip_on(struct shtps_fts *ts)
{
	shtps_lpwg_grip_set_state(ts, 1);
}

static void shtps_func_grip_off(struct shtps_fts *ts)
{
	if(SHTPS_PRM_LPWG_GRIP_OFF_TPS_RESET_ENABLE != 0)
	{
		int driver_state;
		u8 lpwg_sweep_on_req_state;

		shtps_mutex_lock_ctrl();
		driver_state = ts->state_mgr.state;
		lpwg_sweep_on_req_state = ts->lpwg.lpwg_sweep_on_req_state;
		shtps_mutex_unlock_ctrl();

		if( (driver_state == SHTPS_STATE_SLEEP) &&
			(lpwg_sweep_on_req_state == SHTPS_LPWG_SWEEP_ON_REQ_GRIP_ONLY) )
		{
			shtps_mutex_lock_ctrl();
			ts->lpwg.lpwg_switch = 0;
			ts->lpwg.block_touchevent = 0;
			ts->lpwg.grip_state = 0;
			SHTPS_LOG_DBG_PRINT("[LPWG] grip_state = %d\n", ts->lpwg.grip_state);
			shtps_mutex_unlock_ctrl();

			SHTPS_LOG_DBG_PRINT("[LPWG] %s() grip off tps reset start\n", __func__);
			shtps_shutdown(ts);

			shtps_set_startup_min_time(ts, SHTPS_PRM_STARTUP_MIN_TIME);
			if(shtps_start(ts) == 0){
				shtps_wait_startup(ts);
			}
			SHTPS_LOG_DBG_PRINT("[LPWG] %s() grip off tps reset end\n", __func__);
			shtps_set_startup_min_time(ts, SHTPS_STARTUP_MIN_TIME);

			request_event(ts, SHTPS_EVENT_SLEEP, 0);
		}
		else{
			shtps_lpwg_grip_set_state(ts, 0);
		}
	}
	else{
		shtps_lpwg_grip_set_state(ts, 0);
	}
}
#endif /* SHTPS_DEF_LPWG_GRIP_PROC_ASYNC_ENABLE */

/* -----------------------------------------------------------------------------------
 */
#if defined(SHTPS_COVER_ENABLE)
static void shtps_func_cover_on(struct shtps_fts *ts)
{
	shtps_cover_set_state(ts, 1);
}

static void shtps_func_cover_off(struct shtps_fts *ts)
{
	shtps_cover_set_state(ts, 0);
}
#endif /* SHTPS_COVER_ENABLE */

/* -----------------------------------------------------------------------------------
 */
#if defined(SHTPS_BOOT_FWUPDATE_ENABLE)
static void shtps_func_boot_fw_update(struct shtps_fts *ts)
{
	if( shtps_boot_fwupdate_enable_check(ts) != 0 ){
		u8 buf;
		const unsigned char* fw_data = NULL;
		int ver;
		u8 update = 0;
		#if defined(SHTPS_PANEL_POW_CTL_FOR_FWUPDATE_ENABLE)
			int ret;
		#endif /* SHTPS_PANEL_POW_CTL_FOR_FWUPDATE_ENABLE */
		#ifdef SHTPS_SEND_SHTERM_EVENT_ENABLE
			u8 battlog_crc_error_detect = 0;
			u8 battlog_fw_update_success = 0;
			u8 battlog_fw_update_fail = 0;
		#endif /* SHTPS_SEND_SHTERM_EVENT_ENABLE */

		#if defined(SHTPS_PANEL_POW_CTL_FOR_FWUPDATE_ENABLE)
			ret = shdisp_api_panel_pow_ctl(SHDISP_PANEL_POWER_ON);
			if(ret != SHDISP_RESULT_SUCCESS){
				SHTPS_LOG_ERR_PRINT("%s() panel power on error.\n", __func__);
			}
		#endif /* SHTPS_PANEL_POW_CTL_FOR_FWUPDATE_ENABLE */

		if(shtps_start(ts) == 0){
			shtps_wait_startup(ts);
		}

		shtps_fwctl_get_device_status(ts, &buf);

		ver = shtps_fwver(ts);
		SHTPS_LOG_DBG_PRINT("fw version = 0x%04x\n", ver);
		if(ver != shtps_fwver_builtin(ts)){
			SHTPS_LOG_DBG_PRINT("detect differ fw version\n");
			update = 1;
		}

		if(buf == SHTPS_REGVAL_STATUS_CRC_ERROR){
			#ifdef SHTPS_SEND_SHTERM_EVENT_ENABLE
				battlog_crc_error_detect = 1;
				SHTPS_LOG_ERR_PRINT("SHBATTLOG_EVENT_TPS_CRC_ERROR (first check)\n");
			#endif /* SHTPS_SEND_SHTERM_EVENT_ENABLE */
			SHTPS_LOG_ERR_PRINT("Touch panel CRC error detect\n");
			update = 1;
		}

		if(update != 0){
			fw_data = shtps_fwdata_builtin(ts);
			if(fw_data){
				int ret;
				int retry = 5;
				do{
					ret = shtps_fw_update(ts, fw_data, shtps_fwsize_builtin(ts));
					#ifdef SHTPS_SEND_SHTERM_EVENT_ENABLE
						if (ret != 0) {
							battlog_crc_error_detect = 1;
							SHTPS_LOG_ERR_PRINT("SHBATTLOG_EVENT_TPS_CRC_ERROR (retry=%d)\n", retry);

							shtps_reset(ts);
						}
					#endif /* SHTPS_SEND_SHTERM_EVENT_ENABLE */
				}while(ret != 0 && (retry-- > 0));

				#ifdef SHTPS_SEND_SHTERM_EVENT_ENABLE
					if (ret != 0 && retry < 0) {
						battlog_fw_update_fail = 1;
						SHTPS_LOG_ERR_PRINT("SHBATTLOG_EVENT_TPS_CRC_ERROR_MAX\n");
					} else {
						battlog_fw_update_success = 1;
						SHTPS_LOG_ERR_PRINT("SHBATTLOG_EVENT_TPS_CRC_ERROR_FIX\n");
					}
				#endif /* SHTPS_SEND_SHTERM_EVENT_ENABLE */
			}
		}

		#ifdef SHTPS_SEND_SHTERM_EVENT_ENABLE
			if(battlog_crc_error_detect != 0){
				shtps_send_shterm_event(SHBATTLOG_EVENT_TPS_CRC_ERROR);

				if(battlog_fw_update_fail != 0){
					shtps_send_shterm_event(SHBATTLOG_EVENT_TPS_CRC_ERROR_MAX);
				}
				if(battlog_fw_update_success != 0){
					shtps_send_shterm_event(SHBATTLOG_EVENT_TPS_CRC_ERROR_FIX);
				}
			}
		#endif /* SHTPS_SEND_SHTERM_EVENT_ENABLE */

		if(ts->is_lcd_on == 0){
			shtps_shutdown(ts);
		}

		#if defined(SHTPS_PANEL_POW_CTL_FOR_FWUPDATE_ENABLE)
			ret = shdisp_api_panel_pow_ctl(SHDISP_PANEL_POWER_OFF);
			if(ret != SHDISP_RESULT_SUCCESS){
				SHTPS_LOG_ERR_PRINT("%s() panel power off error.\n", __func__);
			}
		#endif /* SHTPS_PANEL_POW_CTL_FOR_FWUPDATE_ENABLE */
	}
}
#endif /* SHTPS_BOOT_FWUPDATE_ENABLE */

/* -----------------------------------------------------------------------------------
 */
#if defined( SHTPS_TPIN_CHECK_ENABLE ) || defined( SHTPS_CHECK_CRC_ERROR_ENABLE )
int shtps_tpin_enable_check(struct shtps_fts *ts)
{
	#ifdef CONFIG_OF
		int val;

		if(ts->tpin == 0){
			return 0;
		}

		gpio_request(ts->tpin, "tpin");
		pinctrl_select_state(ts->pinctrl, ts->tpin_state_active);
		mb();
		udelay(10);
		val = gpio_get_value(ts->tpin);
		SHTPS_LOG_DBG_PRINT("%s() gpio_get_value() val = %d\n", __func__, val);
		pinctrl_select_state(ts->pinctrl, ts->tpin_state_suspend);
		gpio_free(ts->tpin);

		if(!val) {
			SHTPS_LOG_ERR_PRINT("Upper unit does not exist.\n");
			return -1;
		}
	#endif /* CONFIG_OF */

	return 0;
}
#endif /* SHTPS_TPIN_CHECK_ENABLE || SHTPS_CHECK_CRC_ERROR_ENABLE*/

/* -----------------------------------------------------------------------------------
 */
#if defined( SHTPS_DEF_RECORD_LOG_FILE_ENABLE )
int shtps_record_log_api_add_log(int type)
{
	int ret = 0;
	char	logstr[128];

	//SHTPS_LOG_FUNC_CALL();
	SHTPS_RECORD_LOG_FILE_PRINT("%s() [%s]\n", __func__,
							(type == SHTPS_RECORD_LOG_TYPE_LCD_ON)? "LCD_ON" :
							(type == SHTPS_RECORD_LOG_TYPE_LCD_OFF)? "LCD_OFF" :
							(type == SHTPS_RECORD_LOG_TYPE_REZERO)? "REZERO" : "unknown");

	if(shtps_dur_info_p == NULL) {
		PR_ERROR("record_log log info not allocated :%s()\n", __func__);
		return ret;
	}

	if(shtps_dur_info_p->record_log_file_out_flag == 1) {
		SHTPS_RECORD_LOG_FILE_PRINT("%s() status is record_log file out \n", __func__);
	}

	switch(type) {
	case SHTPS_RECORD_LOG_TYPE_LCD_ON:
		ret = shtps_record_log_file_init_read();
		if(ret == SHTPS_RECORD_LOG_RESULT_FAILURE) {
			return ret;
		}
		sprintf(logstr, "LCD_ON\n");
		shtps_record_log_add_event_log_memory(logstr);
		ret = shtps_record_log_event_log_buf();
		ret = shtps_record_log_file();
		break;
	case SHTPS_RECORD_LOG_TYPE_LCD_OFF:
		sprintf(logstr, "LCD_OFF\n");
		shtps_record_log_add_event_log_memory(logstr);
		ret = shtps_record_log_event_log_buf();
		break;
	case SHTPS_RECORD_LOG_TYPE_REZERO:
		ret = shtps_record_log_file_init_read();
		if(ret == SHTPS_RECORD_LOG_RESULT_FAILURE) {
			return ret;
		}
		sprintf(logstr, "REZERO\n");
		shtps_record_log_add_event_log_memory(logstr);
		ret = shtps_record_log_event_log_buf();
		ret = shtps_record_log_file();
		break;
	default:
		SHTPS_RECORD_LOG_FILE_PRINT("%s()  type err[%d] \n", __func__,type);
		break;
	}

	SHTPS_RECORD_LOG_FILE_PRINT("%s() ret[%d] \n", __func__,ret);

	return ret;
}

void shtps_record_log_api_memory_init(void)
{
	shtps_dur_info_p = kzalloc(sizeof(struct shtps_record_log_info), GFP_KERNEL);
	if(shtps_dur_info_p == NULL){
		PR_ERROR("record_log log info memory allocation error:%s()\n", __func__);
		return;
	}

	memset(shtps_dur_info_p, 0x00, sizeof(struct shtps_record_log_info));
	shtps_dur_info_p->write_length = 0;
	shtps_dur_info_p->read_flag = 0;
	shtps_dur_info_p->record_log_file_out_flag = 0;
	shtps_dur_info_p->event_store_tbl_array = 0;
	return;
	
}

void shtps_record_log_api_memory_deinit(void)
{
	if(shtps_dur_info_p != NULL) {
		kfree(shtps_dur_info_p);
	}
	shtps_dur_info_p = NULL;
	return;
}

static ssize_t shtps_record_log_kernel_write(struct file *fp, const char *buf, size_t size)
{
	mm_segment_t old_fs;
	ssize_t res = 0;

	if (buf == NULL) {
		SHTPS_RECORD_LOG_FILE_PRINT("%s() <NULL_POINTER>\n", __func__);
		return res;
	}

	old_fs = get_fs();
	set_fs(get_ds());
	res = fp->f_op->write(fp, buf, size, &fp->f_pos);
	set_fs(old_fs);

	return res;
}

static ssize_t shtps_record_log_kernel_read(struct file *fp, char *buf, size_t size , unsigned int offset)
{
	mm_segment_t old_fs;
	ssize_t res = 0;
	loff_t fpos = offset;

	if (buf == NULL) {
		SHTPS_RECORD_LOG_FILE_PRINT("%s() <NULL_POINTER>\n", __func__);
		return res;
	}

	old_fs = get_fs();
	set_fs(get_ds());
	res = fp->f_op->read(fp, buf, size, &fpos);
	set_fs(old_fs);

	return res;
}
static int shtps_record_log_file_size_check(char *buf, size_t check_size)
{
	int i;
	int k;
	char *move_point;
	if((shtps_dur_info_p->write_length + check_size) >= SHTPS_RECORD_LOG_FILE_CHECK_SIZE) {
		for(i=SHTPS_RECORD_LOG_BUF_SPACING_SIZE;i<SHTPS_RECORD_LOG_FILE_CHECK_SIZE;i++) {
			if(buf[i] == '/') {
				if((buf[i+3] == '/') && (buf[i+6] == '(')) {
        				SHTPS_RECORD_LOG_FILE_PRINT("move point (i-4)=%d\n", i-4);
					move_point = &(buf[i-4]);
					for(k=0;k<(shtps_dur_info_p->write_length -(i-4));k++) {
						buf[k] = move_point[k];
					}
					buf[k] = 0x00;
					shtps_dur_info_p->write_length = (shtps_dur_info_p->write_length - (i-4));
        				SHTPS_RECORD_LOG_FILE_PRINT("check size  --> write length=%d\n", shtps_dur_info_p->write_length);
					break;
				}
			}
		}
	}
        SHTPS_RECORD_LOG_FILE_PRINT("%s()  write_length=%d\n", __func__, shtps_dur_info_p->write_length);
	
	return SHTPS_RECORD_LOG_RESULT_SUCCESS;
}
static int shtps_record_log_file_init_read(void)
{
	struct path  path;
	struct file *fp;
	int ret = -EINVAL;

	int i;
	int event_log_str_cnt = 0;
	int event_log_length_cnt = 0;
	int total_event_log_length = 0;
	int wk_array;

	SHTPS_LOG_FUNC_CALL();
	
	if(shtps_dur_info_p->read_flag == 1) {
		SHTPS_RECORD_LOG_FILE_PRINT("%s()  record_log file already init read\n", __func__);
		return SHTPS_RECORD_LOG_RESULT_SUCCESS;
	}

	memcpy(shtps_dur_info_p->event_init_read_tbl,shtps_dur_info_p->event_store_tbl,sizeof(shtps_dur_info_p->event_init_read_tbl));

	ret = kern_path(SHTPS_RECORD_LOG_FILE, LOOKUP_OPEN, &path);
	if (ret != 0) {
		ret = shtps_record_log_summary_init_file();
		if (ret != SHTPS_RECORD_LOG_RESULT_SUCCESS) {
			return SHTPS_RECORD_LOG_RESULT_FAILURE;
		}
	}
	SHTPS_RECORD_LOG_FILE_PRINT("%s()  record_log file open start\n", __func__);
	fp = filp_open(SHTPS_RECORD_LOG_FILE, O_RDWR, 0660);
	if (IS_ERR_OR_NULL(fp)) {
		SHTPS_RECORD_LOG_FILE_PRINT("%s()  Cannot open file: %s\n", __func__, SHTPS_RECORD_LOG_FILE);
		return SHTPS_RECORD_LOG_RESULT_FAILURE;
	}
	SHTPS_RECORD_LOG_FILE_PRINT("%s()  record_log file open end\n", __func__);

	SHTPS_RECORD_LOG_FILE_PRINT("%s()  record_log file read start\n", __func__);
	memset(shtps_dur_info_p->write_buf, 0x00, sizeof(shtps_dur_info_p->write_buf));
	shtps_dur_info_p->write_length = shtps_record_log_kernel_read(fp, shtps_dur_info_p->write_buf, sizeof(shtps_dur_info_p->write_buf), 0);
	shtps_dur_info_p->read_flag = 1;
	SHTPS_RECORD_LOG_FILE_PRINT("%s()  record_log file read end  length=%d\n", __func__, shtps_dur_info_p->write_length);

	for(i = 0; i < shtps_dur_info_p->write_length; i++){
		if(shtps_dur_info_p->write_buf[i]=='\n'){
			event_log_length_cnt++;
			memcpy(shtps_dur_info_p->event_store_tbl[event_log_str_cnt].event_log_str,&(shtps_dur_info_p->write_buf[1+i-event_log_length_cnt]), event_log_length_cnt);
			shtps_dur_info_p->event_store_tbl[event_log_str_cnt].event_log_length = event_log_length_cnt;
			total_event_log_length = total_event_log_length + event_log_str_cnt;
			event_log_str_cnt++;
			event_log_length_cnt = 0;
		}else{
			event_log_length_cnt++;
		}
	}

	if(event_log_str_cnt >= SHTPS_RECORD_LOG_EVENT_STORE_NUM) {
		event_log_str_cnt = 0;
	}

	wk_array = shtps_dur_info_p->event_store_tbl_array;
	if(wk_array == 0) {
		for(i=0;i<SHTPS_RECORD_LOG_EVENT_STORE_NUM;i++) {
			if(shtps_dur_info_p->event_init_read_tbl[i].event_log_length > 0) {
			memcpy(shtps_dur_info_p->event_store_tbl[event_log_str_cnt].event_log_str,shtps_dur_info_p->event_init_read_tbl[i].event_log_str, shtps_dur_info_p->event_init_read_tbl[i].event_log_length);
			shtps_dur_info_p->event_store_tbl[event_log_str_cnt].event_log_length = shtps_dur_info_p->event_init_read_tbl[i].event_log_length;
			event_log_str_cnt++;
				if(event_log_str_cnt >= SHTPS_RECORD_LOG_EVENT_STORE_NUM) {
					event_log_str_cnt = 0;
				}
			}
		}
	} else {
		for(i=wk_array;i<SHTPS_RECORD_LOG_EVENT_STORE_NUM;i++) {
			if(shtps_dur_info_p->event_init_read_tbl[i].event_log_length > 0) {
			memcpy(shtps_dur_info_p->event_store_tbl[event_log_str_cnt].event_log_str,shtps_dur_info_p->event_init_read_tbl[i].event_log_str, shtps_dur_info_p->event_init_read_tbl[i].event_log_length);
			shtps_dur_info_p->event_store_tbl[event_log_str_cnt].event_log_length = shtps_dur_info_p->event_init_read_tbl[i].event_log_length;
			event_log_str_cnt++;
				if(event_log_str_cnt >= SHTPS_RECORD_LOG_EVENT_STORE_NUM) {
					event_log_str_cnt = 0;
				}
			}
		}
		for(i=0;i<wk_array;i++) {
			if(shtps_dur_info_p->event_init_read_tbl[i].event_log_length > 0) {
			memcpy(shtps_dur_info_p->event_store_tbl[event_log_str_cnt].event_log_str,shtps_dur_info_p->event_init_read_tbl[i].event_log_str, shtps_dur_info_p->event_init_read_tbl[i].event_log_length);
			shtps_dur_info_p->event_store_tbl[event_log_str_cnt].event_log_length = shtps_dur_info_p->event_init_read_tbl[i].event_log_length;
			event_log_str_cnt++;
				if(event_log_str_cnt >= SHTPS_RECORD_LOG_EVENT_STORE_NUM) {
					event_log_str_cnt = 0;
				}
			}
		}
	}

	shtps_dur_info_p->event_store_tbl_array=event_log_str_cnt;

	filp_close(fp, NULL);

	return SHTPS_RECORD_LOG_RESULT_SUCCESS;
}

void shtps_record_log_output(void){
	int i;
	SHTPS_RECORD_LOG_FILE_PRINT("%s() OUTPUT event_store_tbl_array[%d]\n", __func__, shtps_dur_info_p->event_store_tbl_array);
	for(i = 0; i < SHTPS_RECORD_LOG_EVENT_STORE_NUM; i++){
		SHTPS_RECORD_LOG_FILE_PRINT("%s() OUTPUT event_store_tbl[%d] event_log_str[%s] event_log_length[%d]\n", __func__,i,shtps_dur_info_p->event_store_tbl[i].event_log_str,shtps_dur_info_p->event_store_tbl[i].event_log_length);
	}
}

static int shtps_record_log_file(void)
{
	struct path  path;
	struct file *fp;
	int ret = -EINVAL;

	SHTPS_LOG_FUNC_CALL();
	ret = kern_path(SHTPS_RECORD_LOG_FILE, LOOKUP_OPEN, &path);
	if (ret != 0) {
		ret = shtps_record_log_summary_init_file();
		if (ret != SHTPS_RECORD_LOG_RESULT_SUCCESS) {
			return SHTPS_RECORD_LOG_RESULT_FAILURE;
		}
	}
	if(shtps_dur_info_p->write_length) {
		SHTPS_RECORD_LOG_FILE_PRINT("%s()  record_log file open start\n", __func__);
		fp = filp_open(SHTPS_RECORD_LOG_FILE, O_RDWR|O_TRUNC, 0660);
		if (IS_ERR_OR_NULL(fp)) {
			SHTPS_RECORD_LOG_FILE_PRINT("%s()  Cannot open file: %s\n", __func__, SHTPS_RECORD_LOG_FILE);
			return SHTPS_RECORD_LOG_RESULT_FAILURE;
		}
		SHTPS_RECORD_LOG_FILE_PRINT("%s()  record_log file open end\n", __func__);

		SHTPS_RECORD_LOG_FILE_PRINT("%s()  write_length: %d write start\n", __func__, shtps_dur_info_p->write_length);
		shtps_dur_info_p->write_length = shtps_record_log_kernel_write(fp, shtps_dur_info_p->write_buf, shtps_dur_info_p->write_length);
		SHTPS_RECORD_LOG_FILE_PRINT("%s()  write_length: %d write end \n", __func__, shtps_dur_info_p->write_length);

		SHTPS_RECORD_LOG_FILE_PRINT("%s()  record_log file close start\n", __func__);
		filp_close(fp, NULL);
		SHTPS_RECORD_LOG_FILE_PRINT("%s()  record_log file close end\n", __func__);
	}else{
	}

	return SHTPS_RECORD_LOG_RESULT_SUCCESS;
}

static int shtps_record_log_summary_init_file(void)
{
	struct file *fp;

	SHTPS_LOG_FUNC_CALL();

	SHTPS_RECORD_LOG_FILE_PRINT("%s() open file: %s pid=%d tgid=%d comm=%s\n", __func__, SHTPS_RECORD_LOG_FILE, current->pid, current->tgid,
                                                                                                       current->comm);
	fp = filp_open(SHTPS_RECORD_LOG_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0660);
	if (IS_ERR_OR_NULL(fp)) {
		SHTPS_RECORD_LOG_FILE_PRINT("%s()  Cannot create file: %s\n", __func__, SHTPS_RECORD_LOG_FILE);
		return SHTPS_RECORD_LOG_RESULT_FAILURE;
	}

	filp_close(fp, NULL);

	return SHTPS_RECORD_LOG_RESULT_SUCCESS;
}

static void record_log_get_date_str(char *str_buf)
{
	struct timeval tv;
	struct tm tm1, tm2;

	do_gettimeofday(&tv);
	time_to_tm((time_t)tv.tv_sec, 0, &tm1);
	time_to_tm((time_t)tv.tv_sec, (sys_tz.tz_minuteswest * 60 * (-1)), &tm2);

	sprintf(str_buf,"%04d/%02d/%02d(%s)%02d:%02d:%02d.%06d\n",
			(int)(tm2.tm_year + 1900), tm2.tm_mon + 1, tm2.tm_mday, WeekOfDay[tm2.tm_wday],
			tm2.tm_hour, tm2.tm_min, tm2.tm_sec, (int)tv.tv_usec);
	return;
}
static int shtps_record_log_add_event_log_memory(char *logstr)
{
	int ret = 0;
	ret = shtps_record_log_add_event_log_store(logstr, shtps_dur_info_p->event_store_tbl, &(shtps_dur_info_p->event_store_tbl_array));
	return ret;
}

static int shtps_record_log_add_event_log_store(char *logstr, struct shtps_record_log_event_store_log *store_tbl_p, int *array)
{
	char *date_buf_p;
	int date_size;
	int loglen;

	loglen = strlen(logstr);
	if(loglen > 0) {
		store_tbl_p[*array].event_log_str[0] = 0x00;
		store_tbl_p[*array].event_log_length = 0;
		date_buf_p = store_tbl_p[*array].event_log_str;
		record_log_get_date_str(date_buf_p);
		date_size = strlen(date_buf_p);
		date_buf_p[date_size -1] = 0x20;  /* LF --> space */

		strcat(store_tbl_p[*array].event_log_str, logstr);

		if(logstr[loglen -1] != '\n') {
			strcat(store_tbl_p[*array].event_log_str, "\n");
		}
		store_tbl_p[*array].event_log_length = strlen(store_tbl_p[*array].event_log_str);
		SHTPS_RECORD_LOG_FILE_PRINT("%s() event log stored length:%d\n", __func__, store_tbl_p[*array].event_log_length);
		(*array)++;
		if(*array >= SHTPS_RECORD_LOG_EVENT_STORE_NUM) {
			*array = 0;
		}
	}

	return SHTPS_RECORD_LOG_RESULT_SUCCESS;
}

static int shtps_record_log_event_log_buf(void)
{
	int ret = 0;
	int wk_array;
	int i;
	int log_total_length = 0;
	struct shtps_record_log_event_store_log  *store_tbl_p;

	int write_buf_cnt = 0;
	shtps_dur_info_p->write_length = 0;

	SHTPS_LOG_FUNC_CALL();

	store_tbl_p = shtps_dur_info_p->event_store_tbl;
	for(i=0;i<SHTPS_RECORD_LOG_EVENT_STORE_NUM;i++) {
		log_total_length += store_tbl_p[i].event_log_length;
	}
	shtps_record_log_file_size_check(shtps_dur_info_p->write_buf, log_total_length);
	wk_array = shtps_dur_info_p->event_store_tbl_array;

	if(wk_array == 0) {
		for(i=0;i<SHTPS_RECORD_LOG_EVENT_STORE_NUM;i++) {
			if(store_tbl_p[i].event_log_length > 0) {
				memcpy(&(shtps_dur_info_p->write_buf[write_buf_cnt]), store_tbl_p[i].event_log_str, store_tbl_p[i].event_log_length);
				shtps_dur_info_p->write_length += store_tbl_p[i].event_log_length;
				write_buf_cnt += store_tbl_p[i].event_log_length;
			}
		}
	} else {
		for(i=wk_array;i<SHTPS_RECORD_LOG_EVENT_STORE_NUM;i++) {
			if(store_tbl_p[i].event_log_length > 0) {
				memcpy(&(shtps_dur_info_p->write_buf[write_buf_cnt]), store_tbl_p[i].event_log_str, store_tbl_p[i].event_log_length);
				shtps_dur_info_p->write_length += store_tbl_p[i].event_log_length;
				write_buf_cnt += store_tbl_p[i].event_log_length;
			}
		}
		for(i=0;i<wk_array;i++) {
			if(store_tbl_p[i].event_log_length > 0) {
				memcpy(&(shtps_dur_info_p->write_buf[write_buf_cnt]), store_tbl_p[i].event_log_str, store_tbl_p[i].event_log_length);
				shtps_dur_info_p->write_length += store_tbl_p[i].event_log_length;
				write_buf_cnt += store_tbl_p[i].event_log_length;
			}
		}
	}

	shtps_dur_info_p->write_buf[write_buf_cnt] = 0x00;

	return ret;
}
#endif /* SHTPS_DEF_RECORD_LOG_FILE_ENABLE */

/* -----------------------------------------------------------------------------------
 */
static void shtps_func_open(struct shtps_fts *ts)
{
}

static void shtps_func_close(struct shtps_fts *ts)
{
	shtps_shutdown(ts);
}

static int shtps_func_enable(struct shtps_fts *ts)
{
	if(shtps_start(ts) != 0){
		return -EFAULT;
	}
	if(shtps_wait_startup(ts) != 0){
		return -EFAULT;
	}

	return 0;
}

static void shtps_func_wakelock_init(struct shtps_fts *ts)
{
	ts->work_wake_lock_state = 0;
    wake_lock_init(&ts->work_wake_lock, WAKE_LOCK_SUSPEND, "shtps_work_wake_lock");
	ts->work_pm_qos_lock_idle.type = PM_QOS_REQ_ALL_CORES;
	pm_qos_add_request(&ts->work_pm_qos_lock_idle, PM_QOS_CPU_DMA_LATENCY, PM_QOS_DEFAULT_VALUE);
}

static void shtps_func_wakelock_deinit(struct shtps_fts *ts)
{
	wake_lock_destroy(&ts->work_wake_lock);
	pm_qos_remove_request(&ts->work_pm_qos_lock_idle);
}

static void shtps_func_wakelock(struct shtps_fts *ts, int on)
{
	SHTPS_LOG_FUNC_CALL();
	if(on){
	    wake_lock(&ts->work_wake_lock);
	    pm_qos_update_request(&ts->work_pm_qos_lock_idle, SHTPS_QOS_LATENCY_DEF_VALUE);
		SHTPS_LOG_DBG_PRINT("work wake lock ON\n");
	}else{
		wake_unlock(&ts->work_wake_lock);
	    pm_qos_update_request(&ts->work_pm_qos_lock_idle, PM_QOS_DEFAULT_VALUE);
		SHTPS_LOG_DBG_PRINT("work wake lock OFF\n");
	}
}

static void shtps_func_disable(struct shtps_fts *ts)
{
	shtps_shutdown(ts);
}


static void shtps_func_request_async_complete(void *arg_p)
{
	kfree( arg_p );
}

void shtps_func_request_async( struct shtps_fts *ts, int event)
{
	struct shtps_req_msg		*msg_p;
	unsigned long	flags;

	msg_p = (struct shtps_req_msg *)kzalloc( sizeof( struct shtps_req_msg ), GFP_KERNEL );
	if ( msg_p == NULL ){
		SHTPS_LOG_ERR_PRINT("Out of memory [event:%d]\n", event);
		return;
	}

	msg_p->complete = shtps_func_request_async_complete;
	msg_p->event = event;
	msg_p->context = msg_p;
	msg_p->status = -1;

	spin_lock_irqsave( &(ts->queue_lock), flags);
	list_add_tail( &(msg_p->queue), &(ts->queue) );
	spin_unlock_irqrestore( &(ts->queue_lock), flags);
	queue_work(ts->workqueue_p, &(ts->work_data) );
}

static void shtps_func_request_sync_complete(void *arg_p)
{
	complete( arg_p );
}

int shtps_func_request_sync( struct shtps_fts *ts, int event)
{
	DECLARE_COMPLETION_ONSTACK(done);
	struct shtps_req_msg msg;
	unsigned long	flags;

	msg.complete = shtps_func_request_sync_complete;
	msg.event = event;
	msg.context = &done;
	msg.status = -1;

	spin_lock_irqsave( &(ts->queue_lock), flags);
	list_add_tail( &(msg.queue), &(ts->queue) );
	spin_unlock_irqrestore( &(ts->queue_lock), flags);
	queue_work(ts->workqueue_p, &(ts->work_data) );

	wait_for_completion(&done);

	return msg.status;
}

static void shtps_overlap_event_check(struct shtps_fts *ts)
{
	unsigned long flags;
	struct list_head *list_p;
	int overlap_req_cnt_lcd = -1;
	int overlap_req_cnt_grip = -1;
	int overlap_req_cnt_cover = -1;

	spin_lock_irqsave( &(ts->queue_lock), flags );
	list_for_each(list_p, &(ts->queue)){
		ts->cur_msg_p = list_entry(list_p, struct shtps_req_msg, queue);
		spin_unlock_irqrestore( &(ts->queue_lock), flags );

		if( (ts->cur_msg_p->event == SHTPS_FUNC_REQ_EVEMT_LCD_ON) ||
			(ts->cur_msg_p->event == SHTPS_FUNC_REQ_EVEMT_LCD_OFF) )
		{
			overlap_req_cnt_lcd++;
		}

		if( (ts->cur_msg_p->event == SHTPS_FUNC_REQ_EVEMT_GRIP_ON) ||
			(ts->cur_msg_p->event == SHTPS_FUNC_REQ_EVEMT_GRIP_OFF) )
		{
			overlap_req_cnt_grip++;
		}

		if( (ts->cur_msg_p->event == SHTPS_FUNC_REQ_EVEMT_COVER_ON) ||
			(ts->cur_msg_p->event == SHTPS_FUNC_REQ_EVEMT_COVER_OFF) )
		{
			overlap_req_cnt_cover++;
		}

		spin_lock_irqsave( &(ts->queue_lock), flags );
	}
	spin_unlock_irqrestore( &(ts->queue_lock), flags );

	if( (overlap_req_cnt_lcd > 0) || (overlap_req_cnt_grip > 0) || (overlap_req_cnt_cover > 0) ){
		spin_lock_irqsave( &(ts->queue_lock), flags );
		list_for_each(list_p, &(ts->queue)){
			ts->cur_msg_p = list_entry(list_p, struct shtps_req_msg, queue);
			spin_unlock_irqrestore( &(ts->queue_lock), flags );

			if(ts->cur_msg_p != NULL){
				if( (ts->cur_msg_p->event == SHTPS_FUNC_REQ_EVEMT_LCD_ON) ||
					(ts->cur_msg_p->event == SHTPS_FUNC_REQ_EVEMT_LCD_OFF) )
				{
					if(overlap_req_cnt_lcd > 0){
						spin_lock_irqsave( &(ts->queue_lock), flags );
						list_p = list_p->prev;
						list_del_init( &(ts->cur_msg_p->queue) );
						spin_unlock_irqrestore( &(ts->queue_lock), flags );
						ts->cur_msg_p->status = 0;
						if( ts->cur_msg_p->complete ){
							ts->cur_msg_p->complete( ts->cur_msg_p->context );
						}
						overlap_req_cnt_lcd--;
						SHTPS_LOG_DBG_PRINT("FuncReq[%d] overlap canceled\n", ts->cur_msg_p->event);
					}
				}

				if( (ts->cur_msg_p->event == SHTPS_FUNC_REQ_EVEMT_GRIP_ON) ||
					(ts->cur_msg_p->event == SHTPS_FUNC_REQ_EVEMT_GRIP_OFF) )
				{
					if(overlap_req_cnt_grip > 0){
						spin_lock_irqsave( &(ts->queue_lock), flags );
						list_p = list_p->prev;
						list_del_init( &(ts->cur_msg_p->queue) );
						spin_unlock_irqrestore( &(ts->queue_lock), flags );
						ts->cur_msg_p->status = 0;
						if( ts->cur_msg_p->complete ){
							ts->cur_msg_p->complete( ts->cur_msg_p->context );
						}
						overlap_req_cnt_grip--;
						SHTPS_LOG_DBG_PRINT("FuncReq[%d] overlap canceled\n", ts->cur_msg_p->event);
					}
				}

				if( (ts->cur_msg_p->event == SHTPS_FUNC_REQ_EVEMT_COVER_ON) ||
					(ts->cur_msg_p->event == SHTPS_FUNC_REQ_EVEMT_COVER_OFF) )
				{
					if(overlap_req_cnt_cover > 0){
						spin_lock_irqsave( &(ts->queue_lock), flags );
						list_p = list_p->prev;
						list_del_init( &(ts->cur_msg_p->queue) );
						spin_unlock_irqrestore( &(ts->queue_lock), flags );
						ts->cur_msg_p->status = 0;
						if( ts->cur_msg_p->complete ){
							ts->cur_msg_p->complete( ts->cur_msg_p->context );
						}
						overlap_req_cnt_cover--;
						SHTPS_LOG_DBG_PRINT("FuncReq[%d] overlap canceled\n", ts->cur_msg_p->event);
					}
				}
			}else{
				SHTPS_LOG_DBG_PRINT("%s() overlap event delete check msg NULL\n", __func__);
			}

			spin_lock_irqsave( &(ts->queue_lock), flags );
		}

		spin_unlock_irqrestore( &(ts->queue_lock), flags );
	}
}

static void shtps_func_workq( struct work_struct *work_p )
{
	struct shtps_fts	*ts;
	unsigned long			flags;

	ts = container_of(work_p, struct shtps_fts, work_data);

	spin_lock_irqsave( &(ts->queue_lock), flags );
	while( list_empty( &(ts->queue) ) == 0 ){
		ts->cur_msg_p = list_entry( ts->queue.next, struct shtps_req_msg, queue);
		list_del_init( &(ts->cur_msg_p->queue) );
		spin_unlock_irqrestore( &(ts->queue_lock), flags );

		SHTPS_LOG_DBG_PRINT("FuncReq[%d] start\n", ts->cur_msg_p->event);

		shtps_func_wakelock(ts, 1);

		switch(ts->cur_msg_p->event){
			case SHTPS_FUNC_REQ_EVEMT_OPEN:
				#if defined(SHTPS_GUARANTEE_I2C_ACCESS_IN_WAKE_ENABLE)
					if(shtps_check_suspend_state(ts, SHTPS_DETER_SUSPEND_I2C_PROC_OPEN, 0) == 0){
						shtps_func_open(ts);
					}
				#else
					shtps_func_open(ts);
				#endif /* SHTPS_GUARANTEE_I2C_ACCESS_IN_WAKE_ENABLE */
				ts->cur_msg_p->status = 0;
				break;

			case SHTPS_FUNC_REQ_EVEMT_CLOSE:
				#if defined(SHTPS_GUARANTEE_I2C_ACCESS_IN_WAKE_ENABLE)
					if(shtps_check_suspend_state(ts, SHTPS_DETER_SUSPEND_I2C_PROC_CLOSE, 0) == 0){
						shtps_func_close(ts);
					}
				#else
					shtps_func_close(ts);
				#endif /* SHTPS_GUARANTEE_I2C_ACCESS_IN_WAKE_ENABLE */
				ts->cur_msg_p->status = 0;
				break;

			case SHTPS_FUNC_REQ_EVEMT_ENABLE:
				#if defined(SHTPS_GUARANTEE_I2C_ACCESS_IN_WAKE_ENABLE)
					if(shtps_check_suspend_state(ts, SHTPS_DETER_SUSPEND_I2C_PROC_ENABLE, 0) == 0){
						ts->cur_msg_p->status = shtps_func_enable(ts);
					}else{
						ts->cur_msg_p->status = 0;
					}
				#else
					ts->cur_msg_p->status = shtps_func_enable(ts);
				#endif /* SHTPS_GUARANTEE_I2C_ACCESS_IN_WAKE_ENABLE */
				break;

			case SHTPS_FUNC_REQ_EVEMT_DISABLE:
				shtps_func_disable(ts);
				ts->cur_msg_p->status = 0;
				break;

			#if defined( SHTPS_CHECK_CRC_ERROR_ENABLE )
				case SHTPS_FUNC_REQ_EVEMT_CHECK_CRC_ERROR:
					shtps_func_check_crc_error(ts);
					ts->cur_msg_p->status = 0;
					break;
			#endif /* #if defined( SHTPS_CHECK_CRC_ERROR_ENABLE ) */

			case SHTPS_FUNC_REQ_EVEMT_LCD_ON:
				#if defined( SHTPS_DEF_RECORD_LOG_FILE_ENABLE )
				shtps_record_log_api_add_log(SHTPS_RECORD_LOG_TYPE_LCD_ON);
				#endif /* #if defined( SHTPS_DEF_RECORD_LOG_FILE_ENABLE ) */

				shtps_func_lcd_on(ts);
				ts->cur_msg_p->status = 0;
				break;

			case SHTPS_FUNC_REQ_EVEMT_LCD_OFF:
				#if defined( SHTPS_DEF_RECORD_LOG_FILE_ENABLE )
				shtps_record_log_api_add_log(SHTPS_RECORD_LOG_TYPE_LCD_OFF);
				#endif /* #if defined( SHTPS_DEF_RECORD_LOG_FILE_ENABLE ) */

				shtps_func_lcd_off(ts);
				ts->cur_msg_p->status = 0;
				break;

			#if defined(SHTPS_DEF_LPWG_GRIP_PROC_ASYNC_ENABLE)
				case SHTPS_FUNC_REQ_EVEMT_GRIP_ON:
					shtps_func_grip_on(ts);
					ts->cur_msg_p->status = 0;
					break;

				case SHTPS_FUNC_REQ_EVEMT_GRIP_OFF:
					shtps_func_grip_off(ts);
					ts->cur_msg_p->status = 0;
					break;
			#endif /* SHTPS_DEF_LPWG_GRIP_PROC_ASYNC_ENABLE */

			#if defined(SHTPS_PROXIMITY_SUPPORT_ENABLE)
				case SHTPS_FUNC_REQ_EVEMT_PROXIMITY_CHECK:
					shtps_func_proximity_check(ts);
					ts->cur_msg_p->status = 0;
					break;
			#endif /* SHTPS_PROXIMITY_SUPPORT_ENABLE */

			#if defined(SHTPS_COVER_ENABLE)
				case SHTPS_FUNC_REQ_EVEMT_COVER_ON:
					shtps_func_cover_on(ts);
					ts->cur_msg_p->status = 0;
					break;

				case SHTPS_FUNC_REQ_EVEMT_COVER_OFF:
					shtps_func_cover_off(ts);
					ts->cur_msg_p->status = 0;
					break;
			#endif /* SHTPS_COVER_ENABLE */

			#if defined(SHTPS_BOOT_FWUPDATE_ENABLE)
				case SHTPS_FUNC_REQ_BOOT_FW_UPDATE:
					shtps_func_boot_fw_update(ts);
					ts->cur_msg_p->status = 0;
					break;
			#endif /* SHTPS_BOOT_FWUPDATE_ENABLE */

			default:
				ts->cur_msg_p->status = -1;
				break;
		}

		SHTPS_LOG_DBG_PRINT("FuncReq[%d] end\n", ts->cur_msg_p->event);

		if( ts->cur_msg_p->complete ){
			ts->cur_msg_p->complete( ts->cur_msg_p->context );
		}

		shtps_overlap_event_check(ts);

		shtps_func_wakelock(ts, 0);

		spin_lock_irqsave( &(ts->queue_lock), flags );
	}
	spin_unlock_irqrestore( &(ts->queue_lock), flags );
}

int shtps_func_async_init( struct shtps_fts *ts)
{
	ts->workqueue_p = alloc_workqueue("TPS_WORK", WQ_UNBOUND, 1);
	if(ts->workqueue_p == NULL){
		return -ENOMEM;
	}
	INIT_WORK( &(ts->work_data), shtps_func_workq );
	INIT_LIST_HEAD( &(ts->queue) );
	spin_lock_init( &(ts->queue_lock) );
	shtps_func_wakelock_init(ts);

	return 0;
}

void shtps_func_async_deinit( struct shtps_fts *ts)
{
	shtps_func_wakelock_deinit(ts);
	if(ts->workqueue_p){
		destroy_workqueue(ts->workqueue_p);
	}
}
 
/* -----------------------------------------------------------------------------------
 */

#if defined(SHTPS_DEF_RECORD_LOG_FILE_ENABLE)
void shtps_record_log_file_init(struct shtps_fts *ts)
{
	ts->record_log_file_info_p = kzalloc(sizeof(struct shtps_record_log_file_info), GFP_KERNEL);
	if(ts->record_log_file_info_p == NULL){
		PR_ERROR("memory allocation error:%s()\n", __func__);
		return;
	}

	ts->record_log_file_info_p->ts_p = ts;

	shtps_record_log_api_memory_init();   /* for record_log */
}

void shtps_record_log_file_deinit(struct shtps_fts *ts)
{
	if(ts->record_log_file_info_p != NULL){
		kfree(ts->record_log_file_info_p);
	}

	ts->record_log_file_info_p = NULL;

	shtps_record_log_api_memory_deinit();   /* for record_log */
}

#endif /* SHTPS_DEF_RECORD_LOG_FILE_ENABLE */

/* -----------------------------------------------------------------------------------
*/
MODULE_DESCRIPTION("FocalTech fts TouchScreen driver");
MODULE_LICENSE("GPLv2");
