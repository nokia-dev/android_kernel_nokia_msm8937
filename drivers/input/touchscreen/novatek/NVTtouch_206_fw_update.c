/* drivers/input/touchscreen/nt11206/NVTtouch_206_fw_update.c
 *
 * Copyright (C) 2010 - 2016 Novatek, Inc.
 *
 * $Revision: 5216 $
 * $Date: 2016-06-17 16:20:17 +0800 (週五, 17 六月 2016) $
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */

#include <linux/delay.h>
#include <linux/firmware.h>

#include "NVTtouch_206.h"

//#if BOOT_UPDATE_FIRMWARE
#if (BOOT_UPDATE_FIRMWARE||NVT_TOUCH_EXT_FIH)

#define FW_BIN_SIZE_124KB 126976
#define FW_BIN_VER_OFFSET 0x1E000
#define FW_BIN_VER_BAR_OFFSET 0x1E001
#define FLASH_SECTOR_SIZE 4096
#define SIZE_64KB 65536
#define BLOCK_64KB_NUM 2

const struct firmware *fw_entry = NULL;

extern struct nvt_ts_data *ts;
extern int32_t CTP_I2C_READ(struct i2c_client *client, uint16_t address, uint8_t *buf, uint16_t len);
extern int32_t CTP_I2C_READ_DUMMY(struct i2c_client *client, uint16_t address);
extern int32_t CTP_I2C_WRITE(struct i2c_client *client, uint16_t address, uint8_t *buf, uint16_t len);
extern void nvt_hw_reset(void);
extern void nvt_bootloader_reset(void);
extern int32_t nvt_check_fw_reset_state(RST_COMPLETE_STATE check_reset_state);
extern void nvt_sw_reset_idle(void);

/*******************************************************
Description:
	Novatek touchscreen request update firmware function.

return:
	Executive outcomes. 0---succeed. -1,-22---failed.
*******************************************************/
int32_t update_firmware_request(char *filename)
{
	int32_t ret = 0;

	if (NULL == filename) {
		return -1;
	}

	dev_info(&ts->client->dev, "%s: filename is %s\n", __func__, filename);

	ret = request_firmware(&fw_entry, filename, &ts->client->dev);
	if (ret) {
		dev_err(&ts->client->dev, "%s: firmware load failed, ret=%d\n", __func__, ret);
		return ret;
	}

	// check bin file size (124kb)
	if (fw_entry->size != FW_BIN_SIZE_124KB) {
		dev_err(&ts->client->dev, "%s: bin file size not match. (%zu)\n", __func__, fw_entry->size);
		return -EINVAL;
	}

	// check if FW version add FW version bar equals 0xFF
	if (*(fw_entry->data + FW_BIN_VER_OFFSET) + *(fw_entry->data + FW_BIN_VER_BAR_OFFSET) != 0xFF) {
		dev_err(&ts->client->dev, "%s: bin file FW_VER + FW_VER_BAR should be 0xFF!\n", __func__);
		dev_err(&ts->client->dev, "%s: FW_VER=0x%02X, FW_VER_BAR=0x%02X\n", __func__, *(fw_entry->data+FW_BIN_VER_OFFSET), *(fw_entry->data+FW_BIN_VER_BAR_OFFSET));
		return -EINVAL;
	}

	return 0;
}

/*******************************************************
Description:
	Novatek touchscreen release update firmware function.

return:
	n.a.
*******************************************************/
void update_firmware_release(void)
{
	if (fw_entry) {
		release_firmware(fw_entry);
	}
	fw_entry=NULL;
}

