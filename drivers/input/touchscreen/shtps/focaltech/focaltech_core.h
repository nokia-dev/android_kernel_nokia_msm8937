/*
 *
 * FocalTech TouchScreen driver.
 *
 * Copyright (c) 2010-2016, Focaltech Ltd. All rights reserved.
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
/*****************************************************************************
*
* File Name: focaltech_core.h

* Author: Focaltech Driver Team
*
* Created: 2016-08-08
*
* Abstract:
*
* Reference:
*
*****************************************************************************/

#ifndef __LINUX_FOCALTECH_CORE_H__
#define __LINUX_FOCALTECH_CORE_H__
/*****************************************************************************
* Included header files
*****************************************************************************/
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/input/mt.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/firmware.h>
#include <linux/debugfs.h>
//#include <linux/sensors.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/time.h>
#include <linux/workqueue.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/mount.h>
#include <linux/netdevice.h>
#include <linux/unistd.h>
#include <linux/ioctl.h>

#include "focaltech_config.h"

#define tp_ini "8607.ini"

#define FLAGBITS(x, y)          ((0xFFFFFFFF >> (32 - (y) - 1)) << (x))
#define FLAG_ICSERIALS_LEN      5
#define IC_SERIALS              (FTS_CHIP_TYPE & FLAGBITS(0, FLAG_ICSERIALS_LEN-1))
#ifndef ENABLE
	#define ENABLE                              1
#endif
#ifndef DISABLE
	#define DISABLE                             0
#endif

#define CFG_EQS shtps_CFG_EQS
#define CFG_NIS shtps_CFG_NIS
#define CFG_NTS shtps_CFG_NTS
#define CFG_SSL shtps_CFG_SSL
#define CFG_SSR shtps_CFG_SSR
#define Comm_Base_IIC_IO shtps_Comm_Base_IIC_IO
#define EnterFactory shtps_EnterFactory
#define EnterWork shtps_EnterWork
#define GetPrivateProfileString shtps_GetPrivateProfileString
#define OnGetTestItemParam shtps_OnGetTestItemParam
#define OnInit_DThreshold_RawDataTest shtps_OnInit_DThreshold_RawDataTest
#define OnInit_DThreshold_RxLinearityTest shtps_OnInit_DThreshold_RxLinearityTest
#define OnInit_DThreshold_SCapCbTest shtps_OnInit_DThreshold_SCapCbTest
#define OnInit_DThreshold_SCapRawDataTest shtps_OnInit_DThreshold_SCapRawDataTest
#define OnInit_DThreshold_TxLinearityTest shtps_OnInit_DThreshold_TxLinearityTest
#define OnInit_InterfaceCfg shtps_OnInit_InterfaceCfg
#define OnInit_InvalidNode shtps_OnInit_InvalidNode
#define OnInit_MCap_DetailThreshold shtps_OnInit_MCap_DetailThreshold
#define OnInit_SCap_DetailThreshold shtps_OnInit_SCap_DetailThreshold
#define ReadReg shtps_ReadReg
#define ResultSelfTest shtps_ResultSelfTest
#define SysDelay shtps_SysDelay
#define WriteReg shtps_WriteReg
#define focal_abs shtps_focal_abs
#define focal_msleep shtps_focal_msleep
#define g_ScreenSetParam shtps_g_ScreenSetParam
#define g_TestItemNum shtps_g_TestItemNum
#define g_stCfg_MCap_DetailThreshold shtps_g_stCfg_MCap_DetailThreshold
#define g_stCfg_SCap_DetailThreshold shtps_g_stCfg_SCap_DetailThreshold
#define g_stTestItem shtps_g_stTestItem
#define g_st_ini_file_data shtps_g_st_ini_file_data
#define g_strIcName shtps_g_strIcName
#define g_testparamstring shtps_g_testparamstring
#define g_used_key_num shtps_g_used_key_num
#define get_test_data shtps_get_test_data
#define ini_get_key shtps_ini_get_key
#define ini_get_key_data shtps_ini_get_key_data
#define ini_str_trim_l shtps_ini_str_trim_l
#define ini_str_trim_r shtps_ini_str_trim_r
#define init_i2c_read_func shtps_init_i2c_read_func
#define init_i2c_write_func shtps_init_i2c_write_func
#define init_key_data shtps_init_key_data
#define isdigit shtps_isdigit
#define isspace shtps_isspace
#define selftest_result_read shtps_selftest_result_read
#define set_max_channel_num shtps_set_max_channel_num
#define set_param_data shtps_set_param_data
#define start_test_tp shtps_start_test_tp
#define touch_selftest shtps_touch_selftest

int shtps_fts_i2c_read(struct i2c_client *client, char *writebuf, int writelen, char *readbuf, int readlen);
int shtps_fts_i2c_write(struct i2c_client *client, char *writebuf, int writelen);

#define fts_i2c_read shtps_fts_i2c_read
#define fts_i2c_write shtps_fts_i2c_write

extern struct i2c_client *shtps_fts_i2c_client;
#define fts_i2c_client shtps_fts_i2c_client

void shtps_mutex_lock_ctrl(void);
void shtps_mutex_unlock_ctrl(void);

int fts_test_init(struct i2c_client *client);
int fts_test_exit(struct i2c_client *client);

#endif /* __LINUX_FOCALTECH_CORE_H__ */
