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

#ifndef __SHTPS_LOG_H__
#define __SHTPS_LOG_H__

#define	TPS_ID0	"[shtps]"
#define	TPS_ID1	"[shtpsif]"
#define	DBG_PRINTK(...)	printk(KERN_DEBUG TPS_ID0 " "__VA_ARGS__)
#define	ERR_PRINTK(...)	printk(KERN_ERR TPS_ID0 " "__VA_ARGS__)

#define	PR_DEBUG(...)	pr_debug(TPS_ID0 " "__VA_ARGS__)
#define	PR_ERROR(...)	pr_err(TPS_ID0 " "__VA_ARGS__)

//#undef	DBG_PRINTK
//#define	DBG_PRINTK(...)	pr_debug(TPS_ID0 " "__VA_ARGS__)


#if defined( SHTPS_LOG_ERROR_ENABLE )
	#define SHTPS_LOG_ERR_PRINT(...)	DBG_PRINTK( __VA_ARGS__)
#else
	#define SHTPS_LOG_ERR_PRINT(...)
#endif /* defined( SHTPS_LOG_ERROR_ENABLE ) */

extern u8 gLogOutputEnable;

#if defined( SHTPS_LOG_DEBUG_ENABLE )
	#if defined( SHTPS_LOG_OUTPUT_SWITCH_ENABLE )
		#define	SHTPS_LOG_DBG_PRINT(...)					\
		 	if((gLogOutputEnable & 0x02) != 0){				\
				DBG_PRINTK( __VA_ARGS__);					\
			}
		#define SHTPS_LOG_DEBUG(p)							\
			if((gLogOutputEnable & 0x02) != 0){				\
				p											\
			}
		#define SHTPS_LOG_FUNC_CALL()						\
			if((gLogOutputEnable & 0x02) != 0){				\
				DBG_PRINTK("%s()\n", __func__);				\
			}
		#define SHTPS_LOG_FUNC_CALL_INPARAM(param)			\
			if((gLogOutputEnable & 0x02) != 0){				\
				DBG_PRINTK("%s(%d)\n", __func__, param);	\
			}
		#define SHTPS_LOG_SENSOR_DATA_PRINT(...)			\
		 	if((gLogOutputEnable & 0x04) != 0){				\
				DBG_PRINTK( __VA_ARGS__);					\
			}
	#else
		#define	SHTPS_LOG_DBG_PRINT(...)	DBG_PRINTK(__VA_ARGS__)
		#define SHTPS_LOG_DEBUG(p)			p
		#define SHTPS_LOG_FUNC_CALL()		DBG_PRINTK("%s()\n", __func__)
		#define SHTPS_LOG_FUNC_CALL_INPARAM(param)	DBG_PRINTK("%s(%d)\n", __func__, param)
		#define SHTPS_LOG_SENSOR_DATA_PRINT(...)	DBG_PRINTK(__VA_ARGS__)
	#endif /* SHTPS_LOG_OUTPUT_SWITCH_ENABLE */

	// #define SHTPS_LOG_ANALYSIS(...)		DBG_PRINTK(__VA_ARGS__)
	#define SHTPS_LOG_ANALYSIS(...)		SHTPS_LOG_DBG_PRINT(__VA_ARGS__)

#else
	#define	SHTPS_LOG_DBG_PRINT(...)
	#define SHTPS_LOG_DEBUG(p)
	#define SHTPS_LOG_FUNC_CALL()
	#define SHTPS_LOG_FUNC_CALL_INPARAM(param)
	#define SHTPS_LOG_SENSOR_DATA_PRINT(...)
	#define SHTPS_LOG_ANALYSIS(...)
#endif /* defined( SHTPS_LOG_DEBUG_ENABLE ) */

#if defined( SHTPS_LOG_EVENT_ENABLE ) && defined( SHTPS_LOG_OUTPUT_SWITCH_ENABLE )
	#define SHTPS_LOG_EVENT(p)				\
        if((gLogOutputEnable & 0x01) != 0){	\
			p								\
		}
#elif defined( SHTPS_LOG_EVENT_ENABLE )
	#define SHTPS_LOG_EVENT(p)	p
#else
	#define SHTPS_LOG_EVENT(p)
#endif /* defined( SHTPS_LOG_EVENT_ENABLE ) */

#define _log_msg_sync(id, fmt, ...)
#define _log_msg_send(id, fmt, ...)
#define _log_msg_recv(id, fmt, ...)

#endif	/* __SHTPS_LOG_H__ */
