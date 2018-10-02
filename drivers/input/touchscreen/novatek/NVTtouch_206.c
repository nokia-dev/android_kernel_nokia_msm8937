/* drivers/input/touchscreen/nt11206/NVTtouch_206.c
 *
 * Copyright (C) 2010 - 2016 Novatek, Inc.
 *
 * $Revision: 6200 $
 * $Date: 2016-09-02 15:52:16 +0800 (?±‰?, 02 ‰πùÊ? 2016) $
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
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/proc_fs.h>
#include <linux/unistd.h>

#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <asm/uaccess.h>
#include <linux/slab.h>

#include <linux/input/mt.h>
#include <linux/wakelock.h>

#include <linux/of_gpio.h>
#include <linux/of_irq.h>

#if defined(CONFIG_FB)
#include <linux/notifier.h>
#include <linux/fb.h>
#elif defined(CONFIG_HAS_EARLYSUSPEND)
#include <linux/earlysuspend.h>
#endif

#include "NVTtouch_206.h"

#if NVT_TOUCH_EXT_PROC
extern int32_t nvt_extra_proc_init(void);
#endif

#if NVT_TOUCH_EXT_FIH
extern int32_t nvt_extra_FIH_init(void);
#endif

#if NVT_TOUCH_MP
extern int32_t nvt_mp_proc_init(void);
#endif

struct nvt_ts_data *ts;

static struct workqueue_struct *nvt_wq;
//static struct workqueue_struct *nvt_wq_resume;//jason add for new queue to bring up tp during resume

#if BOOT_UPDATE_FIRMWARE
static struct workqueue_struct *nvt_fwu_wq;
extern void Boot_Update_Firmware(struct work_struct *work);
#endif

#if defined(CONFIG_FB)
static int fb_notifier_callback(struct notifier_block *self, unsigned long event, void *data);
#elif defined(CONFIG_HAS_EARLYSUSPEND)
static void nvt_ts_early_suspend(struct early_suspend *h);
static void nvt_ts_late_resume(struct early_suspend *h);
#endif

#if DRAGONBOARD_REGULATOR
#include <linux/regulator/consumer.h>
#endif

#if TOUCH_KEY_NUM > 0
const uint16_t touch_key_array[TOUCH_KEY_NUM] = {
    KEY_BACK,
    KEY_HOME,
    KEY_MENU
};
#endif

#if WAKEUP_GESTURE
const uint16_t gesture_key_array[] = {
    KEY_POWER,  //GESTURE_WORD_C
    KEY_POWER,  //GESTURE_WORD_W
    KEY_POWER,  //GESTURE_WORD_V
    KEY_POWER,  //GESTURE_DOUBLE_CLICK
    KEY_POWER,  //GESTURE_WORD_Z
    KEY_POWER,  //GESTURE_WORD_M
    KEY_POWER,  //GESTURE_WORD_O
    KEY_POWER,  //GESTURE_WORD_e
    KEY_POWER,  //GESTURE_WORD_S
    KEY_POWER,  //GESTURE_SLIDE_UP
    KEY_POWER,  //GESTURE_SLIDE_DOWN
    KEY_POWER,  //GESTURE_SLIDE_LEFT
    KEY_POWER,  //GESTURE_SLIDE_RIGHT
};
#endif

//TP-FTM-00+{_20161110
#include "../../../fih/fih_touch.h"
extern int tp_probe_success;
void touch_tpfwver_read_nvt(char *fw_ver_fih);
int touch_selftest_result_read_nvt(void);
void touch_fwupgrade_read_nvt(char *buf);
extern struct fih_touch_cb touch_cb;
//TP-FTM-00+}_20161110

//TP-BBox-00+{_20161221
/* Black Box */
#define BBOX_TP_PROBE_FAIL	do {printk("BBox;%s: TP firmware update fail!\n", __func__); printk("BBox::UEC;7::0\n");} while (0);
#define BBOX_TP_I2C_READ_FAIL	do {printk("BBox;%s: TP i2c read fail!\n", __func__); printk("BBox::UEC;7::1\n");} while (0);
#define BBOX_TP_I2C_WRITE_FAIL	do {printk("BBox;%s: TP i2c write fail!\n", __func__); printk("BBox::UEC;7::2\n");} while (0);
//TP-BBox-00+}_20161221

static uint8_t bTouchIsAwake = 0;

/*******************************************************
Description:
	Novatek touchscreen i2c read function.

return:
	Executive outcomes. 2---succeed. -5---I/O error
*******************************************************/
int32_t CTP_I2C_READ(struct i2c_client *client, uint16_t address, uint8_t *buf, uint16_t len)
{
	struct i2c_msg msgs[2];
	int32_t ret = -1;
	int32_t retries = 0;

	msgs[0].flags = !I2C_M_RD;
	msgs[0].addr  = address;
	msgs[0].len   = 1;
	msgs[0].buf   = &buf[0];

	msgs[1].flags = I2C_M_RD;
	msgs[1].addr  = address;
	msgs[1].len   = len - 1;
	msgs[1].buf   = &buf[1];

	while (retries < 5) {
		ret = i2c_transfer(client->adapter, msgs, 2);
		if (ret == 2)	break;
		retries++;
	}

	if (unlikely(retries == 5)) {
		dev_err(&client->dev, "%s: error, ret=%d\n", __func__, ret);
		BBOX_TP_I2C_READ_FAIL	//TP-BBox-00+_20161221
		ret = -EIO;
	}

	return ret;
}

/*******************************************************
Description:
	Novatek touchscreen i2c dummy read function.

return:
	Executive outcomes. 1---succeed. -5---I/O error
*******************************************************/
int32_t CTP_I2C_READ_DUMMY(struct i2c_client *client, uint16_t address)
{
	struct i2c_msg msg;
	uint8_t buf[8] = {0};
	int32_t ret = -1;
	int32_t retries = 0;

	msg.flags = I2C_M_RD;
	msg.addr  = address;
	msg.len   = 1;
	msg.buf   = buf;

	while (retries < 5) {
		ret = i2c_transfer(client->adapter, &msg, 1);
		if (ret == 1)	break;
		retries++;
	}

	if (unlikely(retries == 5)) {
		dev_err(&client->dev, "%s: error, ret=%d\n", __func__, ret);
		BBOX_TP_I2C_READ_FAIL	//TP-BBox-00+_20161221
		ret = -EIO;
	}

	return ret;
}

/*******************************************************
Description:
	Novatek touchscreen i2c write function.

return:
	Executive outcomes. 1---succeed. -5---I/O error
*******************************************************/
int32_t CTP_I2C_WRITE(struct i2c_client *client, uint16_t address, uint8_t *buf, uint16_t len)
{
	struct i2c_msg msg;
	int32_t ret = -1;
	int32_t retries = 0;

	msg.flags = !I2C_M_RD;
	msg.addr  = address;
	msg.len   = len;
	msg.buf   = buf;

	while (retries < 5) {
		ret = i2c_transfer(client->adapter, &msg, 1);
		if (ret == 1)	break;
		retries++;
	}

	if (unlikely(retries == 5)) {
		dev_err(&client->dev, "%s: error, ret=%d\n", __func__, ret);
		BBOX_TP_I2C_WRITE_FAIL	//TP-BBox-00+_20161221
		ret = -EIO;
	}

	return ret;
}


