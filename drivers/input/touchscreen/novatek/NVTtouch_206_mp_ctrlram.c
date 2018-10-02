/* drivers/input/touchscreen/nt11206/NVTtouch_206_mp_ctrlram.c
 *
 * Copyright (C) 2010 - 2016 Novatek, Inc.
 *
 * $Revision: 6747 $
 * $Date: 2016-10-11 16:45:37 +0800 (?±‰?, 11 ?ÅÊ? 2016) $
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

#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <asm/uaccess.h>

#include <linux/syscalls.h>

#include "NVTtouch_206.h"

//TP-BBox-00+{_20161221
/* Black Box */
#define BBOX_TP_FIRMWARE_UPDATE_FAIL	do {printk("BBox;%s: TP firmware update fail!\n", __func__); printk("BBox::UEC;7::6\n");} while (0);
//TP-BBox-00+}_20161221

#if NVT_TOUCH_MP

#define IC_TX_CFG_SIZE 20
#define IC_RX_CFG_SIZE 33
#define AIN_TX_NUM	13
#define AIN_RX_NUM	28


#if TOUCH_KEY_NUM > 0
	#define Key_X_Channel_Real	3
	#define Key_Y_Channel_Real	1
#endif /* #if TOUCH_KEY_NUM > 0 */

static int32_t GND_Channel_Real = 0;
static int32_t PSConfig_Tolerance_Postive_Short = 300;
static int32_t PSConfig_Tolerance_Negative_Short = -300;
static int32_t PSConfig_DiffLimitG_Postive_Short = 300;
static int32_t PSConfig_DiffLimitG_Negative_Short = -300;
static int32_t PSConfig_Tolerance_Postive_Mutual = 300;
static int32_t PSConfig_Tolerance_Negative_Mutual = -300;
static int32_t PSConfig_DiffLimitG_Postive_Mutual = 300;
static int32_t PSConfig_DiffLimitG_Negative_Mutual = -300;
static int32_t PSConfig_Rawdata_Limit_Postive_Short_RXRX = 4000000;
static int32_t PSConfig_Rawdata_Limit_Negative_Short_RXRX = -4000000;
static int32_t PSConfig_Rawdata_Limit_Postive_Short_TXRX = 4000000;
static int32_t PSConfig_Rawdata_Limit_Negative_Short_TXRX = -4000000;
static int32_t PSConfig_Rawdata_Limit_Postive_Short_TXTX = 4000000;
static int32_t PSConfig_Rawdata_Limit_Negative_Short_TXTX = -4000000;
#if TOUCH_KEY_NUM > 0
static int32_t PSConfig_Tolerance_Postive_Key_Open = 300;
static int32_t PSConfig_Tolerance_Negative_Key_Open = -400;
static int32_t PSConfig_DiffLimitG_Postive_Key_Open = 300;
static int32_t PSConfig_DiffLimitG_Negative_Key_Open = -400;
static int32_t PSConfig_Tolerance_Postive_Key_Short = 300;
static int32_t PSConfig_Tolerance_Negative_Key_Short = -400;
static int32_t PSConfig_DiffLimitG_Postive_Key_Short = 300;
static int32_t PSConfig_DiffLimitG_Negative_Key_Short = -400;
static int32_t PSConfig_Rawdata_Limit_Postive_Key_Short_RXRX = 4000000;
static int32_t PSConfig_Rawdata_Limit_Negative_Key_Short_RXRX = -4000000;
static int32_t PSConfig_Rawdata_Limit_Postive_Key_Short_TXRX = 4000000;
static int32_t PSConfig_Rawdata_Limit_Negative_Key_Short_TXRX = -4000000;
static int32_t PSConfig_Rawdata_Limit_Postive_Key_Short_TXTX = 4000000;
static int32_t PSConfig_Rawdata_Limit_Negative_Key_Short_TXTX = -4000000;
#endif /* #if TOUCH_KEY_NUM > 0 */
static int32_t RXRX_isDummyCycle = 0;
static int32_t RXRX_Dummy_Count = 1;
static int32_t RXRX_isReadADCCheck = 1;
static int32_t RXRX_TestTimes = 0;
static int32_t RXRX_Dummy_Frames = 0;
static int32_t TXRX_isDummyCycle = 0;
static int32_t TXRX_Dummy_Count = 1;
static int32_t TXRX_isReadADCCheck = 1;
static int32_t TXRX_TestTimes = 0;
static int32_t TXRX_Dummy_Frames = 0;
static int32_t TXTX_isDummyCycle = 0;
static int32_t TXTX_Dummy_Count = 1;
static int32_t TXTX_isReadADCCheck = 1;
static int32_t TXTX_TestTimes = 0;
static int32_t TXTX_Dummy_Frames = 0;
static int32_t Mutual_isDummyCycle = 0;
static int32_t Mutual_Dummy_Count = 1;
static int32_t Mutual_isReadADCCheck = 1;
static int32_t Mutual_TestTimes = 0;
static int32_t Mutual_Dummy_Frames = 0;


//  static uint8_t AIN_RX[IC_RX_CFG_SIZE] = {21, 27, 20, 26, 25, 23, 18, 24, 22, 17, 11, 16, 15, 8, 13, 12, 9, 14, 6, 7, 5, 4, 3, 10, 19, 1, 0, 2, 0xFF, 29, 28, 30, 0xFF};
    static uint8_t AIN_RX[IC_RX_CFG_SIZE] = {21, 27, 20, 26, 25, 23, 18, 24, 22, 17, 11, 16, 15, 8, 13, 12, 9, 14, 6, 7, 5, 4, 3, 10, 19, 1, 0, 2, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};//#RX_BTN: BRx1=29 ,BRx2=30 BRx3=31

//  static uint8_t AIN_TX[IC_TX_CFG_SIZE] = {12, 11, 9, 10, 8, 6, 5, 4, 7, 2, 3, 1, 0, 0xFF, 0xFF, 13, 0xFF, 0xFF, 0xFF, 0xFF};
    static uint8_t AIN_TX[IC_TX_CFG_SIZE] = {12, 11, 9, 10, 8, 6, 5, 4, 7, 2, 3, 1, 0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

static uint8_t AIN_RX_Order[IC_RX_CFG_SIZE] = {0};
static uint8_t AIN_TX_Order[IC_TX_CFG_SIZE] = {0};
#if TOUCH_KEY_NUM > 0
static uint8_t AIN_KEY_RX[IC_RX_CFG_SIZE] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 29, 28, 30, 0xFF};
static uint8_t AIN_KEY_TX[IC_TX_CFG_SIZE] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 13, 0xFF, 0xFF, 0xFF, 0xFF};
static uint8_t AIN_KEY_RX_Order[IC_RX_CFG_SIZE] = {0};
static uint8_t AIN_KEY_TX_Order[IC_TX_CFG_SIZE] = {0};
#endif /* #if TOUCH_KEY_NUM > 0 */

int32_t BoundaryShort_RXRX[40] = {0};
int32_t BoundaryShort_TXRX[40*40] = {0};
int32_t BoundaryShort_TXTX[40] = {0};
int32_t BoundaryOpen[40 * 40] = {0};
#if TOUCH_KEY_NUM > 0
int32_t BoundaryShort_RXRX_Key[IC_RX_CFG_SIZE] = {0};
int32_t BoundaryShort_TXRX_Key[IC_TX_CFG_SIZE * IC_RX_CFG_SIZE] = {0};
int32_t BoundaryShort_TXTX_Key[IC_TX_CFG_SIZE] = {0};
int32_t BoundaryOpen_Key[Key_X_Channel_Real * Key_Y_Channel_Real] = {0};
#endif /* #if TOUCH_KEY_NUM > 0 */

#define MaxStatisticsBuf 100
static int64_t StatisticsNum[MaxStatisticsBuf];
static int64_t StatisticsSum[MaxStatisticsBuf];
static int64_t golden_Ratio[40 * 40] = {0};
static uint8_t RecordResultShort_RXRX[40] = {0};
static uint8_t RecordResultShort_TXRX[40 * 40] = {0};
static uint8_t RecordResultShort_TXTX[40] = {0};
static uint8_t RecordResultOpen[40 * 40] = {0};
#if TOUCH_KEY_NUM > 0
static uint8_t RecordResultShort_RXRX_Key[IC_RX_CFG_SIZE] = {0};
static uint8_t RecordResultShort_TXRX_Key[IC_TX_CFG_SIZE * IC_RX_CFG_SIZE] = {0};
static uint8_t RecordResultShort_TXTX_Key[IC_TX_CFG_SIZE] = {0};
static uint8_t RecordResultOpen_Key[Key_X_Channel_Real * Key_Y_Channel_Real] = {0};
#endif /* #if TOUCH_KEY_NUM > 0 */

static int32_t TestResult_Short_RXRX = 0;
static int32_t TestResult_Short_TXRX = 0;
static int32_t TestResult_Short_TXTX = 0;
static int32_t TestResult_Open = 0;
#if TOUCH_KEY_NUM > 0
static int32_t TestResult_Short_RXRX_Key = 0;
static int32_t TestResult_Short_TXRX_Key = 0;
static int32_t TestResult_Short_TXTX_Key = 0;
static int32_t TestResult_Open_Key = 0;
#endif /* #if TOUCH_KEY_NUM > 0 */

static int32_t rawdata_short_rxrx[40] = {0};
static int32_t rawdata_short_txrx[40 * 40] = {0};
static int32_t rawdata_short_txtx[40] = {0};
static int32_t rawdata_open_raw1[40 * 40] = {0};
static int32_t rawdata_open_raw2[40 * 40] = {0};
static int32_t rawdata_open[40 * 40] = {0};
#if TOUCH_KEY_NUM > 0
static int32_t RXRX_test_keydata[IC_RX_CFG_SIZE] = {0};
static int32_t TXRX_test_keydata[IC_TX_CFG_SIZE * IC_RX_CFG_SIZE] = {0};
static int32_t TXTX_test_keydata[IC_TX_CFG_SIZE] = {0};
static int32_t Mutual_keydata1[Key_Y_Channel_Real * Key_X_Channel_Real] = {0};
static int32_t Mutual_keydata2[Key_Y_Channel_Real * Key_X_Channel_Real] = {0};
static int32_t Mutual_keydata[Key_Y_Channel_Real * Key_X_Channel_Real] = {0};
#endif /* #if TOUCH_KEY_NUM > 0 */

struct test_cmd {
	uint32_t addr;
	uint8_t len;
	uint8_t data[64];
};

struct test_cmd *short_test_rxrx = NULL;
struct test_cmd *short_test_txrx = NULL;
struct test_cmd *short_test_txtx = NULL;
struct test_cmd *open_test = NULL;
int32_t short_test_rxrx_cmd_num = 0;
int32_t short_test_txrx_cmd_num = 0;
int32_t short_test_txtx_cmd_num = 0;
int32_t open_test_cmd_num = 0;

static struct proc_dir_entry *NVT_proc_selftest_entry = NULL;

extern struct nvt_ts_data *ts;
extern void nvt_hw_reset(void);
extern int32_t CTP_I2C_READ(struct i2c_client *client, uint16_t address, uint8_t *buf, uint16_t len);
extern int32_t CTP_I2C_READ_DUMMY(struct i2c_client *client, uint16_t address);
extern int32_t CTP_I2C_WRITE(struct i2c_client *client, uint16_t address, uint8_t *buf, uint16_t len);
extern void nvt_sw_reset_idle(void);
extern void nvt_bootloader_reset(void);

#if NVT_TOUCH_EXT_FIH
	static int nvt_extFIH_MP_show(struct seq_file *m, void *v);
#endif


typedef enum mp_criteria_item {
	GND_Ch_Real,
	Tol_Pos_Short,
	Tol_Neg_Short,
	DifLimG_Pos_Short,
	DifLimG_Neg_Short,
	Tol_Pos_Mutual,
	Tol_Neg_Mutual,
	DifLimG_Pos_Mutual,
	DifLimG_Neg_Mutual,
	Raw_Lim_Pos_Short_RXRX,
	Raw_Lim_Neg_Short_RXRX,
	Raw_Lim_Pos_Short_TXRX,
	Raw_Lim_Neg_Short_TXRX,
	Raw_Lim_Pos_Short_TXTX,
	Raw_Lim_Neg_Short_TXTX,
#if TOUCH_KEY_NUM > 0
	Tol_Pos_Key_Open,
	Tol_Neg_Key_Open,
	DifLimG_Pos_Key_Open,
	DifLimG_Neg_Key_Open,
	Tol_Pos_Key_Short,
	Tol_Neg_Key_Short,
	DifLimG_Pos_Key_Short,
	DifLimG_Neg_Key_Short,
	Raw_Lim_Pos_Key_Short_RXRX,
	Raw_Lim_Neg_Key_Short_RXRX,
	Raw_Lim_Pos_Key_Short_TXRX,
	Raw_Lim_Neg_Key_Short_TXRX,
	Raw_Lim_Pos_Key_Short_TXTX,
	Raw_Lim_Neg_Key_Short_TXTX,
#endif /* #if TOUCH_KEY_NUM > 0 */
	Open_rawdata,
	RXRX_rawdata,
	TXRX_rawdata,
	TXTX_rawdata,
#if TOUCH_KEY_NUM > 0
	Key_Open_rawdata,
	Key_RXRX_rawdata,
	Key_TXRX_rawdata,
	Key_TXTX_rawdata,
#endif /* #if TOUCH_KEY_NUM > 0 */
	Test_Cmd_Short_RXRX,
	Test_Cmd_Short_TXRX,
	Test_Cmd_Short_TXTX,
	Test_Cmd_Open,
	IC_CMD_SET_RXRX_isDummyCycle,
	IC_CMD_SET_RXRX_Dummy_Count,
	IC_CMD_SET_RXRX_isReadADCCheck,
	IC_CMD_SET_RXRX_TestTimes,
	IC_CMD_SET_RXRX_Dummy_Frames,
	IC_CMD_SET_TXRX_isDummyCycle,
	IC_CMD_SET_TXRX_Dummy_Count,
	IC_CMD_SET_TXRX_isReadADCCheck,
	IC_CMD_SET_TXRX_TestTimes,
	IC_CMD_SET_TXRX_Dummy_Frames,
	IC_CMD_SET_TXTX_isDummyCycle,
	IC_CMD_SET_TXTX_Dummy_Count,
	IC_CMD_SET_TXTX_isReadADCCheck,
	IC_CMD_SET_TXTX_TestTimes,
	IC_CMD_SET_TXTX_Dummy_Frames,
	IC_CMD_SET_Mutual_isDummyCycle,
	IC_CMD_SET_Mutual_Dummy_Count,
	IC_CMD_SET_Mutual_isReadADCCheck,
	IC_CMD_SET_Mutual_TestTimes,
	IC_CMD_SET_Mutual_Dummy_Frames,
	MP_Cri_Item_Last
} mp_criteria_item_e;

typedef enum test_cmd_item {
	cmd_item_addr,
	cmd_item_len,
	cmd_item_data,
	cmd_item_last
} test_cmd_item_e;

typedef enum ctrl_ram_item {
	CtrlRam_REGISTER,
	CtrlRam_START_ADDR,
	CtrlRam_TABLE_ADDR,
	CtrlRam_UC_ADDR,
	CtrlRam_TXMODE_ADDR,
	CtrlRam_RXMODE_ADDR,
	CtrlRam_CCMODE_ADDR,
	CtrlRam_OFFSET_ADDR,
	CtrlRam_ITEM_LAST
} ctrl_ram_item_e;

static void goto_next_line(char **ptr)
{
	do {
		*ptr = *ptr + 1;
	} while (**ptr != '\n');
	*ptr = *ptr + 1;
}

static void copy_this_line(char *dest, char *src)
{
	char *copy_from;
	char *copy_to;

	copy_from = src;
	copy_to = dest;
	do {
		*copy_to = *copy_from;
		copy_from++;
		copy_to++;
	} while((*copy_from != '\n') && (*copy_from != '\r'));
	*copy_to = '\0';
}

static void str_low(char *str)
{
	int i;

	for (i = 0; i < strlen(str); i++)
		if ((str[i] >= 65) && (str[i] <= 90))
			str[i] += 32;
}

static unsigned long str_to_hex(char *p)
{
	unsigned long hex = 0;
	unsigned long length = strlen(p), shift = 0;
	unsigned char dig = 0;

	str_low(p);
	length = strlen(p);

	if (length == 0)
		return 0;

	do {
		dig = p[--length];
		dig = dig < 'a' ? (dig - '0') : (dig - 'a' + 0xa);
		hex |= (dig << shift);
		shift += 4;
	} while (length);
	return hex;
}

int32_t parse_mp_criteria_item(char **ptr, const char *item_string, int32_t *item_value)
{
	char *tmp = NULL;

	tmp = strstr(*ptr, item_string);
	if (tmp == NULL) {
		dev_err(&ts->client->dev, "%s not found\n", item_string);
		return -1;
	} else {
		*ptr = tmp;
		goto_next_line(ptr);
		sscanf(*ptr, "%d,", item_value);
		return 0;
	}
}

