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
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>

#include <linux/input/shtps_dev.h>

#include "shtps_fts.h"
#include "shtps_fts_sub.h"
#include "shtps_i2c.h"
#include "shtps_log.h"

#include "shtps_fwctl.h"
#include "shtps_fwctl_focaltech.h"
#include "shtps_param_extern.h"

/* -------------------------------------------------------------------------- */
#if defined( SHTPS_LOG_ERROR_ENABLE )
	#define SHTPS_LOG_FWCTL_FUNC_CALL()		 SHTPS_LOG_FUNC_CALL()
#else
	#define SHTPS_LOG_FWCTL_FUNC_CALL()
#endif

#define SHTPS_READ_SERIAL_WAIT_US		10
#define SHTPS_SERIAL_NUMBER_ALL_SIZE	0x58

/* -------------------------------------------------------------------------- */
#if defined( SHTPS_LOG_DEBUG_ENABLE )
	static char sensor_log_tmp[16];
	static char sensor_log_outstr[256];
#endif /* SHTPS_LOG_DEBUG_ENABLE */

/* -------------------------------------------------------------------------- */
static int shtps_fwctl_focaltech_ic_init(struct shtps_fwctl_info *fc_p);
static int shtps_fwctl_focaltech_get_device_status(struct shtps_fwctl_info *fc_p, u8 *status_p);
static int shtps_fwctl_focaltech_soft_reset(struct shtps_fwctl_info *fc_p);
static int shtps_fwctl_focaltech_irqclear_get_irqfactor(struct shtps_fwctl_info *fc_p, u8 *status_p);
static int shtps_fwctl_focaltech_rezero(struct shtps_fwctl_info *fc_p);
static int shtps_fwctl_focaltech_map_construct(struct shtps_fwctl_info *fc_p, int func_check);
static int shtps_fwctl_focaltech_is_sleeping(struct shtps_fwctl_info *fc_p);
static int shtps_fwctl_focaltech_is_singlefinger(struct shtps_fwctl_info *fc_p);
static int shtps_fwctl_focaltech_set_doze(struct shtps_fwctl_info *fc_p);
static int shtps_fwctl_focaltech_set_doze_param(struct shtps_fwctl_info *fc_p, u8 *param_p, u8 param_size);
static int shtps_fwctl_focaltech_set_active(struct shtps_fwctl_info *fc_p);
static int shtps_fwctl_focaltech_set_sleepmode_on(struct shtps_fwctl_info *fc_p);
static int shtps_fwctl_focaltech_set_sleepmode_off(struct shtps_fwctl_info *fc_p);
static int shtps_fwctl_focaltech_set_lpwg_mode_on(struct shtps_fwctl_info *fc_p);
static int shtps_fwctl_focaltech_set_lpwg_mode_off(struct shtps_fwctl_info *fc_p);
static int shtps_fwctl_focaltech_set_lpwg_mode_cal(struct shtps_fwctl_info *fc_p);
static int shtps_fwctl_focaltech_set_low_reportrate_mode(struct shtps_fwctl_info *fc_p, int mode);
static int shtps_fwctl_focaltech_get_fingermax(struct shtps_fwctl_info *fc_p);
static int shtps_fwctl_focaltech_get_fingerinfo(struct shtps_fwctl_info *fc_p, u8 *buf_p, int read_cnt, u8 *irqsts_p, u8 *extsts_p, u8 **finger_pp);
static int shtps_fwctl_focaltech_get_one_fingerinfo(struct shtps_fwctl_info *fc_p, int id, u8 *buf_p, u8 **finger_pp);
static u8* shtps_fwctl_focaltech_get_finger_info_buf(struct shtps_fwctl_info *fc_p, int fingerid, int fingerMax, u8 *buf_p);
static int shtps_fwctl_focaltech_get_finger_state(struct shtps_fwctl_info *fc_p, int fingerid, int fingerMax, u8 *buf_p);
static int shtps_fwctl_focaltech_get_finger_pos_x(struct shtps_fwctl_info *fc_p, u8 *buf_p);
static int shtps_fwctl_focaltech_get_finger_pos_y(struct shtps_fwctl_info *fc_p, u8 *buf_p);
static int shtps_fwctl_focaltech_get_finger_wx(struct shtps_fwctl_info *fc_p, u8 *buf_p);
static int shtps_fwctl_focaltech_get_finger_wy(struct shtps_fwctl_info *fc_p, u8 *buf_p);
static int shtps_fwctl_focaltech_get_finger_z(struct shtps_fwctl_info *fc_p, u8 *buf_p);
static void shtps_fwctl_focaltech_get_gesture(struct shtps_fwctl_info *fc_p, int fingerMax, u8 *buf_p, u8 *gs1_p, u8 *gs2_p);
static int shtps_fwctl_focaltech_get_keystate(struct shtps_fwctl_info *fc_p, u8 *status_p);
static int shtps_fwctl_focaltech_get_gesturetype(struct shtps_fwctl_info *fc_p, u8 *status_p);
static int shtps_fwctl_focaltech_get_fwdate(struct shtps_fwctl_info *fc_p, u8 *year_p, u8 *month_p);
static int shtps_fwctl_focaltech_get_serial_number(struct shtps_fwctl_info *fc_p, u8 *buf_p);
static int shtps_fwctl_focaltech_get_fwver(struct shtps_fwctl_info *fc_p, u16 *ver_p);
static int shtps_fwctl_focaltech_get_tm_mode(struct shtps_fwctl_info *fc_p);
static int shtps_fwctl_focaltech_get_tm_rxsize(struct shtps_fwctl_info *fc_p);
static int shtps_fwctl_focaltech_get_tm_txsize(struct shtps_fwctl_info *fc_p);
static int shtps_fwctl_focaltech_get_tm_rawdata(struct shtps_fwctl_info *fc_p, u8 tm_mode, u8 *tm_data_p);
static int shtps_fwctl_focaltech_get_tm_cbdata(struct shtps_fwctl_info *fc_p, u8 tm_mode, u8 *tm_data_p);
static int shtps_fwctl_focaltech_get_tm_frameline(struct shtps_fwctl_info *fc_p, u8 tm_mode, u8 *tm_data_p);
static int shtps_fwctl_focaltech_get_tm_baseline(struct shtps_fwctl_info *fc_p, u8 tm_mode, u8 *tm_data_p);
static int shtps_fwctl_focaltech_get_tm_baseline_raw(struct shtps_fwctl_info *fc_p, u8 tm_mode, u8 *tm_data_p);
static int shtps_fwctl_focaltech_cmd_tm_frameline(struct shtps_fwctl_info *fc_p, u8 tm_mode);
static int shtps_fwctl_focaltech_cmd_tm_baseline(struct shtps_fwctl_info *fc_p, u8 tm_mode);
static int shtps_fwctl_focaltech_cmd_tm_baseline_raw(struct shtps_fwctl_info *fc_p, u8 tm_mode);
static int shtps_fwctl_focaltech_initparam(struct shtps_fwctl_info *fc_p);
static int shtps_fwctl_focaltech_initparam_activemode(struct shtps_fwctl_info *fc_p);
static int shtps_fwctl_focaltech_initparam_dozemode(struct shtps_fwctl_info *fc_p);
static int shtps_fwctl_focaltech_initparam_key(struct shtps_fwctl_info *fc_p);
static int shtps_fwctl_focaltech_initparam_lpwgmode(struct shtps_fwctl_info *fc_p);
static int shtps_fwctl_focaltech_initparam_autorezero(struct shtps_fwctl_info *fc_p);
static int shtps_fwctl_focaltech_initparam_reportrate(struct shtps_fwctl_info *fc_p, int mode);
static int shtps_fwctl_focaltech_initparam_set_custom_report_rate(struct shtps_fwctl_info *fc_p);
static int shtps_fwctl_focaltech_pen_enable(struct shtps_fwctl_info *fc_p);
static int shtps_fwctl_focaltech_pen_disable(struct shtps_fwctl_info *fc_p);
static int shtps_fwctl_focaltech_start_testmode(struct shtps_fwctl_info *fc_p, u8 tm_mode);
static int shtps_fwctl_focaltech_stop_testmode(struct shtps_fwctl_info *fc_p);
static int shtps_fwctl_focaltech_baseline_offset_disable(struct shtps_fwctl_info *fc_p);
static void shtps_fwctl_focaltech_set_dev_state(struct shtps_fwctl_info *fc_p, u8 state);
static u8 shtps_fwctl_focaltech_get_dev_state(struct shtps_fwctl_info *fc_p);
static int shtps_fwctl_focaltech_get_maxXPosition(struct shtps_fwctl_info *fc_p);
static int shtps_fwctl_focaltech_get_maxYPosition(struct shtps_fwctl_info *fc_p);
static int shtps_fwctl_focaltech_set_custom_report_rate(struct shtps_fwctl_info *fc_p, u8 rate);
static int shtps_fwctl_focaltech_set_lpwg_sweep_on(struct shtps_fwctl_info *fc_p, u8 enable);
static int shtps_fwctl_focaltech_set_lpwg_double_tap(struct shtps_fwctl_info *fc_p, u8 enable);

