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
#include <linux/delay.h>
#include <linux/sysfs.h>
#include <linux/uaccess.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/time.h>

#include <linux/fs.h>

#include <linux/input/shtps_dev.h>

#include "shtps_fts.h"
#include "fwctl/shtps_fwctl_focaltech.h"
#include "shtps_param_extern.h"
#include "shtps_fts_debug.h"
#include "shtps_log.h"

#if defined( SHTPS_DEVELOP_MODE_ENABLE )

/* -----------------------------------------------------------------------------------
 */
#if defined ( SHTPS_TOUCH_EMURATOR_ENABLE )
	SHTPS_PARAM_DEF( SHTPS_TOUCH_EMURATOR_LOG_ENABLE, 				0  );
	SHTPS_PARAM_DEF( SHTPS_TOUCH_EMURATOR_START_WAIT_TIME_SEC, 		5  );

	#define	SHTPS_TOUCH_EMU_LOG_V(...)				\
		if(SHTPS_TOUCH_EMURATOR_LOG_ENABLE != 0){	\
			DBG_PRINTK("[emu] " __VA_ARGS__);		\
		}

	#define	SHTPS_TOUCH_EMU_LOG_DBG_PRINT(...)												\
		if(((shtps_get_logflag() & 0x02) != 0) || (SHTPS_TOUCH_EMURATOR_LOG_ENABLE != 0)){	\
			DBG_PRINTK("[emu] " __VA_ARGS__);												\
		}

	#define SHTPS_TOUCH_EMU_LOG_FUNC_CALL()													\
		if(((shtps_get_logflag() & 0x02) != 0) || (SHTPS_TOUCH_EMURATOR_LOG_ENABLE != 0)){	\
			DBG_PRINTK("[emu] %s()\n", __func__);											\
		}
	#define SHTPS_TOUCH_EMU_LOG_FUNC_CALL_INPARAM(param)									\
		if(((shtps_get_logflag() & 0x02) != 0) || (SHTPS_TOUCH_EMURATOR_LOG_ENABLE != 0)){	\
			DBG_PRINTK("[emu] %s(%d)\n", __func__, param);									\
		}

	#define SET_TOUCH_DATA_FINGERSTATE(TOUCH_DATA, para)		(TOUCH_DATA[0] = (TOUCH_DATA[0] & 0x3F) | (para << 6))
	#define SET_TOUCH_DATA_XPOS(TOUCH_DATA, para)				{TOUCH_DATA[0] = (TOUCH_DATA[0] & 0xF0) | ((para >> 8) & 0x0F); TOUCH_DATA[1] = para & 0xFF;}
	#define SET_TOUCH_DATA_YPOS(TOUCH_DATA, para)				{TOUCH_DATA[2] = (TOUCH_DATA[2] & 0xF0) | ((para >> 8) & 0x0F); TOUCH_DATA[3] = para & 0xFF;}
	#define SET_TOUCH_DATA_ID(TOUCH_DATA, para)					(TOUCH_DATA[2] = (TOUCH_DATA[2] & 0x0F) | ((para << 4) & 0xF0))
	#define SET_TOUCH_DATA_WEIGHT(TOUCH_DATA, para)				(TOUCH_DATA[4] = para)
	#define SET_TOUCH_DATA_AREA(TOUCH_DATA, para)				(TOUCH_DATA[5] = (TOUCH_DATA[5] & 0x0F) | ((para << 4) & 0xF0))
#endif /* #if defined ( SHTPS_TOUCH_EMURATOR_ENABLE ) */


#if defined ( SHTPS_TOUCH_EVENTLOG_ENABLE )
	SHTPS_PARAM_DEF( SHTPS_TOUCH_EVENTLOG_LOG_ENABLE, 				0  );

	#define	SHTPS_TOUCH_EVENTLOG_LOG_V(...)			\
		if(SHTPS_TOUCH_EVENTLOG_LOG_ENABLE != 0){	\
			DBG_PRINTK("[eventlog] " __VA_ARGS__);	\
		}

	#define	SHTPS_TOUCH_EVENTLOG_LOG_DBG_PRINT(...)											\
		if(((shtps_get_logflag() & 0x02) != 0) || (SHTPS_TOUCH_EVENTLOG_LOG_ENABLE != 0)){	\
			DBG_PRINTK("[eventlog] " __VA_ARGS__);											\
		}

	#define SHTPS_TOUCH_EVENTLOG_LOG_FUNC_CALL()											\
		if(((shtps_get_logflag() & 0x02) != 0) || (SHTPS_TOUCH_EVENTLOG_LOG_ENABLE != 0)){	\
			DBG_PRINTK("[eventlog] %s()\n", __func__);										\
		}
	#define SHTPS_TOUCH_EVENTLOG_LOG_FUNC_CALL_INPARAM(param)								\
		if(((shtps_get_logflag() & 0x02) != 0) || (SHTPS_TOUCH_EVENTLOG_LOG_ENABLE != 0)){	\
			DBG_PRINTK("[eventlog] %s(%d)\n", __func__, param);								\
		}
#endif /* #if defined ( SHTPS_TOUCH_EVENTLOG_ENABLE ) */

/* -----------------------------------------------------------------------------------
 */
#if defined ( SHTPS_TOUCH_EMURATOR_ENABLE )
	enum{
		SHTPS_TOUCH_EMURATOR_STATE_DISABLE = 0,
		SHTPS_TOUCH_EMURATOR_STATE_WAITING,
		SHTPS_TOUCH_EMURATOR_STATE_RUNNING,
		SHTPS_TOUCH_EMURATOR_STATE_RECORDING,
	};

	struct shtps_touch_emurator_list_elem{
		unsigned int	generation_time_ms;
		unsigned char	finger_id;
		unsigned short	finger_x;
		unsigned short	finger_y;
		unsigned char	finger_state;
		unsigned char	finger_wx;
		unsigned char	finger_wy;
		unsigned char	finger_z;
	};

	struct shtps_touch_emurator_info{
		int										state;
		struct timeval							emu_start_timeval;
		int										touch_list_buffering_size;
		int										touch_list_num;
		int										current_touch_index;
		struct shtps_touch_emurator_list_elem	*touch_list;
		struct delayed_work						touch_emu_read_touchevent_delayed_work;
		struct shtps_touch_info					fw_report_info;

		struct file								*touchLogFilep;
		int										rec_num;
		struct timeval							rec_start_timeval;
	};
#endif /* #if defined ( SHTPS_TOUCH_EMURATOR_ENABLE ) */


#if defined ( SHTPS_TOUCH_EVENTLOG_ENABLE )
	enum{
		SHTPS_TOUCH_EVENTLOG_STATE_DISABLE = 0,
		SHTPS_TOUCH_EVENTLOG_STATE_RECORDING,
	};

	struct shtps_touch_eventlog_info{
		int										state;

		struct file								*touchLogFilep;
		int										rec_num;
		struct timeval							rec_start_timeval;
	};
#endif /* #if defined ( SHTPS_TOUCH_EVENTLOG_ENABLE ) */

#if defined ( SHTPS_ACCUMULATE_FW_REPORT_INFO_ENABLE )
	#define SHTPS_ACCUMULATE_FW_REPORT_INFO_MAX			10
	#define SHTPS_ACCUMULATE_FW_REPORT_INFO_SHOW_MAX	10

	struct shtps_accumulate_fw_report_info{
		struct timeval				start_timeval;
		int							index_st;
		int							count;
		int							fw_report_info_current_id;
		int							fw_report_info_id[SHTPS_ACCUMULATE_FW_REPORT_INFO_MAX];
		unsigned long				fw_report_info_time[SHTPS_ACCUMULATE_FW_REPORT_INFO_MAX];
		struct shtps_touch_info		fw_report_info[SHTPS_ACCUMULATE_FW_REPORT_INFO_MAX];
	};