/*******************************************************
Description:
	Novatek touchscreen load MP criteria and golden function.

return:
	Executive outcomes. 0---succeed. negative---failed.
*******************************************************/
int32_t nvt_load_mp_criteria(void)
{
	int32_t retval = 0;
	struct file *fp = NULL;
	char *fbufp = NULL; // buffer for content of file
	mm_segment_t org_fs;
	char file_path[64]="/system/etc/MP_Criteria_Golden.csv";
	struct kstat stat;
	loff_t pos = 0;
	int32_t read_ret = 0;
	char *ptr = NULL;
	mp_criteria_item_e mp_criteria_item = GND_Ch_Real;
	uint32_t i = 0;
	uint32_t j = 0;
	char tx_data[512] = {0};
	char *token = fbufp;
	char *tok_ptr = fbufp;
	size_t offset = 0;
	int8_t skip_TXn = 1;
	char *test_cmd_buf = NULL;
	int32_t test_cmd_num = 0;
	test_cmd_item_e cmd_item;
#if TOUCH_KEY_NUM > 0
	int32_t TX_idx = -1;
	int32_t RX_idx = -1;
	int8_t skip_TXm_RXn = 0;
	int8_t skip_RXn = 0;
#endif /* #if TOUCH_KEY_NUM > 0 */

	dev_info(&ts->client->dev, "%s:++\n", __func__);
	org_fs = get_fs();
	set_fs(KERNEL_DS);

	fp = filp_open(file_path, O_RDONLY, 0);
	if (fp == NULL || IS_ERR(fp)) {
		dev_err(&ts->client->dev, "%s: open %s failed\n", __func__, file_path);
		set_fs(org_fs);
		retval = -1;
		return retval;
	}

	retval = vfs_stat(file_path, &stat);
	if (!retval) {
		fbufp = (char *)kzalloc(stat.size + 1, GFP_KERNEL);
		if (!fbufp) {
			dev_err(&ts->client->dev, "%s: kzalloc %lld bytes failed.\n", __func__, stat.size);
			retval = -2;
			set_fs(org_fs);
			filp_close(fp, NULL);
			return retval;
		}

		read_ret = vfs_read(fp, (char __user *)fbufp, stat.size, &pos);
		if (read_ret > 0) {
			//pr_info("%s: File Size:%lld\n", __func__, stat.size);
			//pr_info("---------------------------------------------------\n");
			//printk("fbufp:\n");
			//for(k = 0; k < stat.size; k++) {
			//	printk("%c", fbufp[k]);
			//}
			//pr_info("---------------------------------------------------\n");

			fbufp[stat.size] = '\n';
			ptr = fbufp;

			test_cmd_buf = (char *)kzalloc(8192, GFP_KERNEL);
			if (!test_cmd_buf) {
				dev_err(&ts->client->dev, "%s: kzalloc for test_cmd_buf failed.\n", __func__);
				retval = -ENOMEM;
				goto exit_free;
			}

			while ( ptr && (ptr < (fbufp + stat.size))) {
				if (mp_criteria_item == GND_Ch_Real) {
					ptr = fbufp;
					parse_mp_criteria_item(&ptr, "GND_Channel_Real:", &GND_Channel_Real);
					dev_info(&ts->client->dev, "GND_Channel_Real = %d\n", GND_Channel_Real);
				} else if (mp_criteria_item == Tol_Pos_Short) {
					parse_mp_criteria_item(&ptr, "PSConfig_Tolerance_Postive_Short:", &PSConfig_Tolerance_Postive_Short);
					dev_info(&ts->client->dev, "PSConfig_Tolerance_Postive_Short = %d\n", PSConfig_Tolerance_Postive_Short);
				} else if (mp_criteria_item == Tol_Neg_Short) {
					ptr = fbufp;
					parse_mp_criteria_item(&ptr, "PSConfig_Tolerance_Negative_Short:", &PSConfig_Tolerance_Negative_Short);
					dev_info(&ts->client->dev, "PSConfig_Tolerance_Negativee_Short = %d\n", PSConfig_Tolerance_Negative_Short);
				} else if (mp_criteria_item == DifLimG_Pos_Short) {
					ptr = fbufp;
					parse_mp_criteria_item(&ptr, "PSConfig_DiffLimitG_Postive_Short:", &PSConfig_DiffLimitG_Postive_Short);
					dev_info(&ts->client->dev, "PSConfig_DiffLimitG_Postive_Short = %d\n", PSConfig_DiffLimitG_Postive_Short );
				} else if (mp_criteria_item == DifLimG_Neg_Short) {
					ptr = fbufp;
					parse_mp_criteria_item(&ptr, "PSConfig_DiffLimitG_Negative_Short:", &PSConfig_DiffLimitG_Negative_Short);
					dev_info(&ts->client->dev, "PSConfig_DiffLimitG_Negative_Short = %d\n", PSConfig_DiffLimitG_Negative_Short);
				} else if (mp_criteria_item == Tol_Pos_Mutual) {
					ptr = fbufp;
					parse_mp_criteria_item(&ptr, "PSConfig_Tolerance_Postive_Mutual:", &PSConfig_Tolerance_Postive_Mutual);
					dev_info(&ts->client->dev, "PSConfig_Tolerance_Postive_Mutual = %d\n", PSConfig_Tolerance_Postive_Mutual);
				} else if (mp_criteria_item == Tol_Neg_Mutual) {
					ptr = fbufp;
					parse_mp_criteria_item(&ptr, "PSConfig_Tolerance_Negative_Mutual:", &PSConfig_Tolerance_Negative_Mutual);
					dev_info(&ts->client->dev, "PSConfig_Tolerance_Negative_Mutual = %d\n", PSConfig_Tolerance_Negative_Mutual);
				} else if (mp_criteria_item == DifLimG_Pos_Mutual) {
					ptr = fbufp;
					parse_mp_criteria_item(&ptr, "PSConfig_DiffLimitG_Postive_Mutual:", &PSConfig_DiffLimitG_Postive_Mutual);
					dev_info(&ts->client->dev, "PSConfig_DiffLimitG_Postive_Mutual = %d\n", PSConfig_DiffLimitG_Postive_Mutual);
				} else if (mp_criteria_item == DifLimG_Neg_Mutual) {
					ptr = fbufp;
					parse_mp_criteria_item(&ptr, "PSConfig_DiffLimitG_Negative_Mutual:", &PSConfig_DiffLimitG_Negative_Mutual);
					dev_info(&ts->client->dev, "PSConfig_DiffLimitG_Negative_Mutual = %d\n", PSConfig_DiffLimitG_Negative_Mutual);
				} else if (mp_criteria_item == Raw_Lim_Pos_Short_RXRX) {
					ptr = fbufp;
					parse_mp_criteria_item(&ptr, "PSConfig_Rawdata_Limit_Postive_Short_RXRX:", &PSConfig_Rawdata_Limit_Postive_Short_RXRX);
					dev_info(&ts->client->dev, "PSConfig_Rawdata_Limit_Postive_Short_RXRX = %d\n", PSConfig_Rawdata_Limit_Postive_Short_RXRX);
				} else if (mp_criteria_item == Raw_Lim_Neg_Short_RXRX) {
					ptr = fbufp;
					parse_mp_criteria_item(&ptr, "PSConfig_Rawdata_Limit_Negative_Short_RXRX:", &PSConfig_Rawdata_Limit_Negative_Short_RXRX);
					dev_info(&ts->client->dev, "PSConfig_Rawdata_Limit_Negative_Short_RXRX = %d\n", PSConfig_Rawdata_Limit_Negative_Short_RXRX);
				} else if (mp_criteria_item == Raw_Lim_Pos_Short_TXRX) {
					ptr = fbufp;
					parse_mp_criteria_item(&ptr, "PSConfig_Rawdata_Limit_Postive_Short_TXRX:", &PSConfig_Rawdata_Limit_Postive_Short_TXRX);
					dev_info(&ts->client->dev, "PSConfig_Rawdata_Limit_Postive_Short_TXRX = %d\n", PSConfig_Rawdata_Limit_Postive_Short_TXRX);
				} else if (mp_criteria_item == Raw_Lim_Neg_Short_TXRX) {
					ptr = fbufp;
					parse_mp_criteria_item(&ptr, "PSConfig_Rawdata_Limit_Negative_Short_TXRX:", &PSConfig_Rawdata_Limit_Negative_Short_TXRX);
					dev_info(&ts->client->dev, "PSConfig_Rawdata_Limit_Negative_Short_TXRX = %d\n", PSConfig_Rawdata_Limit_Negative_Short_TXRX);
				} else if (mp_criteria_item == Raw_Lim_Pos_Short_TXTX) {
					ptr = fbufp;
					parse_mp_criteria_item(&ptr, "PSConfig_Rawdata_Limit_Postive_Short_TXTX:", &PSConfig_Rawdata_Limit_Postive_Short_TXTX);
					dev_info(&ts->client->dev, "PSConfig_Rawdata_Limit_Postive_Short_TXTX = %d\n", PSConfig_Rawdata_Limit_Postive_Short_TXTX);
				} else if (mp_criteria_item == Raw_Lim_Neg_Short_TXTX) {
					ptr = fbufp;
					parse_mp_criteria_item(&ptr, "PSConfig_Rawdata_Limit_Negative_Short_TXTX:", &PSConfig_Rawdata_Limit_Negative_Short_TXTX);
					dev_info(&ts->client->dev, "PSConfig_Rawdata_Limit_Negative_Short_TXTX = %d\n", PSConfig_Rawdata_Limit_Negative_Short_TXTX);
#if TOUCH_KEY_NUM > 0
				} else if (mp_criteria_item == Tol_Pos_Key_Open) {
					ptr = fbufp;
					parse_mp_criteria_item(&ptr, "PSConfig_Tolerance_Postive_Key_Open:", &PSConfig_Tolerance_Postive_Key_Open);
					dev_info(&ts->client->dev, "PSConfig_Tolerance_Postive_Key_Open = %d\n", PSConfig_Tolerance_Postive_Key_Open);
				} else if (mp_criteria_item == Tol_Neg_Key_Open) {
					ptr = fbufp;
					parse_mp_criteria_item(&ptr, "PSConfig_Tolerance_Negative_Key_Open:", &PSConfig_Tolerance_Negative_Key_Open);
					dev_info(&ts->client->dev, "PSConfig_Tolerance_Negative_Key_Open = %d\n", PSConfig_Tolerance_Negative_Key_Open);
				} else if (mp_criteria_item == DifLimG_Pos_Key_Open) {
					ptr = fbufp;
					parse_mp_criteria_item(&ptr, "PSConfig_DiffLimitG_Postive_Key_Open:", &PSConfig_DiffLimitG_Postive_Key_Open);
					dev_info(&ts->client->dev, "PSConfig_DiffLimitG_Postive_Key_Open = %d\n", PSConfig_DiffLimitG_Postive_Key_Open);
				} else if (mp_criteria_item == DifLimG_Neg_Key_Open) {
					ptr = fbufp;
					parse_mp_criteria_item(&ptr, "PSConfig_DiffLimitG_Negative_Key_Open:", &PSConfig_DiffLimitG_Negative_Key_Open);
					dev_info(&ts->client->dev, "PSConfig_DiffLimitG_Negative_Key_Open = %d\n", PSConfig_DiffLimitG_Negative_Key_Open);
				} else if (mp_criteria_item == Tol_Pos_Key_Short) {
					ptr = fbufp;
					parse_mp_criteria_item(&ptr, "PSConfig_Tolerance_Postive_Key_Short:", &PSConfig_Tolerance_Postive_Key_Short);
					dev_info(&ts->client->dev, "PSConfig_Tolerance_Postive_Key_Short = %d\n", PSConfig_Tolerance_Postive_Key_Short);
				} else if (mp_criteria_item == Tol_Neg_Key_Short) {
					ptr = fbufp;
					parse_mp_criteria_item(&ptr, "PSConfig_Tolerance_Negative_Key_Short:", &PSConfig_Tolerance_Negative_Key_Short);
					dev_info(&ts->client->dev, "PSConfig_Tolerance_Negative_Key_Short = %d\n", PSConfig_Tolerance_Negative_Key_Short);
				} else if (mp_criteria_item == DifLimG_Pos_Key_Short) {
					ptr = fbufp;
					parse_mp_criteria_item(&ptr, "PSConfig_DiffLimitG_Postive_Key_Short:", &PSConfig_DiffLimitG_Postive_Key_Short);
					dev_info(&ts->client->dev, "PSConfig_DiffLimitG_Postive_Key_Short = %d\n", PSConfig_DiffLimitG_Postive_Key_Short);
				} else if (mp_criteria_item == DifLimG_Neg_Key_Short) {
					ptr = fbufp;
					parse_mp_criteria_item(&ptr, "PSConfig_DiffLimitG_Negative_Key_Short:", &PSConfig_DiffLimitG_Negative_Key_Short);
					dev_info(&ts->client->dev, "PSConfig_DiffLimitG_Negative_Key_Short = %d\n", PSConfig_DiffLimitG_Negative_Key_Short);
				} else if (mp_criteria_item == Raw_Lim_Pos_Key_Short_RXRX) {
					ptr = fbufp;
					parse_mp_criteria_item(&ptr, "PSConfig_Rawdata_Limit_Postive_Key_Short_RXRX:", &PSConfig_Rawdata_Limit_Postive_Key_Short_RXRX);
					dev_info(&ts->client->dev, "PSConfig_Rawdata_Limit_Postive_Key_Short_RXRX = %d\n", PSConfig_Rawdata_Limit_Postive_Key_Short_RXRX);
				} else if (mp_criteria_item == Raw_Lim_Neg_Key_Short_RXRX) {
					ptr = fbufp;
					parse_mp_criteria_item(&ptr, "PSConfig_Rawdata_Limit_Negative_Key_Short_RXRX:", &PSConfig_Rawdata_Limit_Negative_Key_Short_RXRX);
					dev_info(&ts->client->dev, "PSConfig_Rawdata_Limit_Negative_Key_Short_RXRX = %d\n", PSConfig_Rawdata_Limit_Negative_Key_Short_RXRX);
				} else if (mp_criteria_item == Raw_Lim_Pos_Key_Short_TXRX) {
					ptr = fbufp;
					parse_mp_criteria_item(&ptr, "PSConfig_Rawdata_Limit_Postive_Key_Short_TXRX:", &PSConfig_Rawdata_Limit_Postive_Key_Short_TXRX);
					dev_info(&ts->client->dev, "PSConfig_Rawdata_Limit_Postive_Key_Short_TXRX = %d\n", PSConfig_Rawdata_Limit_Postive_Key_Short_TXRX);
				} else if (mp_criteria_item == Raw_Lim_Neg_Key_Short_TXRX) {
					ptr = fbufp;
					parse_mp_criteria_item(&ptr, "PSConfig_Rawdata_Limit_Negative_Key_Short_TXRX:", &PSConfig_Rawdata_Limit_Negative_Key_Short_TXRX);
					dev_info(&ts->client->dev, "PSConfig_Rawdata_Limit_Negative_Key_Short_TXRX = %d\n", PSConfig_Rawdata_Limit_Negative_Key_Short_TXRX);
				} else if (mp_criteria_item == Raw_Lim_Pos_Key_Short_TXTX) {
					ptr = fbufp;
					parse_mp_criteria_item(&ptr, "PSConfig_Rawdata_Limit_Postive_Key_Short_TXTX:", &PSConfig_Rawdata_Limit_Postive_Key_Short_TXTX);
					dev_info(&ts->client->dev, "PSConfig_Rawdata_Limit_Postive_Key_Short_TXTX = %d\n", PSConfig_Rawdata_Limit_Postive_Key_Short_TXTX);
				} else if (mp_criteria_item == Raw_Lim_Neg_Key_Short_TXTX) {
					ptr = fbufp;
					parse_mp_criteria_item(&ptr, "PSConfig_Rawdata_Limit_Negative_Key_Short_TXTX:", &PSConfig_Rawdata_Limit_Negative_Key_Short_TXTX);
					dev_info(&ts->client->dev, "PSConfig_Rawdata_Limit_Negative_Key_Short_TXTX = %d\n", PSConfig_Rawdata_Limit_Negative_Key_Short_TXTX);
#endif /* #if TOUCH_KEY_NUM > 0 */
				} else if (mp_criteria_item == Open_rawdata) {
					ptr = fbufp;
					dev_info(&ts->client->dev, "%s: load golden Open rawdata:\n", __func__);
					ptr = strstr(ptr, "Open rawdata:");
					if (ptr == NULL) {
						dev_err(&ts->client->dev, "%s: load golden Open rawdata failed!\n", __func__);
						retval = -5;
						goto exit_free;
					}
					// walk thru this line
					goto_next_line(&ptr);
					// skip line ",RX1,RX2, ..."
					ptr = strstr(ptr, "RX1");
					if (ptr == NULL) {
						dev_err(&ts->client->dev, "%s: load golden Open rawdata failed!\n", __func__);
						retval = -6;
						goto exit_free;
					}
					// walk thru this line
					goto_next_line(&ptr);

					for (i = 0; i < AIN_TX_NUM; i++) {
						ptr = strstr(ptr, "TX");
						if (ptr == NULL) {
							dev_err(&ts->client->dev, "%s: load golden Open rawdata failed!\n", __func__);
							retval = -7;
							goto exit_free;
						}
						// parse data after TXn
						// copy this line to tx_data buffer
						memset(tx_data, 0, 512);
						copy_this_line(tx_data, ptr);
						offset = strlen(tx_data);
						tok_ptr = tx_data;
						j = 0;
						skip_TXn = 1;
						while ((token = strsep(&tok_ptr,", \t\r\0"))) {
							if (skip_TXn == 1) {
								skip_TXn = 0;
								continue;
							}
							if (strlen(token) == 0)
								continue;
							BoundaryOpen[i * AIN_RX_NUM + j] = (int32_t) simple_strtol(token, NULL, 10);
							printk("%5d, ", BoundaryOpen[i * AIN_RX_NUM + j]);
							j++;
						}
						printk("\n");
						// go forward
						ptr = ptr + offset;
					}
					printk("\n");

				} else if (mp_criteria_item == RXRX_rawdata) {
					ptr = fbufp;
					dev_info(&ts->client->dev, "%s: load golden RX-RX rawdata:\n", __func__);
					ptr = strstr(ptr, "RX-RX rawdata:");
					if (ptr == NULL) {
						dev_err(&ts->client->dev, "%s: load golden RX-RX rawdata failed!\n", __func__);
						retval = -8;
						goto exit_free;
					}
					// walk thru this line
					goto_next_line(&ptr);
					// skip line ",RX1,RX2, ..."
					ptr = strstr(ptr, "RX1");
					if (ptr == NULL) {
						dev_err(&ts->client->dev, "%s: load golden RX-RX rawdata failed!\n", __func__);
						retval = -9;
						goto exit_free;
					}
					// walk thru this line
					goto_next_line(&ptr);

					// copy this line to tx_data buffer
					memset(tx_data, 0, 512);
					copy_this_line(tx_data, ptr);
					offset = strlen(tx_data);
					tok_ptr = tx_data;
					j = 0;
					while ((token = strsep(&tok_ptr,", \t\r\0"))) {
						if (strlen(token) == 0)
							continue;
						BoundaryShort_RXRX[j] = (int32_t) simple_strtol(token, NULL, 10);
						printk("%5d, ", BoundaryShort_RXRX[j]);
						j++;
					}
					printk("\n");
					if (j != (AIN_RX_NUM + GND_Channel_Real)) {
						dev_err(&ts->client->dev, "%s: load golden RX-RX rawdata failed!, j=%d, (AIN_RX_NUM + GND_Channel_Real)=%d\n", __func__, j, (AIN_RX_NUM + GND_Channel_Real));
						retval = -10;
						goto exit_free;
					}
					// go forward
					ptr = ptr + offset;

				} else if (mp_criteria_item == TXRX_rawdata) {
					ptr = fbufp;
					dev_info(&ts->client->dev, "%s: load golden TX-RX rawdata:\n", __func__);
					ptr = strstr(ptr, "TX-RX rawdata:");
					if (ptr == NULL) {
						dev_err(&ts->client->dev, "%s: load golden TX-RX rawdata failed!\n", __func__);
						retval = -11;
						goto exit_free;
					}
					// walk thru this line
					goto_next_line(&ptr);
					// skip line ",RX1,RX2, ..."
					ptr = strstr(ptr, "RX1");
					if (ptr == NULL) {
						dev_err(&ts->client->dev, "%s: load golden TX-RX rawdata failed!\n", __func__);
						retval = -12;
						goto exit_free;
					}
					// walk thru this line
					goto_next_line(&ptr);

					for (i = 0; i < AIN_TX_NUM; i++) {
						ptr = strstr(ptr, "TX");
						if (ptr == NULL) {
							dev_err(&ts->client->dev, "%s: load golden TX-RX rawdata failed!\n", __func__);
							retval = -13;
							goto exit_free;
						}
						// parse data after TXn
						// copy this line to tx_data buffer
						memset(tx_data, 0, 512);
						copy_this_line(tx_data, ptr);
						offset = strlen(tx_data);
						tok_ptr = tx_data;
						j = 0;
						skip_TXn = 1;
						while ((token = strsep(&tok_ptr,", \t\r\0"))) {
							if (skip_TXn == 1) {
								skip_TXn = 0;
								continue;
							}
							if (strlen(token) == 0)
								continue;
							BoundaryShort_TXRX[i * (AIN_RX_NUM + GND_Channel_Real) + j] = (int32_t) simple_strtol(token, NULL, 10);
							printk("%5d, ", BoundaryShort_TXRX[i * (AIN_RX_NUM + GND_Channel_Real) + j]);
							j++;
						}
						printk("\n");
						// go forward
						ptr = ptr + offset;
					}
					printk("\n");

				} else if (mp_criteria_item == TXTX_rawdata) {
					ptr = fbufp;
					dev_info(&ts->client->dev, "%s: load golden TX-TX rawdata:\n", __func__);
					ptr = strstr(ptr, "TX-TX rawdata:");
					if (ptr == NULL) {
						dev_err(&ts->client->dev, "%s: load golden TX-TX rawdata failed!\n", __func__);
						retval = -14;
						goto exit_free;
					}
					// walk thru this line
					goto_next_line(&ptr);
					// skip line ",RX1,RX2, ..."
					ptr = strstr(ptr, "TX1");
					if (ptr == NULL) {
						dev_err(&ts->client->dev, "%s: load golden TX-TX rawdata failed!\n", __func__);
						retval = -15;
						goto exit_free;
					}
					// walk thru this line
					goto_next_line(&ptr);

					// copy this line to tx_data buffer
					memset(tx_data, 0, 512);
					copy_this_line(tx_data, ptr);
					offset = strlen(tx_data);
					tok_ptr = tx_data;
					j = 0;
					while ((token = strsep(&tok_ptr,", \t\r\0"))) {
						if (strlen(token) == 0)
							continue;
						BoundaryShort_TXTX[j] = (int32_t) simple_strtol(token, NULL, 10);
						printk("%5d, ", BoundaryShort_TXTX[j]);
						j++;
					}
					printk("\n");
					if (j != AIN_TX_NUM) {
						dev_err(&ts->client->dev, "%s: load golden TX-TX rawdata failed!, j=%d, AIN_TX_NUM=%d\n", __func__, j, AIN_TX_NUM);
						retval = -16;
						goto exit_free;
					}
					// go forward
					ptr = ptr + offset;

#if TOUCH_KEY_NUM > 0
				} else if (mp_criteria_item == Key_Open_rawdata) {
					ptr = fbufp;
					dev_info(&ts->client->dev, "%s: load golden Key open rawdata:\n", __func__);
					ptr = strstr(ptr, "Key open rawdata:");
					if (ptr == NULL) {
						dev_err(&ts->client->dev, "%s: load golden Key open rawdata failed!\n", __func__);
						retval = -17;
						goto exit_free;
					}
					// walk thru this line
					goto_next_line(&ptr);

					TX_idx = -1;
					RX_idx = -1;
					for (i = 0; i < IC_TX_CFG_SIZE; i++) {
						if (AIN_KEY_TX[i] == 0xFF)
							continue;
						else
							TX_idx++;

						for (j = 0; j < IC_RX_CFG_SIZE; j ++) {
							if (AIN_KEY_RX[j] == 0xFF)
								continue;
							else
								RX_idx++;

							ptr = strstr(ptr, "TX");
							if (ptr == NULL) {
								dev_err(&ts->client->dev, "%s: load golden Key open rawdata failed!\n", __func__);
								retval = -18;
								goto exit_free;
							}
							// parse data after TXm, RXn
							// copy this line to tx_data buffer
							memset(tx_data, 0, 512);
							copy_this_line(tx_data, ptr);
							offset = strlen(tx_data);
							tok_ptr = tx_data;
							skip_TXm_RXn = 1;
							while ((token = strsep(&tok_ptr,", \t\r\0"))) {
								if (skip_TXm_RXn == 1) {
									skip_TXm_RXn = 2;
									continue;
								} else if (skip_TXm_RXn == 2) {
									skip_TXm_RXn = 0;
									continue;
								}
								if (strlen(token) == 0)
									continue;
								BoundaryOpen_Key[TX_idx * Key_X_Channel_Real + RX_idx] = (int32_t) simple_strtol(token, NULL, 10);
								printk("%5d, ", BoundaryOpen_Key[TX_idx * Key_X_Channel_Real + RX_idx]);
							}
							printk("\n");
							// go forward
							ptr = ptr + offset;
						}
						printk("\n");
					}

				} else if (mp_criteria_item == Key_RXRX_rawdata) {
					ptr = fbufp;
					dev_info(&ts->client->dev, "%s: load golden Key short RXRX rawdata:\n", __func__);
					ptr = strstr(ptr, "Key short RXRX rawdata:");
					if (ptr == NULL) {
						dev_err(&ts->client->dev, "%s: load golden Key short RXRX rawdata failed!\n", __func__);
						retval = -19;
						goto exit_free;
					}
					// walk thru this line
					goto_next_line(&ptr);

					RX_idx = -1;
					for (i = 0; i < IC_RX_CFG_SIZE; i++) {
						if (AIN_KEY_RX[i] == 0xFF)
							continue;
						else
							RX_idx++;

						ptr = strstr(ptr, "RX");
						if (ptr == NULL) {
							dev_err(&ts->client->dev, "%s: load golden Key short RXRX rawdata failed!\n", __func__);
							retval = -20;
							goto exit_free;
						}
						// parse data after RXn
						// copy this line to tx_data buffer
						memset(tx_data, 0, 512);
						copy_this_line(tx_data, ptr);
						offset = strlen(tx_data);
						tok_ptr = tx_data;
						skip_RXn = 1;
						while ((token = strsep(&tok_ptr,", \t\r\0"))) {
							if (skip_RXn == 1) {
								skip_RXn = 0;
								continue;
							}
							if (strlen(token) == 0)
								continue;
							BoundaryShort_RXRX_Key[RX_idx] = (int32_t) simple_strtol(token, NULL, 10);
							printk("%5d, ", BoundaryShort_RXRX_Key[RX_idx]);
						}
						printk("\n");
						// go forward
						ptr = ptr + offset;
					}
				} else if (mp_criteria_item == Key_TXRX_rawdata) {
					ptr = fbufp;
					dev_info(&ts->client->dev, "%s: load golden Key short TXRX rawdata:\n", __func__);
					ptr = strstr(ptr, "Key short TXRX rawdata:");
					if (ptr == NULL) {
						dev_err(&ts->client->dev, "%s: load golden Key short TXRX rawdata failed!\n", __func__);
						retval = -21;
						goto exit_free;
					}
					// walk thru this line
					goto_next_line(&ptr);

					TX_idx = -1;
					RX_idx = -1;
					for (i = 0; i < IC_TX_CFG_SIZE; i++) {
						if (AIN_KEY_TX[i] == 0xFF)
							continue;
						else
							TX_idx++;

						for (j = 0; j < IC_RX_CFG_SIZE; j ++) {
							if (AIN_KEY_RX[j] == 0xFF)
								continue;
							else
								RX_idx++;

							ptr = strstr(ptr, "TX");
							if (ptr == NULL) {
								dev_err(&ts->client->dev, "%s: load golden Key short TXRX rawdata failed!\n", __func__);
								retval = -22;
								goto exit_free;
							}
							// parse data after TXm, RXn
							// copy this line to tx_data buffer
							memset(tx_data, 0, 512);
							copy_this_line(tx_data, ptr);
							offset = strlen(tx_data);
							tok_ptr = tx_data;
							skip_TXm_RXn = 1;
							while ((token = strsep(&tok_ptr,", \t\r\0"))) {
								if (skip_TXm_RXn == 1) {
									skip_TXm_RXn = 2;
									continue;
								} else if (skip_TXm_RXn == 2) {
									skip_TXm_RXn = 0;
									continue;
								}
								if (strlen(token) == 0)
									continue;
								BoundaryShort_TXRX_Key[TX_idx * Key_X_Channel_Real + RX_idx] = (int32_t) simple_strtol(token, NULL, 10);
								printk("%5d, ", BoundaryShort_TXRX_Key[TX_idx * Key_X_Channel_Real + RX_idx]);
							}
							printk("\n");
							// go forward
							ptr = ptr + offset;
						}
						printk("\n");
					}
				} else if (mp_criteria_item == Key_TXTX_rawdata) {
					ptr = fbufp;
					dev_info(&ts->client->dev, "%s: load golden Key short TXTX rawdata:\n", __func__);
					ptr = strstr(ptr, "Key short TXTX rawdata:");
					if (ptr == NULL) {
						dev_err(&ts->client->dev, "%s: load golden Key short TXTX rawdata failed!\n", __func__);
						retval = -23;
						goto exit_free;
					}
					// walk thru this line
					goto_next_line(&ptr);

					TX_idx = -1;
					for (i = 0; i < IC_TX_CFG_SIZE; i++) {
						if (AIN_KEY_TX[i] == 0xFF)
							continue;
						else
							TX_idx++;

						ptr = strstr(ptr, "TX");
						if (ptr == NULL) {
							dev_err(&ts->client->dev, "%s: load golden Key short TXTX rawdata failed!\n", __func__);
							retval = -24;
							goto exit_free;
						}
						// parse data after TXn
						// copy this line to tx_data buffer
						memset(tx_data, 0, 512);
						copy_this_line(tx_data, ptr);
						offset = strlen(tx_data);
						tok_ptr = tx_data;
						skip_TXn = 1;
						while ((token = strsep(&tok_ptr,", \t\r\0"))) {
							if (skip_TXn == 1) {
								skip_TXn = 0;
								continue;
							}
							if (strlen(token) == 0)
								continue;
							BoundaryShort_TXTX_Key[TX_idx] = (int32_t) simple_strtol(token, NULL, 10);
							printk("%5d, ", BoundaryShort_TXTX_Key[TX_idx]);
						}
						printk("\n");
						// go forward
						ptr = ptr + offset;
					}
#endif /* #if TOUCH_KEY_NUM > 0 */
				} else if(mp_criteria_item == Test_Cmd_Short_RXRX) {
					ptr = fbufp;
					dev_info(&ts->client->dev, "%s: load test_cmd_short_test_rxrx:\n", __func__);
					if (parse_mp_criteria_item(&ptr, "test_cmd_short_test_rxrx:", &test_cmd_num) < 0) {
						dev_err(&ts->client->dev, "%s: load test_cmd_short_test_rxrx failed\n", __func__);
						retval = -25;
						goto exit_free;
					}
					dev_info(&ts->client->dev, "test_cmd_num = %d\n", test_cmd_num);
					memset(test_cmd_buf, 0, 8192);
					short_test_rxrx_cmd_num = test_cmd_num;
					if (short_test_rxrx) {
						kfree(short_test_rxrx);
						short_test_rxrx = NULL;
					}
					short_test_rxrx = kzalloc(test_cmd_num * sizeof(struct test_cmd), GFP_KERNEL);
					if (!short_test_rxrx) {
						dev_err(&ts->client->dev, "%s: kzalloc for short_test_rxrx failed.\n", __func__);
						retval = -ENOMEM;
						goto exit_free;
					}
					// load test commands
					for (i = 0; i < test_cmd_num; i++) {
						goto_next_line(&ptr);
						copy_this_line(test_cmd_buf, ptr);
						cmd_item = cmd_item_addr;
						tok_ptr = test_cmd_buf;
						j = 0;
						while((token = strsep(&tok_ptr,", \t\r\0"))) {
							if (strlen(token) == 0)
								continue;
							if (cmd_item == cmd_item_addr) {
								short_test_rxrx[i].addr = str_to_hex(token + 2);
								//printk("short_test_rxrx[%d].addr = 0x%05X\n", i, short_test_rxrx[i].addr);
								cmd_item++;
							} else if (cmd_item == cmd_item_len) {
								short_test_rxrx[i].len = (uint8_t) simple_strtol(token, NULL, 10);
								//printk("short_test_rxrx[%d].len = %d\n", i, short_test_rxrx[i].len);
								cmd_item++;
							} else {
								short_test_rxrx[i].data[j] = (uint8_t) str_to_hex(token + 2);
								//printk("0x%02X, ", short_test_rxrx[i].data[j]);
								j++;
							}
						}
						//printk("\n");
						if (j != short_test_rxrx[i].len) {
							dev_err(&ts->client->dev, "command data and length loaded not match!!!, j=%d, short_test_rxrx[%d].len=%d\n", j, i, short_test_rxrx[i].len);
							retval = -26;
							goto exit_free;
						}
					}

				} else if(mp_criteria_item == Test_Cmd_Short_TXRX) {
					ptr = fbufp;
					dev_info(&ts->client->dev, "%s: load test_cmd_short_test_txrx:\n", __func__);
					if (parse_mp_criteria_item(&ptr, "test_cmd_short_test_txrx:", &test_cmd_num) < 0) {
						dev_err(&ts->client->dev, "%s: load test_cmd_short_test_txrx failed!\n", __func__);
						retval = -27;
						goto exit_free;
					}
					dev_info(&ts->client->dev, "test_cmd_num = %d\n", test_cmd_num);
					memset(test_cmd_buf, 0, 8192);
					short_test_txrx_cmd_num = test_cmd_num;
					if (short_test_txrx) {
						kfree(short_test_txrx);
						short_test_txrx = NULL;
					}
					short_test_txrx = kzalloc(test_cmd_num * sizeof(struct test_cmd), GFP_KERNEL);
					if (!short_test_txrx) {
						dev_err(&ts->client->dev, "%s: kzalloc for short_test_txrx failed.\n", __func__);
						retval = -ENOMEM;
						goto exit_free;
					}
					// load test commands
					for (i = 0; i < test_cmd_num; i++) {
						goto_next_line(&ptr);
						copy_this_line(test_cmd_buf, ptr);
						cmd_item = cmd_item_addr;
						tok_ptr = test_cmd_buf;
						j = 0;
						while((token = strsep(&tok_ptr,", \t\r\0"))) {
							if (strlen(token) == 0)
								continue;
							if (cmd_item == cmd_item_addr) {
								short_test_txrx[i].addr = str_to_hex(token + 2);
								//printk("short_test_txrx[%d].addr = 0x%05X\n", i, short_test_txrx[i].addr);
								cmd_item++;
							} else if (cmd_item == cmd_item_len) {
								short_test_txrx[i].len = (uint8_t) simple_strtol(token, NULL, 10);
								//printk("short_test_txrx[%d].len = %d\n", i, short_test_txrx[i].len);
								cmd_item++;
							} else {
								short_test_txrx[i].data[j] = (uint8_t) str_to_hex(token + 2);
								//printk("0x%02X, ", short_test_txrx[i].data[j]);
								j++;
							}
						}
						//printk("\n");
						if (j != short_test_txrx[i].len) {
							dev_err(&ts->client->dev, "command data and length loaded not match!!!, j=%d, short_test_txrx[%d].len=%d\n", j, i, short_test_txrx[i].len);
							retval = -28;
							goto exit_free;
						}
					}

				} else if(mp_criteria_item == Test_Cmd_Short_TXTX) {
					ptr = fbufp;
					dev_info(&ts->client->dev, "%s: load test_cmd_short_test_txtx:\n", __func__);
					if (parse_mp_criteria_item(&ptr, "test_cmd_short_test_txtx:", &test_cmd_num) < 0) {
						dev_err(&ts->client->dev, "%s: load test_cmd_short_test_txtx failed!\n", __func__);
						retval = -29;
						goto exit_free;
					}
					dev_info(&ts->client->dev, "test_cmd_num = %d\n", test_cmd_num);
					memset(test_cmd_buf, 0, 8192);
					short_test_txtx_cmd_num = test_cmd_num;
					if (short_test_txtx) {
						kfree(short_test_txtx);
						short_test_txtx = NULL;
					}
					short_test_txtx = kzalloc(test_cmd_num * sizeof(struct test_cmd), GFP_KERNEL);
					if (!short_test_txtx) {
						dev_err(&ts->client->dev, "%s: kzalloc for short_test_txtx failed.\n", __func__);
						retval = -ENOMEM;
						goto exit_free;
					}
					// load test commands
					for (i = 0; i < test_cmd_num; i++) {
						goto_next_line(&ptr);
						copy_this_line(test_cmd_buf, ptr);
						cmd_item = cmd_item_addr;
						tok_ptr = test_cmd_buf;
						j = 0;
						while((token = strsep(&tok_ptr,", \t\r\0"))) {
							if (strlen(token) == 0)
								continue;
							if (cmd_item == cmd_item_addr) {
								short_test_txtx[i].addr = str_to_hex(token + 2);
								//printk("test_cmd_shor_txtx[%d].addr = 0x%05X\n", i, short_test_txtx[i].addr);
								cmd_item++;
							} else if (cmd_item == cmd_item_len) {
								short_test_txtx[i].len = (uint8_t) simple_strtol(token, NULL, 10);
								//printk("short_test_txtx[%d].len = %d\n", i, short_test_txtx[i].len);
								cmd_item++;
							} else {
								short_test_txtx[i].data[j] = (uint8_t) str_to_hex(token + 2);
								//printk("0x%02X, ", short_test_txtx[i].data[j]);
								j++;
							}
						}
						//printk("\n");
						if (j != short_test_txtx[i].len) {
							dev_err(&ts->client->dev, "command data and length loaded not match!!!, j=%d, short_test_txtx[%d].len=%d\n", j, i, short_test_txtx[i].len);
							retval = -30;
							goto exit_free;
						}
					}
				} else if(mp_criteria_item == Test_Cmd_Open) {
					ptr = fbufp;
					dev_info(&ts->client->dev, "%s: load test_cmd_open_test:\n", __func__);
					if (parse_mp_criteria_item(&ptr, "test_cmd_open_test:", &test_cmd_num) < 0) {
						dev_err(&ts->client->dev, "%s: load test_cmd_open_test failed!\n", __func__);
						retval = -31;
						goto exit_free;
					}
					dev_info(&ts->client->dev, "test_cmd_num = %d\n", test_cmd_num);
					memset(test_cmd_buf, 0, 8192);
					open_test_cmd_num = test_cmd_num;
					if (open_test) {
						kfree(open_test);
						open_test = NULL;
					}
					open_test = kzalloc(test_cmd_num * sizeof(struct test_cmd), GFP_KERNEL);
					if (!open_test) {
						dev_err(&ts->client->dev, "%s: kzalloc for open_test failed\n", __func__);
						retval = -ENOMEM;
						goto exit_free;
					}
					// load test commands
					for (i = 0; i < test_cmd_num; i++) {
						goto_next_line(&ptr);
						copy_this_line(test_cmd_buf, ptr);
						cmd_item = cmd_item_addr;
						tok_ptr = test_cmd_buf;
						j = 0;
						while((token = strsep(&tok_ptr,", \t\r\0"))) {
							if (strlen(token) == 0)
								continue;
							if (cmd_item == cmd_item_addr) {
								open_test[i].addr = str_to_hex(token + 2);
								//printk("open_test[%d].addr = 0x%05X\n", i, open_test[i].addr);
								cmd_item++;
							} else if (cmd_item == cmd_item_len) {
								open_test[i].len = (uint8_t) simple_strtol(token, NULL, 10);
								//printk("open_test[%d].len = %d\n", i, open_test[i].len);
								cmd_item++;
							} else {
								open_test[i].data[j] = (uint8_t) str_to_hex(token + 2);
								//printk("0x%02X, ", open_test[i].data[j]);
								j++;
							}
						}
						//printk("\n");
						if (j != open_test[i].len) {
							dev_err(&ts->client->dev, "command data and length loaded not match!!!, j=%d, open_test[%d].len=%d\n", j, i, open_test[i].len);
							retval = -32;
							goto exit_free;
						}
					}
				} else if (mp_criteria_item == IC_CMD_SET_RXRX_isDummyCycle) {
					ptr = fbufp;
					parse_mp_criteria_item(&ptr, "IC_CMD_SET_RXRX_isDummyCycle:", &RXRX_isDummyCycle);
					dev_info(&ts->client->dev, "RXRX_isDummyCycle = %d\n", RXRX_isDummyCycle);
				} else if (mp_criteria_item == IC_CMD_SET_RXRX_Dummy_Count) {
					ptr = fbufp;
					parse_mp_criteria_item(&ptr, "IC_CMD_SET_RXRX_Dummy_Count:", &RXRX_Dummy_Count);
					dev_info(&ts->client->dev, "RXRX_Dummy_Count = %d\n", RXRX_Dummy_Count);
				} else if (mp_criteria_item == IC_CMD_SET_RXRX_isReadADCCheck) {
					ptr = fbufp;
					parse_mp_criteria_item(&ptr, "IC_CMD_SET_RXRX_isReadADCCheck:", &RXRX_isReadADCCheck);
					dev_info(&ts->client->dev, "RXRX_isReadADCCheck = %d\n", RXRX_isReadADCCheck);
				} else if (mp_criteria_item == IC_CMD_SET_RXRX_TestTimes) {
					ptr = fbufp;
					parse_mp_criteria_item(&ptr, "IC_CMD_SET_RXRX_TestTimes:", &RXRX_TestTimes);
					dev_info(&ts->client->dev, "RXRX_TestTimes = %d\n", RXRX_TestTimes);
				} else if (mp_criteria_item == IC_CMD_SET_RXRX_Dummy_Frames) {
					ptr = fbufp;
					parse_mp_criteria_item(&ptr, "IC_CMD_SET_RXRX_Dummy_Frames:", &RXRX_Dummy_Frames);
					dev_info(&ts->client->dev, "RXRX_Dummy_Frames = %d\n", RXRX_Dummy_Frames);
				} else if (mp_criteria_item == IC_CMD_SET_TXRX_isDummyCycle) {
					ptr = fbufp;
					parse_mp_criteria_item(&ptr, "IC_CMD_SET_TXRX_isDummyCycle:", &TXRX_isDummyCycle);
					dev_info(&ts->client->dev, "TXRX_isDummyCycle = %d\n", TXRX_isDummyCycle);
				} else if (mp_criteria_item == IC_CMD_SET_TXRX_Dummy_Count) {
					ptr = fbufp;
					parse_mp_criteria_item(&ptr, "IC_CMD_SET_TXRX_Dummy_Count:", &TXRX_Dummy_Count);
					dev_info(&ts->client->dev, "TXRX_Dummy_Count = %d\n", TXRX_Dummy_Count);
				} else if (mp_criteria_item == IC_CMD_SET_TXRX_isReadADCCheck) {
					ptr = fbufp;
					parse_mp_criteria_item(&ptr, "IC_CMD_SET_TXRX_isReadADCCheck:", &TXRX_isReadADCCheck);
					dev_info(&ts->client->dev, "TXRX_isReadADCCheck = %d\n", TXRX_isReadADCCheck);
				} else if (mp_criteria_item == IC_CMD_SET_TXRX_TestTimes) {
					ptr = fbufp;
					parse_mp_criteria_item(&ptr, "IC_CMD_SET_TXRX_TestTimes:", &TXRX_TestTimes);
					dev_info(&ts->client->dev, "TXRX_TestTimes = %d\n", TXRX_TestTimes);
				} else if (mp_criteria_item == IC_CMD_SET_TXRX_Dummy_Frames) {
					ptr = fbufp;
					parse_mp_criteria_item(&ptr, "IC_CMD_SET_TXRX_Dummy_Frames:", &TXRX_Dummy_Frames);
					dev_info(&ts->client->dev, "TXRX_Dummy_Frames = %d\n", TXRX_Dummy_Frames);
				} else if (mp_criteria_item == IC_CMD_SET_TXTX_isDummyCycle) {
					ptr = fbufp;
					parse_mp_criteria_item(&ptr, "IC_CMD_SET_TXTX_isDummyCycle:", &TXTX_isDummyCycle);
					dev_info(&ts->client->dev, "TXTX_isDummyCycle = %d\n", TXTX_isDummyCycle);
				} else if (mp_criteria_item == IC_CMD_SET_TXTX_Dummy_Count) {
					ptr = fbufp;
					parse_mp_criteria_item(&ptr, "IC_CMD_SET_TXTX_Dummy_Count:", &TXTX_Dummy_Count);
					dev_info(&ts->client->dev, "TXTX_Dummy_Count = %d\n", TXTX_Dummy_Count);
				} else if (mp_criteria_item == IC_CMD_SET_TXTX_isReadADCCheck) {
					ptr = fbufp;
					parse_mp_criteria_item(&ptr, "IC_CMD_SET_TXTX_isReadADCCheck:", &TXTX_isReadADCCheck);
					dev_info(&ts->client->dev, "TXTX_isReadADCCheck = %d\n", TXTX_isReadADCCheck);
				} else if (mp_criteria_item == IC_CMD_SET_TXTX_TestTimes) {
					ptr = fbufp;
					parse_mp_criteria_item(&ptr, "IC_CMD_SET_TXTX_TestTimes:", &TXTX_TestTimes);
					dev_info(&ts->client->dev, "TXTX_TestTimes = %d\n", TXTX_TestTimes);
				} else if (mp_criteria_item == IC_CMD_SET_TXTX_Dummy_Frames) {
					ptr = fbufp;
					parse_mp_criteria_item(&ptr, "IC_CMD_SET_TXTX_Dummy_Frames:", &TXTX_Dummy_Frames);
					dev_info(&ts->client->dev, "TXTX_Dummy_Frames = %d\n", TXTX_Dummy_Frames);
				} else if (mp_criteria_item == IC_CMD_SET_Mutual_isDummyCycle) {
					ptr = fbufp;
					parse_mp_criteria_item(&ptr, "IC_CMD_SET_Mutual_isDummyCycle:", &Mutual_isDummyCycle);
					dev_info(&ts->client->dev, "Mutual_isDummyCycle = %d\n", Mutual_isDummyCycle);
				} else if (mp_criteria_item == IC_CMD_SET_Mutual_Dummy_Count) {
					ptr = fbufp;
					parse_mp_criteria_item(&ptr, "IC_CMD_SET_Mutual_Dummy_Count:", &Mutual_Dummy_Count);
					dev_info(&ts->client->dev, "Mutual_Dummy_Count = %d\n", Mutual_Dummy_Count);
				} else if (mp_criteria_item == IC_CMD_SET_Mutual_isReadADCCheck) {
					ptr = fbufp;
					parse_mp_criteria_item(&ptr, "IC_CMD_SET_Mutual_isReadADCCheck:", &Mutual_isReadADCCheck);
					dev_info(&ts->client->dev, "Mutual_isReadADCCheck = %d\n", Mutual_isReadADCCheck);
				} else if (mp_criteria_item == IC_CMD_SET_Mutual_TestTimes) {
					ptr = fbufp;
					parse_mp_criteria_item(&ptr, "IC_CMD_SET_Mutual_TestTimes:", &Mutual_TestTimes);
					dev_info(&ts->client->dev, "Mutual_TestTimes = %d\n", Mutual_TestTimes);
				} else if (mp_criteria_item == IC_CMD_SET_Mutual_Dummy_Frames) {
					ptr = fbufp;
					parse_mp_criteria_item(&ptr, "IC_CMD_SET_Mutual_Dummy_Frames:", &Mutual_Dummy_Frames);
					dev_info(&ts->client->dev, "Mutual_Dummy_Frames = %d\n", Mutual_Dummy_Frames);
				}

				mp_criteria_item++;
				if (mp_criteria_item == MP_Cri_Item_Last) {
					dev_info(&ts->client->dev, "Load MP criteria and golden samples finished.\n");
					break;
				}
			}
		} else {
			dev_err(&ts->client->dev, "%s: retval=%d, read_ret=%d, fbufp=%p, stat.size=%lld\n", __func__, retval, read_ret, fbufp, stat.size);
			retval = -3;
			goto exit_free;
		}
	} else {
		dev_err(&ts->client->dev, "%s: failed to get file stat, retval = %d\n", __func__, retval);
		retval = -4;
		goto exit_free;
	}

exit_free:
	if (test_cmd_buf) {
		kfree(test_cmd_buf);
		test_cmd_buf = NULL;
	}

	set_fs(org_fs);
	if (fbufp) {
		kfree(fbufp);
		fbufp = NULL;
	}
	if (fp) {
		filp_close(fp, NULL);
		fp = NULL;
	}

	dev_info(&ts->client->dev, "%s:--, retval=%d\n", __func__, retval);
	return retval;
}