/* -------------------------------------------------------------------------- */
static int shtps_fwctl_focaltech_ic_init(struct shtps_fwctl_info *fc_p)
{
	SHTPS_LOG_FWCTL_FUNC_CALL();
	return 0;
}

/* -------------------------------------------------------------------------- */
static int shtps_Upgrade_ReadPram(struct shtps_fwctl_info *fc_p,unsigned int Addr, unsigned char * pData, unsigned short Datalen)
{
	int ret=-1;
	unsigned char pDataSend[16];

	{
		pDataSend[0] = 0x85;
		pDataSend[1] = 0x00;
		pDataSend[2] = Addr>>8;
		pDataSend[3] = Addr;
		M_DIRECT_WRITE_FUNC(fc_p, pDataSend, 4);

		ret =M_DIRECT_READ_FUNC(fc_p, NULL, 0, pData, Datalen);
		if (ret < 0)
		{
			SHTPS_LOG_ERR_PRINT("failed Upgrade_ReadPram \n");
			return ret;
		}
	}

	return 0;
}
static int shtps_fwctl_focaltech_loader_write_pram(struct shtps_fwctl_info *fc_p, u8 *pbt_buf, u32 dw_lenth)
{
	u8 reg_val[4] = {0};
	u32 i = 0;
	u32 packet_number;
	u32 j;
	u32 temp,nowAddress=0,StartFlashAddr=0,FlashAddr=0;
	u32 lenght;
	u8 packet_buf[FTS_PACKET_LENGTH + 6];
	u8 *pCheckBuffer = NULL;
	u8 auc_i2c_write_buf[10];
	u8 bt_ecc;
	int i_ret,ReCode=-1;

	SHTPS_LOG_FWCTL_FUNC_CALL();

	SHTPS_LOG_DBG_PRINT("8606 dw_lenth= %d",dw_lenth);
	if(dw_lenth > 0x10000 || dw_lenth ==0)
	{
		return -EIO;
	}
	pCheckBuffer=(u8 *)kzalloc(dw_lenth+1, GFP_KERNEL);
	for (i = 0; i < FTS_UPGRADE_LOOP; i++)
	{
		/********* Step 1:Reset  CTPM *****/
		M_WRITE_FUNC(fc_p, 0xfc, FTS_UPGRADE_AA);
		msleep(FTS_RST_DELAY_AA_MS);
		M_WRITE_FUNC(fc_p, 0xfc, FTS_UPGRADE_55);
		msleep(200);
		/********* Step 2:Enter upgrade mode *****/
		auc_i2c_write_buf[0] = FTS_UPGRADE_55;
		i_ret = M_DIRECT_WRITE_FUNC(fc_p, auc_i2c_write_buf, 1);
		if(i_ret < 0)
		{
			SHTPS_LOG_DBG_PRINT("failed writing  0x55 ! \n");
			continue;
		}

		/********* Step 3:check READ-ID ***********************/
		msleep(1);
		auc_i2c_write_buf[0] = 0x90;
		auc_i2c_write_buf[1] = auc_i2c_write_buf[2] = auc_i2c_write_buf[3] =
			0x00;
		reg_val[0] = reg_val[1] = 0x00;

		M_DIRECT_READ_FUNC(fc_p, auc_i2c_write_buf, 4, reg_val, 2);

		if ((reg_val[0] == 0x86 && reg_val[1] == 0x06) ||
		    (reg_val[0] == 0x86 && reg_val[1] == 0x07))
		{
			msleep(50);
			break;
		}
		else
		{
			SHTPS_LOG_ERR_PRINT("Step 3: CTPM ID,ID1 = 0x%x,ID2 = 0x%x\n",
				reg_val[0], reg_val[1]);

			continue;
		}
	}

	if (i >= FTS_UPGRADE_LOOP ) {
		i_ret = -EIO;
		goto ERROR;
	}

	/********* Step 4:write firmware(FW) to ctpm flash *********/
	bt_ecc = 0;
	SHTPS_LOG_DBG_PRINT("Step 4:write firmware(FW) to ctpm flash\n");
	temp = 0;
	packet_number = (dw_lenth) / FTS_PACKET_LENGTH;
	packet_buf[0] = 0xae;
	packet_buf[1] = 0x00;

	for (j = 0; j < packet_number; j++)
	{
		temp = j * FTS_PACKET_LENGTH;
		packet_buf[2] = (u8) (temp >> 8);
		packet_buf[3] = (u8) temp;
		lenght = FTS_PACKET_LENGTH;
		packet_buf[4] = (u8) (lenght >> 8);
		packet_buf[5] = (u8) lenght;

		for (i = 0; i < FTS_PACKET_LENGTH; i++)
		{
			packet_buf[6 + i] = pbt_buf[j * FTS_PACKET_LENGTH + i];
			bt_ecc ^= packet_buf[6 + i];
		}
		i_ret = M_DIRECT_WRITE_FUNC(fc_p, packet_buf, FTS_PACKET_LENGTH + 6);
		if(i_ret < 0)
		{
			SHTPS_LOG_ERR_PRINT("Step 4:write firmware(FW) error. packet_number=(%d/%d)\n",
				j, packet_number);
			i_ret = -EIO;
			goto ERROR;
		}
		nowAddress=nowAddress+FTS_PACKET_LENGTH;
	}

	if ((dw_lenth) % FTS_PACKET_LENGTH > 0)
	{
		temp = packet_number * FTS_PACKET_LENGTH;
		packet_buf[2] = (u8) (temp >> 8);
		packet_buf[3] = (u8) temp;
		temp = (dw_lenth) % FTS_PACKET_LENGTH;
		packet_buf[4] = (u8) (temp >> 8);
		packet_buf[5] = (u8) temp;

		for (i = 0; i < temp; i++)
		{
			packet_buf[6 + i] = pbt_buf[packet_number * FTS_PACKET_LENGTH + i];
			bt_ecc ^= packet_buf[6 + i];
		}
		i_ret = M_DIRECT_WRITE_FUNC(fc_p, packet_buf, temp + 6);
		if(i_ret < 0)
		{
			SHTPS_LOG_ERR_PRINT("Step 4:write firmware(FW) error. packet_number=(%d/%d)\n",
				packet_number, packet_number);
			i_ret = -EIO;
			goto ERROR;
		}
		nowAddress=nowAddress+temp;
	}

	/********* Step 5: read out checksum ***********************/
	/* send the opration head */
	SHTPS_LOG_DBG_PRINT("Step 5: read out checksum\n");
	msleep(100);
	//-----------------------------------------------------------------------------------------------------
	SHTPS_LOG_DBG_PRINT( "--nowAddress=%02x dw_lenth=%02x\n",nowAddress,dw_lenth);	
	if(nowAddress == dw_lenth)
	{

		FlashAddr=0;
		while(1)
		{
			StartFlashAddr = FlashAddr;
			if(FlashAddr == dw_lenth)
			{
				break;
			}
			else if(FlashAddr+FTS_PACKET_LENGTH > dw_lenth)
			{
				if(shtps_Upgrade_ReadPram(fc_p,StartFlashAddr, pCheckBuffer+FlashAddr, dw_lenth-FlashAddr))
				{
					SHTPS_LOG_ERR_PRINT("read out checksum error\n");
					i_ret = -EIO;
					goto ERROR;
				}
				ReCode = ERROR_CODE_OK;
				FlashAddr = dw_lenth;
			}
			else
			{
				if(shtps_Upgrade_ReadPram(fc_p,StartFlashAddr, pCheckBuffer+FlashAddr, FTS_PACKET_LENGTH))
				{
					SHTPS_LOG_ERR_PRINT("read out checksum error\n");
					i_ret = -EIO;
					goto ERROR;
				}
				FlashAddr += FTS_PACKET_LENGTH;
				ReCode = ERROR_CODE_OK;
			}

			if(ReCode != ERROR_CODE_OK){
				SHTPS_LOG_ERR_PRINT("read out checksum error\n");
				i_ret = -EIO;
				goto ERROR;
			}

		}
		SHTPS_LOG_DBG_PRINT( "--FlashAddr=%02x dw_lenth=%02x\n",FlashAddr,dw_lenth);
		if(FlashAddr == dw_lenth)
		{

			SHTPS_LOG_DBG_PRINT("Checking data...\n");
			for(i=0; i<dw_lenth; i++)
			{
				if(pCheckBuffer[i] != pbt_buf[i])
				{
					SHTPS_LOG_ERR_PRINT("read out checksum error\n");
					i_ret = -EIO;
					goto ERROR;
				}
			}
			//COMM_FLASH_FT5422_Upgrade_StartApp(bOldProtocol, iCommMode);		//Reset
			SHTPS_LOG_DBG_PRINT("read out checksum successful\n");

		}
		else
		{
			SHTPS_LOG_ERR_PRINT("read out checksum error\n");
		}

	}
	else
	{
		SHTPS_LOG_ERR_PRINT("read out checksum error\n");
	}

	/********* Step 6: start app ***********************/
	SHTPS_LOG_DBG_PRINT("Step 6: start app\n");
	auc_i2c_write_buf[0] = 0x08;
	M_DIRECT_WRITE_FUNC(fc_p, auc_i2c_write_buf, 1);
	msleep(20);

	if(pCheckBuffer){
		kfree(pCheckBuffer);
	}

	return 0;

ERROR:
	if(pCheckBuffer){
		kfree(pCheckBuffer);
	}
	return i_ret;
}