/*******************************************************
Description:
	Novatek touchscreen check firmware version function.

return:
	Executive outcomes. 0---need update. 1---need not
	update.
*******************************************************/
int32_t Check_FW_Ver(void)
{
	uint8_t buf[16] = {0};
	int32_t ret = 0;

	//---dummy read to resume TP before writing command---
	CTP_I2C_READ_DUMMY(ts->client, I2C_HW_Address);

	//write i2c index to 0x14700
	buf[0] = 0xFF;
	buf[1] = 0x01;
	buf[2] = 0x47;
	ret = CTP_I2C_WRITE(ts->client, I2C_FW_Address, buf, 3);
	if (ret < 0) {
		dev_err(&ts->client->dev, "%s: i2c write error!(%d)\n", __func__, ret);
		return ret;
	}

	//read Firmware Version
	buf[0] = 0x78;
	buf[1] = 0x00;
	buf[2] = 0x00;
	ret = CTP_I2C_READ(ts->client, I2C_FW_Address, buf, 3);
	if (ret < 0) {
		dev_err(&ts->client->dev, "%s: i2c read error!(%d)\n", __func__, ret);
		return ret;
	}

	dev_info(&ts->client->dev, "IC FW Ver = 0x%02X, FW Ver Bar = 0x%02X\n", buf[1], buf[2]);
	dev_info(&ts->client->dev, "Bin FW Ver = 0x%02X, FW ver Bar = 0x%02X\n",
			fw_entry->data[FW_BIN_VER_OFFSET], fw_entry->data[FW_BIN_VER_BAR_OFFSET]);

	// check IC FW_VER + FW_VER_BAR equals 0xFF or not, need to update if not
	if ((buf[1] + buf[2]) != 0xFF) {
		dev_err(&ts->client->dev, "%s: IC FW_VER + FW_VER_BAR not equals to 0xFF!\n", __func__);
		return 0;
	}

	// compare IC and binary FW version
	if (buf[1] > fw_entry->data[FW_BIN_VER_OFFSET])
		return 1;
	else
		return 0;
}

/*******************************************************
Description:
	Novatek touchscreen check firmware checksum function.

return:
	Executive outcomes. 0---checksum not match.
	1---checksum match. -1--- checksum read failed.
*******************************************************/
int32_t Check_CheckSum(void)
{
	uint8_t buf[64] = {0};
	uint32_t XDATA_Addr = 0x14000;
	int32_t ret = 0;
	int32_t i = 0;
	int32_t k = 0;
	uint16_t WR_Filechksum[BLOCK_64KB_NUM] = {0};
	uint16_t RD_Filechksum[BLOCK_64KB_NUM] = {0};
	size_t fw_bin_size = 0;
	size_t len_in_blk = 0;
	int32_t retry = 0;

	fw_bin_size = fw_entry->size;

	for (i = 0; i < BLOCK_64KB_NUM; i++) {
		if (fw_bin_size > (i * SIZE_64KB)) {
			len_in_blk = min(fw_bin_size - i * SIZE_64KB, (size_t)SIZE_64KB);
			// Calculate WR_Filechksum of each 64KB block
			WR_Filechksum[i] = i + 0x00 + 0x00 + (((len_in_blk - 1) >> 8) & 0xFF) + ((len_in_blk - 1) & 0xFF);
			for (k = 0; k < len_in_blk; k++) {
				WR_Filechksum[i] += fw_entry->data[k + i * SIZE_64KB];
			}
			WR_Filechksum[i] = 65535 - WR_Filechksum[i] + 1;

			if (i == 0) {
				//---dummy read to resume TP before writing command---
				CTP_I2C_READ_DUMMY(ts->client, I2C_HW_Address);
			}

			// Fast Read Command
			buf[0] = 0x00;
			buf[1] = 0x07;
			buf[2] = i;
			buf[3] = 0x00;
			buf[4] = 0x00;
			buf[5] = ((len_in_blk - 1) >> 8) & 0xFF;
			buf[6] = (len_in_blk - 1) & 0xFF;
			ret = CTP_I2C_WRITE(ts->client, I2C_HW_Address, buf, 7);
			if (ret < 0) {
				dev_err(&ts->client->dev,"%s: Fast Read Command error!!(%d)\n", __func__, ret);
				return ret;
			}
			// Check 0xAA (Fast Read Command)
			retry = 0;
			while (1) {
				msleep(80);
				buf[0] = 0x00;
				buf[1] = 0x00;
				ret = CTP_I2C_READ(ts->client, I2C_HW_Address, buf, 2);
				if (ret < 0) {
					dev_err(&ts->client->dev,"%s: Check 0xAA (Fast Read Command) error!!(%d)\n", __func__, ret);
					return ret;
				}
				if (buf[1] == 0xAA) {
					break;
				}
				retry++;
				if (unlikely(retry > 5)) {
					dev_err(&ts->client->dev,"%s: Check 0xAA (Fast Read Command) failed, buf[1]=0x%02X, retry=%d\n", __func__, buf[1], retry);
					return -1;
				}
			}

			// Read Checksum (write addr high byte & middle byte)
			buf[0] = 0xFF;
			buf[1] = XDATA_Addr >> 16;
			buf[2] = (XDATA_Addr >> 8) & 0xFF;
			ret = CTP_I2C_WRITE(ts->client, I2C_FW_Address, buf, 3);
			if (ret < 0) {
				dev_err(&ts->client->dev,"%s: Read Checksum (write addr high byte & middle byte) error!!(%d)\n", __func__, ret);
				return ret;
			}
			// Read Checksum
			buf[0] = (XDATA_Addr) & 0xFF;
			buf[1] = 0x00;
			buf[2] = 0x00;
			ret = CTP_I2C_READ(ts->client, I2C_FW_Address, buf, 3);
			if (ret < 0) {
				dev_err(&ts->client->dev,"%s: Read Checksum error!!(%d)\n", __func__, ret);
				return ret;
			}

			RD_Filechksum[i] = (uint16_t)((buf[2] << 8) | buf[1]);
			if (WR_Filechksum[i] != RD_Filechksum[i]) {
				dev_err(&ts->client->dev,"RD_Filechksum[%d]=0x%04X, WR_Filechksum[%d]=0x%04X\n", i, RD_Filechksum[i], i, WR_Filechksum[i]);
				dev_err(&ts->client->dev, "%s: firmware checksum not match!!\n", __func__);
				return 0;
			}
		}
	}

	dev_info(&ts->client->dev, "%s: firmware checksum match\n", __func__);
	return 1;
}