static void nvt_cal_ain_order(void)
{
	uint32_t i = 0;

	for (i = 0; i < IC_RX_CFG_SIZE; i++) {
		if (AIN_RX[i] == 0xFF)
			continue;
		AIN_RX_Order[AIN_RX[i]] = (uint8_t)i;
	}

	for (i = 0; i < IC_TX_CFG_SIZE; i++) {
		if (AIN_TX[i] == 0xFF)
			continue;
		AIN_TX_Order[AIN_TX[i]] = (uint8_t)i;
	}

#if TOUCH_KEY_NUM > 0
	for (i = 0; i < IC_RX_CFG_SIZE; i++) {
		if (AIN_KEY_RX[i] == 0xFF)
			continue;
		AIN_KEY_RX_Order[AIN_KEY_RX[i]] = (uint8_t)i;
	}

	for (i = 0; i < IC_TX_CFG_SIZE; i++) {
		if (AIN_KEY_TX[i] == 0xFF)
			continue;
		AIN_KEY_TX_Order[AIN_KEY_TX[i]] = (uint8_t)i;
	}
#endif

	dev_info(&ts->client->dev,"AIN_RX_Order:\n");
	for (i = 0; i < IC_RX_CFG_SIZE; i++)
		printk("%d, ", AIN_RX_Order[i]);
	printk("\n");

	dev_info(&ts->client->dev,"AIN_TX_Order:\n");
	for (i = 0; i < IC_TX_CFG_SIZE; i++)
		printk("%d, ", AIN_TX_Order[i]);
	printk("\n");

#if TOUCH_KEY_NUM > 0
	dev_info(&ts->client->dev,"AIN_KEY_RX_Order:\n");
	for (i = 0; i < IC_RX_CFG_SIZE; i++)
		printk("%d, ", AIN_KEY_RX_Order[i]);
	printk("\n");

	dev_info(&ts->client->dev,"AIN_KEY_TX_Order:\n");
	for (i = 0; i < IC_TX_CFG_SIZE; i++)
		printk("%d, ", AIN_KEY_TX_Order[i]);
	printk("\n");
#endif
}