/* -------------------------------------------------------------------------- */
static int shtps_fwctl_focaltech_loader_upgrade(struct shtps_fwctl_info *fc_p, u8 *pbt_buf, u32 dw_lenth)
{
	u8 reg_val[4] = {0};
	u8 reg_val_id[4] = {0};
	u32 i = 0;
	u32 packet_number;
	u32 j;
	u32 temp;
	u32 lenght;
	u8 packet_buf[FTS_PACKET_LENGTH + 6];
	u8 auc_i2c_write_buf[10];
	u8 bt_ecc;
	int i_ret;
	unsigned char cmd[20];
	unsigned char Checksum = 0;
	u32 uCheckStart = 0x1000;
	u32 uCheckOff = 0x20;
	u32 uStartAddr = 0x00;

	SHTPS_LOG_FWCTL_FUNC_CALL();

	auc_i2c_write_buf[0] = 0x05;
	reg_val_id[0] = 0x00;

	i_ret = M_DIRECT_READ_FUNC(fc_p, auc_i2c_write_buf, 1, reg_val_id, 1);
	if(dw_lenth == 0)
	{
		return -EIO;
	}

	if(0x81 == (int)reg_val_id[0])
	{
		if(dw_lenth > 1024*60)
		{
			return -EIO;
		}
	}
	else if(0x80 == (int)reg_val_id[0])
	{
		if(dw_lenth > 1024*64)
		{
			return -EIO;
		}
	}

	for (i = 0; i < FTS_UPGRADE_LOOP; i++) {
		/********* Step 2:Enter upgrade mode *****/
		msleep(10);
		auc_i2c_write_buf[0] = FTS_UPGRADE_55;
		auc_i2c_write_buf[1] = FTS_UPGRADE_AA;
		i_ret = M_DIRECT_WRITE_FUNC(fc_p, auc_i2c_write_buf, 2);
		if(i_ret < 0)
		{
			SHTPS_LOG_ERR_PRINT("failed writing  0x55 and 0xaa ! \n");
			continue;
		}

		/********* Step 3:check READ-ID ***********************/
		msleep(1);
		auc_i2c_write_buf[0] = 0x90;
		auc_i2c_write_buf[1] = auc_i2c_write_buf[2] = auc_i2c_write_buf[3] = 0x00;

		reg_val[0] = reg_val[1] = 0x00;

		M_DIRECT_READ_FUNC(fc_p, auc_i2c_write_buf, 4, reg_val, 2);

		if ((reg_val[0] == 0x86 && reg_val[1] == 0xA6) ||
		    (reg_val[0] == 0x86 && reg_val[1] == 0xA7)) {
			SHTPS_LOG_DBG_PRINT("Step 3: READ OK CTPM ID,ID1 = 0x%x,ID2 = 0x%x\n",
				reg_val[0], reg_val[1]);
			break;
		}
		else {
			SHTPS_LOG_ERR_PRINT("Step 3: CTPM ID,ID1 = 0x%x,ID2 = 0x%x\n",
				reg_val[0], reg_val[1]);

			continue;
		}
	}

	if (i >= FTS_UPGRADE_LOOP )
		return -EIO;

	/* Step 4:erase app and panel paramenter area */
	SHTPS_LOG_DBG_PRINT("Step 4:erase app and panel paramenter area\n");

	{
		cmd[0] = 0x05;
		cmd[1] = reg_val_id[0];//0x80;
		cmd[2] = 0x00;
		M_DIRECT_WRITE_FUNC(fc_p, cmd, 3);
	}

	// Set pramboot download mode
	{
		cmd[0] = 0x09;
		cmd[1] = 0x0B;
		M_DIRECT_WRITE_FUNC(fc_p, cmd, 2);
	}

	for(i=0; i<dw_lenth ; i++)
	{
		Checksum ^= pbt_buf[i];
	}
	msleep(50);

	// erase app area 
	auc_i2c_write_buf[0] = 0x61;
	M_DIRECT_WRITE_FUNC(fc_p, auc_i2c_write_buf, 1);
	msleep(1350);

	for(i = 0;i < 15;i++)
	{
		auc_i2c_write_buf[0] = 0x6a;
		reg_val[0] = reg_val[1] = 0x00;
		M_DIRECT_READ_FUNC(fc_p, auc_i2c_write_buf, 1, reg_val, 2);

		if(0xF0==reg_val[0] && 0xAA==reg_val[1])
		{
			break;
		}
		msleep(50);
	}

	/********* Step 5:write firmware(FW) to ctpm flash *********/
	bt_ecc = 0;
	SHTPS_LOG_DBG_PRINT("Step 5:write firmware(FW) to ctpm flash\n");

	temp = 0;
	packet_number = (dw_lenth) / FTS_PACKET_LENGTH;
	packet_buf[0] = 0xbf;

	for (j = 0; j < packet_number; j++) {
		temp = uCheckStart+j * FTS_PACKET_LENGTH;
		packet_buf[1] = (u8) (temp >> 16);
		packet_buf[2] = (u8) (temp >> 8);
		packet_buf[3] = (u8) temp;
		uStartAddr=temp;
		lenght = FTS_PACKET_LENGTH;
		packet_buf[4] = (u8) (lenght >> 8);
		packet_buf[5] = (u8) lenght;

		uCheckOff = uStartAddr/lenght;

		for (i = 0; i < FTS_PACKET_LENGTH; i++)
		{
			packet_buf[6 + i] = pbt_buf[j * FTS_PACKET_LENGTH + i];
			bt_ecc ^= packet_buf[6 + i];
		}
		i_ret = M_DIRECT_WRITE_FUNC(fc_p, packet_buf, FTS_PACKET_LENGTH + 6);
		if(i_ret < 0)
		{
			SHTPS_LOG_ERR_PRINT("Step 5:write firmware(FW) error. packet_number=(%d/%d)\n",
				j, packet_number);
			return -EIO;
		}

		for(i = 0;i < 30;i++)
		{
			auc_i2c_write_buf[0] = 0x6a;
			reg_val[0] = reg_val[1] = 0x00;
			M_DIRECT_READ_FUNC(fc_p, auc_i2c_write_buf, 1, reg_val, 2);

			if ((uCheckOff +uCheckStart) == (((reg_val[0]) << 8) | reg_val[1]))
			{
				break;
			}
//			msleep(1);
			mdelay(1);

		}

		temp = (((reg_val[0]) << 8) | reg_val[1]);
		if( i == 30) SHTPS_LOG_DBG_PRINT("Query 6a reg time out value: 0x%x, driver set value:0x%x\n",  temp, uCheckOff + uCheckStart);
	}

	if ((dw_lenth) % FTS_PACKET_LENGTH > 0) {
		temp = uCheckStart+packet_number * FTS_PACKET_LENGTH;
		packet_buf[1] = (u8) (temp >> 16);
		packet_buf[2] = (u8) (temp >> 8);
		packet_buf[3] = (u8) temp;
		uStartAddr=temp;
		temp = (dw_lenth) % FTS_PACKET_LENGTH;
		packet_buf[4] = (u8) (temp >> 8);
		packet_buf[5] = (u8) temp;

		uCheckOff=uStartAddr/temp;

		for (i = 0; i < temp; i++) {
			packet_buf[6 + i] = pbt_buf[packet_number * FTS_PACKET_LENGTH + i];
			bt_ecc ^= packet_buf[6 + i];
		}
		i_ret = M_DIRECT_WRITE_FUNC(fc_p, packet_buf, temp + 6);
		if(i_ret < 0)
		{
			SHTPS_LOG_ERR_PRINT("Step 5:write firmware(FW) error. packet_number=(%d/%d)\n",
				packet_number, packet_number);
			return -EIO;
		}

		for(i = 0;i < 30;i++)
		{
			auc_i2c_write_buf[0] = 0x6a;
			reg_val[0] = reg_val[1] = 0x00;
			M_DIRECT_READ_FUNC(fc_p, auc_i2c_write_buf, 1, reg_val, 2);

			if ((uCheckOff + uCheckStart) == (((reg_val[0]) << 8) | reg_val[1]))
			{
				break;
			}
//			msleep(1);
			mdelay(1);

		}

		temp = (((reg_val[0]) << 8) | reg_val[1]);
		if( i == 30) SHTPS_LOG_DBG_PRINT("Query 6a reg time out value: 0x%x, driver set value:0x%x\n",  temp,  uCheckOff+ uCheckStart);
	}

	msleep(50);

	/********* Step 6: read out checksum ***********************/
	/*send the opration head */
	SHTPS_LOG_DBG_PRINT("Step 6: read out checksum\n");
	auc_i2c_write_buf[0] = 0x64;
	M_DIRECT_WRITE_FUNC(fc_p, auc_i2c_write_buf, 1);
	msleep(300);

	temp = uCheckStart+0;

	auc_i2c_write_buf[0] = 0x65;
	auc_i2c_write_buf[1] = (u8)(temp >> 16);
	auc_i2c_write_buf[2] = (u8)(temp >> 8);
	auc_i2c_write_buf[3] = (u8)(temp);

	if (dw_lenth > FTS_LEN_FLASH_ECC_MAX)
	{
		temp = FTS_LEN_FLASH_ECC_MAX;
	}
	else
	{
		temp = dw_lenth;
		SHTPS_LOG_DBG_PRINT("Step 6_1: read out checksum\n");
	}
	auc_i2c_write_buf[4] = (u8)(temp >> 8);
	auc_i2c_write_buf[5] = (u8)(temp);
	i_ret = M_DIRECT_WRITE_FUNC(fc_p, auc_i2c_write_buf, 6);
	msleep(dw_lenth/256);

	for(i = 0;i < 100;i++)
	{
		auc_i2c_write_buf[0] = 0x6a;
		reg_val[0] = reg_val[1] = 0x00;
		M_DIRECT_READ_FUNC(fc_p, auc_i2c_write_buf, 1, reg_val, 2);

		if (0xF0==reg_val[0] && 0x55==reg_val[1])
		{
			break;
		}
		msleep(1);

	}
	//----------------------------------------------------------------------
	if (dw_lenth > FTS_LEN_FLASH_ECC_MAX)
	{
		temp = FTS_LEN_FLASH_ECC_MAX;
		auc_i2c_write_buf[0] = 0x65;
		auc_i2c_write_buf[1] = (u8)(temp >> 16);
		auc_i2c_write_buf[2] = (u8)(temp >> 8);
		auc_i2c_write_buf[3] = (u8)(temp);
		temp = dw_lenth-FTS_LEN_FLASH_ECC_MAX;
		auc_i2c_write_buf[4] = (u8)(temp >> 8);
		auc_i2c_write_buf[5] = (u8)(temp);
		i_ret = M_DIRECT_WRITE_FUNC(fc_p, auc_i2c_write_buf, 6);

		msleep(dw_lenth/256);

		for(i = 0;i < 100;i++)
		{
			auc_i2c_write_buf[0] = 0x6a;
			reg_val[0] = reg_val[1] = 0x00;
			M_DIRECT_READ_FUNC(fc_p, auc_i2c_write_buf, 1, reg_val, 2);

			if (0xF0==reg_val[0] && 0x55==reg_val[1])
			{
				break;
			}
			msleep(1);

		}
	}
	auc_i2c_write_buf[0] = 0x66;
	M_DIRECT_READ_FUNC(fc_p, auc_i2c_write_buf, 1, reg_val, 1);
	if (reg_val[0] != bt_ecc)
	{
		SHTPS_LOG_ERR_PRINT("--ecc error! FW=%02x bt_ecc=%02x\n",
			reg_val[0],
			bt_ecc);

		return -EIO;
	}
	SHTPS_LOG_DBG_PRINT("checksum %X %X \n",reg_val[0],bt_ecc);
	/********* Step 7: reset the new FW ***********************/
	SHTPS_LOG_DBG_PRINT("Step 7: reset the new FW\n");
	auc_i2c_write_buf[0] = 0x07;
	M_DIRECT_WRITE_FUNC(fc_p, auc_i2c_write_buf, 1);
//	msleep(200);
	msleep(300);

	return 0;
}