/*******************************************************
Description:
	Novatek touchscreen IC hardware reset function.

return:
	n.a.
*******************************************************/
void nvt_hw_reset(void)
{
	//---trigger rst-pin to reset---
	gpio_set_value(ts->reset_gpio, 1);
	msleep(20);
	gpio_set_value(ts->reset_gpio, 0);
	msleep(20);
	gpio_set_value(ts->reset_gpio, 1);
	msleep(20);
}

/*******************************************************
Description:
	Novatek touchscreen set i2c debounce function.

return:
	n.a.
*******************************************************/
void nvt_set_i2c_debounce(void)
{
	uint8_t buf[8] = {0};
	uint8_t reg1_val = 0;
	uint8_t reg2_val = 0;
	uint32_t retry = 0;

	do {
		msleep(10);

		// set xdata index to 0x1F000
		buf[0] = 0xFF;
		buf[1] = 0x01;
		buf[2] = 0xF0;
		CTP_I2C_WRITE(ts->client, I2C_FW_Address, buf, 3);

		// set i2c debounce 34ns
		buf[0] = 0x15;
		buf[1] = 0x17;
		CTP_I2C_WRITE(ts->client, I2C_FW_Address, buf, 2);

		buf[0] = 0x15;
		buf[1] = 0x00;
		CTP_I2C_READ(ts->client, I2C_FW_Address, buf, 2);
		reg1_val = buf[1];

		// set schmitt trigger enable
		buf[0] = 0x3E;
		buf[1] = 0x07;
		CTP_I2C_WRITE(ts->client, I2C_FW_Address, buf, 2);

		buf[0] = 0x3E;
		buf[1] = 0x00;
		CTP_I2C_READ(ts->client, I2C_FW_Address, buf, 2);
		reg2_val = buf[1];
	} while (((reg1_val != 0x17) || (reg2_val != 0x07)) && (retry++ < 20));

	if(retry == 20) {
		dev_err(&ts->client->dev,"%s: set i2c debounce failed, reg1_val=0x%02X, reg2_val=0x%02X\n", __func__, reg1_val, reg2_val);
	}
}

/*******************************************************
Description:
	Novatek touchscreen reset MCU then into idle mode
    function.

return:
	n.a.
*******************************************************/
void nvt_sw_reset_idle(void)
{
	uint8_t buf[4]={0};

	//---write i2c cmds to reset idle---
	buf[0]=0x00;
	buf[1]=0xA5;
	CTP_I2C_WRITE(ts->client, I2C_HW_Address, buf, 2);

	msleep(10);

	nvt_set_i2c_debounce();
}

/*******************************************************
Description:
	Novatek touchscreen reset MCU (boot) function.

return:
	n.a.
*******************************************************/
void nvt_bootloader_reset(void)
{
	uint8_t buf[8] = {0};

	//---write i2c cmds to reset---
	buf[0] = 0x00;
	buf[1] = 0x69;
	CTP_I2C_WRITE(ts->client, I2C_HW_Address, buf, 2);

	// need 35ms delay after bootloader reset
	msleep(35);
}

/*******************************************************
Description:
	Novatek touchscreen clear FW status function.

return:
	Executive outcomes. 0---succeed. -1---fail.
*******************************************************/
int32_t nvt_clear_fw_status(void)
{
	uint8_t buf[8] = {0};
	int32_t i = 0;
	const int32_t retry = 10;

	//---dummy read to resume TP before writing command---
	CTP_I2C_READ_DUMMY(ts->client, I2C_FW_Address);

	for (i = 0; i < retry; i++) {
		//---set xdata index to 0x14700---
		buf[0] = 0xFF;
		buf[1] = 0x01;
		buf[2] = 0x47;
		CTP_I2C_WRITE(ts->client, I2C_FW_Address, buf, 3);

		//---clear fw status---
		buf[0] = 0x51;
		buf[1] = 0x00;
		CTP_I2C_WRITE(ts->client, I2C_FW_Address, buf, 2);

		//---read fw status---
		buf[0] = 0x51;
		buf[1] = 0xFF;
		CTP_I2C_READ(ts->client, I2C_FW_Address, buf, 2);

		if (buf[1] == 0x00)
			break;

		msleep(10);
	}

	if (i >= retry)
		return -1;
	else
		return 0;
}

/*******************************************************
Description:
	Novatek touchscreen check FW status function.

return:
	Executive outcomes. 0---succeed. -1---failed.
*******************************************************/
int32_t nvt_check_fw_status(void)
{
	uint8_t buf[8] = {0};
	int32_t i = 0;
	const int32_t retry = 10;

	//---dummy read to resume TP before writing command---
	CTP_I2C_READ_DUMMY(ts->client, I2C_FW_Address);

	for (i = 0; i < retry; i++) {
		//---set xdata index to 0x14700---
		buf[0] = 0xFF;
		buf[1] = 0x01;
		buf[2] = 0x47;
		CTP_I2C_WRITE(ts->client, I2C_FW_Address, buf, 3);

		//---read fw status---
		buf[0] = 0x51;
		buf[1] = 0x00;
		CTP_I2C_READ(ts->client, I2C_FW_Address, buf, 2);

		if ((buf[1] & 0xF0) == 0xA0)
			break;

		msleep(10);
	}

	if (i >= retry)
		return -1;
	else
		return 0;
}

/*******************************************************
Description:
	Novatek touchscreen check FW reset state function.

return:
	Executive outcomes. 0---succeed. -1---failed.
*******************************************************/
int32_t nvt_check_fw_reset_state(RST_COMPLETE_STATE check_reset_state)
{
	uint8_t buf[8] = {0};
	int32_t ret = 0;
	int32_t retry = 0;

	while (1) {
		msleep(10);

		//---read reset state---
		buf[0] = 0x60;
		buf[1] = 0x00;
		CTP_I2C_READ(ts->client, I2C_FW_Address, buf, 2);

		if ((buf[1] >= check_reset_state) && (buf[1] < 0xFF)) {
			ret = 0;
			break;
		}

		retry++;
		if(unlikely(retry > 100)) {
			ret = -1;
			dev_err(&ts->client->dev,"%s: error, retry=%d, buf[1]=0x%02X\n", __func__, retry, buf[1]);
			break;
		}
	}

	return ret;
}

/*******************************************************
  Create Device Node (Proc Entry)
*******************************************************/
#if NVT_TOUCH_PROC
static struct proc_dir_entry *NVT_proc_entry;
#define DEVICE_NAME	"NVTflash"