static void nvt_ctrlram_fill_cmd_table(char **ptr, struct test_cmd *test_cmd_table, uint32_t *ctrlram_cmd_table_idx, uint32_t *ctrlram_cur_addr)
{
	char ctrlram_data_buf[64] = {0};
	uint32_t ctrlram_data = 0;

	goto_next_line(ptr);
	copy_this_line(ctrlram_data_buf, *ptr);
	while (strlen(ctrlram_data_buf) && (ctrlram_data_buf[0] != '\r') && (ctrlram_data_buf[0] != '\n')) {
		ctrlram_data = (uint32_t) str_to_hex(ctrlram_data_buf);
		test_cmd_table[*ctrlram_cmd_table_idx].addr = *ctrlram_cur_addr;
		test_cmd_table[*ctrlram_cmd_table_idx].len = 4;
		test_cmd_table[*ctrlram_cmd_table_idx].data[0] = (uint8_t) ctrlram_data & 0xFF;
		test_cmd_table[*ctrlram_cmd_table_idx].data[1] = (uint8_t) ((ctrlram_data & 0xFF00) >> 8);
		test_cmd_table[*ctrlram_cmd_table_idx].data[2] = (uint8_t) ((ctrlram_data & 0xFF0000) >> 16);
		test_cmd_table[*ctrlram_cmd_table_idx].data[3] = (uint8_t) ((ctrlram_data & 0xFF000000) >> 24);
		*ctrlram_cmd_table_idx = *ctrlram_cmd_table_idx + 1;
		*ctrlram_cur_addr = *ctrlram_cur_addr + 4;
		goto_next_line(ptr);
		copy_this_line(ctrlram_data_buf, *ptr);
	}
}

static int32_t nvt_load_mp_ctrl_ram(char *file_path, struct test_cmd *test_cmd_table, uint32_t *ctrlram_test_cmds_num)
{
	int32_t retval = 0;
	struct file *fp = NULL;
	char *fbufp = NULL; // buffer for content of file
	mm_segment_t org_fs;
	struct kstat stat;
	loff_t pos = 0;
	int32_t read_ret = 0;
	char *ptr = NULL;
	//uint32_t i = 0;
	ctrl_ram_item_e ctrl_ram_item = CtrlRam_REGISTER;
	char ctrlram_data_buf[64] = {0};
	uint32_t ctrlram_cur_addr = 0;
	uint32_t ctrlram_cmd_table_idx = 0;
	uint32_t ctrlram_data = 0;
	uint32_t TXMODE_Table_Addr = 0;
	uint32_t RXMODE_Table_Addr = 0;
	uint32_t CCMODE_Table_Addr = 0;
	uint32_t OFFSET_Table_Addr = 0;

	dev_info(&ts->client->dev, "%s:++\n", __func__);
	org_fs = get_fs();
	set_fs(KERNEL_DS);

	fp = filp_open(file_path, O_RDONLY, 0);
	if (fp == NULL || IS_ERR(fp)) {
		dev_err(&ts->client->dev, "%s: open %s failed\n", __func__, file_path);
		set_fs(org_fs);
		retval = -1;
		return retval;
	}

	retval = vfs_stat(file_path, &stat);
	if (!retval) {
		fbufp = (char *)kzalloc(stat.size + 1, GFP_KERNEL);
		if (!fbufp) {
			dev_err(&ts->client->dev, "%s: kzalloc %lld bytes failed.\n", __func__, stat.size);
			retval = -2;
			set_fs(org_fs);
			filp_close(fp, NULL);
			return retval;
		}

		read_ret = vfs_read(fp, (char __user *)fbufp, stat.size, &pos);
		if (read_ret > 0) {
			//pr_info("%s: File Size:%lld\n", __func__, stat.size);
			//pr_info("---------------------------------------------------\n");
			//printk("fbufp:\n");
			//for(i = 0; i < stat.size; i++) {
			//  printk("%c", fbufp[i]);
			//}
			//pr_info("---------------------------------------------------\n");

			fbufp[stat.size] = '\n';
			ptr = fbufp;

			while ( ptr && (ptr < (fbufp + stat.size))) {
				if (ctrl_ram_item == CtrlRam_REGISTER) {
					uint32_t ctrlram_table_number = 0;
					uint32_t ctrlram_start_addr = 0;
					ptr = strstr(ptr, "REGISTER");
					if (ptr == NULL) {
						dev_err(&ts->client->dev, "%s: REGISTER not found!\n", __func__);
						retval = -5;
						goto exit_free;
					}
					goto_next_line(&ptr);
					copy_this_line(ctrlram_data_buf, ptr);
					ctrlram_table_number = (uint32_t) str_to_hex(ctrlram_data_buf);
					ctrlram_table_number = ((ctrlram_table_number & 0x3F000000) >> 24);
					dev_info(&ts->client->dev, "ctrlram_table_number=%08X\n", ctrlram_table_number);
					test_cmd_table[0].addr = 0x1F200;
					test_cmd_table[0].len = 3;
					test_cmd_table[0].data[2] = (uint8_t)ctrlram_table_number;

					goto_next_line(&ptr);
					copy_this_line(ctrlram_data_buf, ptr);
					ctrlram_start_addr = (uint32_t) str_to_hex(ctrlram_data_buf);
					ctrlram_start_addr = ((ctrlram_start_addr & 0xFFFF0000) >> 16) | 0x10000;
					dev_info(&ts->client->dev, "strat_addr_table=%08X\n", ctrlram_start_addr);
					test_cmd_table[0].data[0] = (uint8_t)(ctrlram_start_addr & 0xFF);
					test_cmd_table[0].data[1] = (uint8_t)((ctrlram_start_addr & 0xFF00) >> 8);

					ctrlram_cur_addr = ctrlram_start_addr;
				} else if (ctrl_ram_item == CtrlRam_START_ADDR) {
					uint32_t table1_addr = 0;
					ptr = strstr(ptr, "START_ADDR");
					if (ptr == NULL) {
						dev_err(&ts->client->dev, "%s: START_ADDR not found!\n", __func__);
						retval = -6;
						goto exit_free;
					}
					goto_next_line(&ptr);
					copy_this_line(ctrlram_data_buf, ptr);
					table1_addr = (uint32_t) str_to_hex(ctrlram_data_buf);
					table1_addr = table1_addr & 0xFFFF;
					dev_info(&ts->client->dev, "table1_addr=%08X\n", table1_addr);
					test_cmd_table[1].addr = ctrlram_cur_addr;
					test_cmd_table[1].len = 4;
					test_cmd_table[1].data[0] = (uint8_t)(table1_addr & 0xFF);
					test_cmd_table[1].data[1] = (uint8_t)((table1_addr & 0xFF00) >> 8);
					test_cmd_table[1].data[2] = 0x00;
					test_cmd_table[1].data[3] = 0x00;

					ctrlram_cmd_table_idx = 2;
					ctrlram_cur_addr = ctrlram_cur_addr + 4;
				} else if (ctrl_ram_item == CtrlRam_TABLE_ADDR) {
					uint32_t table_addr_offset = 0;
					ptr = strstr(ptr, "TABLE_ADDR");
					if (ptr == NULL) {
						dev_err(&ts->client->dev, "%s: TABLE_ADDR not found!\n", __func__);
						retval = -7;
						goto exit_free;
					}
					goto_next_line(&ptr);
					copy_this_line(ctrlram_data_buf, ptr);
					while (strlen(ctrlram_data_buf) && (ctrlram_data_buf[0] != '\r') && (ctrlram_data_buf[0] != '\n')) {
						ctrlram_data = (uint32_t) str_to_hex(ctrlram_data_buf);
						if (table_addr_offset == 0x3C) {
							TXMODE_Table_Addr = (ctrlram_data & 0xFFFF) | 0x10000;
							dev_info(&ts->client->dev, "%s: TXMODE_Table_Addr = 0x%08X\n", __func__, TXMODE_Table_Addr);
							RXMODE_Table_Addr = ((ctrlram_data & 0xFFFF0000) >> 16) | 0x10000;
							dev_info(&ts->client->dev, "%s: RXMODE_Table_Addr = 0x%08X\n", __func__, RXMODE_Table_Addr);
						}
						if (table_addr_offset == 0x40) {
							CCMODE_Table_Addr = (ctrlram_data & 0xFFFF) | 0x10000;
							dev_info(&ts->client->dev, "%s: CCMODE_Table_Addr = 0x%08X\n", __func__, CCMODE_Table_Addr);
							OFFSET_Table_Addr = ((ctrlram_data & 0xFFFF0000) >> 16) | 0x10000;
							dev_info(&ts->client->dev, "%s: OFFSET_Table_Addr = 0x%08X\n", __func__, OFFSET_Table_Addr);
						}
						test_cmd_table[ctrlram_cmd_table_idx].addr = ctrlram_cur_addr;
						test_cmd_table[ctrlram_cmd_table_idx].len = 4;
						test_cmd_table[ctrlram_cmd_table_idx].data[0] = (uint8_t) ctrlram_data & 0xFF;
						test_cmd_table[ctrlram_cmd_table_idx].data[1] = (uint8_t) ((ctrlram_data & 0xFF00) >> 8);
						test_cmd_table[ctrlram_cmd_table_idx].data[2] = (uint8_t) ((ctrlram_data & 0xFF0000) >> 16);
						test_cmd_table[ctrlram_cmd_table_idx].data[3] = (uint8_t) ((ctrlram_data & 0xFF000000) >> 24);
						ctrlram_cmd_table_idx = ctrlram_cmd_table_idx + 1;
						ctrlram_cur_addr = ctrlram_cur_addr + 4;
						table_addr_offset = table_addr_offset + 4;
						goto_next_line(&ptr);
						copy_this_line(ctrlram_data_buf, ptr);
					}
				} else if (ctrl_ram_item == CtrlRam_UC_ADDR) {
					ptr = strstr(ptr, "UC_ADDR");
					if (ptr == NULL) {
						dev_err(&ts->client->dev, "%s: UC_ADDR not found!\n", __func__);
						retval = -8;
						goto exit_free;
					}
					nvt_ctrlram_fill_cmd_table(&ptr, test_cmd_table, &ctrlram_cmd_table_idx, &ctrlram_cur_addr);
				} else if (ctrl_ram_item == CtrlRam_TXMODE_ADDR) {
					if (TXMODE_Table_Addr != ctrlram_cur_addr) {
						dev_err(&ts->client->dev, "%s: TXMODE Table Address not match!\n", __func__);
						retval = -9;
						goto exit_free;
					}
					ptr = strstr(ptr, "TXMODE_ADDR");
					if (ptr == NULL) {
						dev_err(&ts->client->dev, "%s: TXMODE_ADDR not found!\n", __func__);
						retval = -10;
						goto exit_free;
					}
					nvt_ctrlram_fill_cmd_table(&ptr, test_cmd_table, &ctrlram_cmd_table_idx, &ctrlram_cur_addr);
				} else if (ctrl_ram_item == CtrlRam_RXMODE_ADDR) {
					if (RXMODE_Table_Addr != ctrlram_cur_addr) {
						dev_err(&ts->client->dev, "%s: RXMODE Table Address not match!\n", __func__);
						retval = -11;
						goto exit_free;
					}
					ptr = strstr(ptr, "RXMODE_ADDR");
					if (ptr == NULL) {
						dev_err(&ts->client->dev, "%s: RXMODE_ADDR not found!\n", __func__);
						retval = -12;
						goto exit_free;
					}
					nvt_ctrlram_fill_cmd_table(&ptr, test_cmd_table, &ctrlram_cmd_table_idx, &ctrlram_cur_addr);
				} else if (ctrl_ram_item == CtrlRam_CCMODE_ADDR) {
					if (CCMODE_Table_Addr != ctrlram_cur_addr) {
						dev_err(&ts->client->dev, "%s: CCMODE Table Address not match!\n", __func__);
						retval = -13;
						goto exit_free;
					}
					ptr = strstr(ptr, "CCMODE_ADDR");
					if (ptr == NULL) {
						dev_err(&ts->client->dev, "%s: CCMODE_ADDR not found!\n", __func__);
						retval = -14;
						goto exit_free;
					}
					nvt_ctrlram_fill_cmd_table(&ptr, test_cmd_table, &ctrlram_cmd_table_idx, &ctrlram_cur_addr);
				} else if (ctrl_ram_item == CtrlRam_OFFSET_ADDR) {
					if (OFFSET_Table_Addr != ctrlram_cur_addr) {
						dev_err(&ts->client->dev, "%s: OFFSET Table Address not match!\n", __func__);
						retval = -15;
						goto exit_free;
					}
					ptr = strstr(ptr, "OFFSET_ADDR");
					if (ptr == NULL) {
						dev_err(&ts->client->dev, "%s: OFFSET_ADDR not found!\n", __func__);
						retval = -16;
						goto exit_free;
					}
					nvt_ctrlram_fill_cmd_table(&ptr, test_cmd_table, &ctrlram_cmd_table_idx, &ctrlram_cur_addr);
				}

				ctrl_ram_item++;
				if (ctrl_ram_item == CtrlRam_ITEM_LAST) {
					*ctrlram_test_cmds_num = ctrlram_cmd_table_idx;
					dev_info(&ts->client->dev, "%s: ctrlram_test_cmds_num = %d\n", __func__, *ctrlram_test_cmds_num);
					dev_info(&ts->client->dev, "%s: Load control ram items finished\n", __func__);
					retval = 0;
					break;
				}
			}

        } else {
            dev_err(&ts->client->dev, "%s: retval=%d, read_ret=%d, fbufp=%p, stat.size=%lld\n", __func__, retval, read_ret, fbufp, stat.size);
            retval = -3;
            goto exit_free;
        }
    } else {
        dev_err(&ts->client->dev, "%s: failed to get file stat, retval = %d\n", __func__, retval);
        retval = -4;
        goto exit_free;
    }

exit_free:
    set_fs(org_fs);
    if (fbufp) {
        kfree(fbufp);
		fbufp = NULL;
	}
    if (fp) {
        filp_close(fp, NULL);
		fp = NULL;
	}

    dev_info(&ts->client->dev, "%s:--, retval=%d\n", __func__, retval);
    return retval;
}

/*******************************************************
Description:
	Novatek touchscreen set ADC operation function.

return:
	Executive outcomes. 0---succeed. negative---failed.
*******************************************************/
static int32_t nvt_set_adc_oper(int32_t isReadADCCheck)
{
	uint8_t buf[4] = {0};
	int32_t i = 0;
	const int32_t retry = 10;

	//---write i2c cmds to set ADC operation---
	buf[0] = 0xFF;
	buf[1] = 0x01;
	buf[2] = 0xF2;
	CTP_I2C_WRITE(ts->client, I2C_FW_Address, buf, 3);

	//---write i2c cmds to set ADC operation---
	buf[0] = 0x10;
	buf[1] = 0x01;
	CTP_I2C_WRITE(ts->client, I2C_FW_Address, buf, 2);

	if (isReadADCCheck) {
		for (i = 0; i < retry; i++) {
			//---read ADC status---
			buf[0] = 0x10;
			CTP_I2C_READ(ts->client, I2C_FW_Address, buf, 2);

			if (buf[1] == 0x00)
				break;

			msleep(10);
		}

		if (i >= retry) {
			dev_err(&ts->client->dev,"%s: Failed!\n", __func__);
			return -1;
		} else {
			return 0;
		}
	} else {
		return 0;
	}
}

/*******************************************************
Description:
	Novatek touchscreen write test commands function.

return:
	n.a.
*******************************************************/
static void nvt_write_test_cmd(struct test_cmd *cmds, int32_t cmd_num)
{
	int32_t i = 0;
	int32_t j = 0;
	uint8_t buf[64];

	for (i = 0; i < cmd_num; i++) {
		//---set xdata index---
		buf[0] = 0xFF;
		buf[1] = ((cmds[i].addr >> 16) & 0xFF);
		buf[2] = ((cmds[i].addr >> 8) & 0xFF);
		CTP_I2C_WRITE(ts->client, I2C_FW_Address, buf, 3);

		//---write test cmds---
		buf[0] = (cmds[i].addr & 0xFF);
		for (j = 0; j < cmds[i].len; j++) {
			buf[1 + j] = cmds[i].data[j];
		}
		CTP_I2C_WRITE(ts->client, I2C_FW_Address, buf, 1 + cmds[i].len);

/*
		//---read test cmds (debug)---
		buf[0] = (cmds[i].addr & 0xFF);
		CTP_I2C_READ(ts->client, I2C_FW_Address, buf, 1 + cmds[i].len);
		printk("0x%08X, ", cmds[i].addr);
		for (j = 0; j < cmds[i].len; j++) {
			printk("0x%02X, ", buf[j + 1]);
		}
		printk("\n");
*/
	}
}