#endif /* #if defined ( SHTPS_ACCUMULATE_FW_REPORT_INFO_ENABLE ) */


/* -----------------------------------------------------------------------------------
 */
#if defined ( SHTPS_TOUCH_EMURATOR_ENABLE )
	struct shtps_touch_emurator_info gShtps_touch_emu_info;
#endif /* #if defined ( SHTPS_TOUCH_EMURATOR_ENABLE ) */

#if defined ( SHTPS_TOUCH_EVENTLOG_ENABLE )
	struct shtps_touch_eventlog_info gShtps_touch_eventlog_info;
#endif /* #if defined ( SHTPS_TOUCH_EVENTLOG_ENABLE ) */

#if defined ( SHTPS_ACCUMULATE_FW_REPORT_INFO_ENABLE )
	SHTPS_PARAM_DEF( SHTPS_ACCUMULATE_FW_REPORT_INFO_ONOFF, 				0);
	static DEFINE_MUTEX(shtps_accumulate_fw_report_info_lock);
	struct shtps_accumulate_fw_report_info *gShtps_accumulate_fw_report_info_p;
#endif /* #if defined ( SHTPS_ACCUMULATE_FW_REPORT_INFO_ENABLE ) */

/* -----------------------------------------------------------------------------------
 */
#if defined ( SHTPS_TOUCH_EMURATOR_ENABLE )
static int shtps_touch_emu_read_touchevent_timer_start(unsigned long delay_sec)
{
	SHTPS_TOUCH_EMU_LOG_FUNC_CALL_INPARAM((int)delay_sec);

	cancel_delayed_work(&gShtps_touch_emu_info.touch_emu_read_touchevent_delayed_work);
	schedule_delayed_work(&gShtps_touch_emu_info.touch_emu_read_touchevent_delayed_work, msecs_to_jiffies(delay_sec * 1000));

	return 0;
}

static int shtps_touch_emu_read_touchevent_timer_stop(void)
{
	SHTPS_TOUCH_EMU_LOG_FUNC_CALL();

	cancel_delayed_work(&gShtps_touch_emu_info.touch_emu_read_touchevent_delayed_work);

	return 0;
}

static void shtps_touch_emu_read_touchevent_delayed_work_function(struct work_struct *work)
{
	struct timeval current_timeval;
	unsigned long generation_time_us;
	unsigned long specified_time_us;
	unsigned long next_event_time_us;
	unsigned long next_event_time_total_us;
	int ret;
	
	int sleepcount;

	SHTPS_TOUCH_EMU_LOG_DBG_PRINT("shtps_touch_emu_read_touchevent_delayed_work_function() start\n");

	if(gShtps_touch_emu_info.state == SHTPS_TOUCH_EMURATOR_STATE_WAITING){
		gShtps_touch_emu_info.state = SHTPS_TOUCH_EMURATOR_STATE_RUNNING;

		do_gettimeofday(&gShtps_touch_emu_info.emu_start_timeval);
	}

	while(gShtps_touch_emu_info.state == SHTPS_TOUCH_EMURATOR_STATE_RUNNING){
		/* call read touchevent function */
		ret = shtps_read_touchevent_from_outside();
		if(ret < 0){
			gShtps_touch_emu_info.state = SHTPS_TOUCH_EMURATOR_STATE_DISABLE;
			SHTPS_TOUCH_EMU_LOG_DBG_PRINT("touch emurate end by state not active\n");
		}
		else if(gShtps_touch_emu_info.state == SHTPS_TOUCH_EMURATOR_STATE_DISABLE){
			SHTPS_TOUCH_EMU_LOG_DBG_PRINT("touch emurate end\n");
		}

		if(gShtps_touch_emu_info.state != SHTPS_TOUCH_EMURATOR_STATE_RUNNING){
			break;
		}

		generation_time_us = gShtps_touch_emu_info.touch_list[gShtps_touch_emu_info.current_touch_index].generation_time_ms * 1000;

		next_event_time_total_us = 0;

		sleepcount = 0;

		while(gShtps_touch_emu_info.state == SHTPS_TOUCH_EMURATOR_STATE_RUNNING){
		
			do_gettimeofday(&current_timeval);
			
			specified_time_us = ((current_timeval.tv_sec - gShtps_touch_emu_info.emu_start_timeval.tv_sec) * 1000000)
								 + ((current_timeval.tv_usec - gShtps_touch_emu_info.emu_start_timeval.tv_usec));

			if(generation_time_us > specified_time_us){
				next_event_time_us = generation_time_us - specified_time_us;
				if(next_event_time_us > (1 * 1000 * 1000)){
					next_event_time_us = (1 * 1000 * 1000);
				}
				next_event_time_total_us += next_event_time_us;

				usleep_range(next_event_time_us, next_event_time_us);

				sleepcount++;
			}
			else{
				SHTPS_TOUCH_EMU_LOG_V("generation_time_us=%lu, specified_time_us=%lu, next_event_time_total_us=%lu sleepcount=%d\n",
					generation_time_us,
					specified_time_us,
					next_event_time_total_us,
					sleepcount
				);
				break;
			}
		}
	}

	if(gShtps_touch_emu_info.touch_list != NULL){
		kfree(gShtps_touch_emu_info.touch_list);
		gShtps_touch_emu_info.touch_list = NULL;
	}
	gShtps_touch_emu_info.touch_list_num = 0;
	gShtps_touch_emu_info.touch_list_buffering_size = 0;
	gShtps_touch_emu_info.current_touch_index = 0;
	SHTPS_TOUCH_EMU_LOG_DBG_PRINT("shtps_touch_emu_read_touchevent_delayed_work_function() end\n");
}

int shtps_touch_emu_is_running(void)
{
	if(gShtps_touch_emu_info.state == SHTPS_TOUCH_EMURATOR_STATE_RUNNING){
		return 1;
	}
	return 0;
}