/*******************************************************
Description:
	Novatek touchscreen /proc/NVTflash read function.

return:
	Executive outcomes. 2---succeed. -5,-14---failed.
*******************************************************/
static ssize_t nvt_flash_read(struct file *file, char __user *buff, size_t count, loff_t *offp)
{
	struct i2c_msg msgs[2];
	uint8_t str[64] = {0};
	int32_t ret = -1;
	int32_t retries = 0;
	int8_t i2c_wr = 0;

	if (count > sizeof(str))
		return -EFAULT;

	if (copy_from_user(str, buff, count))
		return -EFAULT;

	i2c_wr = str[0] >> 7;

	if (i2c_wr == 0) {	//I2C write
		msgs[0].flags = !I2C_M_RD;
		msgs[0].addr  = str[0] & 0x7F;
		msgs[0].len   = str[1];
		msgs[0].buf   = &str[2];

		while (retries < 20) {
			ret = i2c_transfer(ts->client->adapter, msgs, 1);
			if (ret == 1)
				break;
			else
				dev_err(&ts->client->dev,"%s: error, retries=%d, ret=%d\n", __func__, retries, ret);

			retries++;
		}

		if (unlikely(retries == 20)) {
			dev_err(&ts->client->dev, "%s: error, ret = %d\n", __func__, ret);
			return -EIO;
		}

		return ret;
	} else if (i2c_wr == 1) {	//I2C read
		msgs[0].flags = !I2C_M_RD;
		msgs[0].addr  = str[0] & 0x7F;
		msgs[0].len   = 1;
		msgs[0].buf   = &str[2];

		msgs[1].flags = I2C_M_RD;
		msgs[1].addr  = str[0] & 0x7F;
		msgs[1].len   = str[1]-1;
		msgs[1].buf   = &str[3];

		while (retries < 20) {
			ret = i2c_transfer(ts->client->adapter, msgs, 2);
			if (ret == 2)
				break;
			else
				dev_err(&ts->client->dev,"%s: error, retries=%d, ret=%d\n", __func__, retries, ret);

			retries++;
		}

		// copy buff to user if i2c transfer
		if (retries < 20) {
			if (copy_to_user(buff, str, count))
				return -EFAULT;
		}

		if (unlikely(retries == 20)) {
			dev_err(&ts->client->dev, "%s: error, ret = %d\n", __func__, ret);
			return -EIO;
		}

		return ret;
	} else {
		dev_err(&ts->client->dev,"%s: Call error, str[0]=%d\n", __func__, str[0]);
		return -EFAULT;
	}
}

/*******************************************************
Description:
	Novatek touchscreen /proc/NVTflash open function.

return:
	Executive outcomes. 0---succeed. -12---failed.
*******************************************************/
static int32_t nvt_flash_open(struct inode *inode, struct file *file)
{
	struct nvt_flash_data *dev;

	dev = kmalloc(sizeof(struct nvt_flash_data), GFP_KERNEL);
	if (dev == NULL) {
		dev_err(&ts->client->dev,"%s: Failed to allocate memory for nvt flash data\n", __func__);
		return -ENOMEM;
	}

	rwlock_init(&dev->lock);
	file->private_data = dev;

	return 0;
}

/*******************************************************
Description:
	Novatek touchscreen /proc/NVTflash close function.

return:
	Executive outcomes. 0---succeed.
*******************************************************/
static int32_t nvt_flash_close(struct inode *inode, struct file *file)
{
	struct nvt_flash_data *dev = file->private_data;

	if (dev)
		kfree(dev);

	return 0;
}

static const struct file_operations nvt_flash_fops = {
	.owner = THIS_MODULE,
	.open = nvt_flash_open,
	.release = nvt_flash_close,
	.read = nvt_flash_read,
};

/*******************************************************
Description:
	Novatek touchscreen /proc/NVTflash initial function.

return:
	Executive outcomes. 0---succeed. -12---failed.
*******************************************************/
static int32_t nvt_flash_proc_init(void)
{
	NVT_proc_entry = proc_create(DEVICE_NAME, 0444, NULL,&nvt_flash_fops);
	if (NVT_proc_entry == NULL) {
		dev_err(&ts->client->dev,"%s: Failed!\n", __func__);
		return -ENOMEM;
	} else {
		dev_info(&ts->client->dev,"%s: Succeeded!\n", __func__);
	}

	dev_info(&ts->client->dev,"============================================================\n");
	dev_info(&ts->client->dev,"Create /proc/NVTflash\n");
	dev_info(&ts->client->dev,"============================================================\n");

	return 0;
}
#endif

#if WAKEUP_GESTURE
#define GESTURE_WORD_C			12
#define GESTURE_WORD_W			13
#define GESTURE_WORD_V			14
#define GESTURE_DOUBLE_CLICK	15
#define GESTURE_WORD_Z			16
#define GESTURE_WORD_M			17
#define GESTURE_WORD_O			18
#define GESTURE_WORD_e			19
#define GESTURE_WORD_S			20
#define GESTURE_SLIDE_UP		21
#define GESTURE_SLIDE_DOWN		22
#define GESTURE_SLIDE_LEFT		23
#define GESTURE_SLIDE_RIGHT		24

static struct wake_lock gestrue_wakelock;
static uint8_t bWakeupByGesture = 0;

/*******************************************************
Description:
	Novatek touchscreen wake up gesture key report function.

return:
	n.a.
*******************************************************/
void nvt_ts_wakeup_gesture_report(uint8_t gesture_id)
{
	struct i2c_client *client = ts->client;
	uint32_t keycode = 0;

	dev_info(&client->dev, "gesture_id = %d\n", gesture_id);

	switch (gesture_id) {
		case GESTURE_WORD_C:
			dev_info(&client->dev, "Gesture : Word-C.\n");
			keycode = gesture_key_array[0];
			break;
		case GESTURE_WORD_W:
			dev_info(&client->dev, "Gesture : Word-W.\n");
			keycode = gesture_key_array[1];
			break;
		case GESTURE_WORD_V:
			dev_info(&client->dev, "Gesture : Word-V.\n");
			keycode = gesture_key_array[2];
			break;
		case GESTURE_DOUBLE_CLICK:
			dev_info(&client->dev, "Gesture : Double Click.\n");
			keycode = gesture_key_array[3];
			break;
		case GESTURE_WORD_Z:
			dev_info(&client->dev, "Gesture : Word-Z.\n");
			keycode = gesture_key_array[4];
			break;
		case GESTURE_WORD_M:
			dev_info(&client->dev, "Gesture : Word-M.\n");
			keycode = gesture_key_array[5];
			break;
		case GESTURE_WORD_O:
			dev_info(&client->dev, "Gesture : Word-O.\n");
			keycode = gesture_key_array[6];
			break;
		case GESTURE_WORD_e:
			dev_info(&client->dev, "Gesture : Word-e.\n");
			keycode = gesture_key_array[7];
			break;
		case GESTURE_WORD_S:
			dev_info(&client->dev, "Gesture : Word-S.\n");
			keycode = gesture_key_array[8];
			break;
		case GESTURE_SLIDE_UP:
			dev_info(&client->dev, "Gesture : Slide UP.\n");
			keycode = gesture_key_array[9];
			break;
		case GESTURE_SLIDE_DOWN:
			dev_info(&client->dev, "Gesture : Slide DOWN.\n");
			keycode = gesture_key_array[10];
			break;
		case GESTURE_SLIDE_LEFT:
			dev_info(&client->dev, "Gesture : Slide LEFT.\n");
			keycode = gesture_key_array[11];
			break;
		case GESTURE_SLIDE_RIGHT:
			dev_info(&client->dev, "Gesture : Slide RIGHT.\n");
			keycode = gesture_key_array[12];
			break;
		default:
			break;
	}

	if (keycode > 0) {
		input_report_key(ts->input_dev, keycode, 1);
		input_sync(ts->input_dev);
		input_report_key(ts->input_dev, keycode, 0);
		input_sync(ts->input_dev);

		bWakeupByGesture = 1;
	}

	msleep(250);
}
#endif