#define ABS(x)	(((x) < 0) ? -(x) : (x))
/*******************************************************
Description:
	Novatek touchscreen read short test RX-RX raw data
	function.

return:
	Executive outcomes. 0---succeed. negative---failed.
*******************************************************/
static int32_t nvt_read_short_rxrx(void)
{
	int32_t i = 0;
	int32_t j = 0;
	int32_t k = 0;
	uint8_t buf[128] = {0};
	struct file *fp = NULL;
	char *fbufp = NULL;
	mm_segment_t org_fs;
	char file_path[64]="/data/misc/touch/ShortTestRX-RX.csv";	//TP-FixRunInFail-00*_20161201
	loff_t pos = 0;
	int32_t write_ret = 0;
	uint32_t output_len = 0;
	char ctrlram_short_rxrx_file[64]="/system/etc/CtrlRAM_Short_RXRX.csv";
	char ctrlram_short_rxrx1_file[64]="/system/etc/CtrlRAM_Short_RXRX1.csv";
	struct test_cmd *ctrlram_short_rxrx_cmds = NULL;
	struct test_cmd *ctrlram_short_rxrx1_cmds = NULL;
	uint32_t ctrlram_short_rxrx_cmds_num = 0;
	uint32_t ctrlram_short_rxrx1_cmds_num = 0;
	struct test_cmd cmd_WTG_Disable = {.addr=0x1F028, .len=2, .data={0x07, 0x55}};
	uint8_t rawdata_buf[IC_RX_CFG_SIZE * 2] = {0};
	uint8_t rawdata_buf1[IC_RX_CFG_SIZE * 2] = {0};
	int16_t sh = 0;
	int16_t sh1 = 0;
#if TOUCH_KEY_NUM > 0
	int32_t RX_idx = -1;
#endif

	dev_info(&ts->client->dev, "%s:++\n", __func__);
	// Load CtrlRAM for rxrx
	ctrlram_short_rxrx_cmds = (struct test_cmd *)kzalloc( 512 * sizeof(struct test_cmd), GFP_KERNEL);
	if (!ctrlram_short_rxrx_cmds) {
		dev_err(&ts->client->dev, "%s: kzalloc for ctrlram_short_rxrx_cmds failed.\n", __func__);
		return -ENOMEM;
	}
	if (nvt_load_mp_ctrl_ram(ctrlram_short_rxrx_file, ctrlram_short_rxrx_cmds, &ctrlram_short_rxrx_cmds_num) < 0) {
		dev_err(&ts->client->dev, "%s: load control ram %s failed!\n", __func__, ctrlram_short_rxrx_file);
		if (ctrlram_short_rxrx_cmds) {
			kfree(ctrlram_short_rxrx_cmds);
			ctrlram_short_rxrx_cmds = NULL;
		}
		return -EAGAIN;
	}
	ctrlram_short_rxrx_cmds[1].data[2] = 0x18;

	// Load CtrlRAM for rxrx1
	ctrlram_short_rxrx1_cmds = (struct test_cmd *)kzalloc( 512 * sizeof(struct test_cmd), GFP_KERNEL);
	if (!ctrlram_short_rxrx1_cmds) {
		dev_err(&ts->client->dev, "%s: kzalloc for ctrlram_short_rxrx1_cmds failed.\n", __func__);
		if (ctrlram_short_rxrx_cmds) {
			kfree(ctrlram_short_rxrx_cmds);
			ctrlram_short_rxrx_cmds = NULL;
		}
		return -ENOMEM;
	}
	if (nvt_load_mp_ctrl_ram(ctrlram_short_rxrx1_file, ctrlram_short_rxrx1_cmds, &ctrlram_short_rxrx1_cmds_num) < 0) {
		dev_err(&ts->client->dev, "%s: load control ram %s failed!\n", __func__, ctrlram_short_rxrx1_file);
		if (ctrlram_short_rxrx_cmds) {
			kfree(ctrlram_short_rxrx_cmds);
			ctrlram_short_rxrx_cmds = NULL;
		}
		if (ctrlram_short_rxrx1_cmds) {
			kfree(ctrlram_short_rxrx1_cmds);
			ctrlram_short_rxrx1_cmds = NULL;
		}
		return -EAGAIN;
	}
	ctrlram_short_rxrx1_cmds[1].data[2] = 0x18;

	for (j = 0; j < RXRX_TestTimes; j++) {
		//---Reset IC & into idle---
		nvt_sw_reset_idle();

		// WTG Disable
		nvt_write_test_cmd(&cmd_WTG_Disable, 1);
		// CtrlRAM test commands
		nvt_write_test_cmd(ctrlram_short_rxrx_cmds, ctrlram_short_rxrx_cmds_num);
		// Other test commands
		nvt_write_test_cmd(short_test_rxrx, short_test_rxrx_cmd_num);

		if (RXRX_isDummyCycle) {
			for (k = 0; k < RXRX_Dummy_Count; k++) {
				if (nvt_set_adc_oper(RXRX_isReadADCCheck) != 0) {
					if (ctrlram_short_rxrx_cmds) {
						kfree(ctrlram_short_rxrx_cmds);
						ctrlram_short_rxrx_cmds = NULL;
					}
					if (ctrlram_short_rxrx1_cmds) {
						kfree(ctrlram_short_rxrx1_cmds);
						ctrlram_short_rxrx1_cmds = NULL;
					}
					return -EAGAIN;
				}
			}
		}
		if (nvt_set_adc_oper(RXRX_isReadADCCheck) != 0) {
			if (ctrlram_short_rxrx_cmds) {
				kfree(ctrlram_short_rxrx_cmds);
				ctrlram_short_rxrx_cmds = NULL;
			}
			if (ctrlram_short_rxrx1_cmds) {
				kfree(ctrlram_short_rxrx1_cmds);
				ctrlram_short_rxrx1_cmds = NULL;
			}
			return -EAGAIN;
		}

		//---change xdata index---
		buf[0] = 0xFF;
		buf[1] = 0x01;
		buf[2] = 0x00;
		CTP_I2C_WRITE(ts->client, I2C_FW_Address, buf, 3);
		//---read data---
		buf[0] = 0x00;
		CTP_I2C_READ(ts->client, I2C_FW_Address, buf, IC_RX_CFG_SIZE * 2 + 1);
		memcpy(rawdata_buf, buf + 1, IC_RX_CFG_SIZE * 2);

		//---Reset IC & into idle---
		nvt_sw_reset_idle();

		// WTG Disable
		nvt_write_test_cmd(&cmd_WTG_Disable, 1);
		// CtrlRAM test commands
		nvt_write_test_cmd(ctrlram_short_rxrx1_cmds, ctrlram_short_rxrx1_cmds_num);
		// Other test commands
		nvt_write_test_cmd(short_test_rxrx, short_test_rxrx_cmd_num);

		if (RXRX_isDummyCycle) {
			for (k = 0; k < RXRX_Dummy_Count; k++) {
				if (nvt_set_adc_oper(RXRX_isReadADCCheck) != 0) {
					if (ctrlram_short_rxrx_cmds) {
						kfree(ctrlram_short_rxrx_cmds);
						ctrlram_short_rxrx_cmds = NULL;
					}
					if (ctrlram_short_rxrx1_cmds) {
						kfree(ctrlram_short_rxrx1_cmds);
						ctrlram_short_rxrx1_cmds = NULL;
					}
					return -EAGAIN;
				}
			}
		}
		if (nvt_set_adc_oper(RXRX_isReadADCCheck) != 0) {
			if (ctrlram_short_rxrx_cmds) {
				kfree(ctrlram_short_rxrx_cmds);
				ctrlram_short_rxrx_cmds = NULL;
			}
			if (ctrlram_short_rxrx1_cmds) {
				kfree(ctrlram_short_rxrx1_cmds);
				ctrlram_short_rxrx1_cmds = NULL;
			}
			return -EAGAIN;
		}

		//---change xdata index---
		buf[0] = 0xFF;
		buf[1] = 0x01;
		buf[2] = 0x00;
		CTP_I2C_WRITE(ts->client, I2C_FW_Address, buf, 3);
		//---read data---
		buf[0] = 0x00;
		CTP_I2C_READ(ts->client, I2C_FW_Address, buf, IC_RX_CFG_SIZE * 2 + 1);
		memcpy(rawdata_buf1, buf + 1, IC_RX_CFG_SIZE * 2);

		for (i = 0; i < (AIN_RX_NUM + GND_Channel_Real); i++) {
			sh = (int16_t)(rawdata_buf[AIN_RX_Order[i] * 2] + 256 * rawdata_buf[AIN_RX_Order[i] * 2 + 1]);
			sh1 = (int16_t)(rawdata_buf1[AIN_RX_Order[i] * 2] + 256 * rawdata_buf1[AIN_RX_Order[i] * 2 + 1]);
			if (ABS(sh) < ABS(sh1))
				sh = sh1;
			if (j == RXRX_Dummy_Frames) {
				rawdata_short_rxrx[i] = sh;
			} else if (j > RXRX_Dummy_Frames) {
				rawdata_short_rxrx[i] += sh;
				rawdata_short_rxrx[i] /= 2;
			}
		}
#if TOUCH_KEY_NUM > 0
		RX_idx = -1;
		for (i = 0; i < IC_RX_CFG_SIZE; i++) {
			if (AIN_KEY_RX[i] == 0xFF)
				continue;
			else
				RX_idx++;

			sh = (int16_t)(rawdata_buf[AIN_KEY_RX_Order[AIN_KEY_RX[i]] * 2] + 256 * rawdata_buf[AIN_KEY_RX_Order[AIN_KEY_RX[i]] * 2 + 1]);
			sh1 = (int16_t)(rawdata_buf1[AIN_KEY_RX_Order[AIN_KEY_RX[i]] * 2] + 256 * rawdata_buf1[AIN_KEY_RX_Order[AIN_KEY_RX[i]] * 2 + 1]);
			if (ABS(sh) < ABS(sh1))
				sh = sh1;
			if (j == RXRX_Dummy_Frames) {
				RXRX_test_keydata[RX_idx] = sh;
			} else if (j > RXRX_Dummy_Frames) {
				RXRX_test_keydata[RX_idx] += sh;
				RXRX_test_keydata[RX_idx] /= 2;
			}
		}
#endif /* #if TOUCH_KEY_NUM > 0 */
	} // RXRX_TestTimes

	if (ctrlram_short_rxrx_cmds) {
		kfree(ctrlram_short_rxrx_cmds);
		ctrlram_short_rxrx_cmds = NULL;
	}

	if (ctrlram_short_rxrx1_cmds) {
		kfree(ctrlram_short_rxrx1_cmds);
		ctrlram_short_rxrx1_cmds = NULL;
	}

	fbufp = (char *)kzalloc(8192, GFP_KERNEL);
	if (!fbufp) {
		dev_err(&ts->client->dev, "%s: kzalloc for fbufp failed.\n", __func__);
		return -ENOMEM;
	}

	//---for debug---
	printk("%s:\n", __func__);
	for (i = 0; i < (AIN_RX_NUM + GND_Channel_Real); i++) {
		printk("%5d, ", rawdata_short_rxrx[i]);
		sprintf(fbufp + 7 * i, "%5d, ", rawdata_short_rxrx[i]) ;
	}
	printk("\n");
	sprintf(fbufp + 7 * (AIN_RX_NUM + GND_Channel_Real), "\r\n");
#if TOUCH_KEY_NUM > 0
	for (i = 0; i < Key_X_Channel_Real; i++) {
		printk("%5d, ", RXRX_test_keydata[i]);
		sprintf(fbufp + 7 * (AIN_RX_NUM + GND_Channel_Real) + 2 + 7 * i, "%5d, ", RXRX_test_keydata[i]);
	}
	printk("\n");
	sprintf(fbufp + 7 * (AIN_RX_NUM + GND_Channel_Real) + 2 + 7 * Key_X_Channel_Real, "\r\n");
#endif /* #if TOUCH_KEY_NUM > 0 */

	org_fs = get_fs();
	set_fs(KERNEL_DS);
	fp = filp_open(file_path, O_RDWR | O_CREAT, 0644);
	if (fp == NULL || IS_ERR(fp)) {
		dev_err(&ts->client->dev, "%s: open %s failed\n", __func__, file_path);
		set_fs(org_fs);
		if (fbufp) {
			kfree(fbufp);
			fbufp = NULL;
		}
		return -1;
	}

	output_len = (AIN_RX_NUM + GND_Channel_Real) * 7 + 2;
#if TOUCH_KEY_NUM > 0
	output_len += (Key_X_Channel_Real * 7 + 2);
#endif /* #if TOUCH_KEY_NUM > 0 */
	write_ret = vfs_write(fp, (char __user *)fbufp, output_len, &pos);
	if (write_ret <= 0) {
		dev_err(&ts->client->dev, "%s: write %s failed\n", __func__, file_path);
		set_fs(org_fs);
		if (fp) {
			filp_close(fp, NULL);
			fp = NULL;
		}
		if (fbufp) {
			kfree(fbufp);
			fbufp = NULL;
		}
		return -1;
	}

	set_fs(org_fs);
	if (fp) {
		filp_close(fp, NULL);
		fp = NULL;
	}
	if (fbufp) {
		kfree(fbufp);
		fbufp = NULL;
	}

	dev_info(&ts->client->dev, "%s:--\n", __func__);
	return 0;
}

/*******************************************************
Description:
	Novatek touchscreen read short test TX-RX raw data
	function.

return:
	Executive outcomes. 0---succeed. negative---failed.
*******************************************************/
static int32_t nvt_read_short_txrx(void)
{
	int32_t i = 0;
	int32_t j = 0;
	int32_t k = 0;
	int32_t iArrayIndex = 0;
	uint8_t buf[128] = {0};
	struct file *fp = NULL;
	char *fbufp = NULL;
	mm_segment_t org_fs;
	char file_path[64]="/data/misc/touch/ShortTestTX-RX.csv";	//TP-FixRunInFail-00*_20161201
	loff_t pos = 0;
	int32_t write_ret = 0;
	uint32_t output_len = 0;
	char ctrlram_short_txrx_file[64]="/system/etc/CtrlRAM_Short_TXRX.csv";
	struct test_cmd *ctrlram_short_txrx_cmds = NULL;
	uint32_t ctrlram_short_txrx_cmds_num = 0;
	struct test_cmd cmd_WTG_Disable = {.addr=0x1F028, .len=2, .data={0x07, 0x55}};
	uint8_t *rawdata_buf = NULL;
	int16_t sh = 0;
#if TOUCH_KEY_NUM > 0
	int32_t TX_idx = -1;
	int32_t RX_idx = -1;
	uint32_t keydata_output_offset = 0;
#endif /* #if TOUCH_KEY_NUM > 0 */

	dev_info(&ts->client->dev, "%s:++\n", __func__);
	// Load CtrlRAM
	ctrlram_short_txrx_cmds = (struct test_cmd *)kzalloc( 512 * sizeof(struct test_cmd), GFP_KERNEL);
	if (!ctrlram_short_txrx_cmds) {
		dev_err(&ts->client->dev, "%s: kzalloc for ctrlram_short_txrx_cmds failed.\n", __func__);
		return -ENOMEM;
	}
	if (nvt_load_mp_ctrl_ram(ctrlram_short_txrx_file, ctrlram_short_txrx_cmds, &ctrlram_short_txrx_cmds_num) < 0) {
		dev_err(&ts->client->dev, "%s: load control ram %s failed!\n", __func__, ctrlram_short_txrx_file);
		if (ctrlram_short_txrx_cmds) {
			kfree(ctrlram_short_txrx_cmds);
			ctrlram_short_txrx_cmds = NULL;
		}
		return -EAGAIN;
	}
	ctrlram_short_txrx_cmds[1].data[2] = 0x18;

	//---Reset IC & into idle---
	nvt_sw_reset_idle();

	// WTG Disable
	nvt_write_test_cmd(&cmd_WTG_Disable, 1);
	// CtrlRAM test commands
	nvt_write_test_cmd(ctrlram_short_txrx_cmds, ctrlram_short_txrx_cmds_num);
	// Other test commands
	nvt_write_test_cmd(short_test_txrx, short_test_txrx_cmd_num);

	rawdata_buf = (uint8_t *) kzalloc(IC_TX_CFG_SIZE * IC_RX_CFG_SIZE * 2, GFP_KERNEL);
	if (!rawdata_buf) {
		dev_err(&ts->client->dev, "%s: allocated for rawdata_buf failed.\n", __func__);
		if (ctrlram_short_txrx_cmds) {
			kfree(ctrlram_short_txrx_cmds);
			ctrlram_short_txrx_cmds = NULL;
		}
		return -ENOMEM;
	}

	if (TXRX_isDummyCycle) {
		for (k = 0; k < TXRX_Dummy_Count; k++) {
			if (nvt_set_adc_oper(TXRX_isReadADCCheck) != 0) {
				if (rawdata_buf) {
					kfree(rawdata_buf);
					rawdata_buf = NULL;
				}
				if (ctrlram_short_txrx_cmds) {
					kfree(ctrlram_short_txrx_cmds);
					ctrlram_short_txrx_cmds = NULL;
				}
				return -EAGAIN;
			}
		}
	}

	for (k = 0; k < TXRX_TestTimes; k++) {
		if (nvt_set_adc_oper(TXRX_isReadADCCheck) != 0) {
			if (rawdata_buf) {
				kfree(rawdata_buf);
				rawdata_buf = NULL;
			}
			if (ctrlram_short_txrx_cmds) {
				kfree(ctrlram_short_txrx_cmds);
				ctrlram_short_txrx_cmds = NULL;
			}
			return -EAGAIN;
		}

		for (i = 0; i < IC_TX_CFG_SIZE; i++) {
			//---change xdata index---
			buf[0] = 0xFF;
			buf[1] = 0x01;
			buf[2] = 0x00 + (uint8_t)(((i * IC_RX_CFG_SIZE * 2) & 0xFF00) >> 8);
			CTP_I2C_WRITE(ts->client, I2C_FW_Address, buf, 3);
			//---read data---
			buf[0] = (uint8_t)((i * IC_RX_CFG_SIZE * 2) & 0xFF);
			CTP_I2C_READ(ts->client, I2C_FW_Address, buf, IC_RX_CFG_SIZE * 2 + 1);
			memcpy(rawdata_buf + i * IC_RX_CFG_SIZE * 2, buf + 1, IC_RX_CFG_SIZE * 2);
		}
		//printk("rawdata_short_txrx: k=%d, TXRX_Dummy_Frames=%d\n", k, TXRX_Dummy_Frames);
		for (i = 0; i < AIN_TX_NUM; i++) {
			for (j = 0; j < (AIN_RX_NUM + GND_Channel_Real); j++) {
				sh = (int16_t)(rawdata_buf[(AIN_TX_Order[i] * IC_RX_CFG_SIZE + AIN_RX_Order[j]) * 2] + 256 * rawdata_buf[(AIN_TX_Order[i] * IC_RX_CFG_SIZE + AIN_RX_Order[j]) * 2 + 1]);
				iArrayIndex = i * (AIN_RX_NUM + GND_Channel_Real) + j;
				if (k == TXRX_Dummy_Frames) {
					rawdata_short_txrx[iArrayIndex] = sh;
				} else if (k > TXRX_Dummy_Frames) {
					rawdata_short_txrx[iArrayIndex] = rawdata_short_txrx[iArrayIndex] + sh;
					rawdata_short_txrx[iArrayIndex] = rawdata_short_txrx[iArrayIndex] / 2;
				}
				//if (k >= TXRX_Dummy_Frames)
				//	printk("%5d, ", rawdata_short_txrx[iArrayIndex]);
			}
			//if (k >= TXRX_Dummy_Frames)
			//	printk("\n");
		}
#if TOUCH_KEY_NUM > 0
		TX_idx = -1;
		RX_idx = -1;
		for (i = 0; i < IC_TX_CFG_SIZE; i++) {
			if (AIN_KEY_TX[i] == 0xFF)
				continue;
			else
				TX_idx++;

			for (j = 0; j < IC_RX_CFG_SIZE; j++) {
				if (AIN_KEY_RX[j] == 0xFF)
					continue;
				else
					RX_idx++;

				sh = (int16_t)(rawdata_buf[((AIN_KEY_TX_Order[AIN_KEY_TX[i]] * IC_RX_CFG_SIZE) + AIN_KEY_RX_Order[AIN_KEY_RX[j]]) * 2] + 256 * rawdata_buf[((AIN_KEY_TX_Order[AIN_KEY_TX[i]] * IC_RX_CFG_SIZE) + AIN_KEY_RX_Order[AIN_KEY_RX[j]]) * 2 + 1]);
				iArrayIndex = Key_X_Channel_Real * TX_idx + RX_idx;
				if (k == TXRX_Dummy_Frames) {
					TXRX_test_keydata[iArrayIndex] = sh;
				} else if (k > TXRX_Dummy_Frames) {
					TXRX_test_keydata[iArrayIndex] += sh;
					TXRX_test_keydata[iArrayIndex] /= 2;
				}
			}
		}
#endif /* #if TOUCH_KEY_NUM > 0 */

		//if (k >= TXRX_Dummy_Frames)
		//	printk("\n");
	} // TXRX_TestTimes

	if (rawdata_buf) {
		kfree(rawdata_buf);
		rawdata_buf = NULL;
	}

	if (ctrlram_short_txrx_cmds) {
		kfree(ctrlram_short_txrx_cmds);
		ctrlram_short_txrx_cmds = NULL;
	}

	fbufp = (char *)kzalloc(8192, GFP_KERNEL);
	if (!fbufp) {
		dev_err(&ts->client->dev, "%s: kzalloc for fbufp failed.\n", __func__);
		return -ENOMEM;
	}

	//---for debug---
	printk("%s:\n", __func__);
	for (i = 0; i < AIN_TX_NUM; i++) {
		for (j = 0; j < (AIN_RX_NUM + GND_Channel_Real); j++) {
			iArrayIndex = i * (AIN_RX_NUM + GND_Channel_Real) + j;
			printk("%5d, ", rawdata_short_txrx[iArrayIndex]);
			sprintf(fbufp + 7 * (iArrayIndex) + i * 2, "%5d, ", rawdata_short_txrx[iArrayIndex]) ;
		}
		printk("\n");
		sprintf(fbufp + 7 * (iArrayIndex + 1) + i * 2,"\r\n");
	}
#if TOUCH_KEY_NUM > 0
	keydata_output_offset = 7 * AIN_TX_NUM * (AIN_RX_NUM + GND_Channel_Real) + AIN_TX_NUM * 2;
	for (i = 0; i < Key_Y_Channel_Real; i++) {
		for (j = 0; j < Key_X_Channel_Real; j++) {
			iArrayIndex = i * Key_X_Channel_Real + j;
			printk("%5d, ", TXRX_test_keydata[iArrayIndex]);
			sprintf(fbufp + keydata_output_offset + 7 * (iArrayIndex) + i * 2, "%5d, ", TXRX_test_keydata[iArrayIndex]);
		}
		printk("\n");
		sprintf(fbufp + keydata_output_offset + 7 * (iArrayIndex + 1) + i * 2, "\r\n");
	}
#endif /* #if TOUCH_KEY_NUM > 0 */

	org_fs = get_fs();
	set_fs(KERNEL_DS);
	fp = filp_open(file_path, O_RDWR | O_CREAT, 0644);
	if (fp == NULL || IS_ERR(fp)) {
		dev_err(&ts->client->dev, "%s: open %s failed\n", __func__, file_path);
		set_fs(org_fs);
		if (fbufp) {
			kfree(fbufp);
			fbufp = NULL;
		}
		return -1;
	}

	output_len = AIN_TX_NUM * (AIN_RX_NUM + GND_Channel_Real) * 7 + AIN_TX_NUM * 2;
#if TOUCH_KEY_NUM > 0
	output_len += Key_Y_Channel_Real * Key_X_Channel_Real * 7 + Key_Y_Channel_Real * 2;
#endif /* #if TOUCH_KEY_NUM > 0 */
	write_ret = vfs_write(fp, (char __user *)fbufp, output_len, &pos);
	if (write_ret <= 0) {
		dev_err(&ts->client->dev, "%s: write %s failed\n", __func__, file_path);
		set_fs(org_fs);
		if (fp) {
			filp_close(fp, NULL);
			fp = NULL;
		}
		if (fbufp) {
			kfree(fbufp);
			fbufp = NULL;
		}
		return -1;
	}

	set_fs(org_fs);
	if (fp) {
		filp_close(fp, NULL);
		fp = NULL;
	}
	if (fbufp) {
		kfree(fbufp);
		fbufp = NULL;
	}

	dev_info(&ts->client->dev, "%s:--\n", __func__);
	return 0;
}