/* -------------------------------------------------------------------------- */
static int shtps_fwctl_focaltech_get_device_status(struct shtps_fwctl_info *fc_p, u8 *status_p)
{
	int rc=0;
	u8 buf;

	SHTPS_LOG_FWCTL_FUNC_CALL();

	rc = M_READ_FUNC(fc_p, FTS_REG_STATUS, &buf, 1);
	if(rc == 0){
		*status_p = buf;
	}
	else{
		*status_p = 0;
	}
	return rc;
}

/* -------------------------------------------------------------------------- */
static int shtps_fwctl_focaltech_soft_reset(struct shtps_fwctl_info *fc_p)
{
	return 0;
}

/* -------------------------------------------------------------------------- */
static int shtps_fwctl_focaltech_irqclear_get_irqfactor(struct shtps_fwctl_info *fc_p, u8 *status_p)
{
	return 0;
}

/* -------------------------------------------------------------------------- */
static int shtps_fwctl_focaltech_rezero(struct shtps_fwctl_info *fc_p)
{
	return 0;
}

/* -------------------------------------------------------------------------- */
static int shtps_fwctl_focaltech_map_construct(struct shtps_fwctl_info *fc_p, int func_check)
{
	return 0;
}

/* -------------------------------------------------------------------------- */
static int shtps_fwctl_focaltech_is_sleeping(struct shtps_fwctl_info *fc_p)
{
	return 0;
}

/* -------------------------------------------------------------------------- */
static int shtps_fwctl_focaltech_is_singlefinger(struct shtps_fwctl_info *fc_p)
{
	return 0;
}

/* -------------------------------------------------------------------------- */
static int shtps_fwctl_focaltech_set_doze(struct shtps_fwctl_info *fc_p)
{
	SHTPS_LOG_FWCTL_FUNC_CALL();
	shtps_fwctl_focaltech_set_dev_state(fc_p, SHTPS_DEV_STATE_DOZE);
	return 0;
}

/* -------------------------------------------------------------------------- */
static int shtps_fwctl_focaltech_set_doze_param(struct shtps_fwctl_info *fc_p, u8 *param_p, u8 param_size)
{
	return 0;
}

/* -------------------------------------------------------------------------- */
static int shtps_fwctl_focaltech_set_active(struct shtps_fwctl_info *fc_p)
{
	SHTPS_LOG_FWCTL_FUNC_CALL();
	shtps_fwctl_focaltech_set_dev_state(fc_p, SHTPS_DEV_STATE_ACTIVE);
	return 0;
}

/* -------------------------------------------------------------------------- */
static int shtps_fwctl_focaltech_set_sleepmode_on(struct shtps_fwctl_info *fc_p)
{
	SHTPS_LOG_FWCTL_FUNC_CALL();
	M_WRITE_FUNC(fc_p, FTS_REG_PMODE, FTS_PMODE_HIBERNATE);
	shtps_fwctl_focaltech_set_dev_state(fc_p, SHTPS_DEV_STATE_SLEEP);
	return 0;
}

/* -------------------------------------------------------------------------- */
static int shtps_fwctl_focaltech_set_sleepmode_off(struct shtps_fwctl_info *fc_p)
{
	return 0;
}

/* -------------------------------------------------------------------------- */
static int shtps_fwctl_focaltech_set_lpwg_mode_on(struct shtps_fwctl_info *fc_p)
{
	SHTPS_LOG_FWCTL_FUNC_CALL();

	M_WRITE_FUNC(fc_p, 0xD1, 0x1F);
	M_WRITE_FUNC(fc_p, 0xD2, 0x00);
	M_WRITE_FUNC(fc_p, 0xD5, 0x00);
	M_WRITE_FUNC(fc_p, 0xD6, 0x00);
	M_WRITE_FUNC(fc_p, 0xD7, 0x00);
	M_WRITE_FUNC(fc_p, 0xD8, 0x00);

	M_WRITE_FUNC(fc_p, 0xD0, 0x01);

	shtps_fwctl_focaltech_set_dev_state(fc_p, SHTPS_DEV_STATE_LPWG);

	return 0;
}

/* -------------------------------------------------------------------------- */
static int shtps_fwctl_focaltech_set_lpwg_mode_off(struct shtps_fwctl_info *fc_p)
{
	SHTPS_LOG_FWCTL_FUNC_CALL();

	M_WRITE_FUNC(fc_p, 0xD0, 0x00);

	return 0;
}

/* -------------------------------------------------------------------------- */
static int shtps_fwctl_focaltech_set_lpwg_mode_cal(struct shtps_fwctl_info *fc_p)
{
	return 0;
}

/* -------------------------------------------------------------------------- */
static int shtps_fwctl_focaltech_set_low_reportrate_mode(struct shtps_fwctl_info *fc_p, int mode)
{
	return 0;
}

/* -------------------------------------------------------------------------- */
static int shtps_fwctl_focaltech_get_fingermax(struct shtps_fwctl_info *fc_p)
{
	return SHTPS_FINGER_MAX;
}