int shtps_touch_emu_set_finger_info(u8 *buf, int bufsize)
{
	int				i;
	struct shtps_touch_emurator_list_elem	*touch_list;
	int				set_data_flg = 0;
	unsigned long	set_data_time = 0;
	struct timeval current_timeval;
	unsigned long specified_time_ms;
	struct fingers *fw_repot_finger_tmp;
	int				td_finger_num = 0;
	
	SHTPS_TOUCH_EMU_LOG_V("shtps_touch_emu_set_finger_info() start\n");

	do_gettimeofday(&current_timeval);
	
	specified_time_ms = ((current_timeval.tv_sec - gShtps_touch_emu_info.emu_start_timeval.tv_sec) * 1000)
						 + ((current_timeval.tv_usec - gShtps_touch_emu_info.emu_start_timeval.tv_usec) / 1000);

	for(i = gShtps_touch_emu_info.current_touch_index; i < gShtps_touch_emu_info.touch_list_num; i++){
		touch_list = &gShtps_touch_emu_info.touch_list[i];
		if(touch_list->generation_time_ms <= specified_time_ms){
			if((set_data_flg != 0) && (set_data_time != touch_list->generation_time_ms)){
				break;
			}
			else{
				fw_repot_finger_tmp = &gShtps_touch_emu_info.fw_report_info.fingers[touch_list->finger_id];
				fw_repot_finger_tmp->state	= touch_list->finger_state;
				fw_repot_finger_tmp->x		= touch_list->finger_x;
				fw_repot_finger_tmp->y		= touch_list->finger_y;
				fw_repot_finger_tmp->z		= touch_list->finger_z;
				fw_repot_finger_tmp->wx		= touch_list->finger_wx;
				fw_repot_finger_tmp->wy		= touch_list->finger_wy;

				set_data_flg = 1;

				set_data_time = touch_list->generation_time_ms;

				gShtps_touch_emu_info.current_touch_index++;
				SHTPS_TOUCH_EMU_LOG_V("touch emu set time=%u [%d] state=%d x=%d y=%d wx=%d wy=%d z=%d\n",
										touch_list->generation_time_ms,
										touch_list->finger_id,
										touch_list->finger_state,
										touch_list->finger_x,
										touch_list->finger_y,
										touch_list->finger_wx,
										touch_list->finger_wy,
										touch_list->finger_z
										);
			}
		}
		else{
			break;
		}
	}

	td_finger_num = 0;
	memset(buf, 0xFF, 3 + 6 * SHTPS_FINGER_MAX);
	for(i = 0; i < SHTPS_FINGER_MAX; i++){
		fw_repot_finger_tmp = &gShtps_touch_emu_info.fw_report_info.fingers[i];
		if(fw_repot_finger_tmp->state != SHTPS_TOUCH_STATE_NO_TOUCH){
			u8 *TOUCH_DATA = buf + 3 + (6 * td_finger_num);
			SET_TOUCH_DATA_FINGERSTATE(	TOUCH_DATA,	((fw_repot_finger_tmp->state == SHTPS_TOUCH_STATE_FINGER) ? 0 : 3));
			SET_TOUCH_DATA_XPOS(		TOUCH_DATA,	fw_repot_finger_tmp->x);
			SET_TOUCH_DATA_YPOS(		TOUCH_DATA,	fw_repot_finger_tmp->y);
			SET_TOUCH_DATA_ID(			TOUCH_DATA,	i);
			SET_TOUCH_DATA_WEIGHT(		TOUCH_DATA,	fw_repot_finger_tmp->z);
			SET_TOUCH_DATA_AREA(		TOUCH_DATA,	(fw_repot_finger_tmp->wx > fw_repot_finger_tmp->wy ? fw_repot_finger_tmp->wx : fw_repot_finger_tmp->wy));
			td_finger_num++;
		}
	}
	buf[0] = 0;
	buf[1] = 0;
	buf[2] = td_finger_num;

//	SHTPS_TOUCH_EMU_LOG_V("0x%02X 0x%02X 0x%02X | 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X | 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\n", 
//							buf[0],buf[1],buf[2],buf[3],buf[4],buf[5],buf[6],buf[7],buf[8],buf[9],buf[10],buf[11],buf[12],buf[13],buf[14]);

	if(gShtps_touch_emu_info.current_touch_index >= gShtps_touch_emu_info.touch_list_num){
		gShtps_touch_emu_info.state = SHTPS_TOUCH_EMURATOR_STATE_DISABLE;
	}
	SHTPS_TOUCH_EMU_LOG_V("shtps_touch_emu_set_finger_info() end current index=(%d/%d)\n", gShtps_touch_emu_info.current_touch_index, gShtps_touch_emu_info.touch_list_num);
	return 0;
}


int shtps_touch_emu_is_recording(void)
{
	if(gShtps_touch_emu_info.state == SHTPS_TOUCH_EMURATOR_STATE_RECORDING){
		return 1;
	}
	return 0;
}

static void shtps_touch_emu_recording_data(unsigned char *buffer, ssize_t size)
{
	mm_segment_t fs;
	int nr_write;

	if(gShtps_touch_emu_info.touchLogFilep){
		fs = get_fs();
		set_fs(get_ds());

		nr_write = gShtps_touch_emu_info.touchLogFilep->f_op->write(gShtps_touch_emu_info.touchLogFilep, buffer, size, &gShtps_touch_emu_info.touchLogFilep->f_pos);

		set_fs(fs);
	}
}

int shtps_touch_emu_rec_finger_info(u8 *buf)
{
	int				i;
	struct timeval	current_timeval;
	unsigned long	recording_time_ms;
	u8				*fingerInfo;
	struct shtps_touch_info	fw_report_info_tmp;
	u8				strbuf[100];
	int				strsize;
	struct shtps_fts	*ts = gShtps_fts;
	int					fingerMax = shtps_get_fingermax(ts);
	int					point_num = shtps_fwctl_get_num_of_touch_fingers(ts, buf);
	int					pointid;

	if(gShtps_touch_emu_info.state != SHTPS_TOUCH_EMURATOR_STATE_RECORDING){
		SHTPS_TOUCH_EMU_LOG_DBG_PRINT( "touch is not recording.\n" );
		return -1;
	}

	SHTPS_TOUCH_EMU_LOG_V("shtps_touch_emu_rec_finger_info() start\n");
	
	if(gShtps_touch_emu_info.rec_num == 0){
		do_gettimeofday(&gShtps_touch_emu_info.rec_start_timeval);
	}

	do_gettimeofday(&current_timeval);
	recording_time_ms = ((current_timeval.tv_sec - gShtps_touch_emu_info.rec_start_timeval.tv_sec) * 1000)
						 + ((current_timeval.tv_usec - gShtps_touch_emu_info.rec_start_timeval.tv_usec) / 1000);


	memset(&fw_report_info_tmp, 0, sizeof(fw_report_info_tmp));
	for(i = 0; i < point_num; i++){
		fingerInfo = shtps_fwctl_get_finger_info_buf(ts, i, fingerMax, buf);
		pointid = shtps_fwctl_get_finger_pointid(ts, fingerInfo);
		if (pointid >= fingerMax){
			break;
		}

		fw_report_info_tmp.fingers[pointid].state	= shtps_fwctl_get_finger_state(ts, i, fingerMax, buf);
		fw_report_info_tmp.fingers[pointid].x		= shtps_fwctl_get_finger_pos_x(ts, fingerInfo);
		fw_report_info_tmp.fingers[pointid].y		= shtps_fwctl_get_finger_pos_y(ts, fingerInfo);
		fw_report_info_tmp.fingers[pointid].wx		= shtps_fwctl_get_finger_wx(ts, fingerInfo);
		fw_report_info_tmp.fingers[pointid].wy		= shtps_fwctl_get_finger_wy(ts, fingerInfo);
		fw_report_info_tmp.fingers[pointid].z		= shtps_fwctl_get_finger_z(ts, fingerInfo);
	}

	for(i = 0; i < fingerMax; i++){
		if(	(fw_report_info_tmp.fingers[i].state	!= gShtps_touch_emu_info.fw_report_info.fingers[i].state) ||
			(fw_report_info_tmp.fingers[i].x		!= gShtps_touch_emu_info.fw_report_info.fingers[i].x	) ||
			(fw_report_info_tmp.fingers[i].y		!= gShtps_touch_emu_info.fw_report_info.fingers[i].y	) ||
			(fw_report_info_tmp.fingers[i].wx		!= gShtps_touch_emu_info.fw_report_info.fingers[i].wx	) ||
			(fw_report_info_tmp.fingers[i].wy		!= gShtps_touch_emu_info.fw_report_info.fingers[i].wy	) ||
			(fw_report_info_tmp.fingers[i].z		!= gShtps_touch_emu_info.fw_report_info.fingers[i].z	) ){

			// time,state,id,x,y,wx,wy,z
			strsize = sprintf(strbuf, "%lu,%d,%d,%d,%d,%d,%d,%d\n",
								recording_time_ms,
								(fw_report_info_tmp.fingers[i].state),
								i,
								(fw_report_info_tmp.fingers[i].x	),
								(fw_report_info_tmp.fingers[i].y	),
								(fw_report_info_tmp.fingers[i].wx	),
								(fw_report_info_tmp.fingers[i].wy	),
								(fw_report_info_tmp.fingers[i].z	) );

			shtps_touch_emu_recording_data(strbuf, strsize);
			SHTPS_TOUCH_EMU_LOG_V("shtps_touch_emu_rec_finger_info() add %s", strbuf);

			gShtps_touch_emu_info.rec_num++;
		}
	}

	memcpy(&gShtps_touch_emu_info.fw_report_info, &fw_report_info_tmp, sizeof(fw_report_info_tmp));

	SHTPS_TOUCH_EMU_LOG_V("shtps_touch_emu_rec_finger_info() end current rectime=%lu ms\n", recording_time_ms);
	return 0;
}