#if DRAGONBOARD_REGULATOR
/*******************************************************
Description:
	Novatek touchscreen power on function.

return:
	Executive outcomes. 0---succeed. not 0---failed.
*******************************************************/
static int32_t nvt_power_on(struct nvt_ts_data *ts)
{
	int32_t ret = 0;

	if (ts->power_on) {
		dev_info(&ts->client->dev, "Device already power on\n");
		return 0;
	}

	if (!IS_ERR(ts->avdd)) {
		ret = regulator_enable(ts->avdd);
		if (ret) {
			dev_err(&ts->client->dev, "Regulator avdd enable failed ret=%d\n", ret);
			goto err_enable_avdd;
		}
	}

	if (!IS_ERR(ts->vdd)) {
		ret = regulator_enable(ts->vdd);
		if (ret) {
			dev_err(&ts->client->dev, "Regulator vdd enable failed ret=%d\n", ret);
			goto err_enable_vdd;
		}
	}

	ts->power_on = true;
	return 0;

err_enable_vdd:
	if (!IS_ERR(ts->avdd))
		regulator_disable(ts->avdd);
err_enable_avdd:
	ts->power_on = false;
	return ret;
}

/*******************************************************
Description:
	Novatek touchscreen power off function.

return:
	Executive outcomes. 0---succeed. not 0---failed.
*******************************************************/
static int32_t nvt_power_off(struct nvt_ts_data *ts)
{
	int32_t ret = 0;

	if (!ts->power_on) {
		dev_info(&ts->client->dev,
				"Device already power off\n");
		return 0;
	}

	if (!IS_ERR(ts->vdd)) {
		ret = regulator_disable(ts->vdd);
		if (ret)
			dev_err(&ts->client->dev, "Regulator vdd disable failed ret=%d\n", ret);
	}

	if (!IS_ERR(ts->avdd)) {
		ret = regulator_disable(ts->avdd);
		if (ret)
			dev_err(&ts->client->dev, "Regulator avdd disable failed ret=%d\n", ret);
	}

	ts->power_on = false;
	return 0;
}

/*******************************************************
Description:
	Novatek touchscreen power initial function.

return:
	Executive outcomes. 0---succeed. not 0---failed.
*******************************************************/
static int32_t nvt_power_init(struct nvt_ts_data *ts)
{
	int32_t ret = 0;

	ts->avdd = regulator_get(&ts->client->dev, "avdd");
	if (IS_ERR(ts->avdd)) {
		ret = PTR_ERR(ts->avdd);
		dev_err(&ts->client->dev, "Regulator get failed avdd ret=%d\n", ret);
	}

	ts->vdd = regulator_get(&ts->client->dev, "vdd");
	if (IS_ERR(ts->vdd)) {
		ret = PTR_ERR(ts->vdd);
		dev_err(&ts->client->dev, "Regulator get failed vdd ret=%d\n", ret);
	}

	return 0;
}

/*******************************************************
Description:
	Novatek touchscreen power remove function.

return:
	Executive outcomes. 0---succeed.
*******************************************************/
static int32_t nvt_power_remove(struct nvt_ts_data *ts)
{
	regulator_put(ts->vdd);
	regulator_put(ts->avdd);

	return 0;
}
#endif


/*******************************************************
Description:
	Novatek touchscreen parse device tree function.

return:
	n.a.
*******************************************************/
#ifdef CONFIG_OF
static void novatek_parse_dt(struct device *dev)
{
	struct device_node *np = dev->of_node;

	ts->reset_gpio = of_get_named_gpio_flags(np, "novatek,reset-gpio", 0, &ts->reset_flags);
	ts->irq_gpio = of_get_named_gpio_flags(np, "novatek,irq-gpio", 0, &ts->irq_flags);

	dev_info(dev,"%s: novatek,reset-gpio=%d, novatek,irq-gpio=%d\n", __func__, ts->reset_gpio, ts->irq_gpio);
}
#else
static void novatek_parse_dt(struct device *dev)
{
	ts->reset_gpio = NVTTOUCH_RST_PIN;
	ts->irq_gpio = NVTTOUCH_INT_PIN;
}
#endif

#if POINT_DATA_CHECKSUM
/*******************************************************
Description:
       Novatek touchscreen check i2c packet checksum function.

return:
       Executive outcomes. 0---succeed. not 0---failed.
*******************************************************/
static int32_t nvt_ts_point_data_checksum(struct i2c_client *client, uint8_t *buf, uint8_t length)
{
	uint8_t checksum = 0;
	int32_t i = 0;

	// Generate checksum
	for (i = 0; i < length; i++) {
		checksum += buf[i+1];
	}
	checksum = (~checksum + 1);

	// Compare ckecksum and dump fail data
	if (checksum != buf[length + 1]) {
		dev_err(&client->dev, "%s: i2c packet checksum not match. (point_data[%d]=0x%02X, checksum=0x%02X)\n", __func__, (length+1), buf[length+1], checksum);

		for (i = 0; i < 10; i++) {
			dev_err(&client->dev, "%02X %02X %02X %02X %02X %02X\n", buf[1+i*6], buf[2+i*6], buf[3+i*6], buf[4+i*6], buf[5+i*6], buf[6+i*6]);
		}

		for (i = 0; i < (length - 60); i++) {
			dev_err(&client->dev, "%02X ", buf[1+60+i]);
		}

		return -1;
	}

	return 0;
}
#endif /* POINT_DATA_CHECKSUM */