/* -------------------------------------------------------------------------- */
static int shtps_fwctl_focaltech_get_fingerinfo(struct shtps_fwctl_info *fc_p, u8 *buf_p, int read_cnt, u8 *irqsts_p, u8 *extsts_p, u8 **finger_pp)
{
	if(SHTPS_TOUCH_PERFORMANCE_UP_MODE == 1){
		buf_p[0] = 0x00;
		buf_p[1] = 0x00;
		M_READ_PACKET_FUNC(fc_p, 0x02, &buf_p[2], 1);
		if(buf_p[2] < FTS_MAX_POINTS){
			M_READ_PACKET_FUNC(fc_p, 0x03, &buf_p[3], buf_p[2]*FTS_ONE_TCH_LEN);
		}
		else{
			buf_p[2] = 0x00;
		}
	}
	else{
		M_READ_PACKET_FUNC(fc_p, 0x00, buf_p, FTS_POINT_READ_BUFSIZE);
	}
#if defined ( SHTPS_TOUCH_EMURATOR_ENABLE )
	if(shtps_touch_emu_is_running() != 0){
		shtps_touch_emu_set_finger_info(buf_p, sizeof(buf_p));
	}
	else if(shtps_touch_emu_is_recording() != 0){
		shtps_touch_emu_rec_finger_info(&buf_p[0]);
	}
#endif /* #if defined ( SHTPS_TOUCH_EMURATOR_ENABLE ) */
	*finger_pp = &buf_p[0];
	return 0;
}

/* -------------------------------------------------------------------------- */
static int shtps_fwctl_focaltech_get_one_fingerinfo(struct shtps_fwctl_info *fc_p, int id, u8 *buf_p, u8 **finger_pp)
{
	return 0;
}

/* -------------------------------------------------------------------------- */
static int shtps_fwctl_focaltech_get_num_of_touch_fingers(struct shtps_fwctl_info *fc_p, u8 *buf_p)
{
	return buf_p[FTS_TOUCH_POINT_NUM];
}

/* -------------------------------------------------------------------------- */
static u8* shtps_fwctl_focaltech_get_finger_info_buf(struct shtps_fwctl_info *fc_p, int fingerid, int fingerMax, u8 *buf_p)
{
	return &buf_p[FTS_TCH_LEN(fingerid)];
}

/* -------------------------------------------------------------------------- */
static int shtps_fwctl_focaltech_get_finger_state(struct shtps_fwctl_info *fc_p, int fingerid, int fingerMax, u8 *buf_p)
{
	u8 event = buf_p[FTS_TOUCH_EVENT_POS] >> 6;
	if(event == FTS_TOUCH_DOWN || event == FTS_TOUCH_CONTACT){
		return SHTPS_TOUCH_STATE_FINGER;
	}
	return SHTPS_TOUCH_STATE_NO_TOUCH;
}

/* -------------------------------------------------------------------------- */
static int shtps_fwctl_focaltech_get_finger_pointid(struct shtps_fwctl_info *fc_p, u8 *buf_p)
{
	return buf_p[FTS_TOUCH_ID_POS] >> 4;
}

/* -------------------------------------------------------------------------- */
static int shtps_fwctl_focaltech_get_finger_pos_x(struct shtps_fwctl_info *fc_p, u8 *buf_p)
{
	return (s16) (buf_p[FTS_TOUCH_X_H_POS] & 0x0F) << 8
	     | (s16) buf_p[FTS_TOUCH_X_L_POS];
}

/* -------------------------------------------------------------------------- */
static int shtps_fwctl_focaltech_get_finger_pos_y(struct shtps_fwctl_info *fc_p, u8 *buf_p)
{
	return (s16) (buf_p[FTS_TOUCH_Y_H_POS] & 0x0F) << 8
	     | (s16) buf_p[FTS_TOUCH_Y_L_POS];
}

/* -------------------------------------------------------------------------- */
static int shtps_fwctl_focaltech_get_finger_wx(struct shtps_fwctl_info *fc_p, u8 *buf_p)
{
	return buf_p[FTS_TOUCH_AREA] >> 4;
}

/* -------------------------------------------------------------------------- */
static int shtps_fwctl_focaltech_get_finger_wy(struct shtps_fwctl_info *fc_p, u8 *buf_p)
{
	return buf_p[FTS_TOUCH_AREA] >> 4;
}

/* -------------------------------------------------------------------------- */
static int shtps_fwctl_focaltech_get_finger_z(struct shtps_fwctl_info *fc_p, u8 *buf_p)
{
	u8 weight = buf_p[FTS_TOUCH_WEIGHT];
	if(weight == 0){
		return FTS_PRESS;
	}
	return weight;
}

/* -------------------------------------------------------------------------- */
static void shtps_fwctl_focaltech_get_gesture(struct shtps_fwctl_info *fc_p, int fingerMax, u8 *buf_p, u8 *gs1_p, u8 *gs2_p)
{
	SHTPS_LOG_FWCTL_FUNC_CALL();
	*gs1_p = 0;
	*gs2_p = 0;
}

/* -------------------------------------------------------------------------- */
static int shtps_fwctl_focaltech_get_keystate(struct shtps_fwctl_info *fc_p, u8 *status_p)
{
	return 0;
}

/* -------------------------------------------------------------------------- */
static int shtps_fwctl_focaltech_get_gesturetype(struct shtps_fwctl_info *fc_p, u8 *status_p)
{
	int rc = 0;

	rc = M_READ_FUNC(fc_p, 0xD3, status_p, 1);

	SHTPS_LOG_DBG_PRINT("%s() get gesture <0x%02X><%s>\n", __func__, *status_p,
							(*status_p == FTS_GESTURE_LEFT) ? "left" :
							(*status_p == FTS_GESTURE_RIGHT) ? "right" :
							(*status_p == FTS_GESTURE_UP) ? "up" :
							(*status_p == FTS_GESTURE_DOWN) ? "down" :
							(*status_p == FTS_GESTURE_DOUBLECLICK) ? "double click" :
							(*status_p == FTS_GESTURE_O) ? "O" :
							(*status_p == FTS_GESTURE_W) ? "W" :
							(*status_p == FTS_GESTURE_M) ? "M" :
							(*status_p == FTS_GESTURE_E) ? "E" :
							(*status_p == FTS_GESTURE_L) ? "L" :
							(*status_p == FTS_GESTURE_S) ? "S" :
							(*status_p == FTS_GESTURE_V) ? "V" :
							(*status_p == FTS_GESTURE_Z) ? "Z" : "Unkown");

	switch(*status_p){
		case FTS_GESTURE_LEFT:
		case FTS_GESTURE_RIGHT:
		case FTS_GESTURE_UP:
		case FTS_GESTURE_DOWN:
			*status_p = SHTPS_GESTURE_TYPE_ONE_FINGER_SWIPE;
			break;
		case FTS_GESTURE_DOUBLECLICK:
			*status_p = SHTPS_GESTURE_TYPE_ONE_FINGER_DOUBLE_TAP;
			break;
	}

	return rc;
}

/* -------------------------------------------------------------------------- */
static int shtps_fwctl_focaltech_get_fwdate(struct shtps_fwctl_info *fc_p, u8 *year_p, u8 *month_p)
{
	return 0;
}

static int shtps_fwctl_focaltech_get_serial_number(struct shtps_fwctl_info *fc_p, u8 *buf_p)
{
	return -1;
}

/* -------------------------------------------------------------------------- */
static int shtps_fwctl_focaltech_get_fwver(struct shtps_fwctl_info *fc_p, u16 *ver_p)
{
	int rc=0;
	u8 buf[2] = { 0x00, 0x00 };

	SHTPS_LOG_FWCTL_FUNC_CALL();
	*ver_p = 0;

	rc = M_READ_FUNC(fc_p, FTS_REG_FW_VER, &buf[0], 1);
	if(rc == 0){
		rc = M_READ_FUNC(fc_p, FTS_REG_MODEL_VER, &buf[1], 1);
		if(rc == 0){
			*ver_p = (buf[1] << 8) + buf[0];
		}
	}
	return rc;
}

/* -------------------------------------------------------------------------- */
static int shtps_fwctl_focaltech_get_tm_mode(struct shtps_fwctl_info *fc_p)
{
	return 0;
}

/* -------------------------------------------------------------------------- */
static int shtps_fwctl_focaltech_get_tm_rxsize(struct shtps_fwctl_info *fc_p)
{
	SHTPS_LOG_FWCTL_FUNC_CALL();
	return SHTPS_TM_RXNUM_MAX;
}

/* -------------------------------------------------------------------------- */
static int shtps_fwctl_focaltech_get_tm_txsize(struct shtps_fwctl_info *fc_p)
{
	SHTPS_LOG_FWCTL_FUNC_CALL();
	return SHTPS_TM_TXNUM_MAX;
}