/*******************************************************
Description:
	Novatek touchscreen read short test TX-TX raw data
	function.

return:
	Executive outcomes. 0---succeed. negative---failed.
*******************************************************/
static int32_t nvt_read_short_txtx(void)
{
	int32_t i = 0;
	int32_t k = 0;
	uint8_t buf[128] = {0};
	struct file *fp = NULL;
	char *fbufp = NULL;
	mm_segment_t org_fs;
	char file_path[64]="/data/misc/touch/ShortTestTX-TX.csv";	//TP-FixRunInFail-00*_20161201
	loff_t pos = 0;
	int32_t write_ret = 0;
	uint32_t output_len = 0;
	char ctrlram_short_txtx_file[64]="/system/etc/CtrlRAM_Short_TXTX.csv";
	struct test_cmd *ctrlram_short_txtx_cmds = NULL;
	uint32_t ctrlram_short_txtx_cmds_num = 0;
	struct test_cmd cmd_WTG_Disable = {.addr=0x1F028, .len=2, .data={0x07, 0x55}};
	int16_t sh = 0;
#if TOUCH_KEY_NUM > 0
	int32_t TX_idx = -1;
#endif /* #if TOUCH_KEY_NUM > 0 */

	dev_info(&ts->client->dev, "%s:++\n", __func__);
	// Load CtrlRAM
	ctrlram_short_txtx_cmds = (struct test_cmd *)kzalloc( 512 * sizeof(struct test_cmd), GFP_KERNEL);
	if (!ctrlram_short_txtx_cmds) {
		dev_err(&ts->client->dev, "%s: kzalloc for ctrlram_short_txtx_cmds failed.\n", __func__);
		return -ENOMEM;
	}
	if (nvt_load_mp_ctrl_ram(ctrlram_short_txtx_file, ctrlram_short_txtx_cmds, &ctrlram_short_txtx_cmds_num) < 0) {
		dev_err(&ts->client->dev, "%s: load control ram %s failed!\n", __func__, ctrlram_short_txtx_file);
		if (ctrlram_short_txtx_cmds) {
			kfree(ctrlram_short_txtx_cmds);
			ctrlram_short_txtx_cmds = NULL;
		}
		return -EAGAIN;
	}
	ctrlram_short_txtx_cmds[1].data[2] = 0x18;

	//---Reset IC & into idle---
	nvt_sw_reset_idle();

	// WTG Disable
	nvt_write_test_cmd(&cmd_WTG_Disable, 1);
	// CtrlRAM test commands
	nvt_write_test_cmd(ctrlram_short_txtx_cmds, ctrlram_short_txtx_cmds_num);
	// Other test commands
	nvt_write_test_cmd(short_test_txtx, short_test_txtx_cmd_num);

	if (TXTX_isDummyCycle) {
		for (k = 0; k < TXTX_Dummy_Count; k++) {
			if (nvt_set_adc_oper(TXTX_isReadADCCheck) != 0) {
				if (ctrlram_short_txtx_cmds) {
					kfree(ctrlram_short_txtx_cmds);
					ctrlram_short_txtx_cmds = NULL;
				}
				return -EAGAIN;
			}
		}
	}

	for (k = 0; k < TXTX_TestTimes; k++) {
		if (nvt_set_adc_oper(TXTX_isReadADCCheck) != 0) {
			if (ctrlram_short_txtx_cmds) {
				kfree(ctrlram_short_txtx_cmds);
				ctrlram_short_txtx_cmds = NULL;
			}
			return -EAGAIN;
		}

		//---change xdata index---
		buf[0] = 0xFF;
		buf[1] = 0x01;
		buf[2] = 0x00;
		CTP_I2C_WRITE(ts->client, I2C_FW_Address, buf, 3);
		//---read data---
		buf[0] = 0x00;
		CTP_I2C_READ(ts->client, I2C_FW_Address, buf, IC_TX_CFG_SIZE * 4 + 1);
		//printk("rawdata_short_txtx: k=%d, TXTX_Dummy_Frames=%d\n", k, TXTX_Dummy_Frames);
		for (i = 0; i < AIN_TX_NUM; i++) {
			if (AIN_TX_Order[i] < 10)
				sh = (int16_t)(buf[AIN_TX_Order[i] * 4 + 1] + 256 * buf[AIN_TX_Order[i] * 4 + 2]);
			else
				sh = (int16_t)(buf[AIN_TX_Order[i] * 4 + 2 + 1] + 256 * buf[AIN_TX_Order[i] * 4 + 2 + 2]);

			if (k == TXTX_Dummy_Frames) {
				rawdata_short_txtx[i] = sh;
			} else if (k > TXTX_Dummy_Frames) {
				rawdata_short_txtx[i] = rawdata_short_txtx[i] + sh;
				rawdata_short_txtx[i] = rawdata_short_txtx[i] / 2;
			}
			//if (k >= TXTX_Dummy_Frames)
			//	printk("%5d, ", rawdata_short_txtx[i]);
		}
#if TOUCH_KEY_NUM > 0
		TX_idx = -1;
		for (i = 0; i < IC_TX_CFG_SIZE; i++) {
			if (AIN_KEY_TX[i] == 0xFF)
				continue;
			else
				TX_idx++;

			if (AIN_KEY_TX_Order[AIN_KEY_TX[i]] < 10)
				sh = (int16_t)(buf[AIN_KEY_TX_Order[AIN_KEY_TX[i]] * 4 + 1] + 256 * buf[AIN_KEY_TX_Order[AIN_KEY_TX[i]] * 4 + 2]);
			else
				sh = (int16_t)(buf[AIN_KEY_TX_Order[AIN_KEY_TX[i]] * 4 + 2 + 1] + 256 * buf[AIN_KEY_TX_Order[AIN_KEY_TX[i]] * 4 + 2 + 2]);

			if (k == TXTX_Dummy_Frames) {
				TXTX_test_keydata[TX_idx] = sh;
			} else if (k > TXTX_Dummy_Frames) {
				TXTX_test_keydata[TX_idx] += sh;
				TXTX_test_keydata[TX_idx] /= 2;
			}
		}
#endif /* #if TOUCH_KEY_NUM > 0 */

		//if (k >= TXTX_Dummy_Frames)
		//	printk("\n");
	} // TXTX_TestTimes

	if (ctrlram_short_txtx_cmds) {
		kfree(ctrlram_short_txtx_cmds);
		ctrlram_short_txtx_cmds = NULL;
	}

	fbufp = (char *)kzalloc(8192, GFP_KERNEL);
	if (!fbufp) {
		dev_err(&ts->client->dev, "%s: kzalloc for fbufp failed.\n", __func__);
		return -ENOMEM;
	}

	//---for debug---
	printk("%s:\n", __func__);
	for (i = 0; i < AIN_TX_NUM; i++) {
		printk("%5d, ", rawdata_short_txtx[i]);
		sprintf(fbufp + 7 * i, "%5d, ", rawdata_short_txtx[i]) ;
	}
	printk("\n");
	sprintf(fbufp + 7 * AIN_TX_NUM, "\r\n");
#if TOUCH_KEY_NUM > 0
	for (i = 0; i < Key_Y_Channel_Real; i++) {
		printk("%5d, ", TXTX_test_keydata[i]);
		sprintf(fbufp + 7 * AIN_TX_NUM + 2 + 7 * i, "%5d, ", TXTX_test_keydata[i]);
	}
	sprintf(fbufp + 7 * AIN_TX_NUM + 2 + 7 * Key_Y_Channel_Real, "\r\n");
#endif /* #if TOUCH_KEY_NUM > 0 */

	org_fs = get_fs();
	set_fs(KERNEL_DS);
	fp = filp_open(file_path, O_RDWR | O_CREAT, 0644);
	if (fp == NULL || IS_ERR(fp)) {
		dev_err(&ts->client->dev, "%s: open %s failed\n", __func__, file_path);
		set_fs(org_fs);
		if (fbufp) {
			kfree(fbufp);
			fbufp = NULL;
		}
		return -1;
	}

	output_len = AIN_TX_NUM * 7 + 2;
#if TOUCH_KEY_NUM > 0
	output_len += Key_Y_Channel_Real * 7 + 2;
#endif /* #if TOUCH_KEY_NUM > 0 */
	write_ret = vfs_write(fp, (char __user *)fbufp, output_len, &pos);
	if (write_ret <= 0) {
		dev_err(&ts->client->dev, "%s: write %s failed\n", __func__, file_path);
		set_fs(org_fs);
		if (fp) {
			filp_close(fp, NULL);
			fp = NULL;
		}
		if (fbufp) {
			kfree(fbufp);
			fbufp = NULL;
		}
		return -1;
	}

	set_fs(org_fs);
	if (fp) {
		filp_close(fp, NULL);
		fp = NULL;
	}
	if (fbufp) {
		kfree(fbufp);
		fbufp = NULL;
	}

	dev_info(&ts->client->dev, "%s:--\n", __func__);
	return 0;
}

/*******************************************************
Description:
	Novatek touchscreen calculate square root function.

return:
	Executive outcomes. square root of input
*******************************************************/
static uint32_t nvt_sqrt(uint32_t sqsum)
{
	uint32_t sq_rt = 0;

	int32_t g0 = 0;
	int32_t g1 = 0;
	int32_t g2 = 0;
	int32_t g3 = 0;
	int32_t g4 = 0;
	int32_t seed = 0;
	int32_t next = 0;
	int32_t step = 0;

	g4 =  sqsum / 100000000;
	g3 = (sqsum - g4 * 100000000) / 1000000;
	g2 = (sqsum - g4 * 100000000 - g3 * 1000000) / 10000;
	g1 = (sqsum - g4 * 100000000 - g3 * 1000000 - g2 * 10000) / 100;
	g0 = (sqsum - g4 * 100000000 - g3 * 1000000 - g2 * 10000 - g1 * 100);

	next = g4;
	step = 0;
	seed = 0;
	while (((seed + 1) * (step + 1)) <= next) {
		step++;
		seed++;
	}

	sq_rt = seed * 10000;
	next = (next - (seed * step)) * 100 + g3;

	step = 0;
	seed = 2 * seed * 10;
	while (((seed + 1) * (step + 1)) <= next) {
		step++;
		seed++;
	}

	sq_rt = sq_rt + step * 1000;
	next = (next - seed * step) * 100 + g2;
	seed = (seed + step) * 10;
	step = 0;
	while (((seed + 1) * (step + 1)) <= next) {
		step++;
		seed++;
	}

	sq_rt = sq_rt + step * 100;
	next = (next - seed * step) * 100 + g1;
	seed = (seed + step) * 10;
	step = 0;

	while (((seed + 1) * (step + 1)) <= next) {
		step++;
		seed++;
	}

	sq_rt = sq_rt + step * 10;
	next = (next - seed * step) * 100 + g0;
	seed = (seed + step) * 10;
	step = 0;

	while (((seed + 1) * (step + 1)) <= next) {
		step++;
		seed++;
	}

	sq_rt = sq_rt + step;

	return sq_rt;
}

/*******************************************************
Description:
	Novatek touchscreen read open test raw data function.

return:
	Executive outcomes. 0---succeed. negative---failed.
*******************************************************/
static int32_t nvt_read_open(void)
{
	int32_t i = 0;
	int32_t j = 0;
	int32_t k = 0;
	int32_t iArrayIndex = 0;
	uint8_t buf[128]={0};
	struct file *fp = NULL;
	char *fbufp = NULL;
	mm_segment_t org_fs;
	char file_path[64]="/data/misc/touch/OpenTest.csv";	//TP-FixRunInFail-00*_20161201
	loff_t pos = 0;
	int32_t write_ret = 0;
	uint32_t output_len = 0;
	char ctrlram_open_mutual_file[64]="/system/etc/CtrlRAM_Open_Mutual.csv";
	struct test_cmd *ctrlram_open_mutual_cmds = NULL;
	uint32_t ctrlram_open_mutual_cmds_num = 0;
	struct test_cmd cmd_WTG_Disable = {.addr=0x1F028, .len=2, .data={0x07, 0x55}};
	uint8_t *rawdata_buf1 = NULL;
	uint8_t *rawdata_buf2 = NULL;
	int16_t sh1 = 0;
	int16_t sh2 = 0;
#if TOUCH_KEY_NUM > 0
	int32_t TX_idx = -1;
	int32_t RX_idx = -1;
	uint32_t keydata_output_offset = 0;
#endif /* #if TOUCH_KEY_NUM > 0 */

	dev_info(&ts->client->dev, "%s:++\n", __func__);
	// Load CtrlRAM
	ctrlram_open_mutual_cmds = (struct test_cmd *)kzalloc( 512 * sizeof(struct test_cmd), GFP_KERNEL);
	if (!ctrlram_open_mutual_cmds) {
		dev_err(&ts->client->dev, "%s: kzalloc for ctrlram_open_mutual_cmds failed.\n", __func__);
		return -ENOMEM;
	}
	if (nvt_load_mp_ctrl_ram(ctrlram_open_mutual_file, ctrlram_open_mutual_cmds, &ctrlram_open_mutual_cmds_num) < 0) {
		dev_err(&ts->client->dev, "%s: load control ram %s failed!\n", __func__, ctrlram_open_mutual_file);
		if (ctrlram_open_mutual_cmds) {
			kfree(ctrlram_open_mutual_cmds);
			ctrlram_open_mutual_cmds = NULL;
		}
		return -EAGAIN;
	}
	ctrlram_open_mutual_cmds[1].data[2] = 0x18;

	//---Reset IC & into idle---
	nvt_sw_reset_idle();

	// WTG Disable
	nvt_write_test_cmd(&cmd_WTG_Disable, 1);
	// CtrlRAM test commands
	nvt_write_test_cmd(ctrlram_open_mutual_cmds, ctrlram_open_mutual_cmds_num);
	// Other test commands
	nvt_write_test_cmd(open_test, open_test_cmd_num);

	rawdata_buf1 = (uint8_t *) kzalloc(IC_TX_CFG_SIZE * IC_RX_CFG_SIZE * 2, GFP_KERNEL);
	if (!rawdata_buf1) {
		dev_err(&ts->client->dev, "%s: kzalloc for rawdata_buf1 failed\n", __func__);
		if (ctrlram_open_mutual_cmds) {
			kfree(ctrlram_open_mutual_cmds);
			ctrlram_open_mutual_cmds = NULL;
		}
		return -ENOMEM;
	}
	rawdata_buf2 = (uint8_t *) kzalloc(IC_TX_CFG_SIZE * IC_RX_CFG_SIZE * 2, GFP_KERNEL);
	if (!rawdata_buf2) {
		dev_err(&ts->client->dev, "%s: kzalloc for rawdata_buf2 failed\n", __func__);
		if (ctrlram_open_mutual_cmds) {
			kfree(ctrlram_open_mutual_cmds);
			ctrlram_open_mutual_cmds = NULL;
		}
		if (rawdata_buf1) {
			kfree(rawdata_buf1);
			rawdata_buf1 = NULL;
		}
		return -ENOMEM;
	}

	if (Mutual_isDummyCycle) {
		for (k = 0; k < Mutual_Dummy_Count; k++) {
			if (nvt_set_adc_oper(Mutual_isReadADCCheck) != 0) {
				if (ctrlram_open_mutual_cmds) {
					kfree(ctrlram_open_mutual_cmds);
					ctrlram_open_mutual_cmds = NULL;
				}
				if (rawdata_buf1) {
					kfree(rawdata_buf1);
					rawdata_buf1 = NULL;
				}
				if (rawdata_buf2) {
					kfree(rawdata_buf2);
					rawdata_buf2 = NULL;
				}
				return -EAGAIN;
			}
		}
	}

	for (k = 0; k < Mutual_TestTimes; k++) {
		if (nvt_set_adc_oper(Mutual_isReadADCCheck) != 0) {
			if (ctrlram_open_mutual_cmds) {
				kfree(ctrlram_open_mutual_cmds);
				ctrlram_open_mutual_cmds = NULL;
			}
			if (rawdata_buf1) {
				kfree(rawdata_buf1);
				rawdata_buf1 = NULL;
			}
			if (rawdata_buf2) {
				kfree(rawdata_buf2);
				rawdata_buf2 = NULL;
			}
			return -EAGAIN;
		}

		for (i = 0; i < IC_TX_CFG_SIZE; i++) {
			//---change xdata index---
			buf[0] = 0xFF;
			buf[1] = 0x01;
			buf[2] = 0x00 + (uint8_t)(((i * IC_RX_CFG_SIZE * 2) & 0xFF00) >> 8);
			CTP_I2C_WRITE(ts->client, I2C_FW_Address, buf, 3);
			//---read data---
			buf[0] = (uint8_t)((i * IC_RX_CFG_SIZE * 2) & 0xFF);
			CTP_I2C_READ(ts->client, I2C_FW_Address, buf, IC_RX_CFG_SIZE * 2 + 1);
			memcpy(rawdata_buf1 + i * IC_RX_CFG_SIZE * 2, buf + 1, IC_RX_CFG_SIZE * 2);

			//---change xdata index---
			buf[0] = 0xFF;
			buf[1] = 0x01;
			buf[2] = 0x30 + (uint8_t)(((i * IC_RX_CFG_SIZE * 2) & 0xFF00) >> 8);
			CTP_I2C_WRITE(ts->client, I2C_FW_Address, buf, 3);
			//---read data---
			buf[0] = (uint8_t)((i * IC_RX_CFG_SIZE * 2) & 0xFF);
			CTP_I2C_READ(ts->client, I2C_FW_Address, buf, IC_RX_CFG_SIZE * 2 + 1);
			memcpy(rawdata_buf2 + i * IC_RX_CFG_SIZE * 2, buf + 1, IC_RX_CFG_SIZE * 2);
		}
		//printk("rawdata_open_raw1: k=%d, Mutual_Dummy_Frames=%d\n", k, Mutual_Dummy_Frames);
		//printk("rawdata_open_raw2: k=%d, Mutual_Dummy_Frames=%d\n", k, Mutual_Dummy_Frames);
		for (i = 0; i < AIN_TX_NUM; i++) {
			for (j = 0; j < AIN_RX_NUM; j++) {
				iArrayIndex = i *  AIN_RX_NUM + j;
				sh1 = (int16_t)(rawdata_buf1[(AIN_TX_Order[i] * IC_RX_CFG_SIZE + AIN_RX_Order[j]) * 2] + 256 * rawdata_buf1[(AIN_TX_Order[i] * IC_RX_CFG_SIZE + AIN_RX_Order[j]) * 2 + 1]);
				if (k == Mutual_Dummy_Frames) {
					rawdata_open_raw1[iArrayIndex] = sh1;
				} else if (k > Mutual_Dummy_Frames) {
					rawdata_open_raw1[iArrayIndex] = rawdata_open_raw1[iArrayIndex] + sh1;
					rawdata_open_raw1[iArrayIndex] = rawdata_open_raw1[iArrayIndex] / 2;
				}
				//if (k >= Mutual_Dummy_Frames)
				//	printk("%5d, ", rawdata_open_raw1[iArrayIndex]);
				sh2 = (int16_t)(rawdata_buf2[(AIN_TX_Order[i] * IC_RX_CFG_SIZE + AIN_RX_Order[j]) * 2] + 256 * rawdata_buf2[(AIN_TX_Order[i] * IC_RX_CFG_SIZE + AIN_RX_Order[j]) * 2 + 1]);
				if (k == Mutual_Dummy_Frames) {
					rawdata_open_raw2[iArrayIndex] = sh2;
				} else if (k > Mutual_Dummy_Frames) {
					rawdata_open_raw2[iArrayIndex] = rawdata_open_raw2[iArrayIndex] + sh2;
					rawdata_open_raw2[iArrayIndex] = rawdata_open_raw2[iArrayIndex] / 2;
				}
				//if (k >= Mutual_Dummy_Frames)
				//	printk("%5d, ", rawdata_open_raw2[iArrayIndex]);
			}
			//if (k >= Mutual_Dummy_Frames)
			//	printk("\n");
		}
		//if (k >= Mutual_Dummy_Frames)
		//	printk("\n");
#if TOUCH_KEY_NUM > 0
		TX_idx = -1;
		RX_idx = -1;
		for (i = 0; i < IC_TX_CFG_SIZE; i++) {
			if (AIN_KEY_TX[i] == 0xFF)
				continue;
			else
				TX_idx++;

			for (j = 0; j < IC_RX_CFG_SIZE; j++) {
				if (AIN_KEY_RX[j] == 0xFF)
					continue;
				else
					RX_idx++;

				sh1 = (int16_t)(rawdata_buf1[((AIN_KEY_TX_Order[AIN_KEY_TX[i]] * IC_RX_CFG_SIZE) + AIN_KEY_RX_Order[AIN_KEY_RX[j]]) * 2] + 256 * rawdata_buf1[((AIN_KEY_TX_Order[AIN_KEY_TX[i]] * IC_RX_CFG_SIZE) + AIN_KEY_RX_Order[AIN_KEY_RX[j]]) * 2 + 1]);
				sh2 = (int16_t)(rawdata_buf2[((AIN_KEY_TX_Order[AIN_KEY_TX[i]] * IC_RX_CFG_SIZE) + AIN_KEY_RX_Order[AIN_KEY_RX[j]]) * 2] + 256 * rawdata_buf2[((AIN_KEY_TX_Order[AIN_KEY_TX[i]] * IC_RX_CFG_SIZE) + AIN_KEY_RX_Order[AIN_KEY_RX[j]]) * 2 + 1]);
				iArrayIndex = Key_X_Channel_Real * TX_idx + RX_idx;
				if (k == Mutual_Dummy_Frames) {
					Mutual_keydata1[iArrayIndex] = sh1;
					Mutual_keydata2[iArrayIndex] = sh2;
				} else if (k > Mutual_Dummy_Frames) {
					Mutual_keydata1[iArrayIndex] += sh1;
					Mutual_keydata1[iArrayIndex] /= 2;
					Mutual_keydata2[iArrayIndex] += sh2;
					Mutual_keydata2[iArrayIndex] /= 2;
				}
			}
		}
#endif /* #if TOUCH_KEY_NUM > 0 */
	} // Mutual_TestTimes

	//--IQ---
	for (i = 0; i < AIN_TX_NUM; i++) {
		for (j = 0; j < AIN_RX_NUM; j++) {
			iArrayIndex = i * AIN_RX_NUM + j;
			rawdata_open[iArrayIndex] = nvt_sqrt(rawdata_open_raw1[iArrayIndex] * rawdata_open_raw1[iArrayIndex] + rawdata_open_raw2[iArrayIndex] * rawdata_open_raw2[iArrayIndex]);
		}
	}
#if TOUCH_KEY_NUM > 0
	for (i = 0; i < Key_Y_Channel_Real; i++) {
		for (j = 0; j < Key_X_Channel_Real; j++) {
			iArrayIndex = i * Key_X_Channel_Real + j;
			Mutual_keydata[iArrayIndex] = nvt_sqrt(Mutual_keydata1[iArrayIndex] * Mutual_keydata1[iArrayIndex] + Mutual_keydata2[iArrayIndex] * Mutual_keydata2[iArrayIndex]);
		}
	}
#endif /* #if TOUCH_KEY_NUM > 0 */

	if (rawdata_buf1) {
		kfree(rawdata_buf1);
		rawdata_buf1 = NULL;
	}
	if (rawdata_buf2) {
		kfree(rawdata_buf2);
		rawdata_buf2 = NULL;
	}
	if (ctrlram_open_mutual_cmds) {
		kfree(ctrlram_open_mutual_cmds);
		ctrlram_open_mutual_cmds = NULL;
	}

	fbufp = (char *)kzalloc(8192, GFP_KERNEL);
	if (!fbufp) {
		dev_err(&ts->client->dev, "%s: kzalloc for fbufp failed.\n", __func__);
		return -ENOMEM;
	}

	//---for debug---
	printk("%s:\n", __func__);
	for (i = 0; i < AIN_TX_NUM; i++) {
		for (j = 0; j < AIN_RX_NUM; j++) {
			iArrayIndex = i * AIN_RX_NUM + j;
			printk("%5d, ", rawdata_open[iArrayIndex]);
			sprintf(fbufp + 7 * (iArrayIndex) + i * 2, "%5d, ", rawdata_open[iArrayIndex]) ;
		}
		printk("\n");
		sprintf(fbufp + 7 * (iArrayIndex + 1) + i * 2,"\r\n");
	}
#if TOUCH_KEY_NUM > 0
	keydata_output_offset = AIN_TX_NUM * AIN_RX_NUM * 7 + AIN_TX_NUM * 2;
	for (i = 0; i < Key_Y_Channel_Real; i++) {
		for (j = 0; j < Key_X_Channel_Real; j++) {
			iArrayIndex = i * Key_X_Channel_Real + j;
			printk("%5d, ", Mutual_keydata[iArrayIndex]);
			sprintf(fbufp + keydata_output_offset + 7 * iArrayIndex + i * 2, "%5d, ", Mutual_keydata[iArrayIndex]);
		}
		printk("\n");
		sprintf(fbufp + keydata_output_offset + 7 * (iArrayIndex + 1) + i * 2, "\r\n");
	}
#endif /* #if TOUCH_KEY_NUM > 0 */

	org_fs = get_fs();
	set_fs(KERNEL_DS);
	fp = filp_open(file_path, O_RDWR | O_CREAT, 0644);
	if (fp == NULL || IS_ERR(fp)) {
		dev_err(&ts->client->dev, "%s: open %s failed\n", __func__, file_path);
		set_fs(org_fs);
		if (fbufp) {
			kfree(fbufp);
			fbufp = NULL;
		}
		return -1;
	}

	output_len = AIN_TX_NUM * AIN_RX_NUM * 7 + AIN_TX_NUM * 2;
#if TOUCH_KEY_NUM > 0
	output_len += Key_Y_Channel_Real * Key_X_Channel_Real * 7 + Key_Y_Channel_Real * 2;
#endif /* #if TOUCH_KEY_NUM > 0 */
	write_ret = vfs_write(fp, (char __user *)fbufp, output_len, &pos);
	if (write_ret <= 0) {
		dev_err(&ts->client->dev, "%s: write %s failed\n", __func__, file_path);
		set_fs(org_fs);
		if (fp) {
			filp_close(fp, NULL);
			fp = NULL;
		}
		if (fbufp) {
			kfree(fbufp);
			fbufp = NULL;
		}
		return -1;
	}

	set_fs(org_fs);
	if (fp) {
		filp_close(fp, NULL);
		fp = NULL;
	}
	if (fbufp) {
		kfree(fbufp);
		fbufp = NULL;
	}

	dev_info(&ts->client->dev, "%s:--\n", __func__);
	return 0;
}