/*******************************************************
Description:
	Novatek touchscreen work function.

return:
	n.a.
*******************************************************/
static void nvt_ts_work_func(struct work_struct *work)
{
	//struct nvt_ts_data *ts = container_of(work, struct nvt_ts_data, work);
	struct i2c_client *client = ts->client;

	int32_t ret = -1;
	uint8_t point_data[70] = {0};
	uint32_t position = 0;
	uint32_t input_x = 0;
	uint32_t input_y = 0;
	uint32_t input_w = 0;
	uint8_t input_id = 0;
	uint8_t press_id[TOUCH_MAX_FINGER_NUM] = {0};

	int32_t i = 0;
	int32_t finger_cnt = 0;

	mutex_lock(&ts->lock);

	ret = CTP_I2C_READ(ts->client, I2C_FW_Address, point_data, 65 + 1);
	if (ret < 0) {
		dev_err(&client->dev, "%s: CTP_I2C_READ failed.(%d)\n", __func__, ret);
		goto XFER_ERROR;
	}
/*
	//--- dump I2C buf ---
	for (i = 0; i < 10; i++) {
		printk("%02X %02X %02X %02X %02X %02X  ", point_data[1+i*6], point_data[2+i*6], point_data[3+i*6], point_data[4+i*6], point_data[5+i*6], point_data[6+i*6]);
	}
	printk("\n");
*/
#if POINT_DATA_CHECKSUM
	ret = nvt_ts_point_data_checksum(ts->client, point_data, 64);
	if (ret < 0) {
		goto XFER_ERROR;
	}
#endif /* POINT_DATA_CHECKSUM */

	finger_cnt = 0;
	input_id = (uint8_t)(point_data[1] >> 3);


#if WAKEUP_GESTURE
	if (bTouchIsAwake == 0) {
		nvt_ts_wakeup_gesture_report(input_id);
		enable_irq(ts->client->irq);
		mutex_unlock(&ts->lock);
		return;
	}
#endif

#if MT_PROTOCOL_B
	for (i = 0; i < ts->max_touch_num; i++) {
		position = 1 + 6 * i;
		input_id = (uint8_t)(point_data[position + 0] >> 3);
		if (input_id > TOUCH_MAX_FINGER_NUM)
			continue;

		if (((point_data[position] & 0x07) == 0x01) || ((point_data[position] & 0x07) == 0x02)) {	//finger down (enter & moving)
			input_x = (uint32_t)(point_data[position + 1] << 4) + (uint32_t) (point_data[position + 3] >> 4);
			input_y = (uint32_t)(point_data[position + 2] << 4) + (uint32_t) (point_data[position + 3] & 0x0F);
			input_w = (uint32_t)(point_data[position + 4]) + 10;
			if (input_w > 255)
				input_w = 255;

			if ((input_x < 0) || (input_y < 0))
				continue;
			if ((input_x > ts->abs_x_max)||(input_y > ts->abs_y_max))
				continue;

			press_id[input_id - 1] = 1;
			input_mt_slot(ts->input_dev, input_id - 1);
			input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, true);

			input_report_abs(ts->input_dev, ABS_MT_POSITION_X, input_x);
			input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, input_y);
			input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, input_w);
			input_report_abs(ts->input_dev, ABS_MT_PRESSURE, input_w);

			finger_cnt++;
		}
	}

	for (i = 0; i < ts->max_touch_num; i++) {
		if (press_id[i] != 1) {
			input_mt_slot(ts->input_dev, i);
			input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0);
			input_report_abs(ts->input_dev, ABS_MT_PRESSURE, 0);
			input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, false);
		}
	}

	input_report_key(ts->input_dev, BTN_TOUCH, (finger_cnt > 0));

#else

	for (i = 0; i < ts->max_touch_num; i++) {
		position = 1 + 6 * i;
		input_id = (uint8_t)(point_data[position + 0] >> 3);

		if ((point_data[position] & 0x07) == 0x03) {	// finger up (break)
			continue;//input_report_key(ts->input_dev, BTN_TOUCH, 0);
		} else if (((point_data[position] & 0x07) == 0x01) || ((point_data[position] & 0x07) == 0x02)) {	//finger down (enter & moving)
			input_x = (uint32_t)(point_data[position + 1] << 4) + (uint32_t) (point_data[position + 3] >> 4);
			input_y = (uint32_t)(point_data[position + 2] << 4) + (uint32_t) (point_data[position + 3] & 0x0F);
			input_w = (uint32_t)(point_data[position + 4]) + 10;
			if (input_w > 255)
				input_w = 255;

			if ((input_x < 0) || (input_y < 0))
				continue;
			if ((input_x > ts->abs_x_max)||(input_y > ts->abs_y_max))
				continue;

			press_id[input_id - 1] = 1;
			input_report_key(ts->input_dev, BTN_TOUCH, 1);
			input_report_abs(ts->input_dev, ABS_MT_POSITION_X, input_x);
			input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, input_y);
			input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, input_w);
			input_report_abs(ts->input_dev, ABS_MT_PRESSURE, input_w);
			input_report_abs(ts->input_dev, ABS_MT_TRACKING_ID, input_id - 1);

			input_mt_sync(ts->input_dev);

			finger_cnt++;
		}
	}
	if (finger_cnt == 0) {
		input_report_key(ts->input_dev, BTN_TOUCH, 0);

		input_mt_sync(ts->input_dev);
	}
#endif


#if TOUCH_KEY_NUM > 0
	if (point_data[61] == 0xF8) {
		for (i = 0; i < TOUCH_KEY_NUM; i++) {
			input_report_key(ts->input_dev, touch_key_array[i], ((point_data[62] >> i) & 0x01));
		}
	} else {
		for (i = 0; i < TOUCH_KEY_NUM; i++) {
			input_report_key(ts->input_dev, touch_key_array[i], 0);
		}
	}
#endif

	input_sync(ts->input_dev);

XFER_ERROR:
	enable_irq(ts->client->irq);

	mutex_unlock(&ts->lock);
}

/*******************************************************
Description:
	External interrupt service routine.

return:
	irq execute status.
*******************************************************/
static irqreturn_t nvt_ts_irq_handler(int32_t irq, void *dev_id)
{
	//struct nvt_ts_data *ts = dev_id;

	disable_irq_nosync(ts->client->irq);

#if WAKEUP_GESTURE
	if (bTouchIsAwake == 0) {
		wake_lock_timeout(&gestrue_wakelock, msecs_to_jiffies(5000));
	}
#endif

	queue_work(nvt_wq, &ts->nvt_work);

	return IRQ_HANDLED;
}


/*******************************************************
Description:
	Novatek touchscreen print chip version function.

return:
	Executive outcomes. 2---succeed. -5---failed.
*******************************************************/
static int32_t nvt_ts_chip_version(void)
{
	uint8_t buf[8] = {0};
	int32_t ret = 0;
	uint8_t cut_number = 0;
	uint8_t cut_version = 0;

	//---dummy read to resume TP before writing command---
	CTP_I2C_READ_DUMMY(ts->client, I2C_FW_Address);

	//write i2c index to 0x1F000
	buf[0] = 0xFF;
	buf[1] = 0x01;
	buf[2] = 0xF0;
	ret = CTP_I2C_WRITE(ts->client, 0x01, buf, 3);
	if (ret < 0) {
		dev_err(&ts->client->dev, "%s: write i2c index error!!(%d)\n", __func__, ret);
		return ret;
	}

	//read chip version
	buf[0] = 0x01;
	buf[1] = 0x00;
	ret = CTP_I2C_READ(ts->client, 0x01, buf, 3);
	if (ret < 0) {
		dev_err(&ts->client->dev, "%s: read chip version error!!(%d)\n", __func__, ret);
		return ret;
	}

	// [4:0]: Cut Number
	cut_number = buf[1] & 0x1F;
	// [7:5]: Cut Version
	cut_version = (buf[1] & 0xE0) >> 5;

	dev_info(&ts->client->dev, "chip version: cut number=%d, cut version=%d\n", cut_number, cut_version);

	return ret;
}

/*******************************************************
Description:
	Novatek touchscreen read chip id function.

return:
	Executive outcomes. 0x26---succeed.
*******************************************************/
static uint8_t nvt_ts_read_chipid(void)
{
	uint8_t buf[8] = {0};
	int32_t retry = 0;

	//---dummy read to resume TP before writing command---
	CTP_I2C_READ_DUMMY(ts->client, I2C_HW_Address);

	// reset idle to keep default addr 0x01 to read chipid
	buf[0] = 0x00;
	buf[1] = 0xA5;
	CTP_I2C_WRITE(ts->client, I2C_HW_Address, buf, 2);

	msleep(10);

	//---Check NT11206 for 5 times---
	for (retry = 5; retry > 0; retry--) {
		//write i2c index to 0x1F000
		buf[0] = 0xFF;
		buf[1] = 0x01;
		buf[2] = 0xF0;
		CTP_I2C_WRITE(ts->client, 0x01, buf, 3);

		//read hw chip id
		buf[0] = 0x00;
		buf[1] = 0x00;
		CTP_I2C_READ(ts->client, 0x01, buf, 3);

		if (buf[1] == 0x26)
			break;
	}

	return buf[1];
}