/* -------------------------------------------------------------------------- */
static int shtps_fwctl_focaltech_get_tm_startscan(struct shtps_fwctl_info *fc_p, int scan_mode)
{
	unsigned char RegVal = 0x00;
	unsigned char times = 0;
//	const unsigned char MaxTimes = 20;
	const unsigned char MaxTimes = 200;
	int ReCode = ERROR_CODE_COMM_ERROR;

	if(fc_p->scan_mode_set_flg == 0){
		fc_p->scan_mode_set_flg = 1;
		M_WRITE_FUNC(fc_p, FTS_REG_SCAN_MODE_ADDR, scan_mode);
	}

	ReCode = M_READ_FUNC(fc_p, FTS_DEVIDE_MODE_ADDR,&RegVal, 1);
	if(ReCode == ERROR_CODE_OK)
	{
		RegVal |= 0x80;
		ReCode = M_WRITE_FUNC(fc_p, FTS_DEVIDE_MODE_ADDR,RegVal);
		if(ReCode == ERROR_CODE_OK)
		{
			while(times++ < MaxTimes)
			{
//				msleep(8);
				msleep(10);
				ReCode = M_READ_FUNC(fc_p, FTS_DEVIDE_MODE_ADDR, &RegVal, 1);
				if(ReCode == ERROR_CODE_OK)
				{
					if((RegVal>>7) == 0)	break;
				}
				else
				{
					break;
				}
			}
			if(times < MaxTimes)	ReCode = ERROR_CODE_OK;
			else ReCode = ERROR_CODE_COMM_ERROR;
		}
	}
	return ReCode;
}

/* -------------------------------------------------------------------------- */
static int shtps_fwctl_focaltech_get_tm_rawdata(struct shtps_fwctl_info *fc_p, u8 tm_mode, u8 *tm_data_p)
{
	int ReCode=ERROR_CODE_COMM_ERROR;
	unsigned char *pReadData;
	int i, iReadNum;
	int ByteNum;
	unsigned short BytesNumInTestMode1=0;
	int ch_y_num = shtps_fwctl_focaltech_get_tm_txsize(fc_p);
	int ch_x_num = shtps_fwctl_focaltech_get_tm_rxsize(fc_p);
	int j;
	unsigned short *tm_data_short_p, *read_data_short_p;

	ByteNum = ch_y_num * ch_x_num * 2;
	pReadData = (u8 *)kzalloc(ByteNum, GFP_KERNEL);
	if(pReadData == NULL){
		SHTPS_LOG_ERR_PRINT("%s(): alloc error size=%d\n", __func__, ByteNum);
		return ERROR_CODE_ALLOCATE_BUFFER_ERROR;
	}

	iReadNum=ByteNum/342;

	if(0 != (ByteNum%342)) iReadNum++;

	if(ByteNum <= 342)
	{
		BytesNumInTestMode1 = ByteNum;
	}
	else
	{
		BytesNumInTestMode1 = 342;
	}

	ReCode = M_WRITE_FUNC(fc_p, FTS_REG_LINE_NUM, FTS_REGVAL_LINE_NUM_CHANNEL_AREA);


	//***********************************************************Read raw data in test mode1
	if(ReCode == ERROR_CODE_OK)
	{
		msleep(10);
		ReCode = M_READ_FUNC(fc_p, FTS_REG_RawBuf0, pReadData, BytesNumInTestMode1);
	}

	for(i=1; i<iReadNum; i++)
	{
		if(ReCode != ERROR_CODE_OK) break;

		if(i==iReadNum-1)//last packet
		{
			msleep(10);
			ReCode = M_READ_FUNC(fc_p, 0x7FFF, pReadData+342*i, ByteNum-342*i);
		}
		else
		{
			msleep(10);
			ReCode = M_READ_FUNC(fc_p, 0x7FFF, pReadData+342*i, 342);	
		}

	}

	if(ReCode == ERROR_CODE_OK)
	{
		for(i=0; i<(ByteNum>>1); i++)
		{
			char temp_data;
			temp_data = pReadData[i<<1];
			pReadData[i<<1] = pReadData[(i<<1)+1];
			pReadData[(i<<1)+1] = temp_data;
//			pRevBuffer[i] = (pReadData[i<<1]<<8)+pReadData[(i<<1)+1];
			//if(pRevBuffer[i] & 0x8000)
			//{
			//	pRevBuffer[i] -= 0xffff + 1;
			//}
		}

		// 1 2 3 4 5      B 6 1
		// 6 7 8 9 A  ->  C 7 2
		// B C D E F      D 8 3
		//                E 9 4
		//                F A 5
		//
		tm_data_short_p = (unsigned short*)tm_data_p;
		read_data_short_p = (unsigned short*)pReadData;
		for(i = 0; i < ch_y_num; i++){
			for(j = 0; j < ch_x_num; j++){
				tm_data_short_p[(i * ch_x_num) + j] = read_data_short_p[((ch_x_num - j - 1) * ch_y_num) + i];
			}
		}
	}

	kfree(pReadData);
	return ReCode;
}

/* -------------------------------------------------------------------------- */
static int shtps_fwctl_focaltech_get_tm_cbdata(struct shtps_fwctl_info *fc_p, u8 tm_mode, u8 *tm_data_p)
{
	int ReCode = ERROR_CODE_OK;
	unsigned short usReturnNum = 0;
	unsigned short usTotalReturnNum = 0;
	int i, iReadNum;
	int ch_y_num = shtps_fwctl_focaltech_get_tm_txsize(fc_p);
	int ch_x_num = shtps_fwctl_focaltech_get_tm_rxsize(fc_p);
	int j;
	unsigned short ReadNum;
	unsigned char *pReadBuffer;
	unsigned short StartNodeNo = 0;
	unsigned short *tm_data_short_p;

	ReadNum = ch_y_num * ch_x_num;
	pReadBuffer = (u8 *)kzalloc(ReadNum, GFP_KERNEL);
	if(pReadBuffer == NULL){
		SHTPS_LOG_ERR_PRINT("%s(): alloc error size=%d\n", __func__, ReadNum);
		return ERROR_CODE_ALLOCATE_BUFFER_ERROR;
	}

	iReadNum = ReadNum/342;

	if(0 != (ReadNum%342)) iReadNum++;

	usTotalReturnNum = 0;

	for(i = 1; i <= iReadNum; i++)
	{
		if(i*342 > ReadNum)
			usReturnNum = ReadNum - (i-1)*342;
		else
			usReturnNum = 342;

		M_WRITE_FUNC(fc_p, FTS_REG_CbAddrH, (StartNodeNo+usTotalReturnNum) >>8);
		M_WRITE_FUNC(fc_p, FTS_REG_CbAddrL, (StartNodeNo+usTotalReturnNum)&0xff);

		ReCode = M_READ_FUNC(fc_p, FTS_REG_CbBuf0, pReadBuffer+usTotalReturnNum, usReturnNum);

		usTotalReturnNum += usReturnNum;

		if(ReCode != ERROR_CODE_OK){
			break;
		}
	}

	if(ReCode == ERROR_CODE_OK)
	{
		// (char)         (short)
		// 1 2 3 4 5      B 6 1
		// 6 7 8 9 A  ->  C 7 2
		// B C D E F      D 8 3
		//                E 9 4
		//                F A 5
		//
		tm_data_short_p = (unsigned short*)tm_data_p;
		for(i = 0; i < ch_y_num; i++){
			for(j = 0; j < ch_x_num; j++){
				tm_data_short_p[(i * ch_x_num) + j] = (unsigned short)pReadBuffer[((ch_x_num - j - 1) * ch_y_num) + i];
			}
		}
	}

	#if defined( SHTPS_LOG_DEBUG_ENABLE )
	{
		for(i = 0;i < ch_y_num;i++){
			for(j = 0, sensor_log_outstr[0] = '\0';j < ch_x_num;j++){
				sprintf(sensor_log_tmp, "%05d", 
					(signed short)(tm_data_p[(i * ch_x_num * 2) + (j * 2) + 1] << 0x08 | 
					               tm_data_p[(i * ch_x_num * 2) + (j * 2)]));
				if(j < (ch_x_num - 1)){
					strcat(sensor_log_tmp, ", ");
				}
				strcat(sensor_log_outstr, sensor_log_tmp);
			}
			SHTPS_LOG_SENSOR_DATA_PRINT("[%02d]%s\n", i, sensor_log_outstr);
		}
	}
	#endif /* SHTPS_LOG_DEBUG_ENABLE */

	kfree(pReadBuffer);
	return ReCode;
}

/* -------------------------------------------------------------------------- */
static int shtps_fwctl_focaltech_get_tm_frameline(struct shtps_fwctl_info *fc_p, u8 tm_mode, u8 *tm_data_p)
{
	int ret;

	ret = shtps_fwctl_focaltech_get_tm_startscan(fc_p, FTS_REGVAL_SCAN_MODE_DIFFER);
	if(ret < 0){
		SHTPS_LOG_ERR_PRINT("%s(): startscan error!\n", __func__);
		return ret;
	}

	ret = shtps_fwctl_focaltech_get_tm_rawdata(fc_p, tm_mode, tm_data_p);
	if(ret < 0){
		SHTPS_LOG_ERR_PRINT("%s(): get rawdata error!\n", __func__);
		return ret;
	}

	#if defined( SHTPS_LOG_DEBUG_ENABLE )
	{
		int ch_y_num = shtps_fwctl_focaltech_get_tm_txsize(fc_p);
		int ch_x_num = shtps_fwctl_focaltech_get_tm_rxsize(fc_p);
		int i, j;
		for(i = 0; i < ch_y_num; i++){
			for(j = 0, sensor_log_outstr[0] = '\0'; j < ch_x_num; j++){
				sprintf(sensor_log_tmp, "%6d",
					(signed short)(tm_data_p[(i * ch_x_num * 2) + (j * 2) + 1] << 0x08 | 
					               tm_data_p[(i * ch_x_num * 2) + (j * 2)]));
				if(j < (ch_x_num - 1)){
					strcat(sensor_log_tmp, ", ");
				}
				strcat(sensor_log_outstr, sensor_log_tmp);
			}
			SHTPS_LOG_SENSOR_DATA_PRINT("[%02d]%s\n", i, sensor_log_outstr);
		}
	}
	#endif /* SHTPS_LOG_DEBUG_ENABLE */
	return 0;
}