/*******************************************************
Description:
	Novatek touchscreen initial bootloader and flash
	block function.

return:
	Executive outcomes. 0---succeed. negative---failed.
*******************************************************/
int32_t Init_BootLoader(void)
{
	uint8_t buf[64] = {0};
	int32_t ret = 0;

	// SW Reset & Idle
	nvt_sw_reset_idle();

	// Initiate Flash Block
	buf[0] = 0x00;
	buf[1] = 0x00;
	ret = CTP_I2C_WRITE(ts->client, I2C_HW_Address, buf, 2);
	if (ret < 0) {
		dev_err(&ts->client->dev,"%s: Inittial Flash Block error!!(%d)\n", __func__, ret);
		return ret;
	}
	msleep(5);

	// Check 0xAA (Initiate Flash Block)
	buf[0] = 0x00;
	buf[1] = 0x00;
	ret = CTP_I2C_READ(ts->client, I2C_HW_Address, buf, 2);
	if (ret < 0) {
		dev_err(&ts->client->dev,"%s: Check 0xAA (Inittial Flash Block) error!!(%d)\n", __func__, ret);
		return ret;
	}
	if (buf[1] != 0xAA) {
		dev_info(&ts->client->dev,"%s: Check 0xAA (Inittial Flash Block) error!! status=0x%02X\n", __func__, buf[1]);
		return -1;
	}

	dev_info(&ts->client->dev,"Init OK \n");
	msleep(20);

	return 0;
}