/*******************************************************
Description:
	Novatek touchscreen driver probe function.

return:
	Executive outcomes. 0---succeed. negative---failed
*******************************************************/
static int32_t nvt_ts_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int32_t ret = 0;
#if ((TOUCH_KEY_NUM > 0) || WAKEUP_GESTURE)
	int32_t retry = 0;
#endif

	dev_info(&client->dev, "%s:11/08 999 start.......\n", __func__);
	ts = kmalloc(sizeof(struct nvt_ts_data), GFP_KERNEL);
	if (ts == NULL) {
		dev_err(&client->dev, "%s: failed to allocated memory for nvt ts data\n", __func__);
		BBOX_TP_PROBE_FAIL	//TP-BBox-00+_20161221
		return -ENOMEM;
	}
	ts->client = client;
	i2c_set_clientdata(client, ts);


	//---parse dts---
	novatek_parse_dt(&client->dev);


#if DRAGONBOARD_REGULATOR
	ts->power_on = false;

	//---request regulator for DragonBoard---
	ret = nvt_power_init(ts);
	if (ret) {
		dev_err(&client->dev, "nvt power init failed\n");
		goto err_power_init;
	}

	//---turn on regulator for DragonBoard---
	ret = nvt_power_on(ts);
	if (ret) {
		dev_err(&client->dev, "nvt power on failed\n");
		goto err_nvt_power_on;
	}
#endif


	//---request RST-pin & INT-pin---
	ret = gpio_request_one(ts->reset_gpio, GPIOF_OUT_INIT_HIGH, "NVT-rst");
	if (ret)
		dev_err(&client->dev, "Failed to get NVT-rst GPIO\n");

	ret = gpio_request_one(ts->irq_gpio, GPIOF_IN, "NVT-int");
	if (ret)
		dev_err(&client->dev, "Failed to get NVT-int GPIO\n");

	//---check i2c func.---
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_err(&client->dev, "i2c_check_functionality failed. (no I2C_FUNC_I2C)\n");
		ret = -ENODEV;
		goto err_check_functionality_failed;
	}

	// need 10ms delay after POR(power on reset)
	msleep(10);

	//---check chip id---
	ret = nvt_ts_read_chipid();
	if (ret != 0x26) {
		dev_err(&client->dev, "nvt_ts_read_chipid is not 0x26. ret=0x%02X\n", ret);
		ret = -EINVAL;
		goto err_chipid_failed;
	}

	//---check chip version---
	ret = nvt_ts_chip_version();

	mutex_init(&ts->lock);

	//---create workqueue---
	nvt_wq = create_workqueue("nvt_wq");
	if (!nvt_wq) {
		dev_err(&client->dev,"%s: nvt_wq create workqueue failed\n", __func__);
		ret = -ENOMEM;
		goto err_create_nvt_wq_failed;
	}
	INIT_WORK(&ts->nvt_work, nvt_ts_work_func);

	//jason add for new queue to bring up tp during resume start
	//nvt_wq_resume = create_workqueue("nvt_wq_resume");
	//if (!nvt_wq_resume)
	//	dev_err(&client->dev,"%s: nvt_wq_resume create workqueue failed\n", __func__);
	//else
	//	INIT_WORK(&ts->work, nvt_ts_work_func_resume);

	//jason add for new queue to bring up tp during resume end
	//---allocate input device---
	ts->input_dev = input_allocate_device();
	if (ts->input_dev == NULL) {
		dev_err(&client->dev, "%s: allocate input device failed\n", __func__);
		ret = -ENOMEM;
		goto err_input_dev_alloc_failed;
	}

#if BOOT_UPDATE_FIRMWARE
	nvt_fwu_wq = create_singlethread_workqueue("nvt_fwu_wq");
	if (!nvt_fwu_wq) {
		dev_err(&client->dev, "%s: nvt_fwu_wq create workqueue failed\n", __func__);
		ret = -ENOMEM;
		goto err_create_nvt_fwu_wq_failed;
	}
	INIT_DELAYED_WORK(&ts->nvt_fwu_work, Boot_Update_Firmware);
	queue_delayed_work(nvt_fwu_wq, &ts->nvt_fwu_work, msecs_to_jiffies(4000));
#endif

	ts->abs_x_max = TOUCH_MAX_WIDTH;
	ts->abs_y_max = TOUCH_MAX_HEIGHT;
	ts->max_touch_num = TOUCH_MAX_FINGER_NUM;

#if TOUCH_KEY_NUM > 0
	ts->max_button_num = TOUCH_KEY_NUM;
#endif

	ts->int_trigger_type = INT_TRIGGER_TYPE;


	//---set input device info.---
	ts->input_dev->evbit[0] = BIT_MASK(EV_SYN) | BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS) ;
	ts->input_dev->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);
	ts->input_dev->propbit[0] = BIT(INPUT_PROP_DIRECT);

#if MT_PROTOCOL_B
	input_mt_init_slots(ts->input_dev, ts->max_touch_num, 0);
#endif

	input_set_abs_params(ts->input_dev, ABS_MT_PRESSURE, 0, 255, 0, 0);    //pressure = 255

#if TOUCH_MAX_FINGER_NUM > 1
	input_set_abs_params(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);    //area = 255

	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_X, 0, ts->abs_x_max, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_Y, 0, ts->abs_y_max, 0, 0);
#if MT_PROTOCOL_B
	// no need to set ABS_MT_TRACKING_ID, input_mt_init_slots() already set it
#else
	input_set_abs_params(ts->input_dev, ABS_MT_TRACKING_ID, 0, ts->max_touch_num, 0, 0);
#endif
#endif

#if TOUCH_KEY_NUM > 0
	for (retry = 0; retry < TOUCH_KEY_NUM; retry++) {
		input_set_capability(ts->input_dev, EV_KEY, touch_key_array[retry]);
	}
#endif

#if WAKEUP_GESTURE
	for (retry = 0; retry < (sizeof(gesture_key_array) / sizeof(gesture_key_array[0])); retry++) {
		input_set_capability(ts->input_dev, EV_KEY, gesture_key_array[retry]);
	}
	wake_lock_init(&gestrue_wakelock, WAKE_LOCK_SUSPEND, "poll-wake-lock");
#endif

	sprintf(ts->phys, "input/ts");
	ts->input_dev->name = NVT_TS_NAME;
	ts->input_dev->phys = ts->phys;
	ts->input_dev->id.bustype = BUS_I2C;


	//---register input device---
	ret = input_register_device(ts->input_dev);
	if (ret) {
		dev_err(&client->dev, "register input device (%s) failed. ret=%d\n", ts->input_dev->name, ret);
		goto err_input_register_device_failed;
	}


	//---set int-pin & request irq---
	client->irq = gpio_to_irq(ts->irq_gpio);
	if (client->irq) {
		dev_info(&client->dev, "int_trigger_type=%d\n", ts->int_trigger_type);

#if WAKEUP_GESTURE
		ret = request_irq(client->irq, nvt_ts_irq_handler, ts->int_trigger_type | IRQF_NO_SUSPEND, client->name, ts);
#else
		ret = request_irq(client->irq, nvt_ts_irq_handler, ts->int_trigger_type, client->name, ts);
#endif
		if (ret != 0) {
			dev_err(&client->dev, "request irq failed. ret=%d\n", ret);
			goto err_int_request_failed;
		} else {
			disable_irq(client->irq);
			dev_info(&client->dev, "request irq %d succeed\n", client->irq);
		}
	}

	mutex_lock(&ts->lock);
	nvt_hw_reset();
	msleep(5);
	nvt_bootloader_reset();
	nvt_check_fw_reset_state(RESET_STATE_INIT);
	mutex_unlock(&ts->lock);


	//---set device node---