/* -------------------------------------------------------------------------- */
static int shtps_fwctl_focaltech_get_tm_baseline(struct shtps_fwctl_info *fc_p, u8 tm_mode, u8 *tm_data_p)
{
	int ret;
	ret = shtps_fwctl_focaltech_get_tm_startscan(fc_p, FTS_REGVAL_SCAN_MODE_RAW);
	if(ret < 0){
		return ret;
	}
	ret = shtps_fwctl_focaltech_get_tm_rawdata(fc_p, tm_mode, tm_data_p);

	#if defined( SHTPS_LOG_DEBUG_ENABLE )
		if(ret == 0){
			int ch_y_num = shtps_fwctl_focaltech_get_tm_txsize(fc_p);
			int ch_x_num = shtps_fwctl_focaltech_get_tm_rxsize(fc_p);
			int i, j;
			for(i = 0;i < ch_y_num;i++){
				for(j = 0, sensor_log_outstr[0] = '\0';j < ch_x_num;j++){
					sprintf(sensor_log_tmp, "%6d", 
						(signed short)(tm_data_p[(i * ch_x_num * 2) + (j * 2) + 1] << 0x08 | 
						               tm_data_p[(i * ch_x_num * 2) + (j * 2)]));
					if(j < (ch_x_num - 1)){
						strcat(sensor_log_tmp, ", ");
					}
					strcat(sensor_log_outstr, sensor_log_tmp);
				}
				SHTPS_LOG_SENSOR_DATA_PRINT("[%02d]%s\n", i, sensor_log_outstr);
			}
		}
	#endif /* SHTPS_LOG_DEBUG_ENABLE */

	return ret;
}

/* -------------------------------------------------------------------------- */
static int shtps_fwctl_focaltech_get_tm_baseline_raw(struct shtps_fwctl_info *fc_p, u8 tm_mode, u8 *tm_data_p)
{
	return shtps_fwctl_focaltech_get_tm_baseline(fc_p, tm_mode, tm_data_p);
}

/* -------------------------------------------------------------------------- */
static int shtps_fwctl_focaltech_cmd_tm_frameline(struct shtps_fwctl_info *fc_p, u8 tm_mode)
{
	return 0;
}

/* -------------------------------------------------------------------------- */
static int shtps_fwctl_focaltech_cmd_tm_baseline(struct shtps_fwctl_info *fc_p, u8 tm_mode)
{
	return 0;
}

/* -------------------------------------------------------------------------- */
static int shtps_fwctl_focaltech_cmd_tm_baseline_raw(struct shtps_fwctl_info *fc_p, u8 tm_mode)
{
	return 0;
}

/* -------------------------------------------------------------------------- */
static int shtps_fwctl_focaltech_initparam(struct shtps_fwctl_info *fc_p)
{
	return 0;
}

/* -------------------------------------------------------------------------- */
static int shtps_fwctl_focaltech_initparam_activemode(struct shtps_fwctl_info *fc_p)
{
	return 0;
}

/* -------------------------------------------------------------------------- */
static int shtps_fwctl_focaltech_initparam_dozemode(struct shtps_fwctl_info *fc_p)
{
	return 0;
}
/* -------------------------------------------------------------------------- */
static int shtps_fwctl_focaltech_initparam_key(struct shtps_fwctl_info *fc_p)
{
	return 0;
}

/* -------------------------------------------------------------------------- */
static int shtps_fwctl_focaltech_initparam_lpwgmode(struct shtps_fwctl_info *fc_p)
{
	return 0;
}

/* -------------------------------------------------------------------------- */
static int shtps_fwctl_focaltech_initparam_autorezero(struct shtps_fwctl_info *fc_p)
{
	return 0;
}

/* -------------------------------------------------------------------------- */
static int shtps_fwctl_focaltech_initparam_reportrate(struct shtps_fwctl_info *fc_p, int mode)
{
	return 0;
}

/* -------------------------------------------------------------------------- */
static int shtps_fwctl_focaltech_initparam_set_custom_report_rate(struct shtps_fwctl_info *fc_p)
{
	return 0;
}

/* -------------------------------------------------------------------------- */
static int shtps_fwctl_focaltech_pen_enable(struct shtps_fwctl_info *fc_p)
{
	return 0;
}

/* -------------------------------------------------------------------------- */
static int shtps_fwctl_focaltech_pen_disable(struct shtps_fwctl_info *fc_p)
{
	return 0;
}

/* -------------------------------------------------------------------------- */
static int shtps_fwctl_focaltech_start_testmode(struct shtps_fwctl_info *fc_p, u8 tm_mode)
{
	unsigned char RunState = 0;
	int ReCode = ERROR_CODE_COMM_ERROR;
	
	ReCode = M_READ_FUNC(fc_p, FTS_DEVIDE_MODE_ADDR, &RunState, 1);
	if(ReCode == ERROR_CODE_OK)
	{
		if(((RunState>>4)&0x07) == 0x04)	//factory
		{
			ReCode = ERROR_CODE_OK;
		}
		else
		{
			ReCode = M_WRITE_FUNC(fc_p, FTS_DEVIDE_MODE_ADDR, 0x40);
			if(ReCode == ERROR_CODE_OK)
			{
				ReCode = M_READ_FUNC(fc_p, FTS_DEVIDE_MODE_ADDR, &RunState, 1);
				if(ReCode == ERROR_CODE_OK)
				{	
					if(((RunState>>4)&0x07) == 0x04){
						ReCode = ERROR_CODE_OK;
					}
					else{
						ReCode = ERROR_CODE_COMM_ERROR;
					}
				}
			}
		}
	}

	fc_p->scan_mode_set_flg = 0;

	return ReCode;
}

/* -------------------------------------------------------------------------- */
static int shtps_fwctl_focaltech_stop_testmode(struct shtps_fwctl_info *fc_p)
{
	unsigned char RunState = 0;
	int ReCode = ERROR_CODE_COMM_ERROR;

	ReCode = M_READ_FUNC(fc_p, FTS_DEVIDE_MODE_ADDR, &RunState, 1);
	if(ReCode == ERROR_CODE_OK)
	{
		if(((RunState>>4)&0x07) == 0x00)	//work
		{
			ReCode = ERROR_CODE_OK;
		}
		else
		{
			ReCode = M_WRITE_FUNC(fc_p, FTS_DEVIDE_MODE_ADDR, 0);
			if(ReCode == ERROR_CODE_OK)
			{
				ReCode = M_READ_FUNC(fc_p, FTS_DEVIDE_MODE_ADDR, &RunState, 1);
				if(ReCode == ERROR_CODE_OK)
				{	
					if(((RunState>>4)&0x07) == 0x00)	ReCode = ERROR_CODE_OK;
					else	ReCode = ERROR_CODE_COMM_ERROR;
				}
			}
		}
	}

	return ReCode;
}

/* -------------------------------------------------------------------------- */
static int shtps_fwctl_focaltech_baseline_offset_disable(struct shtps_fwctl_info *fc_p)
{
	return 0;
}
/* -------------------------------------------------------------------------- */
static void shtps_fwctl_focaltech_set_dev_state(struct shtps_fwctl_info *fc_p, u8 state)
{
	SHTPS_LOG_FWCTL_FUNC_CALL();
	if(fc_p->dev_state != state){
		SHTPS_LOG_ANALYSIS("[dev_state] set (%s -> %s)\n",
								(fc_p->dev_state == SHTPS_DEV_STATE_SLEEP) ? "sleep" :
								(fc_p->dev_state == SHTPS_DEV_STATE_DOZE) ? "doze" :
								(fc_p->dev_state == SHTPS_DEV_STATE_ACTIVE) ? "active" :
								(fc_p->dev_state == SHTPS_DEV_STATE_LPWG) ? "lpwg" :
								(fc_p->dev_state == SHTPS_DEV_STATE_LOADER) ? "loader" :
								(fc_p->dev_state == SHTPS_DEV_STATE_TESTMODE) ? "testmode" : "unknown",
								(state == SHTPS_DEV_STATE_SLEEP) ? "sleep" :
								(state == SHTPS_DEV_STATE_DOZE) ? "doze" :
								(state == SHTPS_DEV_STATE_ACTIVE) ? "active" :
								(state == SHTPS_DEV_STATE_LPWG) ? "lpwg" :
								(state == SHTPS_DEV_STATE_LOADER) ? "loader" :
								(state == SHTPS_DEV_STATE_TESTMODE) ? "testmode" : "unknown" );
	}

	fc_p->dev_state = state;
}