static int shtps_touch_emu_start_recording(u8 *fileName)
{
	int ret = 0;
	if(!gShtps_touch_emu_info.touchLogFilep){
		gShtps_touch_emu_info.touchLogFilep = filp_open(fileName, O_RDWR | O_CREAT | O_TRUNC, 0);
		if(IS_ERR(gShtps_touch_emu_info.touchLogFilep)){
			SHTPS_TOUCH_EMU_LOG_DBG_PRINT("touch record file open [%s] error!\n", fileName);
			gShtps_touch_emu_info.touchLogFilep = NULL;
			ret = -EINVAL;
		}
		else{
			SHTPS_TOUCH_EMU_LOG_DBG_PRINT("touch record file open [%s]\n", fileName);

			gShtps_touch_emu_info.state = SHTPS_TOUCH_EMURATOR_STATE_RECORDING;
			memset(&gShtps_touch_emu_info.fw_report_info, 0, sizeof(gShtps_touch_emu_info.fw_report_info));

			gShtps_touch_emu_info.rec_num = 0;
			ret = 0;
		}
	}
	else{
		SHTPS_TOUCH_EMU_LOG_DBG_PRINT("touch record file already open!\n");
		ret = -1;
	}
	return ret;
}
static int shtps_touch_emu_stop_recording(void)
{
	if(gShtps_touch_emu_info.touchLogFilep){
		filp_close(gShtps_touch_emu_info.touchLogFilep, NULL);
		gShtps_touch_emu_info.touchLogFilep = NULL;

		gShtps_touch_emu_info.state = SHTPS_TOUCH_EMURATOR_STATE_DISABLE;
		SHTPS_TOUCH_EMU_LOG_DBG_PRINT("touch record file close. rec_num=%d\n", gShtps_touch_emu_info.rec_num);
	}
	return 0;
}




static int shtps_get_arguments(
	char	*argStr,		/* [I/O] arguments strings (processed in function) */
	char	**argList,		/* [I/O] arguments pointer output buffer */
	int		argListMaxSize	/* [I/ ] arguments list size */
)
{
	int i;
	int argListNum = 0;
	int isParam;

	if((argStr == NULL) || (argList == NULL) || (argListMaxSize < 1)){
		return 0;
	}

	isParam = 0;

	for(i = 0; i < PAGE_SIZE; i++){
		if( (argStr[i] == '\0') ){
			if(isParam == 1){
				argListNum++;
			}
			break;
		}
		else if( (argStr[i] == '\n') || (argStr[i] == ',') || (argStr[i] == ' ') ){
			argStr[i] = '\0';
			if(isParam == 1){
				argListNum++;
			}
			isParam = 0;
			if(argListNum >= argListMaxSize){
				break;
			}
			continue;
		}
		else{
			if(isParam == 0){
				isParam = 1;
				argList[argListNum] = &argStr[i];
			}
		}
	}

	return argListNum;
}

static
int shtps_sysfs_start_touch_emurator(const char *buf, size_t count)
{
	u8 *data;

	if(NULL == buf || 0 == count){
		if(gShtps_touch_emu_info.touch_list == NULL){
			SHTPS_TOUCH_EMU_LOG_DBG_PRINT("touch emu no list data! please set data.\n");
			return -EINVAL;
		}
	}
	else{
		data = (u8*)kmalloc(count, GFP_KERNEL);
		if(data == NULL){
			return -ENOMEM;
		}
		memcpy(data, buf, count);

		gShtps_touch_emu_info.touch_list_num = count / sizeof(struct shtps_touch_emurator_list_elem);
		if(gShtps_touch_emu_info.touch_list != NULL){
			kfree(gShtps_touch_emu_info.touch_list);
		}
		gShtps_touch_emu_info.touch_list = (struct shtps_touch_emurator_list_elem *)data;
	}

	gShtps_touch_emu_info.current_touch_index = 0;
	gShtps_touch_emu_info.state = SHTPS_TOUCH_EMURATOR_STATE_WAITING;

	{
		int i;
		struct shtps_touch_emurator_list_elem *list_elem;
		for(i = 0; i < gShtps_touch_emu_info.touch_list_num; i++){
			list_elem = &gShtps_touch_emu_info.touch_list[i];
			SHTPS_TOUCH_EMU_LOG_DBG_PRINT("time=%u state[%d] Touch Info[%d] x=%d, y=%d, wx=%d, wy=%d, z=%d\n",
					list_elem->generation_time_ms,
					list_elem->finger_state,
					list_elem->finger_id,
					list_elem->finger_x,
					list_elem->finger_y,
					list_elem->finger_wx,
					list_elem->finger_wy,
					list_elem->finger_z);
		}
	}
	SHTPS_TOUCH_EMU_LOG_DBG_PRINT("touch emu start list_num=%d\n",
							gShtps_touch_emu_info.touch_list_num);

	memset(&gShtps_touch_emu_info.fw_report_info, 0, sizeof(gShtps_touch_emu_info.fw_report_info));

	shtps_touch_emu_read_touchevent_timer_stop();
	shtps_touch_emu_read_touchevent_timer_start(SHTPS_TOUCH_EMURATOR_START_WAIT_TIME_SEC);

	return 0;
}

static
int shtps_sysfs_stop_touch_emurator(void)
{
	SHTPS_TOUCH_EMU_LOG_DBG_PRINT("stop touch emurator. state(%d => %d)\n", gShtps_touch_emu_info.state, SHTPS_TOUCH_EMURATOR_STATE_DISABLE);
	shtps_touch_emu_read_touchevent_timer_stop();
	gShtps_touch_emu_info.state = SHTPS_TOUCH_EMURATOR_STATE_DISABLE;

	return 0;
}


static
ssize_t show_sysfs_emurator_ctrl(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	sprintf(buf, "emurator state=%d\n", gShtps_touch_emu_info.state);
	
	return( strlen(buf) );
}

static
ssize_t store_sysfs_emurator_ctrl(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	int ret = 0;
	char *argbuf;
	int argc;
	char *argv[10];

	
	argbuf = (char*)kmalloc(count, GFP_KERNEL);
	if(argbuf == NULL){
		SHTPS_TOUCH_EMU_LOG_DBG_PRINT( "argbuf alloc error.\n" );
		return( count );
	}
	memcpy(argbuf, buf, count);

	argc = shtps_get_arguments( argbuf, argv, sizeof(argv)/sizeof(char *) );

	SHTPS_TOUCH_EMU_LOG_DBG_PRINT( "store_sysfs_emurator_ctrl() call start command = %s\n", argv[0] );

	if(strcmp(argv[0], "start") == 0){
		if(gShtps_touch_emu_info.state == SHTPS_TOUCH_EMURATOR_STATE_DISABLE){
			// call emurator start function
			ret = shtps_sysfs_start_touch_emurator(NULL, 0);
		}
		else{
			SHTPS_TOUCH_EMU_LOG_DBG_PRINT( "emurator is already running.\n" );
		}
	}
	else if(strcmp(argv[0], "stop") == 0){
		shtps_sysfs_stop_touch_emurator();
	}
	else if(strcmp(argv[0], "clear") == 0){
		if(gShtps_touch_emu_info.state == SHTPS_TOUCH_EMURATOR_STATE_RUNNING){
			SHTPS_TOUCH_EMU_LOG_DBG_PRINT( "emurator is running. stop emurator and clear buffer.\n" );
			shtps_sysfs_stop_touch_emurator();
		}
		else{
			SHTPS_TOUCH_EMU_LOG_DBG_PRINT( "clear emurator data buffer. %d byte(list num = %d) => 0\n",
								gShtps_touch_emu_info.touch_list_buffering_size,
								gShtps_touch_emu_info.touch_list_num );
			if(gShtps_touch_emu_info.touch_list != NULL){
				kfree(gShtps_touch_emu_info.touch_list);
				gShtps_touch_emu_info.touch_list = NULL;
			}
			gShtps_touch_emu_info.touch_list_num = 0;
			gShtps_touch_emu_info.touch_list_buffering_size = 0;
		}
	}
	else if(strcmp(argv[0], "start_wait_time") == 0){
		if(argc >= 2){
			SHTPS_TOUCH_EMURATOR_START_WAIT_TIME_SEC = simple_strtol(argv[1], NULL, 0);
			SHTPS_TOUCH_EMU_LOG_DBG_PRINT( "set emurator start wait time = %d(sec)\n", SHTPS_TOUCH_EMURATOR_START_WAIT_TIME_SEC );
		}
		else{
			SHTPS_TOUCH_EMU_LOG_DBG_PRINT( "few parameters!\n" );
		}
	}
	else{
		SHTPS_TOUCH_EMU_LOG_DBG_PRINT( "command [%s] is not support!\n", argv[0] );
	}

	SHTPS_TOUCH_EMU_LOG_DBG_PRINT( "store_sysfs_emurator_ctrl() call end\n" );
	kfree(argbuf);

	if(ret < 0){
		return ret;
	}
	return( count );
}