#if NVT_TOUCH_PROC
	ret = nvt_flash_proc_init();
	if (ret != 0) {
		dev_err(&client->dev, "nvt flash proc init failed. ret=%d\n", ret);
		goto err_init_NVT_ts;
	}
#endif

#if NVT_TOUCH_EXT_PROC
	ret = nvt_extra_proc_init();
	if (ret != 0) {
		dev_err(&client->dev, "nvt extra proc init failed. ret=%d\n", ret);
		goto err_init_NVT_ts;
	}
#endif

#if NVT_TOUCH_EXT_FIH
	ret = nvt_extra_FIH_init();
	if (ret != 0) {
		dev_err(&client->dev, "nvt extra FIH init failed. ret=%d\n", ret);
		goto err_init_NVT_ts;
	}
#endif

#if NVT_TOUCH_MP
	ret = nvt_mp_proc_init();
	if (ret != 0) {
		dev_err(&client->dev, "nvt mp proc init failed. ret=%d\n", ret);
		goto err_init_NVT_ts;
	}
#endif

#if defined(CONFIG_FB)
	ts->fb_notif.notifier_call = fb_notifier_callback;
	ret = fb_register_client(&ts->fb_notif);
	if(ret) {
		dev_err(&client->dev, "register fb_notifier failed. ret=%d\n", ret);
		goto err_register_fb_notif_failed;
	}
#elif defined(CONFIG_HAS_EARLYSUSPEND)
	ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	ts->early_suspend.suspend = nvt_ts_early_suspend;
	ts->early_suspend.resume = nvt_ts_late_resume;
	ret = register_early_suspend(&ts->early_suspend);
	if(ret) {
		dev_err(&client->dev, "register early suspend failed. ret=%d\n", ret);
		goto err_register_early_suspend_failed;
	}
#endif

	enable_irq(client->irq);
	bTouchIsAwake = 1;
	dev_info(&client->dev, "%s: finished\n", __func__);

	//TP-FTM-00+{_20161110
    tp_probe_success = 1;
    touch_cb.touch_selftest_result = touch_selftest_result_read_nvt;
    touch_cb.touch_tpfwver_read = touch_tpfwver_read_nvt;
    touch_cb.touch_fwupgrade_read = touch_fwupgrade_read_nvt;
	//TP-FTM-00+}_20161110

	return 0;

#if defined(CONFIG_FB)
err_register_fb_notif_failed:
#elif defined(CONFIG_HAS_EARLYSUSPEND)
err_register_early_suspend_failed:
#endif
err_init_NVT_ts:
	free_irq(client->irq,ts);
err_int_request_failed:
err_input_register_device_failed:
#if BOOT_UPDATE_FIRMWARE
err_create_nvt_fwu_wq_failed:
#endif
	input_free_device(ts->input_dev);
err_input_dev_alloc_failed:
err_create_nvt_wq_failed:
	mutex_destroy(&ts->lock);
err_chipid_failed:
err_check_functionality_failed:
#if DRAGONBOARD_REGULATOR
	nvt_power_off(ts);
err_nvt_power_on:
	nvt_power_remove(ts);
err_power_init:
#endif
	i2c_set_clientdata(client, NULL);
	kfree(ts);
	BBOX_TP_PROBE_FAIL	//TP-BBox-00+_20161221
	return ret;
}

/*******************************************************
Description:
	Novatek touchscreen driver release function.

return:
	Executive outcomes. 0---succeed.
*******************************************************/
static int32_t nvt_ts_remove(struct i2c_client *client)
{
	//struct nvt_ts_data *ts = i2c_get_clientdata(client);

#if defined(CONFIG_FB)
    if (fb_unregister_client(&ts->fb_notif))
        dev_err(&ts->client->dev, "Error occurred while unregistering fb_notifier.\n");
#elif defined(CONFIG_HAS_EARLYSUSPEND)
	unregister_early_suspend(&ts->early_suspend);
#endif

	mutex_destroy(&ts->lock);

	dev_info(&client->dev, "%s: removing driver...\n", __func__);

	free_irq(client->irq, ts);
	input_unregister_device(ts->input_dev);
	i2c_set_clientdata(client, NULL);
	kfree(ts);

	return 0;
}

/*******************************************************
Description:
	Novatek touchscreen driver suspend function.

return:
	Executive outcomes. 0---succeed.
*******************************************************/
static int32_t nvt_ts_suspend(struct device *dev)
{
	uint8_t buf[4] = {0};

	if (!bTouchIsAwake) {
		dev_info(dev, "%s: Touch is already suspend\n", __func__);
		return 0;
	}

	mutex_lock(&ts->lock);

	dev_info(dev, "%s: begin...\n", __func__);

	bTouchIsAwake = 0;

#if WAKEUP_GESTURE

	//---dummy read to resume TP before writing command---
	CTP_I2C_READ_DUMMY(ts->client, I2C_FW_Address);

	//---write i2c command to enter "wakeup gesture mode"---
	buf[0] = 0x50;
	buf[1] = 0x13;
	buf[2] = 0xFF;
	buf[3] = 0xFF;
	CTP_I2C_WRITE(ts->client, I2C_FW_Address, buf, 4);

	enable_irq_wake(ts->client->irq);

	dev_info(dev, "%s: Enabled touch wakeup gesture\n", __func__);
#else
	disable_irq(ts->client->irq);

	//---dummy read to resume TP before writing command---
	CTP_I2C_READ_DUMMY(ts->client, I2C_FW_Address);

	//---write i2c command to enter "deep sleep mode"---
	buf[0] = 0x50;
	buf[1] = 0x12;
	CTP_I2C_WRITE(ts->client, I2C_FW_Address, buf, 2);
#endif

	msleep(50);

    #if NVT_TOUCH_EXT_FIH	//TP-FixCurrentLeakgeIssue-00+_20161213
	nvt_power_off(ts);
    #endif//#if NVT_TOUCH_EXT_FIH

	mutex_unlock(&ts->lock);

	dev_info(dev, "%s: end\n", __func__);

	return 0;
}