/*******************************************************
Description:
	Novatek touchscreen erase flash sectors function.

return:
	Executive outcomes. 0---succeed. negative---failed.
*******************************************************/
#if NVT_TOUCH_EXT_FIH
int32_t Erase_Flash(void)
{
	uint8_t buf[64] = {0};
	int32_t ret = 0;
	int32_t count = 0;
	int32_t i = 0;
	int32_t Flash_Address = 0;
	int32_t retry = 0;

	if (fw_entry->size % FLASH_SECTOR_SIZE)
		count = fw_entry->size / FLASH_SECTOR_SIZE + 1;
	else
		count = fw_entry->size / FLASH_SECTOR_SIZE;

	for(i = 0; i < count; i++) {
		// Write Enable
		buf[0] = 0x00;
		buf[1] = 0x06;
		ret = CTP_I2C_WRITE(ts->client, I2C_HW_Address, buf, 2);
		if (ret < 0) {
			dev_err(&ts->client->dev,"%s: Write Enable error!!(%d,%d)\n", __func__, ret, i);
			return ret;
		}
		msleep(4);//msleep(10);

		// Check 0xAA (Write Enable)
		buf[0] = 0x00;
		buf[1] = 0x00;
		ret = CTP_I2C_READ(ts->client, I2C_HW_Address, buf, 2);
		if (ret < 0) {
			dev_err(&ts->client->dev,"%s: Check 0xAA (Write Enable) error!!(%d,%d)\n", __func__, ret, i);
			return ret;
		}
		if (buf[1] != 0xAA) {
			dev_info(&ts->client->dev,"%s: Check 0xAA (Write Enable) error!! status=0x%02X\n", __func__, buf[1]);
			return -1;
		}
		//msleep(10);

		Flash_Address = i * FLASH_SECTOR_SIZE;

		// Sector Erase
		buf[0] = 0x00;
		buf[1] = 0x20;    // Command : Sector Erase
		buf[2] = ((Flash_Address >> 16) & 0xFF);
		buf[3] = ((Flash_Address >> 8) & 0xFF);
		buf[4] = (Flash_Address & 0xFF);
		ret = CTP_I2C_WRITE(ts->client, I2C_HW_Address, buf, 5);
		if (ret < 0) {
			dev_err(&ts->client->dev,"%s: Sector Erase error!!(%d,%d)\n", __func__, ret, i);
			return ret;
		}
		msleep(1);//msleep(20);

		retry = 0;
		while (1) {
			// Check 0xAA (Sector Erase)
			buf[0] = 0x00;
			buf[1] = 0x00;
			ret = CTP_I2C_READ(ts->client, I2C_HW_Address, buf, 2);
			if (ret < 0) {
				dev_err(&ts->client->dev,"%s: Check 0xAA (Sector Erase) error!!(%d,%d)\n", __func__, ret, i);
				return ret;
			}
			if (buf[1] == 0xAA) {
				break;
			}
			retry++;
			if (unlikely(retry > 5)) {
				dev_err(&ts->client->dev,"%s: Check 0xAA (Sector Erase) failed, buf[1]=0x%02X, retry=%d\n", __func__, buf[1], retry);
				return -1;
			}
		}

		// Read Status
		retry = 0;
		while (1) {
			msleep(4);//msleep(30);
			buf[0] = 0x00;
			buf[1] = 0x05;
			ret = CTP_I2C_WRITE(ts->client, I2C_HW_Address, buf, 2);
			if (ret < 0) {
				dev_err(&ts->client->dev,"%s: Read Status error!!(%d,%d)\n", __func__, ret, i);
				return ret;
			}

			// Check 0xAA (Read Status)
			buf[0] = 0x00;
			buf[1] = 0x00;
			buf[2] = 0x00;
			ret = CTP_I2C_READ(ts->client, I2C_HW_Address, buf, 3);
			if (ret < 0) {
				dev_err(&ts->client->dev,"%s: Check 0xAA (Read Status) error!!(%d,%d)\n", __func__, ret, i);
				return ret;
			}
			if ((buf[1] == 0xAA) && (buf[2] == 0x00)) {
				break;
			}
			retry++;
			if (unlikely(retry > 101)) {//if (unlikely(retry > 5)) {
				dev_err(&ts->client->dev,"%s:Check 0xAA (Read Status) failed, buf[1]=0x%02X, buf[2]=0x%02X, retry=%d\n", __func__, buf[1], buf[2], retry);
				return -1;
			}
		}
	}

	dev_info(&ts->client->dev,"Erase OK \n");
	return 0;
}
#else
int32_t Erase_Flash(void)
{
	uint8_t buf[64] = {0};
	int32_t ret = 0;
	int32_t count = 0;
	int32_t i = 0;
	int32_t Flash_Address = 0;
	int32_t retry = 0;

	if (fw_entry->size % FLASH_SECTOR_SIZE)
		count = fw_entry->size / FLASH_SECTOR_SIZE + 1;
	else
		count = fw_entry->size / FLASH_SECTOR_SIZE;

	for(i = 0; i < count; i++) {
		// Write Enable
		buf[0] = 0x00;
		buf[1] = 0x06;
		ret = CTP_I2C_WRITE(ts->client, I2C_HW_Address, buf, 2);
		if (ret < 0) {
			dev_err(&ts->client->dev,"%s: Write Enable error!!(%d,%d)\n", __func__, ret, i);
			return ret;
		}
		msleep(10);

		// Check 0xAA (Write Enable)
		buf[0] = 0x00;
		buf[1] = 0x00;
		ret = CTP_I2C_READ(ts->client, I2C_HW_Address, buf, 2);
		if (ret < 0) {
			dev_err(&ts->client->dev,"%s: Check 0xAA (Write Enable) error!!(%d,%d)\n", __func__, ret, i);
			return ret;
		}
		if (buf[1] != 0xAA) {
			dev_info(&ts->client->dev,"%s: Check 0xAA (Write Enable) error!! status=0x%02X\n", __func__, buf[1]);
			return -1;
		}
		msleep(10);

		Flash_Address = i * FLASH_SECTOR_SIZE;

		// Sector Erase
		buf[0] = 0x00;
		buf[1] = 0x20;    // Command : Sector Erase
		buf[2] = ((Flash_Address >> 16) & 0xFF);
		buf[3] = ((Flash_Address >> 8) & 0xFF);
		buf[4] = (Flash_Address & 0xFF);
		ret = CTP_I2C_WRITE(ts->client, I2C_HW_Address, buf, 5);
		if (ret < 0) {
			dev_err(&ts->client->dev,"%s: Sector Erase error!!(%d,%d)\n", __func__, ret, i);
			return ret;
		}
		msleep(20);

		retry = 0;
		while (1) {
			// Check 0xAA (Sector Erase)
			buf[0] = 0x00;
			buf[1] = 0x00;
			ret = CTP_I2C_READ(ts->client, I2C_HW_Address, buf, 2);
			if (ret < 0) {
				dev_err(&ts->client->dev,"%s: Check 0xAA (Sector Erase) error!!(%d,%d)\n", __func__, ret, i);
				return ret;
			}
			if (buf[1] == 0xAA) {
				break;
			}
			retry++;
			if (unlikely(retry > 5)) {
				dev_err(&ts->client->dev,"%s: Check 0xAA (Sector Erase) failed, buf[1]=0x%02X, retry=%d\n", __func__, buf[1], retry);
				return -1;
			}
		}

		// Read Status
		retry = 0;
		while (1) {
			msleep(30);
			buf[0] = 0x00;
			buf[1] = 0x05;
			ret = CTP_I2C_WRITE(ts->client, I2C_HW_Address, buf, 2);
			if (ret < 0) {
				dev_err(&ts->client->dev,"%s: Read Status error!!(%d,%d)\n", __func__, ret, i);
				return ret;
			}

			// Check 0xAA (Read Status)
			buf[0] = 0x00;
			buf[1] = 0x00;
			buf[2] = 0x00;
			ret = CTP_I2C_READ(ts->client, I2C_HW_Address, buf, 3);
			if (ret < 0) {
				dev_err(&ts->client->dev,"%s: Check 0xAA (Read Status) error!!(%d,%d)\n", __func__, ret, i);
				return ret;
			}
			if ((buf[1] == 0xAA) && (buf[2] == 0x00)) {
				break;
			}
			retry++;
			if (unlikely(retry > 5)) {
				dev_err(&ts->client->dev,"%s:Check 0xAA (Read Status) failed, buf[1]=0x%02X, buf[2]=0x%02X, retry=%d\n", __func__, buf[1], buf[2], retry);
				return -1;
			}
		}
	}

	dev_info(&ts->client->dev,"Erase OK \n");
	return 0;
}
#endif//#if NVT_TOUCH_EXT_FIH
/*******************************************************
Description:
	Novatek touchscreen write flash sectors function.

return:
	Executive outcomes. 0---succeed. negative---failed.
*******************************************************/
int32_t Write_Flash(void)
{
	uint8_t buf[64] = {0};
	uint32_t XDATA_Addr = 0x14002;
	uint32_t Flash_Address = 0;
	int32_t i = 0, j = 0, k = 0;
	uint8_t tmpvalue = 0;
	int32_t count = 0;
	int32_t ret = 0;
	int32_t retry = 0;

	// change I2C buffer index
	buf[0] = 0xFF;
	buf[1] = XDATA_Addr >> 16;
	buf[2] = (XDATA_Addr >> 8) & 0xFF;
	ret = CTP_I2C_WRITE(ts->client, I2C_FW_Address, buf, 3);
	if (ret < 0) {
		dev_err(&ts->client->dev,"%s: change I2C buffer index error!!(%d)\n", __func__, ret);
		return ret;
	}

	if (fw_entry->size % 256)
		count = fw_entry->size / 256 + 1;
	else
		count = fw_entry->size / 256;

	for (i = 0; i < count; i++) {
		Flash_Address = i * 256;

		// Write Enable
		buf[0] = 0x00;
		buf[1] = 0x06;
		ret = CTP_I2C_WRITE(ts->client, I2C_HW_Address, buf, 2);
		if (ret < 0) {
			dev_err(&ts->client->dev,"%s: Write Enable error!!(%d)\n", __func__, ret);
			return ret;
		}
		udelay(100);

		// Write Page : 256 bytes
		#if NVT_TOUCH_EXT_FIH
		{
			int32_t step = 55;
			int32_t restSize = min(fw_entry->size - i * 256, (size_t)256);
		    for (j = 0; j < restSize; j += step) {

				//[20161129,jx]Modify for non-factor of 256
					int32_t i32tail = step;
					if( (j + step) > restSize)
						i32tail = restSize - j;
				
				buf[0] = (XDATA_Addr + j) & 0xFF;	
				for (k = 0; k < i32tail; k++) {				
					buf[1 + k] = fw_entry->data[Flash_Address + j + k];
				}
				ret = CTP_I2C_WRITE(ts->client, I2C_FW_Address, buf, (i32tail+1));
				if (ret < 0) {
					dev_err(&ts->client->dev,"%s: Write Page error!!(%d), j=%d\n", __func__, ret, j);
					return ret;
				}
			}
		}
		#else
		for (j = 0; j < min(fw_entry->size - i * 256, (size_t)256); j += 32) {
			buf[0] = (XDATA_Addr + j) & 0xFF;
			for (k = 0; k < 32; k++) {
				buf[1 + k] = fw_entry->data[Flash_Address + j + k];
			}
			ret = CTP_I2C_WRITE(ts->client, I2C_FW_Address, buf, 33);
			if (ret < 0) {
				dev_err(&ts->client->dev,"%s: Write Page error!!(%d), j=%d\n", __func__, ret, j);
				return ret;
			}
		}		
		#endif//    #if NVT_TOUCH_EXT_FIH
		if (fw_entry->size - Flash_Address >= 256)
			tmpvalue=(Flash_Address >> 16) + ((Flash_Address >> 8) & 0xFF) + (Flash_Address & 0xFF) + 0x00 + (255);
		else
			tmpvalue=(Flash_Address >> 16) + ((Flash_Address >> 8) & 0xFF) + (Flash_Address & 0xFF) + 0x00 + (fw_entry->size - Flash_Address - 1);

		for (k = 0;k < min(fw_entry->size - Flash_Address,(size_t)256); k++)
			tmpvalue += fw_entry->data[Flash_Address + k];

		tmpvalue = 255 - tmpvalue + 1;

		// Page Program
		buf[0] = 0x00;
		buf[1] = 0x02;
		buf[2] = ((Flash_Address >> 16) & 0xFF);
		buf[3] = ((Flash_Address >> 8) & 0xFF);
		buf[4] = (Flash_Address & 0xFF);
		buf[5] = 0x00;
		buf[6] = min(fw_entry->size - Flash_Address,(size_t)256) - 1;
		buf[7] = tmpvalue;
		ret = CTP_I2C_WRITE(ts->client, I2C_HW_Address, buf, 8);
		if (ret < 0) {
			dev_err(&ts->client->dev,"%s: Page Program error!!(%d), i=%d\n", __func__, ret, i);
			return ret;
		}

		// Check 0xAA (Page Program)
		retry = 0;
		while (1) {
			mdelay(3);
			buf[0] = 0x00;
			buf[1] = 0x00;
			ret = CTP_I2C_READ(ts->client, I2C_HW_Address, buf, 2);
			if (ret < 0) {
				dev_err(&ts->client->dev,"%s: Page Program error!!(%d)\n", __func__, ret);
				return ret;
			}
			if (buf[1] == 0xAA || buf[1] == 0xEA) {
				break;
			}
			retry++;
			if (unlikely(retry > 5)) {
				dev_err(&ts->client->dev,"%s: Check 0xAA (Page Program) failed, buf[1]=0x%02X, retry=%d\n", __func__, buf[1], retry);
				return -1;
			}
		}
		if (buf[1] == 0xEA) {
			dev_err(&ts->client->dev,"%s: Page Program error!! i=%d\n", __func__, i);
			return -3;
		}

		// Read Status
		retry = 0;
		while (1) {
			mdelay(2);
			buf[0] = 0x00;
			buf[1] = 0x05;
			ret = CTP_I2C_WRITE(ts->client, I2C_HW_Address, buf, 2);
			if (ret < 0) {
				dev_err(&ts->client->dev,"%s: Read Status error!!(%d)\n", __func__, ret);
				return ret;
			}

			// Check 0xAA (Read Status)
			buf[0] = 0x00;
			buf[1] = 0x00;
			buf[2] = 0x00;
			ret = CTP_I2C_READ(ts->client, I2C_HW_Address, buf, 3);
			if (ret < 0) {
				dev_err(&ts->client->dev,"%s: Check 0xAA (Read Status) error!!(%d)\n", __func__, ret);
				return ret;
			}
			if (((buf[1] == 0xAA) && (buf[2] == 0x00)) || (buf[1] == 0xEA)) {
				break;
			}
			retry++;
			if (unlikely(retry > 5)) {
				dev_err(&ts->client->dev,"%s: Check 0xAA (Read Status) failed, buf[1]=0x%02X, buf[2]=0x%02X, retry=%d\n", __func__, buf[1], buf[2], retry);
				return -1;
			}
		}
		if (buf[1] == 0xEA) {
			dev_err(&ts->client->dev,"%s: Page Program error!! i=%d\n", __func__, i);
			return -4;
		}

		dev_info(&ts->client->dev,"Programming...%2d%%\r", ((i * 100) / count));
	}

	dev_info(&ts->client->dev,"Programming...%2d%%\r", 100);
	dev_info(&ts->client->dev,"Program OK         \n");
	return 0;
}