static
ssize_t show_sysfs_emurator_data(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	sprintf(buf, "emurator_data_size = %d byte(list num = %d)\n",
							gShtps_touch_emu_info.touch_list_buffering_size,
							gShtps_touch_emu_info.touch_list_num );
	
	return( strlen(buf) );
}

static
ssize_t store_sysfs_emurator_data(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	char *data;
	int copy_size;

	SHTPS_TOUCH_EMU_LOG_DBG_PRINT( "store_sysfs_emurator_data() call start count = %d\n", (int)count );

	copy_size = count;

	if(gShtps_touch_emu_info.state == SHTPS_TOUCH_EMURATOR_STATE_DISABLE){
		// buffering data
		if(gShtps_touch_emu_info.touch_list == NULL){
			gShtps_touch_emu_info.touch_list_num = 0;
			gShtps_touch_emu_info.touch_list_buffering_size = 0;

			data = (char *)kmalloc(copy_size, GFP_KERNEL);
			if(data == NULL){
				return -ENOMEM;
			}
			memcpy(data, buf, copy_size);
		}
		else{
			data = (char *)kmalloc(gShtps_touch_emu_info.touch_list_buffering_size + copy_size, GFP_KERNEL);
			if(data == NULL){
				return -ENOMEM;
			}
			memcpy(data, gShtps_touch_emu_info.touch_list, gShtps_touch_emu_info.touch_list_buffering_size);
			memcpy(data + gShtps_touch_emu_info.touch_list_buffering_size, buf, copy_size);
			kfree(gShtps_touch_emu_info.touch_list);
		}
		gShtps_touch_emu_info.touch_list = (struct shtps_touch_emurator_list_elem *)data;
		gShtps_touch_emu_info.touch_list_buffering_size += copy_size;
		gShtps_touch_emu_info.touch_list_num = gShtps_touch_emu_info.touch_list_buffering_size / sizeof(struct shtps_touch_emurator_list_elem);

		SHTPS_TOUCH_EMU_LOG_DBG_PRINT( "store_sysfs_emurator_data() call end current size = %d byte(list num = %d)\n",
								gShtps_touch_emu_info.touch_list_buffering_size,
								gShtps_touch_emu_info.touch_list_num );
	}
	else{
		SHTPS_TOUCH_EMU_LOG_DBG_PRINT( "emurator is running. not set data.\n" );
	}

	return( copy_size );
}

static
ssize_t store_sysfs_emurator_record_start(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	int ret = 0;
	char *argbuf;
	int argc;
	char *argv[10];

	argbuf = (char*)kmalloc(count, GFP_KERNEL);
	if(argbuf == NULL){
		SHTPS_TOUCH_EMU_LOG_DBG_PRINT( "argbuf alloc error.\n" );
		return -ENOMEM;
	}
	memcpy(argbuf, buf, count);

	argc = shtps_get_arguments( argbuf, argv, sizeof(argv)/sizeof(char *) );

	SHTPS_TOUCH_EMU_LOG_DBG_PRINT( "store_sysfs_emurator_record_start() call start\n" );

	if(gShtps_touch_emu_info.state == SHTPS_TOUCH_EMURATOR_STATE_DISABLE){
		if(argc >= 1){
			ret = shtps_touch_emu_start_recording(argv[0]);
		}
		else{
			SHTPS_TOUCH_EMU_LOG_DBG_PRINT( "touch recording not start because few paramters.\n" );
			ret = -EINVAL;
		}
	}
	else{
		SHTPS_TOUCH_EMU_LOG_DBG_PRINT( "touch recording not start because emurator is running.\n" );
		ret = -1;
	}

	kfree(argbuf);
	if(ret < 0){
		return ret;
	}
	return( count );
}

static
ssize_t store_sysfs_emurator_record_stop(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	int ret = 0;

	SHTPS_TOUCH_EMU_LOG_DBG_PRINT( "store_sysfs_emurator_record_stop() call start\n" );
	ret = shtps_touch_emu_stop_recording();

	if(ret < 0){
		return ret;
	}
	return( count );
}

struct shtps_debug_param_regist_info emurator_sysfs_regist_info[] = {
	{"emurator", SHTPS_DEBUG_PARAM_NAME_LOG, &SHTPS_TOUCH_EMURATOR_LOG_ENABLE, NULL, NULL},
	{"emurator", "ctrl", NULL, show_sysfs_emurator_ctrl, store_sysfs_emurator_ctrl},
	{"emurator", "data", NULL, show_sysfs_emurator_data, store_sysfs_emurator_data},
	{"emurator", "start_wait_time", &SHTPS_TOUCH_EMURATOR_START_WAIT_TIME_SEC, NULL, NULL},
	{"emurator", "record_start", NULL, NULL, store_sysfs_emurator_record_start},
	{"emurator", "record_stop", NULL, NULL, store_sysfs_emurator_record_stop},

	{NULL, NULL, NULL, NULL, NULL}
};
#endif /* #if defined ( SHTPS_TOUCH_EMURATOR_ENABLE ) */


#if defined ( SHTPS_TOUCH_EVENTLOG_ENABLE )
int shtps_touch_eventlog_is_recording(void)
{
	if(gShtps_touch_eventlog_info.state == SHTPS_TOUCH_EVENTLOG_STATE_RECORDING){
		return 1;
	}
	return 0;
}

static void shtps_touch_eventlog_recording_data(unsigned char *buffer, ssize_t size)
{
	mm_segment_t fs;
	int nr_write;

	if(gShtps_touch_eventlog_info.touchLogFilep){
		fs = get_fs();
		set_fs(get_ds());

		nr_write = gShtps_touch_eventlog_info.touchLogFilep->f_op->write(gShtps_touch_eventlog_info.touchLogFilep, buffer, size, &gShtps_touch_eventlog_info.touchLogFilep->f_pos);

		set_fs(fs);
	}
}
int shtps_touch_eventlog_rec_event_info(int state, int finger, int x, int y, int w, int wx, int wy, int z)
{
	struct timeval	current_timeval;
	unsigned long	recording_time_ms;
	u8				strbuf[100];
	int				strsize;
	
	if(gShtps_touch_eventlog_info.state != SHTPS_TOUCH_EVENTLOG_STATE_RECORDING){
		SHTPS_TOUCH_EVENTLOG_LOG_DBG_PRINT( "touch eventlog is not recording.\n" );
		return -1;
	}

	if(gShtps_touch_eventlog_info.rec_num == 0){
		do_gettimeofday(&gShtps_touch_eventlog_info.rec_start_timeval);
	}

	do_gettimeofday(&current_timeval);
	recording_time_ms = ((current_timeval.tv_sec - gShtps_touch_eventlog_info.rec_start_timeval.tv_sec) * 1000)
						 + ((current_timeval.tv_usec - gShtps_touch_eventlog_info.rec_start_timeval.tv_usec) / 1000);

	
	// time,state,id,x,y,w,wx,wy,z
	strsize = sprintf(strbuf, "%lu,%d,%d,%d,%d,%d,%d,%d,%d\n",
						recording_time_ms,
						state,
						finger,
						x,
						y,
						w,
						wx,
						wy,
						z);

	shtps_touch_eventlog_recording_data(strbuf, strsize);
	SHTPS_TOUCH_EVENTLOG_LOG_V("shtps_touch_eventlog_recording_data() add %s", strbuf);

	gShtps_touch_eventlog_info.rec_num++;

	SHTPS_TOUCH_EVENTLOG_LOG_V("shtps_touch_eventlog_rec_event_info() current rectime=%lu ms\n", recording_time_ms);
	return 0;
}