/*******************************************************
Description:
	Novatek touchscreen driver resume function.

return:
	Executive outcomes. 0---succeed.
*******************************************************/
static int32_t nvt_ts_resume(struct device *dev)
{
#if WAKEUP_GESTURE
	uint8_t buf[4] = {0};
#endif

	if (bTouchIsAwake) {
		dev_info(dev, "%s: Touch is already resume\n", __func__);
		return 0;
	}

	mutex_lock(&ts->lock);

	dev_info(dev, "%s: begin...\n", __func__);

#if WAKEUP_GESTURE
	if (bWakeupByGesture == 1) {
		bWakeupByGesture = 0;

		//---write i2c command to leave "wakeup gesture mode"---
		buf[0] = 0x50;
		buf[1] = 0x14;
		CTP_I2C_WRITE(ts->client, I2C_FW_Address, buf, 2);
	} else {
		nvt_hw_reset();
		nvt_bootloader_reset();
		nvt_check_fw_reset_state(RESET_STATE_INIT);
	}

#else

    #if NVT_TOUCH_EXT_FIH		//TP-FixCurrentLeakgeIssue-00+_20161213
	if (nvt_power_on(ts))
	{
		dev_err(dev, "nvt power on failed\n");
		goto err_nvt_power_on;
	}
   	msleep(10);
    #endif//#if NVT_TOUCH_EXT_FIH

	nvt_hw_reset();
	nvt_bootloader_reset();
	nvt_check_fw_reset_state(RESET_STATE_INIT);
	enable_irq(ts->client->irq);
#endif

	bTouchIsAwake = 1;

	mutex_unlock(&ts->lock);

	dev_info(dev, "%s: end\n", __func__);

	return 0;

#if NVT_TOUCH_EXT_FIH		//TP-FixCurrentLeakgeIssue-00+_20161213
err_nvt_power_on:
	nvt_power_remove(ts);
	return -1;
#endif
}
//jason add for new queue to bring up tp during resume start
void nvt_ts_work_func_resume(struct work_struct *work)
{
	pr_debug("\n\n[JSH] nvt_ts_work_func_resume start\n\n");
	nvt_ts_resume(&ts->client->dev);
	pr_debug("\n\n[JSH] nvt_ts_work_func_resume end\n\n");
}
//jason add for new queue to bring up tp during resume end
#if defined(CONFIG_FB)
static int fb_notifier_callback(struct notifier_block *self, unsigned long event, void *data)
{
	struct fb_event *evdata = data;
	int *blank;
	struct nvt_ts_data *ts =
		container_of(self, struct nvt_ts_data, fb_notif);

	//TP-FixForAmbientDisplayIssue-00*{_20170302
	if (evdata && evdata->data && event == FB_EARLY_EVENT_BLANK)
	{
		pr_debug("[HL]%s: event == FB_EARLY_EVENT_BLANK\n", __func__);
		blank = evdata->data;
		pr_debug("[HL]%s: *blank = (%d)\n", __func__, *blank);
		if ((*blank == FB_BLANK_UNBLANK) || (*blank == FB_BLANK_NORMAL))
		{
			pr_debug("[HL]%s: (*blank == FB_BLANK_UNBLANK) || (*blank == FB_BLANK_NORMAL)\n", __func__);
		}
		else	//FB_BLANK_VSYNC_SUSPEND or FB_BLANK_HSYNC_SUSPEND or FB_BLANK_POWERDOWN
		{
			pr_debug("[HL]%s: *blank = FB_BLANK_VSYNC_SUSPEND or FB_BLANK_HSYNC_SUSPEND or FB_BLANK_POWERDOWN\n", __func__);
			nvt_ts_suspend(&ts->client->dev);
		}
	}
	else if (evdata && evdata->data && event == FB_EVENT_BLANK)
	{
		pr_debug("[HL]%s: event == FB_EVENT_BLANK\n", __func__);
		blank = evdata->data;
		pr_debug("[HL]%s: *blank = (%d)\n", __func__, *blank);
		if ((*blank == FB_BLANK_UNBLANK) || (*blank == FB_BLANK_NORMAL))
		{
			pr_debug("[HL]%s: (*blank == FB_BLANK_UNBLANK) || (*blank == FB_BLANK_NORMAL)\n", __func__);
			//if (!nvt_wq_resume) //jason add for new queue to bring up tp during resume start
				nvt_ts_resume(&ts->client->dev);
			//else
			//	queue_work(nvt_wq_resume, &ts->work); //jason add for new queue to bring up tp during resume end
		}
		else	//FB_BLANK_VSYNC_SUSPEND or FB_BLANK_HSYNC_SUSPEND or FB_BLANK_POWERDOWN
		{
			pr_debug("[HL]%s: *blank = FB_BLANK_VSYNC_SUSPEND or FB_BLANK_HSYNC_SUSPEND or FB_BLANK_POWERDOWN\n", __func__);
		}

	}
	//TP-FixForAmbientDisplayIssue-00*}_20170302

	return 0;
}
#elif defined(CONFIG_HAS_EARLYSUSPEND)
/*******************************************************
Description:
	Novatek touchscreen driver early suspend function.

return:
	n.a.
*******************************************************/
static void nvt_ts_early_suspend(struct early_suspend *h)
{
	nvt_ts_suspend(ts->client, PMSG_SUSPEND);
}

/*******************************************************
Description:
	Novatek touchscreen driver late resume function.

return:
	n.a.
*******************************************************/
static void nvt_ts_late_resume(struct early_suspend *h)
{
	nvt_ts_resume(ts->client);
}
#endif

static const struct i2c_device_id nvt_ts_id[] = {
	{ NVT_I2C_NAME, 0 },
	{ }
};

#ifdef CONFIG_OF
static struct of_device_id nvt_match_table[] = {
	{ .compatible = "novatek,NVT-ts",},
	{ },
};
#endif
/*
static struct i2c_board_info __initdata nvt_i2c_boardinfo[] = {
	{
		I2C_BOARD_INFO(NVT_I2C_NAME, I2C_FW_Address),
	},
};
*/

static struct i2c_driver nvt_i2c_driver = {
	.probe		= nvt_ts_probe,
	.remove		= nvt_ts_remove,
//	.suspend	= nvt_ts_suspend,
//	.resume		= nvt_ts_resume,
	.id_table	= nvt_ts_id,
	.driver = {
		.name	= NVT_I2C_NAME,
		.owner	= THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = nvt_match_table,
#endif
	},
};

/*******************************************************
Description:
	Driver Install function.

return:
	Executive Outcomes. 0---succeed. not 0---failed.
********************************************************/
static int32_t __init nvt_driver_init(void)
{
	int32_t ret = 0;

	//---add i2c driver---
	ret = i2c_add_driver(&nvt_i2c_driver);
	if (ret) {
		pr_err("%s: failed to add i2c driver", __func__);
		goto err_driver;
	}

	pr_info("%s: finished\n", __func__);

err_driver:
	return ret;
}

/*******************************************************
Description:
	Driver uninstall function.

return:
	n.a.
********************************************************/
static void __exit nvt_driver_exit(void)
{
	i2c_del_driver(&nvt_i2c_driver);

	if (nvt_wq)
		destroy_workqueue(nvt_wq);

	//if (nvt_wq_resume)//jason add for new queue to bring up tp during resume
	//	destroy_workqueue(nvt_wq_resume);


#if BOOT_UPDATE_FIRMWARE
	if (nvt_fwu_wq)
		destroy_workqueue(nvt_fwu_wq);
#endif

#if DRAGONBOARD_REGULATOR
	nvt_power_off(ts);
	nvt_power_remove(ts);
#endif
}

//late_initcall(nvt_driver_init);
module_init(nvt_driver_init);
module_exit(nvt_driver_exit);

MODULE_DESCRIPTION("Novatek Touchscreen Driver");
MODULE_LICENSE("GPL");