/*******************************************************
Description:
	Novatek touchscreen verify checksum of written
	flash function.

return:
	Executive outcomes. 0---succeed. negative---failed.
*******************************************************/
int32_t Verify_Flash(void)
{
	uint8_t buf[64] = {0};
	uint32_t XDATA_Addr = 0x14000;
	int32_t ret = 0;
	int32_t i = 0;
	int32_t k = 0;
	uint16_t WR_Filechksum[BLOCK_64KB_NUM] = {0};
	uint16_t RD_Filechksum[BLOCK_64KB_NUM] = {0};
	size_t fw_bin_size = 0;
	size_t len_in_blk = 0;
	int32_t retry = 0;

	fw_bin_size = fw_entry->size;

	for (i = 0; i < BLOCK_64KB_NUM; i++) {
		if (fw_bin_size > (i * SIZE_64KB)) {
			len_in_blk = min(fw_bin_size - i * SIZE_64KB, (size_t)SIZE_64KB);
			// Calculate WR_Filechksum of each 64KB block
			WR_Filechksum[i] = i + 0x00 + 0x00 + (((len_in_blk - 1) >> 8) & 0xFF) + ((len_in_blk - 1) & 0xFF);
			for (k = 0; k < len_in_blk; k++) {
				WR_Filechksum[i] +=  fw_entry->data[k + i * SIZE_64KB];
			}
			WR_Filechksum[i] = 65535 - WR_Filechksum[i] + 1;

			// Fast Read Command
			buf[0] = 0x00;
			buf[1] = 0x07;
			buf[2] = i;
			buf[3] = 0x00;
			buf[4] = 0x00;
			buf[5] = ((len_in_blk - 1) >> 8) & 0xFF;
			buf[6] = (len_in_blk - 1) & 0xFF;
			ret = CTP_I2C_WRITE(ts->client, I2C_HW_Address, buf, 7);
			if (ret < 0) {
				dev_err(&ts->client->dev,"%s: Fast Read Command error!!(%d)\n", __func__, ret);
				return ret;
			}
			// Check 0xAA (Fast Read Command)
			retry = 0;
			while (1) {
				msleep(80);
				buf[0] = 0x00;
				buf[1] = 0x00;
				ret = CTP_I2C_READ(ts->client, I2C_HW_Address, buf, 2);
				if (ret < 0) {
					dev_err(&ts->client->dev,"%s: Check 0xAA (Fast Read Command) error!!(%d)\n", __func__, ret);
					return ret;
				}
				if (buf[1] == 0xAA) {
					break;
				}
				retry++;
				if (unlikely(retry > 5)) {
					dev_err(&ts->client->dev,"%s: Check 0xAA (Fast Read Command) failed, buf[1]=0x%02X, retry=%d\n", __func__, buf[1], retry);
					return -1;
				}
			}

			// Read Checksum (write addr high byte & middle byte)
			buf[0] = 0xFF;
			buf[1] = XDATA_Addr >> 16;
			buf[2] = (XDATA_Addr >> 8) & 0xFF;
			ret = CTP_I2C_WRITE(ts->client, I2C_FW_Address, buf, 3);
			if (ret < 0) {
				dev_err(&ts->client->dev,"%s: Read Checksum (write addr high byte & middle byte) error!!(%d)\n", __func__, ret);
				return ret;
			}
			// Read Checksum
			buf[0] = (XDATA_Addr) & 0xFF;
			buf[1] = 0x00;
			buf[2] = 0x00;
			ret = CTP_I2C_READ(ts->client, I2C_FW_Address, buf, 3);
			if (ret < 0) {
				dev_err(&ts->client->dev,"%s: Read Checksum error!!(%d)\n", __func__, ret);
				return ret;
			}

			RD_Filechksum[i] = (uint16_t)((buf[2] << 8) | buf[1]);
			if (WR_Filechksum[i] != RD_Filechksum[i]) {
				dev_err(&ts->client->dev,"Verify Fail%d!!\n", i);
				dev_err(&ts->client->dev,"RD_Filechksum[%d]=0x%04X, WR_Filechksum[%d]=0x%04X\n", i, RD_Filechksum[i], i, WR_Filechksum[i]);
				return -1;
			}
		}
	}

	dev_info(&ts->client->dev,"Verify OK \n");
	return 0;
}