static int shtps_eventlog_start_recording(u8 *fileName)
{
	int ret = 0;
	if(!gShtps_touch_eventlog_info.touchLogFilep){
		gShtps_touch_eventlog_info.touchLogFilep = filp_open(fileName, O_RDWR | O_CREAT | O_TRUNC, 0);
		if(IS_ERR(gShtps_touch_eventlog_info.touchLogFilep)){
			SHTPS_TOUCH_EVENTLOG_LOG_DBG_PRINT("touch eventlog record file open [%s] error!\n", fileName);
			gShtps_touch_eventlog_info.touchLogFilep = NULL;
			ret = -EINVAL;
		}
		else{
			SHTPS_TOUCH_EVENTLOG_LOG_DBG_PRINT("touch eventlog record file open [%s]\n", fileName);

			gShtps_touch_eventlog_info.state = SHTPS_TOUCH_EVENTLOG_STATE_RECORDING;

			gShtps_touch_eventlog_info.rec_num = 0;
			ret = 0;
		}
	}
	else{
		SHTPS_TOUCH_EVENTLOG_LOG_DBG_PRINT("touch eventlog record file already open!\n");
		ret = -1;
	}
	return ret;
}
static int shtps_eventlog_stop_recording(void)
{
	if(gShtps_touch_eventlog_info.touchLogFilep){
		filp_close(gShtps_touch_eventlog_info.touchLogFilep, NULL);
		gShtps_touch_eventlog_info.touchLogFilep = NULL;

		gShtps_touch_eventlog_info.state = SHTPS_TOUCH_EVENTLOG_STATE_DISABLE;
		SHTPS_TOUCH_EVENTLOG_LOG_DBG_PRINT("touch eventlog record file close. rec_num=%d\n", gShtps_touch_eventlog_info.rec_num);
	}
	return 0;
}

static
ssize_t store_sysfs_eventlog_record_start(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	int ret = 0;
	char *argbuf;
	int argc;
	char *argv[10];

	argbuf = (char*)kmalloc(count, GFP_KERNEL);
	if(argbuf == NULL){
		SHTPS_TOUCH_EVENTLOG_LOG_DBG_PRINT( "argbuf alloc error.\n" );
		return -ENOMEM;
	}
	memcpy(argbuf, buf, count);

	argc = shtps_get_arguments( argbuf, argv, sizeof(argv)/sizeof(char *) );

	SHTPS_TOUCH_EVENTLOG_LOG_DBG_PRINT( "store_sysfs_eventlog_record_start() call start\n" );

	if(gShtps_touch_eventlog_info.state == SHTPS_TOUCH_EVENTLOG_STATE_DISABLE){
		if(argc >= 1){
			ret = shtps_eventlog_start_recording(argv[0]);
		}
		else{
			SHTPS_TOUCH_EVENTLOG_LOG_DBG_PRINT( "touch eventlog recording not start because few paramters.\n" );
			ret = -EINVAL;
		}
	}
	else{
		SHTPS_TOUCH_EVENTLOG_LOG_DBG_PRINT( "touch eventlog recording not start because already running.\n" );
		ret = -1;
	}

	kfree(argbuf);
	if(ret < 0){
		return ret;
	}
	return( count );
}

static
ssize_t store_sysfs_eventlog_record_stop(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	int ret = 0;

	SHTPS_TOUCH_EVENTLOG_LOG_DBG_PRINT( "store_sysfs_eventlog_record_stop() call start\n" );
	ret = shtps_eventlog_stop_recording();

	if(ret < 0){
		return ret;
	}
	return( count );
}

struct shtps_debug_param_regist_info eventlog_rec_sysfs_regist_info[] = {
	{"eventlog", SHTPS_DEBUG_PARAM_NAME_LOG, &SHTPS_TOUCH_EVENTLOG_LOG_ENABLE, NULL, NULL},
	{"eventlog", "record_start", NULL, NULL, store_sysfs_eventlog_record_start},
	{"eventlog", "record_stop", NULL, NULL, store_sysfs_eventlog_record_stop},

	{NULL, NULL, NULL, NULL, NULL}
};
#endif /* #if defined ( SHTPS_TOUCH_EVENTLOG_ENABLE ) */


#if defined ( SHTPS_ACCUMULATE_FW_REPORT_INFO_ENABLE )
static int shtps_accumulate_fw_report_info_push(struct shtps_accumulate_fw_report_info *mng_p, struct shtps_touch_info *touch_info, unsigned long time_us)
{
	int index_target;

	mutex_lock(&shtps_accumulate_fw_report_info_lock);

	index_target = mng_p->index_st + mng_p->count;
	index_target %= SHTPS_ACCUMULATE_FW_REPORT_INFO_MAX;

	if(mng_p->count > 0){
		if(index_target == mng_p->index_st){
			mng_p->index_st++;
			mng_p->index_st %= SHTPS_ACCUMULATE_FW_REPORT_INFO_MAX;
		}
	}

	if(mng_p->count < SHTPS_ACCUMULATE_FW_REPORT_INFO_MAX){
		mng_p->count++;
	}

	memcpy(&mng_p->fw_report_info[index_target], touch_info, sizeof(struct shtps_touch_info));
	mng_p->fw_report_info_id[index_target] = mng_p->fw_report_info_current_id;
	mng_p->fw_report_info_time[index_target] = time_us;
	mng_p->fw_report_info_current_id++;

	mutex_unlock(&shtps_accumulate_fw_report_info_lock);

	return 0;
}

static struct shtps_touch_info* shtps_accumulate_fw_report_info_pop(struct shtps_accumulate_fw_report_info *mng_p, int *id, unsigned long *time_us)
{
	struct shtps_touch_info *touch_info = NULL;

	mutex_lock(&shtps_accumulate_fw_report_info_lock);

	if(mng_p->count > 0){
		touch_info = &mng_p->fw_report_info[mng_p->index_st];
		if(id != NULL){
			*id = mng_p->fw_report_info_id[mng_p->index_st];
		}
		if(time_us != NULL){
			*time_us = mng_p->fw_report_info_time[mng_p->index_st];
		}
		mng_p->count--;
		mng_p->index_st++;
		mng_p->index_st %= SHTPS_ACCUMULATE_FW_REPORT_INFO_MAX;
	}

	mutex_unlock(&shtps_accumulate_fw_report_info_lock);

	return touch_info;
}

void shtps_accumulate_fw_report_info_add(struct shtps_touch_info *touch_info)
{
	struct timeval current_timeval;
	unsigned long specified_time_us;
	if(gShtps_accumulate_fw_report_info_p == NULL){
		return;
	}
	if(SHTPS_ACCUMULATE_FW_REPORT_INFO_ONOFF != 1){
		return;
	}
	if(touch_info == NULL){
		return;
	}

	do_gettimeofday(&current_timeval);
	specified_time_us = ((current_timeval.tv_sec - gShtps_accumulate_fw_report_info_p->start_timeval.tv_sec) * 1000000)
						 + ((current_timeval.tv_usec - gShtps_accumulate_fw_report_info_p->start_timeval.tv_usec));

	shtps_accumulate_fw_report_info_push(gShtps_accumulate_fw_report_info_p, touch_info, specified_time_us);
	return;
}