/*******************************************************
Description:
	Novatek touchscreen calculate G Ratio and Normal
	function.

return:
	Executive outcomes. 0---succeed. 1---failed.
*******************************************************/
static int32_t Test_CaluateGRatioAndNormal(int32_t boundary[], int32_t rawdata[], uint8_t x_len, uint8_t y_len)
{
	int32_t i = 0;
	int32_t j = 0;
	int32_t k = 0;
	int64_t tmpValue = 0;
	int64_t MaxSum = 0;
	int32_t SumCnt = 0;
	int32_t MaxNum = 0;
	int32_t MaxIndex = 0;
	int32_t Max = -99999999;
	int32_t Min =  99999999;
	int32_t offset = 0;
	int32_t Data = 0; // double
	int32_t StatisticsStep=0;


	//--------------------------------------------------
	//1. (Testing_CM - Golden_CM ) / Testing_CM
	//--------------------------------------------------
	for (j = 0; j < y_len; j++) {
		for (i = 0; i < x_len; i++) {
			Data = rawdata[j * x_len + i];
			if (Data == 0)
				Data = 1;

			golden_Ratio[j * x_len + i] = Data - boundary[j * x_len + i];
			golden_Ratio[j * x_len + i] = ((golden_Ratio[j * x_len + i] * 1000) / Data); // *1000 before division
		}
	}

	//--------------------------------------------------------
	// 2. Mutual_GoldenRatio*1000
	//--------------------------------------------------------
	for (j = 0; j < y_len; j++) {
		for (i = 0; i < x_len; i++) {
			golden_Ratio[j * x_len + i] *= 1000;
		}
	}

	//--------------------------------------------------------
	// 3. Calculate StatisticsStep
	//--------------------------------------------------------
	for (j = 0; j < y_len; j++) {
		for (i = 0; i < x_len; i++) {
			if (Max < golden_Ratio[j * x_len + i])
				Max = (int32_t)golden_Ratio[j * x_len + i];
			if (Min > golden_Ratio[j * x_len + i])
				Min = (int32_t)golden_Ratio[j * x_len + i];
		}
	}

	offset = 0;
	if (Min < 0) { // add offset to get erery element Positive
		offset = 0 - Min;
		for (j = 0; j < y_len; j++) {
			for (i = 0; i < x_len; i++) {
				golden_Ratio[j * x_len + i] += offset;
			}
		}
		Max += offset;
	}
	StatisticsStep = Max / MaxStatisticsBuf;
	StatisticsStep += 1;
	if (StatisticsStep < 0) {
		dev_err(&ts->client->dev, "%s: FAIL! (StatisticsStep < 0)\n", __func__);
		return 1;
	}

	//--------------------------------------------------------
	// 4. Start Statistics and Average
	//--------------------------------------------------------
	memset(StatisticsSum, 0, sizeof(int64_t) * MaxStatisticsBuf);
	memset(StatisticsNum, 0, sizeof(int64_t) * MaxStatisticsBuf);
	for (i = 0; i < MaxStatisticsBuf; i++) {
		StatisticsSum[i] = 0;
		StatisticsNum[i] = 0;
	}
	for (j = 0; j < y_len; j++) {
		for(i = 0; i < x_len; i++) {
			tmpValue = golden_Ratio[j * x_len + i];
			tmpValue /= StatisticsStep;
			StatisticsNum[tmpValue] += 2;
			StatisticsSum[tmpValue] += (2 * golden_Ratio[j * x_len + i]);

			if ((tmpValue + 1) < MaxStatisticsBuf) {
				StatisticsNum[tmpValue + 1] += 1;
				StatisticsSum[tmpValue + 1] += golden_Ratio[j * x_len + i];
			}

			if ((tmpValue - 1) >= 0) {
				StatisticsNum[tmpValue - 1] += 1;
				StatisticsSum[tmpValue - 1] += golden_Ratio[j * x_len + i];
			}
		}
	}
	//Find out Max Statistics
	MaxNum = 0;
	for (k = 0; k < MaxStatisticsBuf; k++) {
		if (MaxNum < StatisticsNum[k]) {
			MaxSum = StatisticsSum[k];
			MaxNum = StatisticsNum[k];
			MaxIndex = k;
		}
	}

	//Caluate Statistics Average
	if (MaxSum > 0 ) {
		if (StatisticsNum[MaxIndex] != 0) {
			tmpValue = (int64_t)(StatisticsSum[MaxIndex] / StatisticsNum[MaxIndex]) * 2;
			SumCnt += 2;
		}

		if ((MaxIndex + 1) < (MaxStatisticsBuf)) {
			if (StatisticsNum[MaxIndex + 1] != 0) {
				tmpValue += (int64_t)(StatisticsSum[MaxIndex + 1] / StatisticsNum[MaxIndex + 1]);
				SumCnt++;
			}
		}

		if ((MaxIndex - 1) >= 0) {
			if (StatisticsNum[MaxIndex - 1] != 0) {
				tmpValue += (int64_t)(StatisticsSum[MaxIndex - 1] / StatisticsNum[MaxIndex - 1]);
				SumCnt++;
			}
		}

		if (SumCnt > 0)
			tmpValue /= SumCnt;
	} else { // Too Separately
		StatisticsSum[0] = 0;
		StatisticsNum[0] = 0;
		for (j = 0; j < y_len; j++) {
			for (i = 0; i < x_len; i++) {
				StatisticsSum[0] += (int64_t)golden_Ratio[j * x_len + i];
				StatisticsNum[0]++;
			}
		}
		tmpValue = StatisticsSum[0] / StatisticsNum[0];
	}
	//----------------------------------------------------------
	//----------------------------------------------------------
	//----------------------------------------------------------
	tmpValue -= offset;
	for (j = 0; j < y_len; j++) {
		for (i = 0; i < x_len; i++) {
			golden_Ratio[j * x_len + i] -= offset;

			golden_Ratio[j * x_len + i] = golden_Ratio[j * x_len + i] - tmpValue;
			golden_Ratio[j * x_len + i] = golden_Ratio[j * x_len + i] / 1000;
		}
	}

	return 0;
}

/*******************************************************
Description:
	Novatek touchscreen raw data test function.

return:
	Executive outcomes. 0---passed. negative---failed.
*******************************************************/
static int32_t RawDataTest_Sub(int32_t boundary[], int32_t rawdata[], uint8_t RecordResult[], uint8_t x_ch, uint8_t y_ch, int32_t Tol_P, int32_t Tol_N, int32_t Dif_P, int32_t Dif_N, int32_t Rawdata_Limit_Postive, int32_t Rawdata_Limit_Negative)
{
	int32_t i = 0;
	int32_t j = 0;
	int32_t iArrayIndex = 0;
	int32_t iBoundary = 0;
	int32_t iTolLowBound = 0;
	int32_t iTolHighBound = 0;
	bool isAbsCriteria = false;
	bool isPass = true;

	if ((Rawdata_Limit_Postive != 0) || (Rawdata_Limit_Negative != 0))
		isAbsCriteria = true;

	for (j = 0; j < y_ch; j++) {
		for (i = 0; i < x_ch; i++) {
			iArrayIndex = j * x_ch + i;
			iBoundary = boundary[iArrayIndex];

			RecordResult[iArrayIndex] = 0x00; // default value for PASS

			if (isAbsCriteria) {
				iTolLowBound = Rawdata_Limit_Negative;
				iTolHighBound = Rawdata_Limit_Postive;
			} else {
				if (iBoundary > 0) {
					iTolLowBound = (iBoundary * (1000 + Tol_N));
					iTolHighBound = (iBoundary * (1000 + Tol_P));
				} else {
					iTolLowBound = (iBoundary * (1000 - Tol_N));
					iTolHighBound = (iBoundary * (1000 - Tol_P));
				}
			}

			if((rawdata[iArrayIndex] * 1000) > iTolHighBound)
				RecordResult[iArrayIndex] |= 0x01;

			if((rawdata[iArrayIndex] * 1000) < iTolLowBound)
				RecordResult[iArrayIndex] |= 0x02;
		}
	}

	if (!isAbsCriteria) {
		Test_CaluateGRatioAndNormal(boundary, rawdata, x_ch, y_ch);

		for (j = 0; j < y_ch; j++) {
			for (i = 0; i < x_ch; i++) {
				iArrayIndex = j * x_ch + i;

				if (golden_Ratio[iArrayIndex] > Dif_P)
					RecordResult[iArrayIndex] |= 0x04;

				if (golden_Ratio[iArrayIndex] < Dif_N)
					RecordResult[iArrayIndex] |= 0x08;
			}
		}
	}

	//---Check RecordResult---
	for (j = 0; j < y_ch; j++) {
		for (i = 0; i < x_ch; i++) {
			if (RecordResult[j * x_ch + i] != 0) {
				isPass = false;
				break;
			}
		}
	}

	if( isPass == false) {
		return -1; // FAIL
	} else {
		return 0; // PASS
	}
}


/*******************************************************
Description:
	Novatek touchscreen print self-test result function.

return:
	n.a.
*******************************************************/
void print_selftest_result(struct seq_file *m, int32_t TestResult, uint8_t RecordResult[], int32_t rawdata[], uint8_t x_len, uint8_t y_len)
{
	int32_t i = 0;
	int32_t j = 0;

	switch (TestResult) {
		case 0:
			seq_printf(m, " PASS!");
			seq_puts(m, "\n");
			break;

		case 1:
			seq_printf(m, " ERROR! Read Data FAIL!");
			seq_puts(m, "\n");
			break;

		case -1:
			seq_printf(m, " FAIL!");
			seq_puts(m, "\n");
			seq_printf(m, "RecordResult:");
			seq_puts(m, "\n");
			for (i = 0; i < y_len; i++) {
				for (j = 0; j < x_len; j++) {
					seq_printf(m, "0x%02X, ", RecordResult[i * x_len + j]);
				}
				seq_puts(m, "\n");
			}
			seq_printf(m, "ReadData:");
			seq_puts(m, "\n");
			for (i = 0; i < y_len; i++) {
				for (j = 0; j < x_len; j++) {
					seq_printf(m, "%5d, ", rawdata[i * x_len + j]);
				}
				seq_puts(m, "\n");
			}
			break;
	}
	seq_printf(m, "\n");
}

/*******************************************************
Description:
	Novatek touchscreen self-test sequence print show
	function.

return:
	Executive outcomes. 0---succeed.
*******************************************************/
int i32_nvt_selftest_Result=-1;
#if NVT_TOUCH_EXT_FIH
	//No compilie c_show_selftest()
#else
static int32_t c_show_selftest(struct seq_file *m, void *v)
{
	seq_printf(m, "Short Test RXRX");
	print_selftest_result(m, TestResult_Short_RXRX, RecordResultShort_RXRX, rawdata_short_rxrx, (uint8_t)(AIN_RX_NUM + GND_Channel_Real), 1);
#if TOUCH_KEY_NUM > 0
	seq_printf(m, "Short Test RXRX Key");
	print_selftest_result(m, TestResult_Short_RXRX_Key, RecordResultShort_RXRX_Key, RXRX_test_keydata, Key_X_Channel_Real, 1);
#endif /* #if TOUCH_KEY_NUM > 0 */

	seq_printf(m, "Short Test TXRX");
	print_selftest_result(m, TestResult_Short_TXRX, RecordResultShort_TXRX, rawdata_short_txrx, (uint8_t)(AIN_RX_NUM + GND_Channel_Real), AIN_TX_NUM);
#if TOUCH_KEY_NUM > 0
	seq_printf(m, "Short Test TXRX Key");
	print_selftest_result(m, TestResult_Short_TXRX_Key, RecordResultShort_TXRX_Key, TXRX_test_keydata, Key_X_Channel_Real, Key_Y_Channel_Real);
#endif /* #if TOUCH_KEY_NUM > 0 */

	seq_printf(m, "Short Test TXTX");
	print_selftest_result(m, TestResult_Short_TXTX, RecordResultShort_TXTX, rawdata_short_txtx, 1, AIN_TX_NUM);
#if TOUCH_KEY_NUM > 0
	seq_printf(m, "Short Test TXTX Key");
	print_selftest_result(m, TestResult_Short_TXTX_Key, RecordResultShort_TXTX_Key, TXTX_test_keydata, 1, Key_Y_Channel_Real);
#endif /* #if TOUCH_KEY_NUM > 0 */

	seq_printf(m, "Open Test");
	print_selftest_result(m, TestResult_Open, RecordResultOpen, rawdata_open, AIN_RX_NUM, AIN_TX_NUM);
#if TOUCH_KEY_NUM > 0
	seq_printf(m, "Open Test Key");
	print_selftest_result(m, TestResult_Open_Key, RecordResultOpen_Key, Mutual_keydata, Key_X_Channel_Real, Key_Y_Channel_Real);
#endif /* #if TOUCH_KEY_NUM > 0 */

    return 0;
}
#endif//#if NVT_TOUCH_EXT_FIH

/*******************************************************
Description:
	Novatek touchscreen self-test sequence print start
	function.

return:
	Executive outcomes. 1---call next function.
	NULL---not call next function and sequence loop
	stop.
*******************************************************/
static void *c_start(struct seq_file *m, loff_t *pos)
{
	return *pos < 1 ? (void *)1 : NULL;
}

/*******************************************************
Description:
	Novatek touchscreen self-test sequence print next
	function.

return:
	Executive outcomes. NULL---no next and call sequence
	stop function.
*******************************************************/
static void *c_next(struct seq_file *m, void *v, loff_t *pos)
{
	++*pos;
	return NULL;
}

/*******************************************************
Description:
	Novatek touchscreen self-test sequence print stop
	function.

return:
	n.a.
*******************************************************/
static void c_stop(struct seq_file *m, void *v)
{
	return;
}

const struct seq_operations nvt_selftest_seq_ops = {
	.start  = c_start,
	.next   = c_next,
	.stop   = c_stop,
	#if NVT_TOUCH_EXT_FIH
	.show	= nvt_extFIH_MP_show
	#else
	.show   = c_show_selftest
	#endif
};



/*******************************************************
Description:


return:
	Executive outcomes. 0-Pass. 1-NG.
*******************************************************/

int nvt_selftest(void)
{
	int32_t ret = 0;
	TestResult_Short_RXRX = 0;
	TestResult_Short_TXRX = 0;
	TestResult_Short_TXTX = 0;
	TestResult_Open = 0;
#if TOUCH_KEY_NUM > 0
	TestResult_Short_RXRX_Key = 0;
	TestResult_Short_TXRX_Key = 0;
	TestResult_Short_TXTX_Key = 0;
	TestResult_Open_Key = 0;
#endif /* #if TOUCH_KEY_NUM > 0 */

	nvt_cal_ain_order();

	// load MP criteria
	if (nvt_load_mp_criteria())
		return -EAGAIN;

	if(mutex_lock_interruptible(&ts->lock)) {
		return -ERESTARTSYS;
	}

	//---Reset IC & into idle---
	nvt_sw_reset_idle();


	//---Short Test RX-RX---
	if (nvt_read_short_rxrx() != 0) {
		TestResult_Short_RXRX = 1; // 1:ERROR
#if TOUCH_KEY_NUM > 0
		TestResult_Short_RXRX_Key = 1; // 1:ERROR
#endif /* #if TOUCH_KEY_NUM > 0 */
	} else {
		//---Self Test Check --- // 0:PASS, -1:FAIL
		TestResult_Short_RXRX = RawDataTest_Sub(BoundaryShort_RXRX, rawdata_short_rxrx, RecordResultShort_RXRX, (uint8_t)(AIN_RX_NUM + GND_Channel_Real), 1,
												PSConfig_Tolerance_Postive_Short, PSConfig_Tolerance_Negative_Short, PSConfig_DiffLimitG_Postive_Short, PSConfig_DiffLimitG_Negative_Short,
												PSConfig_Rawdata_Limit_Postive_Short_RXRX, PSConfig_Rawdata_Limit_Negative_Short_RXRX);
#if TOUCH_KEY_NUM > 0
		TestResult_Short_RXRX_Key = RawDataTest_Sub(BoundaryShort_RXRX_Key, RXRX_test_keydata, RecordResultShort_RXRX_Key, Key_X_Channel_Real, 1,
												PSConfig_Tolerance_Postive_Key_Short, PSConfig_Tolerance_Negative_Key_Short, PSConfig_DiffLimitG_Postive_Key_Short, PSConfig_DiffLimitG_Negative_Key_Short,
												PSConfig_Rawdata_Limit_Postive_Key_Short_RXRX, PSConfig_Rawdata_Limit_Negative_Key_Short_RXRX);
#endif /* #if TOUCH_KEY_NUM > 0 */
	}

	//---Short Test TX-RX---
	if (nvt_read_short_txrx() != 0) {
		TestResult_Short_TXRX = 1; // 1:ERROR
#if TOUCH_KEY_NUM > 0
		TestResult_Short_TXRX_Key = 1; // 1:ERROR
#endif /* #if TOUCH_KEY_NUM > 0 */
	} else {
		//---Self Test Check --- // 0:PASS, -1:FAIL
		TestResult_Short_TXRX = RawDataTest_Sub(BoundaryShort_TXRX, rawdata_short_txrx, RecordResultShort_TXRX, (uint8_t)(AIN_RX_NUM + GND_Channel_Real), AIN_TX_NUM,
												PSConfig_Tolerance_Postive_Short, PSConfig_Tolerance_Negative_Short, PSConfig_DiffLimitG_Postive_Short, PSConfig_DiffLimitG_Negative_Short,
												PSConfig_Rawdata_Limit_Postive_Short_TXRX, PSConfig_Rawdata_Limit_Negative_Short_TXRX);
#if TOUCH_KEY_NUM > 0
		TestResult_Short_TXRX_Key = RawDataTest_Sub(BoundaryShort_TXRX_Key, TXTX_test_keydata, RecordResultShort_TXRX_Key, Key_X_Channel_Real, Key_Y_Channel_Real,
												PSConfig_Tolerance_Postive_Key_Short, PSConfig_Tolerance_Negative_Key_Short, PSConfig_DiffLimitG_Postive_Key_Short, PSConfig_DiffLimitG_Negative_Key_Short,
												PSConfig_Rawdata_Limit_Postive_Key_Short_TXRX, PSConfig_Rawdata_Limit_Negative_Key_Short_TXRX);
#endif /* #if TOUCH_KEY_NUM > 0 */
	}

	//---Short Test TX-TX---
	if (nvt_read_short_txtx() != 0) {
		TestResult_Short_TXTX = 1; // 1:ERROR
#if TOUCH_KEY_NUM > 0
		TestResult_Short_TXTX_Key = 1; // 1:ERROR
#endif /* #if TOUCH_KEY_NUM > 0 */
	} else {
		//---Self Test Check --- // 0:PASS, -1:FAIL
		TestResult_Short_TXTX = RawDataTest_Sub(BoundaryShort_TXTX, rawdata_short_txtx, RecordResultShort_TXTX, 1, AIN_TX_NUM,
												PSConfig_Tolerance_Postive_Short, PSConfig_Tolerance_Negative_Short, PSConfig_DiffLimitG_Postive_Short, PSConfig_DiffLimitG_Negative_Short,
												PSConfig_Rawdata_Limit_Postive_Short_TXTX, PSConfig_Rawdata_Limit_Negative_Short_TXTX);
#if TOUCH_KEY_NUM > 0
		TestResult_Short_TXTX_Key = RawDataTest_Sub(BoundaryShort_TXTX_Key, TXTX_test_keydata, RecordResultShort_TXTX_Key, 1, Key_Y_Channel_Real,
												PSConfig_Tolerance_Postive_Key_Short, PSConfig_Tolerance_Negative_Key_Short, PSConfig_DiffLimitG_Postive_Key_Short, PSConfig_DiffLimitG_Negative_Key_Short,
												PSConfig_Rawdata_Limit_Postive_Key_Short_TXTX, PSConfig_Rawdata_Limit_Negative_Key_Short_TXTX);
#endif /* #if TOUCH_KEY_NUM > 0 */
	}


	//---Reset IC & into idle---
	nvt_sw_reset_idle();


	//---Open Test---
	if (nvt_read_open() != 0) {
		TestResult_Open = 1; // 1:ERROR
#if TOUCH_KEY_NUM > 0
		TestResult_Open_Key = 1; // 1:ERROR
#endif /* #if TOUCH_KEY_NUM > 0 */
	} else {
		//---Self Test Check --- // 0:PASS, -1:FAIL
		TestResult_Open = RawDataTest_Sub(BoundaryOpen, rawdata_open, RecordResultOpen, AIN_RX_NUM, AIN_TX_NUM,
											PSConfig_Tolerance_Postive_Mutual, PSConfig_Tolerance_Negative_Mutual, PSConfig_DiffLimitG_Postive_Mutual, PSConfig_DiffLimitG_Negative_Mutual,
											0, 0);
#if TOUCH_KEY_NUM > 0
		TestResult_Open_Key = RawDataTest_Sub(BoundaryOpen_Key, Mutual_keydata, RecordResultOpen_Key, Key_X_Channel_Real, Key_Y_Channel_Real,
											PSConfig_Tolerance_Postive_Key_Open, PSConfig_Tolerance_Negative_Key_Open, PSConfig_DiffLimitG_Postive_Key_Open, PSConfig_DiffLimitG_Negative_Key_Open,
											0, 0);
#endif /* #if TOUCH_KEY_NUM > 0 */
	}


	//---Reset IC---
	nvt_hw_reset();
	nvt_bootloader_reset();

	mutex_unlock(&ts->lock);


	if(   (TestResult_Short_RXRX==0)&&(TestResult_Short_TXRX==0)
		&&(TestResult_Short_TXTX==0)&&(TestResult_Open==0)
#if TOUCH_KEY_NUM > 0
		&&(TestResult_Short_RXRX_Key==0)&&(TestResult_Short_TXRX_Key==0)
		&&(TestResult_Short_TXTX_Key==0)&&(TestResult_Open_Key==0)
#endif /* #if TOUCH_KEY_NUM > 0 */
	){
		ret = 0;
	}
	else{
		ret = 1;
	}
	return ret;

}