/* -------------------------------------------------------------------------- */
static u8 shtps_fwctl_focaltech_get_dev_state(struct shtps_fwctl_info *fc_p)
{
	SHTPS_LOG_FWCTL_FUNC_CALL();
	return fc_p->dev_state;
}

/* -------------------------------------------------------------------------- */
static int shtps_fwctl_focaltech_get_maxXPosition(struct shtps_fwctl_info *fc_p)
{
	return CONFIG_SHTPS_FOCALTECH_LCD_SIZE_X;
}

/* -------------------------------------------------------------------------- */
static int shtps_fwctl_focaltech_get_maxYPosition(struct shtps_fwctl_info *fc_p)
{
	return CONFIG_SHTPS_FOCALTECH_LCD_SIZE_Y;
}

/* -------------------------------------------------------------------------- */
static int shtps_fwctl_focaltech_cover_mode_on(struct shtps_fwctl_info *fc_p)
{
	M_WRITE_FUNC(fc_p, 0xC1, 0x01);
	return 0;
}

/* -------------------------------------------------------------------------- */
static int shtps_fwctl_focaltech_cover_mode_off(struct shtps_fwctl_info *fc_p)
{
	M_WRITE_FUNC(fc_p, 0xC1, 0x00);
	return 0;
}

/* -------------------------------------------------------------------------- */
static int shtps_fwctl_focaltech_set_custom_report_rate(struct shtps_fwctl_info *fc_p, u8 rate)
{
	return 0;
}

/* -------------------------------------------------------------------------- */
static int shtps_fwctl_focaltech_set_lpwg_sweep_on(struct shtps_fwctl_info *fc_p, u8 enable)
{
	return 0;
}

/* -------------------------------------------------------------------------- */
static int shtps_fwctl_focaltech_set_lpwg_double_tap(struct shtps_fwctl_info *fc_p, u8 enable)
{
	return 0;
}

/* -------------------------------------------------------------------------- */
static int shtps_fwctl_focaltech_glove_enable(struct shtps_fwctl_info *fc_p)
{
	return 0;
}

static int shtps_fwctl_focaltech_glove_disable(struct shtps_fwctl_info *fc_p)
{
	return 0;
}


/* -------------------------------------------------------------------------- */
struct shtps_fwctl_functbl		shtps_fwctl_focaltech_function_table = {
		.ic_init_f									= shtps_fwctl_focaltech_ic_init,
		.loader_write_pram_f						= shtps_fwctl_focaltech_loader_write_pram,
		.loader_upgrade_f							= shtps_fwctl_focaltech_loader_upgrade,
		.get_device_status_f						= shtps_fwctl_focaltech_get_device_status,
		.soft_reset_f								= shtps_fwctl_focaltech_soft_reset,
		.irqclear_get_irqfactor_f					= shtps_fwctl_focaltech_irqclear_get_irqfactor,
		.rezero_f									= shtps_fwctl_focaltech_rezero,
		.map_construct_f							= shtps_fwctl_focaltech_map_construct,
		.is_sleeping_f								= shtps_fwctl_focaltech_is_sleeping,
		.is_singlefinger_f							= shtps_fwctl_focaltech_is_singlefinger,
		.set_doze_f									= shtps_fwctl_focaltech_set_doze,
		.set_doze_param_f							= shtps_fwctl_focaltech_set_doze_param,
		.set_active_f								= shtps_fwctl_focaltech_set_active,
		.set_sleepmode_on_f							= shtps_fwctl_focaltech_set_sleepmode_on,
		.set_sleepmode_off_f						= shtps_fwctl_focaltech_set_sleepmode_off,
		.set_lpwg_mode_on_f							= shtps_fwctl_focaltech_set_lpwg_mode_on,
		.set_lpwg_mode_off_f						= shtps_fwctl_focaltech_set_lpwg_mode_off,
		.set_lpwg_mode_cal_f						= shtps_fwctl_focaltech_set_lpwg_mode_cal,
		.set_low_reportrate_mode_f					= shtps_fwctl_focaltech_set_low_reportrate_mode,
		.get_fingermax_f							= shtps_fwctl_focaltech_get_fingermax,
		.get_fingerinfo_f							= shtps_fwctl_focaltech_get_fingerinfo,
		.get_one_fingerinfo_f						= shtps_fwctl_focaltech_get_one_fingerinfo,
		.get_num_of_touch_fingers_f					= shtps_fwctl_focaltech_get_num_of_touch_fingers,
		.get_finger_info_buf_f						= shtps_fwctl_focaltech_get_finger_info_buf,
		.get_finger_state_f							= shtps_fwctl_focaltech_get_finger_state,
		.get_finger_pointid_f						= shtps_fwctl_focaltech_get_finger_pointid,
		.get_finger_pos_x_f							= shtps_fwctl_focaltech_get_finger_pos_x,
		.get_finger_pos_y_f							= shtps_fwctl_focaltech_get_finger_pos_y,
		.get_finger_wx_f							= shtps_fwctl_focaltech_get_finger_wx,
		.get_finger_wy_f							= shtps_fwctl_focaltech_get_finger_wy,
		.get_finger_z_f								= shtps_fwctl_focaltech_get_finger_z,
		.get_gesture_f								= shtps_fwctl_focaltech_get_gesture,
		.get_keystate_f								= shtps_fwctl_focaltech_get_keystate,
		.get_gesturetype_f							= shtps_fwctl_focaltech_get_gesturetype,
		.get_fwdate_f								= shtps_fwctl_focaltech_get_fwdate,
		.get_serial_number_f						= shtps_fwctl_focaltech_get_serial_number,
		.get_fwver_f								= shtps_fwctl_focaltech_get_fwver,
		.get_tm_mode_f								= shtps_fwctl_focaltech_get_tm_mode,
		.get_tm_rxsize_f							= shtps_fwctl_focaltech_get_tm_rxsize,
		.get_tm_txsize_f							= shtps_fwctl_focaltech_get_tm_txsize,
		.get_tm_frameline_f							= shtps_fwctl_focaltech_get_tm_frameline,
		.get_tm_baseline_f							= shtps_fwctl_focaltech_get_tm_baseline,
		.get_tm_baseline_raw_f						= shtps_fwctl_focaltech_get_tm_baseline_raw,
		.get_tm_cbdata_f							= shtps_fwctl_focaltech_get_tm_cbdata,
		.cmd_tm_frameline_f							= shtps_fwctl_focaltech_cmd_tm_frameline,
		.cmd_tm_baseline_f							= shtps_fwctl_focaltech_cmd_tm_baseline,
		.cmd_tm_baseline_raw_f						= shtps_fwctl_focaltech_cmd_tm_baseline_raw,
		.initparam_f								= shtps_fwctl_focaltech_initparam,
		.initparam_activemode_f						= shtps_fwctl_focaltech_initparam_activemode,
		.initparam_dozemode_f						= shtps_fwctl_focaltech_initparam_dozemode,
		.initparam_key_f							= shtps_fwctl_focaltech_initparam_key,
		.initparam_lpwgmode_f						= shtps_fwctl_focaltech_initparam_lpwgmode,
		.initparam_autorezero_f						= shtps_fwctl_focaltech_initparam_autorezero,
		.initparam_reportrate_f						= shtps_fwctl_focaltech_initparam_reportrate,
		.initparam_set_custom_report_rate_f			= shtps_fwctl_focaltech_initparam_set_custom_report_rate,
		.pen_enable_f								= shtps_fwctl_focaltech_pen_enable,
		.pen_disable_f								= shtps_fwctl_focaltech_pen_disable,
		.start_testmode_f							= shtps_fwctl_focaltech_start_testmode,
		.stop_testmode_f							= shtps_fwctl_focaltech_stop_testmode,
		.baseline_offset_disable_f					= shtps_fwctl_focaltech_baseline_offset_disable,
		.set_dev_state_f							= shtps_fwctl_focaltech_set_dev_state,
		.get_dev_state_f							= shtps_fwctl_focaltech_get_dev_state,
		.get_maxXPosition_f							= shtps_fwctl_focaltech_get_maxXPosition,
		.get_maxYPosition_f							= shtps_fwctl_focaltech_get_maxYPosition,
		.cover_mode_on_f							= shtps_fwctl_focaltech_cover_mode_on,
		.cover_mode_off_f							= shtps_fwctl_focaltech_cover_mode_off,
		.set_custom_report_rate_f					= shtps_fwctl_focaltech_set_custom_report_rate,
		.set_lpwg_sweep_on_f						= shtps_fwctl_focaltech_set_lpwg_sweep_on,
		.set_lpwg_double_tap_f						= shtps_fwctl_focaltech_set_lpwg_double_tap,
		.glove_enable_f								= shtps_fwctl_focaltech_glove_enable,
		.glove_disable_f							= shtps_fwctl_focaltech_glove_disable,
};
/* -----------------------------------------------------------------------------------
*/
MODULE_DESCRIPTION("FocalTech fts TouchScreen driver");
MODULE_LICENSE("GPLv2");