static
ssize_t show_sysfs_fw_report_info(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	int i;
	int cnt;
	static char workbuf[512];
	struct shtps_touch_info* info;
	int fingerMax = SHTPS_FINGER_MAX;
	int fw_report_info_id;
	int loopmax;
	unsigned long time_us;

	loopmax = gShtps_accumulate_fw_report_info_p->count;
	if(loopmax < SHTPS_ACCUMULATE_FW_REPORT_INFO_SHOW_MAX){
		loopmax = SHTPS_ACCUMULATE_FW_REPORT_INFO_SHOW_MAX;
	}

	buf[0] = '\0';
	for(cnt = 0; cnt < loopmax; cnt++){
		info = shtps_accumulate_fw_report_info_pop(gShtps_accumulate_fw_report_info_p, &fw_report_info_id, &time_us);
		if(info != NULL){
			for (i = 0; i < fingerMax; i++) {
				sprintf(workbuf, "[%d] time=%lu,id=%d,state=%d,x=%d,y=%d,wx=%d,wy=%d,z=%d\n",
									fw_report_info_id,
									time_us,
									i,
									info->fingers[i].state,
									info->fingers[i].x,
									info->fingers[i].y,
									info->fingers[i].wx,
									info->fingers[i].wy,
									info->fingers[i].z);
				strcat(buf, workbuf);
			}
		}
	}

	return( strlen(buf) );
}

struct shtps_debug_param_regist_info fw_report_info_sysfs_regist_info[] = {
	{"fw_report_info", SHTPS_DEBUG_PARAM_NAME_ONOFF, &SHTPS_ACCUMULATE_FW_REPORT_INFO_ONOFF, NULL, NULL},
	{"fw_report_info", "fw_report_info", NULL, show_sysfs_fw_report_info, NULL},

	{NULL, NULL, NULL, NULL, NULL}
};
#endif /* #if defined ( SHTPS_ACCUMULATE_FW_REPORT_INFO_ENABLE ) */

/* -----------------------------------------------------------------------------------
 */
#define SHTPS_DEBUG_PARAM_DIR_NAME_ROOT		"debug"
#define SHTPS_DEBUG_PARAM_DIR_NAME_PARAM	"parameter"

struct shtps_debug_info{
	struct list_head	kobj_list;
	struct list_head	attr_list;

};

static struct shtps_debug_info *gDegbuInfo;


struct shtps_debug_kobj_list_info{
	struct list_head	kobj_list;
	struct kobject		*parent_kobj_p;
	struct kobject		*kobj_p;
};

struct shtps_debug_attr_list_info{
	struct list_head		attr_list;
	struct kobject			*parent_kobj_p;
	struct kobj_attribute	*kobj_attr_p;
	struct kobj_attribute	kobj_attr;
	int						*param_p;
};

static struct kobject* shtps_debug_search_kobj(struct kobject *parent_kobj_p, const char *search_name)
{
	struct list_head *list_p;
	struct shtps_debug_kobj_list_info *item_info_p;
	struct kobject *find_kobj_p = NULL;

	list_for_each(list_p, &(gDegbuInfo->kobj_list)){
		item_info_p = list_entry(list_p, struct shtps_debug_kobj_list_info, kobj_list);

		if( strcmp(kobject_name(item_info_p->kobj_p), search_name) == 0 ){
			if(parent_kobj_p == NULL){
				find_kobj_p = item_info_p->kobj_p;
				break;
			}else{
				if(item_info_p->parent_kobj_p == parent_kobj_p){
					find_kobj_p = item_info_p->kobj_p;
					break;
				}
			}
		}
	}

	return find_kobj_p;
}

static struct shtps_debug_attr_list_info* shtps_debug_search_attr(struct kobject *parent_kobj_p, const char *search_name)
{
	struct list_head *list_p;
	struct shtps_debug_attr_list_info *item_info_p;
	struct shtps_debug_attr_list_info *find_attr_p = NULL;

	list_for_each(list_p, &(gDegbuInfo->attr_list)){
		item_info_p = list_entry(list_p, struct shtps_debug_attr_list_info, attr_list);

		if( strcmp(item_info_p->kobj_attr.attr.name, search_name) == 0 ){
			if(parent_kobj_p == NULL){
				find_attr_p = item_info_p;
				break;
			}else{
				if(item_info_p->parent_kobj_p == parent_kobj_p){
					find_attr_p = item_info_p;
					break;
				}
			}
		}
	}

	return find_attr_p;
}

static ssize_t shtps_debug_common_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	ssize_t count;
	struct shtps_debug_attr_list_info *find_attr_p = NULL;

	find_attr_p = shtps_debug_search_attr(kobj, attr->attr.name);

	if(find_attr_p == NULL){
		count = snprintf(buf, PAGE_SIZE, "-\n");
	}else if(find_attr_p->param_p == NULL){
		count = snprintf(buf, PAGE_SIZE, "-\n");
	}else{
		count = snprintf(buf, PAGE_SIZE, "%d\n", *(find_attr_p->param_p));
	}

	return count;
}

static ssize_t shtps_debug_common_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	struct shtps_debug_attr_list_info *find_attr_p = NULL;
	int val;

	sscanf(buf,"%d", &val);

	find_attr_p = shtps_debug_search_attr(kobj, attr->attr.name);

	if( (find_attr_p != NULL) && (find_attr_p->param_p != NULL) ){
		*(find_attr_p->param_p) = val;
	}

	return count;
}

static struct kobject* shtps_debug_create_kobj(struct kobject *parent_kobj_p, const char *name)
{
	struct shtps_debug_kobj_list_info *list_info_p = NULL;
	struct kobject *create_kobj_p = NULL;

	list_info_p = kzalloc(sizeof(struct shtps_debug_kobj_list_info), GFP_KERNEL);
	if(list_info_p != NULL){
		create_kobj_p = kobject_create_and_add(name, parent_kobj_p);
		if(create_kobj_p != NULL){
			list_info_p->kobj_p = create_kobj_p;
			list_info_p->parent_kobj_p = parent_kobj_p;
			list_add_tail(&(list_info_p->kobj_list), &(gDegbuInfo->kobj_list));
		}
		else{
			kfree(list_info_p);
		}
	}

	return create_kobj_p;
}

static void shtps_debug_delete_kobj_all(void)
{
	struct shtps_debug_kobj_list_info *item_info_p;

	while( list_empty( &(gDegbuInfo->kobj_list) ) == 0 ){
		item_info_p = list_entry(gDegbuInfo->kobj_list.next, struct shtps_debug_kobj_list_info, kobj_list);

		if(item_info_p != NULL){
			kobject_put(item_info_p->kobj_p);
			list_del_init(&(item_info_p->kobj_list));
			kfree(item_info_p);
		}
	}

	list_del_init(&(gDegbuInfo->kobj_list));
}

static int shtps_debug_create_attr(struct kobject *parent_kobj_p, struct kobj_attribute attr, int *param_p)
{
	struct shtps_debug_attr_list_info *list_info_p = NULL;
	int rc;

	list_info_p = kzalloc(sizeof(struct shtps_debug_attr_list_info), GFP_KERNEL);
	if(list_info_p != NULL){
		list_info_p->parent_kobj_p = parent_kobj_p;
		list_info_p->kobj_attr_p = &(list_info_p->kobj_attr);
		list_info_p->kobj_attr = attr;
		if(param_p == NULL){
			list_info_p->param_p = NULL;
		}else{
			list_info_p->param_p = param_p;
		}
		rc = sysfs_create_file(parent_kobj_p, &(list_info_p->kobj_attr.attr));
		if(rc == 0){
			list_add_tail(&(list_info_p->attr_list), &(gDegbuInfo->attr_list));
		}else{
			kfree(list_info_p);
		}
	}

	return 0;
}