/*******************************************************
Description:
	Novatek touchscreen /proc/nvt_selftest open function.

return:
	Executive outcomes. 0---succeed. negative---failed.
*******************************************************/


static int32_t nvt_selftest_open(struct inode *inode, struct file *file)
{
#if NVT_TOUCH_EXT_FIH
	i32_nvt_selftest_Result=	nvt_selftest();
	return seq_open(file, &nvt_selftest_seq_ops);
#else
	nvt_selftest();
	return seq_open(file, &nvt_selftest_seq_ops);
#endif//#if NVT_TOUCH_EXT_FIH

}


static const struct file_operations nvt_selftest_fops = {
	.owner = THIS_MODULE,
	.open = nvt_selftest_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

/*******************************************************
Description:
	Novatek touchscreen MP function proc. file node
	initial function.

return:
	Executive outcomes. 0---succeed. -1---failed.
*******************************************************/
int32_t nvt_mp_proc_init(void)
{
	NVT_proc_selftest_entry = proc_create("nvt_selftest", 0444, NULL, &nvt_selftest_fops);
	if (NVT_proc_selftest_entry == NULL) {
		dev_err(&ts->client->dev, "%s: create /proc/nvt_selftest Failed!\n", __func__);
		return -1;
	} else {
		dev_info(&ts->client->dev, "%s: create /proc/nvt_selftest Succeeded!\n", __func__);
		return 0;
	}
}
#endif /* #if NVT_TOUCH_MP */










/******************************************************************************/
#if NVT_TOUCH_EXT_FIH
#define EXT_FIH_MP 			"NVT_extFIH_MP"
#define EXT_FIH_FWUPDATE	"NVT_extFIH_fwUpdate"

static struct proc_dir_entry *NVT_extFIH_MP_entry;
static struct proc_dir_entry *NVT_extFIH_fwUpdate_entry;

static int nvt_extFIH_MP_show(struct seq_file *m, void *v) {
	int32_t i = 0;
	int32_t j = 0;
	uint8_t x_len = AIN_RX_NUM;
	uint8_t y_len = AIN_TX_NUM;
#if TOUCH_KEY_NUM > 0
	uint32_t u32NodeNumbers = AIN_RX_NUM * AIN_TX_NUM + Key_X_Channel_Real * Key_Y_Channel_Real;
	int maxSize=(11+5*5)*(AIN_RX_NUM * AIN_TX_NUM + Key_X_Channel_Real * Key_Y_Channel_Real);
#else
	uint32_t u32NodeNumbers = AIN_RX_NUM * AIN_TX_NUM ;
	int maxSize=(11+5*5)*(AIN_RX_NUM * AIN_TX_NUM );
#endif
	//write to file
	loff_t pos = 0;
	int32_t write_ret = 0;
	struct file *fp = NULL;
	char *fbufp = NULL;
	mm_segment_t org_fs;
  	char file_path[64]="/data/misc/touch/D1_sensortest.txt";	//TP-FixRunInFail-00*_20161201
	uint32_t output_len = 15;

	int i32Ret=0;
//-----------

//FIH_D1, Pass show 0; NG show -1.
	if(i32_nvt_selftest_Result==0){	//Success
		seq_printf(m, "%d\n",0);
	}else{	//Fail
		seq_printf(m, "%d\n",-1);
	}
//write to file

		//-----
		fbufp = (char *)kzalloc(maxSize, GFP_KERNEL);
		if (!fbufp) {
			dev_err(&ts->client->dev, "%s: kzalloc for fbufp failed.\n", __func__);
			return -ENOMEM;
		}else{
		    memset(fbufp, 0xff, maxSize);
		}

		i32Ret=0;

//-----------
	seq_printf(m, "DATA=%d;", u32NodeNumbers );
	i32Ret += sprintf(fbufp + i32Ret,"DATA=%d;", u32NodeNumbers );

	for (i = 0; i < y_len; i++) {
		for (j = 0; j < x_len; j++) {
			int32_t iBoundary = BoundaryOpen[i * x_len + j];
			int32_t iTolLowBound  = (iBoundary * (1000 + PSConfig_Tolerance_Negative_Mutual));
			int32_t iTolHighBound = (iBoundary * (1000 + PSConfig_Tolerance_Postive_Mutual));
			seq_printf(m, "(C,%d,%d,%d,%d,%d"
				,i ,j , iTolHighBound/1000, iTolLowBound/1000, rawdata_open[i * x_len + j]);
			i32Ret += sprintf(fbufp + i32Ret,"(C,%d,%d,%d,%d,%d"
				,i ,j , iTolHighBound/1000, iTolLowBound/1000, rawdata_open[i * x_len + j]);
			if(RecordResultOpen[i * x_len + j]==0x00){
				seq_printf(m, ",P);");
				i32Ret += sprintf(fbufp + i32Ret,",P);");
			}else{
				seq_printf(m, ",F);");
				i32Ret += sprintf(fbufp + i32Ret,",F);");
			}
		}
	}

	#if TOUCH_KEY_NUM > 0
	x_len = Key_X_Channel_Real;
	y_len = Key_Y_Channel_Real;

	for (i = 0; i < y_len; i++) {
		for (j = 0; j < x_len; j++) {
			int32_t iBoundary = BoundaryOpen_Key[i * x_len + j];
			int32_t iTolLowBound  = (iBoundary * (1000 + PSConfig_Tolerance_Negative_Key_Open));
			int32_t iTolHighBound = (iBoundary * (1000 + PSConfig_Tolerance_Postive_Key_Open));
			seq_printf(m, "(K,%d,%d,%d,%d,%d"
				,i ,j , iTolHighBound/1000, iTolLowBound/1000, Mutual_keydata[i * x_len + j]);
			i32Ret += sprintf(fbufp + i32Ret,"(K,%d,%d,%d,%d,%d"
				,i ,j , iTolHighBound/1000, iTolLowBound/1000, Mutual_keydata[i * x_len + j]);

			if(RecordResultOpen_Key[i * x_len + j]==0x00){
				seq_printf(m, ",P);");
				i32Ret += sprintf(fbufp + i32Ret,",P);");
			}else{
				seq_printf(m, ",F);");
				i32Ret += sprintf(fbufp + i32Ret,",F);");
			}
		}
	}

	#endif /* #if TOUCH_KEY_NUM > 0 */

	//------
	org_fs = get_fs();
	set_fs(KERNEL_DS);
	fp = filp_open(file_path, O_RDWR | O_CREAT, 0644);
	if (fp == NULL || IS_ERR(fp)) {
		dev_err(&ts->client->dev, "%s: open %s failed\n", __func__, file_path);
		set_fs(org_fs);
		if (fbufp) {
			kfree(fbufp);
			fbufp = NULL;
		}
		return -1;
	}
	//------
	output_len = (i32Ret>maxSize)?maxSize:i32Ret;
    write_ret = vfs_write(fp, (char __user *)fbufp, output_len, &pos);
  //write_ret = vfs_write(fp, (char __user *)fbufp, maxSize, &pos);
	if (write_ret <= 0) {
		dev_err(&ts->client->dev, "%s: write %s failed\n", __func__, file_path);
		set_fs(org_fs);
		if (fp) {
			filp_close(fp, NULL);
			fp = NULL;
		}
		if (fbufp) {
			kfree(fbufp);
			fbufp = NULL;
		}
		return -1;
	}

	set_fs(org_fs);
	if (fp) {
		filp_close(fp, NULL);
		fp = NULL;
	}
	if (fbufp) {
		kfree(fbufp);
		fbufp = NULL;
	}



  return 0;
}

const struct seq_operations nvt_extFIH_seq_ops = {
	.start  = c_start,
	.next   = c_next,
	.stop   = c_stop,
	.show   = nvt_extFIH_MP_show
};

static int nvt_extFIH_MP_open(struct inode *inode, struct	file *file) {
  	return seq_open(file, &nvt_extFIH_seq_ops);
}

static const struct file_operations extFIH_MP_fops = {
	.owner = THIS_MODULE,
	.open = nvt_extFIH_MP_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};
//-------------------
#if (BOOT_UPDATE_FIRMWARE||NVT_TOUCH_EXT_FIH)
	extern int32_t update_firmware_request(char *filename);
	extern void nvt_sw_reset_idle(void);
	extern int32_t Check_CheckSum(void);
	extern int32_t Update_Firmware(void);
	extern void update_firmware_release(void);
#endif


static int nvt_extFIH_fwUpdate_singleOps(struct seq_file *m, void *v) {


	struct i2c_client *client = ts->client;
	int32_t ret = 0;

	char firmware_name[256] = "";
	sprintf(firmware_name, BOOT_UPDATE_FIRMWARE_NAME);

	// request bin file in "/etc/firmware"
	ret = update_firmware_request(firmware_name);
	if (ret) {
		dev_err(&client->dev, "%s: update_firmware_request failed. (%d)\n", __func__, ret);
		BBOX_TP_FIRMWARE_UPDATE_FAIL	//TP-BBox-00+_20161221
		seq_printf(m, "%d\n",1);
		return 0;//return;
	}

	mutex_lock(&ts->lock);

	nvt_sw_reset_idle();

	ret = Update_Firmware();
	if (ret != 0)
	{
		dev_err(&client->dev, "%s: Update_Firmware failed. (%d)\n", __func__, ret);
		BBOX_TP_FIRMWARE_UPDATE_FAIL	//TP-BBox-00+_20161221
		seq_printf(m, "%d\n",-1);
		mutex_unlock(&ts->lock);
		update_firmware_release();
		return 0;
	}


	mutex_unlock(&ts->lock);

	update_firmware_release();

	seq_printf(m, "%d\n",0);

	return 0;
}

static int nvt_extFIH_fwUpdate_open(struct inode *inode, struct	file *file) {
  //return seq_open(file, &nvt_extFIH_seq_ops);
    return single_open(file, &nvt_extFIH_fwUpdate_singleOps, NULL);
}

static const struct file_operations extFIH_fwUpdate_fops = {
	.owner = THIS_MODULE,
	.open = nvt_extFIH_fwUpdate_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

/*******************************************************
Description:
	Novatek touchscreen for FIH MP function proc. file node
	initial function.

return:
	Executive outcomes. 0---succeed. -1---failed.
*******************************************************/
int32_t nvt_extra_FIH_init(void)
{
	NVT_extFIH_MP_entry = proc_create(EXT_FIH_MP, 0444, NULL,&extFIH_MP_fops);
	if (NVT_extFIH_MP_entry == NULL) {
		dev_info(&ts->client->dev,"%s: create proc/%s Failed! \n", __func__,EXT_FIH_MP);
		return -ENOMEM;
	} else {
		dev_info(&ts->client->dev,"%s: create proc/%s Succeeded! \n", __func__,EXT_FIH_MP);
	}

	NVT_extFIH_fwUpdate_entry= proc_create(EXT_FIH_FWUPDATE, 0444, NULL,&extFIH_fwUpdate_fops);
	if (NVT_extFIH_fwUpdate_entry == NULL) {
		dev_err(&ts->client->dev,"%s: create proc/%s Failed!\n", __func__,EXT_FIH_FWUPDATE);
		return -ENOMEM;
	} else {
		dev_info(&ts->client->dev,"%s: create proc/%s Succeeded!\n", __func__,EXT_FIH_FWUPDATE);
	}
	return 0;
}


//TP-FTM-00+{_20161110
int touch_selftest_result_show_nvt(void) {
	int32_t i = 0;
	int32_t j = 0;
	uint8_t x_len = AIN_RX_NUM;
	uint8_t y_len = AIN_TX_NUM;
#if TOUCH_KEY_NUM > 0
	uint32_t u32NodeNumbers = AIN_RX_NUM * AIN_TX_NUM + Key_X_Channel_Real * Key_Y_Channel_Real;
	int maxSize=(11+5*5)*(AIN_RX_NUM * AIN_TX_NUM + Key_X_Channel_Real * Key_Y_Channel_Real);
#else
	uint32_t u32NodeNumbers = AIN_RX_NUM * AIN_TX_NUM ;
	int maxSize=(11+5*5)*(AIN_RX_NUM * AIN_TX_NUM );
#endif
	//write to file
	loff_t pos = 0;
	int32_t write_ret = 0;
	struct file *fp = NULL;
	char *fbufp = NULL;
	mm_segment_t org_fs;
  	char file_path[64]="/data/misc/touch/D1_sensortest.txt";	//TP-FixRunInFail-00*_20161201
	uint32_t output_len = 15;

	int i32Ret=0;
//-----------

#if 0	//No Need to Print Result Here in FIH
//FIH_D1, Pass show 0; NG show -1.
	if(i32_nvt_selftest_Result==0){	//Success
		seq_printf(m, "%d\n",0);
	}else{	//Fail
		seq_printf(m, "%d\n",-1);
	}
//write to file
#endif

		//-----
		fbufp = (char *)kzalloc(maxSize, GFP_KERNEL);
		if (!fbufp) {
			dev_err(&ts->client->dev, "%s: kzalloc for fbufp failed.\n", __func__);
			return -ENOMEM;
		}else{
		    memset(fbufp, 0xff, maxSize);
		}

		i32Ret=0;

//-----------
	//seq_printf(m, "DATA=%d;", u32NodeNumbers );
	i32Ret += sprintf(fbufp + i32Ret,"DATA=%d;", u32NodeNumbers );

	for (i = 0; i < y_len; i++) {
		for (j = 0; j < x_len; j++) {
			int32_t iBoundary = BoundaryOpen[i * x_len + j];
			int32_t iTolLowBound  = (iBoundary * (1000 + PSConfig_Tolerance_Negative_Mutual));
			int32_t iTolHighBound = (iBoundary * (1000 + PSConfig_Tolerance_Postive_Mutual));
			//seq_printf(m, "(C,%d,%d,%d,%d,%d"
			//	,i ,j , iTolHighBound/1000, iTolLowBound/1000, rawdata_open[i * x_len + j]);
			i32Ret += sprintf(fbufp + i32Ret,"(C,%d,%d,%d,%d,%d"
				,i ,j , iTolHighBound/1000, iTolLowBound/1000, rawdata_open[i * x_len + j]);
			if(RecordResultOpen[i * x_len + j]==0x00){
				//seq_printf(m, ",P);");
				i32Ret += sprintf(fbufp + i32Ret,",P);");
			}else{
				//seq_printf(m, ",F);");
				i32Ret += sprintf(fbufp + i32Ret,",F);");
			}
		}
	}

	#if TOUCH_KEY_NUM > 0
	x_len = Key_X_Channel_Real;
	y_len = Key_Y_Channel_Real;

	for (i = 0; i < y_len; i++) {
		for (j = 0; j < x_len; j++) {
			int32_t iBoundary = BoundaryOpen_Key[i * x_len + j];
			int32_t iTolLowBound  = (iBoundary * (1000 + PSConfig_Tolerance_Negative_Key_Open));
			int32_t iTolHighBound = (iBoundary * (1000 + PSConfig_Tolerance_Postive_Key_Open));
			//seq_printf(m, "(K,%d,%d,%d,%d,%d"
			//	,i ,j , iTolHighBound/1000, iTolLowBound/1000, Mutual_keydata[i * x_len + j]);
			i32Ret += sprintf(fbufp + i32Ret,"(K,%d,%d,%d,%d,%d"
				,i ,j , iTolHighBound/1000, iTolLowBound/1000, Mutual_keydata[i * x_len + j]);

			if(RecordResultOpen_Key[i * x_len + j]==0x00){
				//seq_printf(m, ",P);");
				i32Ret += sprintf(fbufp + i32Ret,",P);");
			}else{
				//seq_printf(m, ",F);");
				i32Ret += sprintf(fbufp + i32Ret,",F);");
			}
		}
	}

	#endif /* #if TOUCH_KEY_NUM > 0 */

	//------
	org_fs = get_fs();
	set_fs(KERNEL_DS);
	fp = filp_open(file_path, O_RDWR | O_CREAT, 0644);
	if (fp == NULL || IS_ERR(fp)) {
		dev_err(&ts->client->dev, "%s: open %s failed\n", __func__, file_path);
		set_fs(org_fs);
		if (fbufp) {
			kfree(fbufp);
			fbufp = NULL;
		}
		return -1;
	}
	//------
	output_len = (i32Ret>maxSize)?maxSize:i32Ret;
    write_ret = vfs_write(fp, (char __user *)fbufp, output_len, &pos);
  //write_ret = vfs_write(fp, (char __user *)fbufp, maxSize, &pos);
	if (write_ret <= 0) {
		dev_err(&ts->client->dev, "%s: write %s failed\n", __func__, file_path);
		set_fs(org_fs);
		if (fp) {
			filp_close(fp, NULL);
			fp = NULL;
		}
		if (fbufp) {
			kfree(fbufp);
			fbufp = NULL;
		}
		return -1;
	}

	set_fs(org_fs);
	if (fp) {
		filp_close(fp, NULL);
		fp = NULL;
	}
	if (fbufp) {
		kfree(fbufp);
		fbufp = NULL;
	}



  return 0;
}


int touch_selftest_result_read_nvt(void)
{
	i32_nvt_selftest_Result = nvt_selftest();
	touch_selftest_result_show_nvt();

	if(i32_nvt_selftest_Result==0){	//Success
		return 0;
	}else{	//Fail
		return -1;
	}
}

void touch_fwupgrade_read_nvt(char *buf)
{
	struct i2c_client *client = ts->client;
	int32_t ret = 0;

	char firmware_name[256] = "";
	sprintf(firmware_name, BOOT_UPDATE_FIRMWARE_NAME);

	// request bin file in "/etc/firmware"
	ret = update_firmware_request(firmware_name);
	if (ret) {
		dev_err(&client->dev, "%s: update_firmware_request failed. (%d)\n", __func__, ret);
		BBOX_TP_FIRMWARE_UPDATE_FAIL	//TP-BBox-00+_20161221
		sprintf(buf, "%d\n", 1);
		return;
	}

	mutex_lock(&ts->lock);

	nvt_sw_reset_idle();

	ret = Update_Firmware();
	if (ret != 0)
	{
		dev_err(&client->dev, "%s: Update_Firmware failed. (%d)\n", __func__, ret);
		BBOX_TP_FIRMWARE_UPDATE_FAIL	//TP-BBox-00+_20161221
		sprintf(buf, "%d\n", -1);
		mutex_unlock(&ts->lock);
		update_firmware_release();
		return;
	}

	//TP-FixFirmwareUpgradeFai-00-{_20161129
	#if 0
	ret = Check_CheckSum();
	if (ret != 1)
	{
		dev_err(&client->dev, "%s: Check_CheckSum failed. (%d)\n", __func__, ret);
		sprintf(buf, "%d\n", -1);
		mutex_unlock(&ts->lock);
		update_firmware_release();
		return;
	}
	#endif
	//TP-FixFirmwareUpgradeFai-00-}_20161129

	mutex_unlock(&ts->lock);

	update_firmware_release();

	sprintf(buf, "%d\n", 0);
}
//TP-FTM-00+}_20161110

#endif /* #if NVT_TOUCH_EXT_FIH */