/*******************************************************
Description:
	Novatek touchscreen update firmware function.

return:
	Executive outcomes. 0---succeed. negative---failed.
*******************************************************/
int32_t Update_Firmware(void)
{
	int32_t ret = 0;

	// Step 1 : initial bootloader
	ret = Init_BootLoader();
	if (ret) {
		return ret;
	}

	// Step 2 : Erase
	ret = Erase_Flash();
	if (ret) {
		return ret;
	}

	// Stap 3 : Program
	ret = Write_Flash();
	if (ret) {
		return ret;
	}

	// Stap 4 : Verify
	ret = Verify_Flash();
	if (ret) {
		return ret;
	}

	//Step 5 : Bootloader Reset
	nvt_bootloader_reset();
	nvt_check_fw_reset_state(RESET_STATE_INIT);

	return ret;
}

/*******************************************************
Description:
	Novatek touchscreen update firmware when booting
	function.

return:
	n.a.
*******************************************************/
void Boot_Update_Firmware(struct work_struct *work)
{
	struct i2c_client *client = ts->client;
	int32_t ret = 0;

	char firmware_name[256] = "";
	sprintf(firmware_name, BOOT_UPDATE_FIRMWARE_NAME);

	// request bin file in "/etc/firmware"
	ret = update_firmware_request(firmware_name);
	if (ret) {
		dev_err(&client->dev, "%s: update_firmware_request failed. (%d)\n", __func__, ret);
		return;
	}

	mutex_lock(&ts->lock);

	nvt_sw_reset_idle();

	ret = Check_CheckSum();

	if (ret < 0) {	// read firmware checksum failed
		dev_err(&client->dev, "%s: read firmware checksum failed\n", __func__);
		Update_Firmware();
	} else if ((ret == 0) && (Check_FW_Ver() == 0)) {	// (fw checksum not match) && (bin fw version >= ic fw version)
		dev_info(&client->dev, "%s: firmware version not match\n", __func__);
		Update_Firmware();
	} else {
		// Bootloader Reset
		nvt_bootloader_reset();
		nvt_check_fw_reset_state(RESET_STATE_INIT);
	}

	mutex_unlock(&ts->lock);

	update_firmware_release();
}
#endif /* BOOT_UPDATE_FIRMWARE */