static void shtps_debug_delete_attr_all(void)
{
	struct shtps_debug_attr_list_info *item_info_p;

	while( list_empty( &(gDegbuInfo->attr_list) ) == 0 ){
		item_info_p = list_entry(gDegbuInfo->attr_list.next, struct shtps_debug_attr_list_info, attr_list);

		if(item_info_p != NULL){
			sysfs_remove_file(item_info_p->parent_kobj_p, &(item_info_p->kobj_attr_p->attr));
			list_del_init(&(item_info_p->attr_list));
			kfree(item_info_p);
		}
	}

	list_del_init(&(gDegbuInfo->attr_list));
}

static int shtps_debug_param_obj_init(struct kobject *shtps_root_kobj)
{
	struct kobject *kobj_p;

	gDegbuInfo = (struct shtps_debug_info*)kmalloc(sizeof(struct shtps_debug_info), GFP_KERNEL);
	if(gDegbuInfo == NULL){
		return -1;
	}

	INIT_LIST_HEAD(&(gDegbuInfo->kobj_list));
	INIT_LIST_HEAD(&(gDegbuInfo->attr_list));

	if(shtps_root_kobj == NULL){
		return -1;
	}

	kobj_p = shtps_debug_create_kobj(shtps_root_kobj, SHTPS_DEBUG_PARAM_DIR_NAME_ROOT);
	if(kobj_p == NULL){
		return -1;
	}

	return 0;
}

static void shtps_debug_param_obj_deinit(void)
{
	shtps_debug_delete_attr_all();
	shtps_debug_delete_kobj_all();
	if(gDegbuInfo != NULL){
		kfree(gDegbuInfo);
		gDegbuInfo = NULL;
	}
}

int shtps_debug_param_add(struct shtps_debug_param_regist_info *info_p)
{
	int i;
	struct kobject *shtps_root_kobj_p = NULL;

	shtps_root_kobj_p = shtps_debug_search_kobj(NULL, SHTPS_DEBUG_PARAM_DIR_NAME_ROOT);

	if(shtps_root_kobj_p == NULL){
		return -1;
	}

	for(i = 0; info_p[i].param_name != NULL; i++)
	{
		struct kobject *parent_kobj_p = NULL;
		struct kobject *param_kobj_p = NULL;
		struct kobj_attribute kobj_attr;

		parent_kobj_p = shtps_debug_search_kobj(shtps_root_kobj_p, info_p[i].parent_name);
		if(parent_kobj_p == NULL){
			parent_kobj_p = shtps_debug_create_kobj(shtps_root_kobj_p, info_p[i].parent_name);
			if(parent_kobj_p == NULL){
				return -1;
			}
		}

		param_kobj_p = shtps_debug_search_kobj(parent_kobj_p, SHTPS_DEBUG_PARAM_DIR_NAME_PARAM);
		if(param_kobj_p == NULL){
			param_kobj_p = shtps_debug_create_kobj(parent_kobj_p, SHTPS_DEBUG_PARAM_DIR_NAME_PARAM);
			if(param_kobj_p == NULL){
				return -1;
			}
		}

		kobj_attr.attr.name = info_p[i].param_name;
#if defined(SHTPS_ENGINEER_BUILD_ENABLE)
		kobj_attr.attr.mode = (S_IRUGO | S_IWUGO);
#else
		kobj_attr.attr.mode = (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
#endif /* SHTPS_ENGINEER_BUILD_ENABLE */
		if(info_p[i].show == NULL){
			kobj_attr.show = shtps_debug_common_show;
		}else{
			kobj_attr.show = info_p[i].show;
		}

		if(info_p[i].store == NULL){
			kobj_attr.store = shtps_debug_common_store;
		}else{
			kobj_attr.store = info_p[i].store;
		}

		if( (strcmp(info_p[i].param_name, SHTPS_DEBUG_PARAM_NAME_ONOFF) == 0) ||
			(strcmp(info_p[i].param_name, SHTPS_DEBUG_PARAM_NAME_LOG) == 0) )
		{
			shtps_debug_create_attr(parent_kobj_p, kobj_attr, info_p[i].param_p);
		}else{
			shtps_debug_create_attr(param_kobj_p, kobj_attr, info_p[i].param_p);
		}
	}

	return 0;
}
/* -----------------------------------------------------------------------------------
 */
int shtps_debug_init(struct shtps_debug_init_param *param)
{
	if(!param){
		return -1;
	}

	/*  */
	shtps_debug_param_obj_init(param->shtps_root_kobj);

	#if defined ( SHTPS_TOUCH_EMURATOR_ENABLE )
		memset(&gShtps_touch_emu_info, 0, sizeof(gShtps_touch_emu_info));
		gShtps_touch_emu_info.state = SHTPS_TOUCH_EMURATOR_STATE_DISABLE;
		INIT_DELAYED_WORK(&gShtps_touch_emu_info.touch_emu_read_touchevent_delayed_work, shtps_touch_emu_read_touchevent_delayed_work_function);
		shtps_debug_param_add(emurator_sysfs_regist_info);
	#endif /* #if defined ( SHTPS_TOUCH_EMURATOR_ENABLE ) */

	#if defined ( SHTPS_TOUCH_EVENTLOG_ENABLE )
		memset(&gShtps_touch_eventlog_info, 0, sizeof(gShtps_touch_eventlog_info));
		gShtps_touch_eventlog_info.state = SHTPS_TOUCH_EVENTLOG_STATE_DISABLE;
		shtps_debug_param_add(eventlog_rec_sysfs_regist_info);
	#endif /* #if defined ( SHTPS_TOUCH_EVENTLOG_ENABLE ) */

	#if defined ( SHTPS_ACCUMULATE_FW_REPORT_INFO_ENABLE )
		gShtps_accumulate_fw_report_info_p = kzalloc(sizeof(struct shtps_accumulate_fw_report_info), GFP_KERNEL);
		if(gShtps_accumulate_fw_report_info_p != NULL){
			do_gettimeofday(&gShtps_accumulate_fw_report_info_p->start_timeval);
			shtps_debug_param_add(fw_report_info_sysfs_regist_info);
		}
	#endif /* #if defined ( SHTPS_ACCUMULATE_FW_REPORT_INFO_ENABLE ) */

	return 0;
}

void shtps_debug_deinit(void)
{
	#if defined ( SHTPS_TOUCH_EMURATOR_ENABLE )
		if(gShtps_touch_emu_info.touch_list != NULL){
			kfree(gShtps_touch_emu_info.touch_list);
		}
	#endif /* #if defined ( SHTPS_TOUCH_EMURATOR_ENABLE ) */

	/*  */
	shtps_debug_param_obj_deinit();
}

void shtps_debug_sleep_enter(void)
{
	#if defined ( SHTPS_TOUCH_EMURATOR_ENABLE )
		if(shtps_touch_emu_is_running() != 0){
			shtps_sysfs_stop_touch_emurator();
		}
		else if(shtps_touch_emu_is_recording() != 0){
			shtps_touch_emu_stop_recording();
		}
	#endif /* #if defined ( SHTPS_TOUCH_EMURATOR_ENABLE ) */

	#if defined ( SHTPS_TOUCH_EVENTLOG_ENABLE )
		if(shtps_touch_eventlog_is_recording() != 0){
			shtps_eventlog_stop_recording();
		}
	#endif /* #if defined ( SHTPS_TOUCH_EVENTLOG_ENABLE ) */
}

#endif /* #if defined( SHTPS_DEVELOP_MODE_ENABLE ) */
/* -----------------------------------------------------------------------------------
*/
MODULE_DESCRIPTION("FocalTech fts TouchScreen driver");
MODULE_LICENSE("GPLv2");
