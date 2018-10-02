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

/*******************************************************************************
* 1.Included header files
*******************************************************************************/
#include "shtps_fts.h"
#include "fwctl/shtps_fwctl_focaltech.h"
/*******************************************************************************
* Private constant and macro definitions using #define
*******************************************************************************/
/*create apk debug channel*/
#define PROC_UPGRADE			0
#define PROC_READ_REGISTER		1
#define PROC_WRITE_REGISTER	2
#define PROC_AUTOCLB			4
#define PROC_UPGRADE_INFO		5
#define PROC_WRITE_DATA		6
#define PROC_READ_DATA			7
#define PROC_SET_TEST_FLAG				8
#define PROC_NAME	"ftxxxx-debug"

#define WRITE_BUF_SIZE		512
#define READ_BUF_SIZE		512


#define FTS_DRIVER_INFO  "Qualcomm_Ver 1.1 2015-04-30"
//#define FTS_REG_FW_VER		0xA6

#define GTP_ESD_PROTECT 0

/*******************************************************************************
* Private enumerations, structures and unions using typedef
*******************************************************************************/


/*******************************************************************************
* Static variables
*******************************************************************************/
static unsigned char proc_operate_mode = PROC_UPGRADE;
static struct proc_dir_entry *shtps_fts_proc_entry;
/*******************************************************************************
* Global variable or extern global variabls/functions
*******************************************************************************/
#if GTP_ESD_PROTECT
int apk_debug_flag = 0;
#endif
struct i2c_client *shtps_fts_i2c_client = NULL;

/*******************************************************************************
* Static function prototypes
*******************************************************************************/

/*******************************************************************************
*  Name: shtps_fts_i2c_read
*  Brief:
*  Input:
*  Output: 
*  Return: 
*******************************************************************************/
int shtps_fts_i2c_read(struct i2c_client *client, char *writebuf, int writelen, char *readbuf, int readlen)
{
	return M_DIRECT_READ_FUNC(gShtps_fts->fwctl_p, writebuf, writelen, readbuf, readlen);
}

/*******************************************************************************
*  Name: shtps_fts_i2c_write
*  Brief:
*  Input:
*  Output: 
*  Return: 
*******************************************************************************/
int shtps_fts_i2c_write(struct i2c_client *client, char *writebuf, int writelen)
{
	return M_DIRECT_WRITE_FUNC(gShtps_fts->fwctl_p, writebuf, writelen);
}

/*******************************************************************************
*  Name: shtps_fts_write_reg
*  Brief:
*  Input:
*  Output: 
*  Return: 
*******************************************************************************/
static int shtps_fts_write_reg(struct i2c_client *client, u8 addr, const u8 val)
{
	return M_WRITE_FUNC(gShtps_fts->fwctl_p, addr, val);
}

/*******************************************************************************
*  Name: shtps_fts_read_reg
*  Brief:
*  Input:
*  Output: 
*  Return: 
*******************************************************************************/
static int shtps_fts_read_reg(struct i2c_client *client, u8 addr, u8 *val)
{
	return M_READ_FUNC(gShtps_fts->fwctl_p, addr, val, 1);
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))//2015.5.20 changed
/*interface of write proc*/
/************************************************************************
*   Name: shtps_fts_debug_write
*  Brief:interface of write proc
* Input: file point, data buf, data len, no use
* Output: no
* Return: data len
***********************************************************************/
static ssize_t shtps_fts_debug_write(struct file *filp, const char __user *buff, size_t count, loff_t *ppos)
{
	unsigned char writebuf[WRITE_BUF_SIZE];
	int buflen = count;
	int writelen = 0;
	int ret = 0;
	
	if (copy_from_user(&writebuf, buff, buflen)) {
		dev_err(&shtps_fts_i2c_client->dev, "%s:copy from user error\n", __func__);
		return -EFAULT;
	}
	proc_operate_mode = writebuf[0];

	switch (proc_operate_mode) {
	case PROC_UPGRADE:
		{
			#if 1
				dev_err(&shtps_fts_i2c_client->dev, "%s:upgrade not support.\n", __func__);
			#else
			char upgrade_file_path[128];
			memset(upgrade_file_path, 0, sizeof(upgrade_file_path));
			sprintf(upgrade_file_path, "%s", writebuf + 1);
			upgrade_file_path[buflen-1] = '\0';
			SHTPS_LOG_DBG_PRINT("%s\n", upgrade_file_path);
			disable_irq(shtps_fts_i2c_client->irq);
			#if GTP_ESD_PROTECT
			apk_debug_flag = 1;
			#endif
			
			ret = shtps_fts_ctpm_fw_upgrade_with_app_file(shtps_fts_i2c_client, upgrade_file_path);
			#if GTP_ESD_PROTECT
			apk_debug_flag = 0;
			#endif
			enable_irq(shtps_fts_i2c_client->irq);
			if (ret < 0) {
				dev_err(&shtps_fts_i2c_client->dev, "%s:upgrade failed.\n", __func__);
				return ret;
			}
			#endif
		}
		break;
	case PROC_READ_REGISTER:
		writelen = 1;
		ret = shtps_fts_i2c_write(shtps_fts_i2c_client, writebuf + 1, writelen);
		if (ret < 0) {
			dev_err(&shtps_fts_i2c_client->dev, "%s:write iic error\n", __func__);
			return ret;
		}
		break;
	case PROC_WRITE_REGISTER:
		writelen = 2;
		ret = shtps_fts_i2c_write(shtps_fts_i2c_client, writebuf + 1, writelen);
		if (ret < 0) {
			dev_err(&shtps_fts_i2c_client->dev, "%s:write iic error\n", __func__);
			return ret;
		}
		break;
	case PROC_AUTOCLB:
		#if 1
			dev_err(&shtps_fts_i2c_client->dev, "%s:autoclb not support.\n", __func__);
		#else
		SHTPS_LOG_DBG_PRINT("%s: autoclb\n", __func__);
		shtps_fts_ctpm_auto_clb(shtps_fts_i2c_client);
		#endif
		break;
	case PROC_READ_DATA:
	case PROC_WRITE_DATA:
		writelen = count - 1;
		ret = shtps_fts_i2c_write(shtps_fts_i2c_client, writebuf + 1, writelen);
		if (ret < 0) {
			dev_err(&shtps_fts_i2c_client->dev, "%s:write iic error\n", __func__);
			return ret;
		}
		break;
	default:
		break;
	}
	

	return count;
}

/*interface of read proc*/
/************************************************************************
*   Name: shtps_fts_debug_read
*  Brief:interface of read proc
* Input: point to the data, no use, no use, read len, no use, no use 
* Output: page point to data
* Return: read char number
***********************************************************************/
static ssize_t shtps_fts_debug_read(struct file *filp, char __user *buff, size_t count, loff_t *ppos)
{
	int ret = 0;
	int num_read_chars = 0;
	int readlen = 0;
//	u8 regvalue = 0x00, regaddr = 0x00;
	unsigned char buf[READ_BUF_SIZE];
	
	switch (proc_operate_mode) {
	case PROC_UPGRADE:
		#if 1
			dev_err(&shtps_fts_i2c_client->dev, "%s:upgrade not support.\n", __func__);
		#else
		//after calling shtps_fts_debug_write to upgrade
		regaddr = 0xA6;
		ret = shtps_fts_read_reg(shtps_fts_i2c_client, regaddr, &regvalue);
		if (ret < 0)
			num_read_chars = sprintf(buf, "%s", "get fw version failed.\n");
		else
			num_read_chars = sprintf(buf, "current fw version:0x%02x\n", regvalue);
		#endif
		break;
	case PROC_READ_REGISTER:
		readlen = 1;
		ret = shtps_fts_i2c_read(shtps_fts_i2c_client, NULL, 0, buf, readlen);
		if (ret < 0) {
			dev_err(&shtps_fts_i2c_client->dev, "%s:read iic error\n", __func__);
			return ret;
		} 
		num_read_chars = 1;
		break;
	case PROC_READ_DATA:
		readlen = count;
		ret = shtps_fts_i2c_read(shtps_fts_i2c_client, NULL, 0, buf, readlen);
		if (ret < 0) {
			dev_err(&shtps_fts_i2c_client->dev, "%s:read iic error\n", __func__);
			return ret;
		}
		
		num_read_chars = readlen;
		break;
	case PROC_WRITE_DATA:
		break;
	default:
		break;
	}
	
	if (copy_to_user(buff, buf, num_read_chars)) {
		dev_err(&shtps_fts_i2c_client->dev, "%s:copy to user error\n", __func__);
		return -EFAULT;
	}

	return num_read_chars;
}
static const struct file_operations shtps_fts_proc_fops = {
		.owner = THIS_MODULE,
		.read = shtps_fts_debug_read,
		.write = shtps_fts_debug_write,
		
};
#else
/*interface of write proc*/
/************************************************************************
*   Name: shtps_fts_debug_write
*  Brief:interface of write proc
* Input: file point, data buf, data len, no use
* Output: no
* Return: data len
***********************************************************************/
static int shtps_fts_debug_write(struct file *filp, 
	const char __user *buff, unsigned long len, void *data)
{
	unsigned char writebuf[WRITE_BUF_SIZE];
	int buflen = len;
	int writelen = 0;
	int ret = 0;
	
	
	if (copy_from_user(&writebuf, buff, buflen)) {
		dev_err(&shtps_fts_i2c_client->dev, "%s:copy from user error\n", __func__);
		return -EFAULT;
	}
	proc_operate_mode = writebuf[0];
	SHTPS_LOG_DBG_PRINT("%s() proc_operate_mode = %d\n", __func__, proc_operate_mode);

	switch (proc_operate_mode) {
	
	case PROC_UPGRADE:
		{
			#if 1
				dev_err(&shtps_fts_i2c_client->dev, "%s:upgrade not support.\n", __func__);
			#else
			char upgrade_file_path[128];
			memset(upgrade_file_path, 0, sizeof(upgrade_file_path));
			sprintf(upgrade_file_path, "%s", writebuf + 1);
			upgrade_file_path[buflen-1] = '\0';
			SHTPS_LOG_DBG_PRINT("%s\n", upgrade_file_path);
			disable_irq(shtps_fts_i2c_client->irq);
			#if GTP_ESD_PROTECT
				apk_debug_flag = 1;
			#endif
			ret = shtps_fts_ctpm_fw_upgrade_with_app_file(shtps_fts_i2c_client, upgrade_file_path);
			#if GTP_ESD_PROTECT
				apk_debug_flag = 0;
			#endif
			enable_irq(shtps_fts_i2c_client->irq);
			if (ret < 0) {
				dev_err(&shtps_fts_i2c_client->dev, "%s:upgrade failed.\n", __func__);
				return ret;
			}
			#endif
		}
		break;
	case PROC_READ_REGISTER:
		writelen = 1;
		ret = shtps_fts_i2c_write(shtps_fts_i2c_client, writebuf + 1, writelen);
		if (ret < 0) {
			dev_err(&shtps_fts_i2c_client->dev, "%s:write iic error\n", __func__);
			return ret;
		}
		break;
	case PROC_WRITE_REGISTER:
		writelen = 2;
		ret = shtps_fts_i2c_write(shtps_fts_i2c_client, writebuf + 1, writelen);
		if (ret < 0) {
			dev_err(&shtps_fts_i2c_client->dev, "%s:write iic error\n", __func__);
			return ret;
		}
		break;
	case PROC_AUTOCLB:
		#if 1
			dev_err(&shtps_fts_i2c_client->dev, "%s:autoclb not support.\n", __func__);
		#else
		SHTPS_LOG_DBG_PRINT("%s: autoclb\n", __func__);
		shtps_fts_ctpm_auto_clb(shtps_fts_i2c_client);
		#endif
		break;
	case PROC_READ_DATA:
	case PROC_WRITE_DATA:
		writelen = len - 1;
		ret = shtps_fts_i2c_write(shtps_fts_i2c_client, writebuf + 1, writelen);
		if (ret < 0) {
			dev_err(&shtps_fts_i2c_client->dev, "%s:write iic error\n", __func__);
			return ret;
		}
		break;
	default:
		break;
	}
	

	return len;
}

/*interface of read proc*/
/************************************************************************
*   Name: shtps_fts_debug_read
*  Brief:interface of read proc
* Input: point to the data, no use, no use, read len, no use, no use 
* Output: page point to data
* Return: read char number
***********************************************************************/
static int shtps_fts_debug_read( char *page, char **start,
	off_t off, int count, int *eof, void *data )
{
	int ret = 0;
	unsigned char buf[READ_BUF_SIZE];
	int num_read_chars = 0;
	int readlen = 0;
//	u8 regvalue = 0x00, regaddr = 0x00;
	SHTPS_LOG_DBG_PRINT("%s() proc_operate_mode = %d\n", __func__, proc_operate_mode);
	
	switch (proc_operate_mode) {
	case PROC_UPGRADE:
		#if 1
			dev_err(&shtps_fts_i2c_client->dev, "%s:upgrade not support.\n", __func__);
		#else
		//after calling shtps_fts_debug_write to upgrade
		regaddr = 0xA6;
		ret = shtps_fts_read_reg(shtps_fts_i2c_client, regaddr, &regvalue);
		if (ret < 0)
			num_read_chars = sprintf(buf, "%s", "get fw version failed.\n");
		else
			num_read_chars = sprintf(buf, "current fw version:0x%02x\n", regvalue);
		#endif
		break;
	case PROC_READ_REGISTER:
		readlen = 1;
		ret = shtps_fts_i2c_read(shtps_fts_i2c_client, NULL, 0, buf, readlen);
		if (ret < 0) {
			dev_err(&shtps_fts_i2c_client->dev, "%s:read iic error\n", __func__);
			return ret;
		} 
		num_read_chars = 1;
		break;
	case PROC_READ_DATA:
		readlen = count;
		ret = shtps_fts_i2c_read(shtps_fts_i2c_client, NULL, 0, buf, readlen);
		if (ret < 0) {
			dev_err(&shtps_fts_i2c_client->dev, "%s:read iic error\n", __func__);
			return ret;
		}
		
		num_read_chars = readlen;
		break;
	case PROC_WRITE_DATA:
		break;
	default:
		break;
	}
	
	memcpy(page, buf, num_read_chars);
	return num_read_chars;
}
#endif
/************************************************************************
* Name: shtps_fts_create_apk_debug_channel
* Brief:  create apk debug channel
* Input: i2c info
* Output: no
* Return: success =0
***********************************************************************/
int shtps_fts_create_apk_debug_channel(struct shtps_fts *ts)
{	
	struct i2c_client *client = ts->fwctl_p->tps_ctrl_p;

	shtps_fts_i2c_client = client;

#if defined(SHTPS_ENGINEER_BUILD_ENABLE)
	#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))//2015.5.20 changed
		shtps_fts_proc_entry = proc_create(PROC_NAME, 0777, NULL, &shtps_fts_proc_fops);
	#else
		shtps_fts_proc_entry = create_proc_entry(PROC_NAME, 0777, NULL);
	#endif
#else
	#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))//2015.5.20 changed
		shtps_fts_proc_entry = proc_create(PROC_NAME, 0700, NULL, &shtps_fts_proc_fops);
	#else
		shtps_fts_proc_entry = create_proc_entry(PROC_NAME, 0700, NULL);
	#endif
#endif /* SHTPS_ENGINEER_BUILD_ENABLE */
	if (NULL == shtps_fts_proc_entry) 
	{
		dev_err(&client->dev, "Couldn't create proc entry!\n");
		
		return -ENOMEM;
	} 
	else 
	{
		dev_info(&client->dev, "Create proc entry success!\n");
		
		#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 10, 0))
			shtps_fts_proc_entry->write_proc = shtps_fts_debug_write;
			shtps_fts_proc_entry->read_proc = shtps_fts_debug_read;
		#endif
	}
	return 0;
}
/************************************************************************
* Name: shtps_fts_release_apk_debug_channel
* Brief:  release apk debug channel
* Input: no
* Output: no
* Return: no
***********************************************************************/
void shtps_fts_release_apk_debug_channel(struct shtps_fts *ts)
{
	
	if (shtps_fts_proc_entry)
//		#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))//2015.5.20 changed
//			proc_remove(PROC_NAME);
//		#else
			remove_proc_entry(PROC_NAME, NULL);
//		#endif
}

/************************************************************************
* Name: shtps_fts_tpfwver_show
* Brief:  show tp fw vwersion
* Input: device, device attribute, char buf
* Output: no
* Return: char number
***********************************************************************/
static ssize_t shtps_fts_tpfwver_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t num_read_chars = 0;
	u8 fwver = 0;
	shtps_mutex_lock_ctrl();
	if (shtps_fts_read_reg(shtps_fts_i2c_client, FTS_REG_FW_VER, &fwver) < 0)
		return -1;
	
	
	if (fwver == 255)
		num_read_chars = snprintf(buf, 128,"get tp fw version fail!\n");
	else
	{
		num_read_chars = snprintf(buf, 128, "%02X\n", fwver);
	}
	
	shtps_mutex_unlock_ctrl();
	
	return num_read_chars;
}
void fih_touch_tpfwver_read(char *fw_ver)
{
    #if 0
    ssize_t num_read_chars = 0;
    u8 fwver = 0;

    shtps_mutex_lock_ctrl();

    if (shtps_fts_read_reg(shtps_fts_i2c_client, FTS_REG_FW_VER, &fwver) < 0)
    {
        num_read_chars = snprintf(fw_ver, PAGE_SIZE,"I2c transfer error!\n");
    }

    if (fwver == 255)
        num_read_chars = snprintf(fw_ver, PAGE_SIZE,"get tp fw version fail!\n");
    else
    {
        num_read_chars = snprintf(fw_ver, PAGE_SIZE, "%02X\n", fwver);
    }
    shtps_mutex_unlock_ctrl();
#endif
	int rc=0;
	u8 buf[2] = { 0x00, 0x00 };

	rc = M_READ_FUNC(gShtps_fts->fwctl_p, FTS_REG_FW_VER, &buf[0], 1);
	if(rc == 0){
		rc = M_READ_FUNC(gShtps_fts->fwctl_p, FTS_REG_MODEL_VER, &buf[1], 1);
		if(rc == 0){
            snprintf(fw_ver, 128, "%02X%02X\n", buf[1], buf[0]);
		}
		else if(rc < 0)
		    pr_err("I2c transfer error! \n");
	}
	else if(rc < 0)
		    pr_err("I2c transfer error2! \n");

}
void fih_touch_vendor_read(char *buf)
{
    sprintf(buf, "%s\n","FocalTech");
}

/************************************************************************
* Name: shtps_fts_tpfwver_store
* Brief:  no
* Input: device, device attribute, char buf, char count
* Output: no
* Return: EPERM
***********************************************************************/
static ssize_t shtps_fts_tpfwver_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	/* place holder for future use */
	return -EPERM;
}
/************************************************************************
* Name: shtps_fts_tpdriver_version_show
* Brief:  show tp fw vwersion
* Input: device, device attribute, char buf
* Output: no
* Return: char number
***********************************************************************/
static ssize_t shtps_fts_tpdriver_version_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t num_read_chars = 0;
	
	shtps_mutex_lock_ctrl();
	
	num_read_chars = snprintf(buf, 128,"%s \n", FTS_DRIVER_INFO);
	
	shtps_mutex_unlock_ctrl();
	
	return num_read_chars;
}
/************************************************************************
* Name: shtps_fts_tpdriver_version_store
* Brief:  no
* Input: device, device attribute, char buf, char count
* Output: no
* Return: EPERM
***********************************************************************/
static ssize_t shtps_fts_tpdriver_version_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	/* place holder for future use */
	return -EPERM;
}
/************************************************************************
* Name: shtps_fts_tprwreg_show
* Brief:  no
* Input: device, device attribute, char buf
* Output: no
* Return: EPERM
***********************************************************************/
static ssize_t shtps_fts_tprwreg_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	/* place holder for future use */
	return -EPERM;
}

static int shtps_fts_strtol(char *param, int len, int base)
{
	int i;
	int tmp;
	int ret = 0;
	
	for(i = 0;i < len;i++){
		if('0' <= param[i] && param[i] <= '9'){
			tmp = param[i] - '0';
		}else if(base == 16){
			if('a' <= param[i] && param[i] <= 'f'){
				tmp = param[i] - 'a' + 10;
			}else if('A' <= param[i] && param[i] <= 'F'){
				tmp = param[i] - 'A' + 10;
			}else{
				tmp = 0;
			}
		}else{
			tmp = 0;
		}
		
		ret = ret * base + tmp;
	}
	
	return ret;
}
/************************************************************************
* Name: shtps_fts_tprwreg_store
* Brief:  read/write register
* Input: device, device attribute, char buf, char count
* Output: print register value
* Return: char count
***********************************************************************/
static ssize_t shtps_fts_tprwreg_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);
	ssize_t num_read_chars = 0;
	int retval;
	long unsigned int wmreg=0;
	u8 regaddr=0xff,regvalue=0xff;
	u8 valbuf[5]={0};
	int i = 0;

	memset(valbuf, 0, sizeof(valbuf));
	shtps_mutex_lock_ctrl();	
	num_read_chars = count - 1;
	dev_err(dev,"count=%d\n",(int)count);
        for(i = 0;i< count;i++) {
	  dev_err(dev,"%x",buf[i]);
	}
	dev_err(dev,"\n");
	if (num_read_chars != 2) 
	{
		if (num_read_chars != 4) 
		{
		  dev_err(dev, "please input 2 or 4 character(%d %s)\n",(int)num_read_chars, buf);
			goto error_return;
		}
	}
	memcpy(valbuf, buf, num_read_chars);
#if 1
	wmreg = shtps_fts_strtol(valbuf, num_read_chars, 16);
	retval = 0;
#else
	retval = strict_strtoul(valbuf, 16, &wmreg);
#endif
	if (0 != retval) 
	{
		dev_err(dev, "%s() - ERROR: Could not convert the given input to a number. The given input was: \"%s\"\n", __FUNCTION__, buf);
		goto error_return;
	}
	if (2 == num_read_chars) 
	{
		/*read register*/
		regaddr = wmreg;
		printk("[focal][test](0x%02x)\n", regaddr);
		if (shtps_fts_read_reg(client, regaddr, &regvalue) < 0)
			printk("[Focal] %s : Could not read the register(0x%02x)\n", __func__, regaddr);
		else
			printk("[Focal] %s : the register(0x%02x) is 0x%02x\n", __func__, regaddr, regvalue);
	} 
	else 
	{
		regaddr = wmreg>>8;
		regvalue = wmreg;
		if (shtps_fts_write_reg(client, regaddr, regvalue)<0)
			dev_err(dev, "[Focal] %s : Could not write the register(0x%02x)\n", __func__, regaddr);
		else
			dev_dbg(dev, "[Focal] %s : Write 0x%02x into register(0x%02x) successful\n", __func__, regvalue, regaddr);
	}
	error_return:
	shtps_mutex_unlock_ctrl();
	
	return count;
}
/************************************************************************
* Name: shtps_fts_fwupdate_show
* Brief:  no
* Input: device, device attribute, char buf
* Output: no
* Return: EPERM
***********************************************************************/
static ssize_t shtps_fts_fwupdate_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	/* place holder for future use */
	return -EPERM;
}

/************************************************************************
* Name: shtps_fts_fwupdate_store
* Brief:  upgrade from *.i
* Input: device, device attribute, char buf, char count
* Output: no
* Return: char count
***********************************************************************/
static ssize_t shtps_fts_fwupdate_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	#if 1
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);
	dev_err(&client->dev, "%s:upgrade not support.\n", __func__);
	#else
	u8 uc_host_fm_ver;
	int i_ret;
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);
	shtps_mutex_lock_ctrl();
	
	disable_irq(client->irq);
	#if GTP_ESD_PROTECT
		apk_debug_flag = 1;
	#endif
	
	i_ret = shtps_fts_ctpm_fw_upgrade_with_i_file(client);
	if (i_ret == 0)
	{
		msleep(300);
		uc_host_fm_ver = shtps_fts_ctpm_get_i_file_ver();
		dev_dbg(dev, "%s [FTS] upgrade to new version 0x%x\n", __func__, uc_host_fm_ver);
	}
	else
	{
		dev_err(dev, "%s ERROR:[FTS] upgrade failed ret=%d.\n", __func__, i_ret);
	}
	
	#if GTP_ESD_PROTECT
		apk_debug_flag = 0;
	#endif
	enable_irq(client->irq);
	shtps_mutex_unlock_ctrl();
	
	#endif
	return count;
}
/************************************************************************
* Name: shtps_fts_fwupgradeapp_show
* Brief:  no
* Input: device, device attribute, char buf
* Output: no
* Return: EPERM
***********************************************************************/
static ssize_t shtps_fts_fwupgradeapp_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	/* place holder for future use */
	return -EPERM;
}

/************************************************************************
* Name: shtps_fts_fwupgradeapp_store
* Brief:  upgrade from app.bin
* Input: device, device attribute, char buf, char count
* Output: no
* Return: char count
***********************************************************************/
static ssize_t shtps_fts_fwupgradeapp_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	#if 1
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);
	dev_err(&client->dev, "%s:upgrade not support.\n", __func__);
	#else
	char fwname[128];
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);
	memset(fwname, 0, sizeof(fwname));
	sprintf(fwname, "%s", buf);
	fwname[count-1] = '\0';

	shtps_mutex_lock_ctrl();
	
	disable_irq(client->irq);
	#if GTP_ESD_PROTECT
				apk_debug_flag = 1;
			#endif
	shtps_fts_ctpm_fw_upgrade_with_app_file(client, fwname);
	#if GTP_ESD_PROTECT
				apk_debug_flag = 0;
			#endif
	enable_irq(client->irq);
	
	shtps_mutex_unlock_ctrl();
	#endif
	return count;
}
/************************************************************************
* Name: shtps_fts_ftsgetprojectcode_show
* Brief:  no
* Input: device, device attribute, char buf
* Output: no
* Return: EPERM
***********************************************************************/
static ssize_t shtps_fts_getprojectcode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	
	return -EPERM;
}
/************************************************************************
* Name: shtps_fts_ftsgetprojectcode_store
* Brief:  no
* Input: device, device attribute, char buf, char count
* Output: no
* Return: EPERM
***********************************************************************/
static ssize_t shtps_fts_getprojectcode_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	/* place holder for future use */
	return -EPERM;
}

/****************************************/
/* sysfs */
/*get the fw version
*example:cat ftstpfwver
*/
static DEVICE_ATTR(ftstpfwver, S_IRUGO|S_IWUSR, shtps_fts_tpfwver_show, shtps_fts_tpfwver_store);

static DEVICE_ATTR(ftstpdriverver, S_IRUGO|S_IWUSR, shtps_fts_tpdriver_version_show, shtps_fts_tpdriver_version_store);
/*upgrade from *.i
*example: echo 1 > ftsfwupdate
*/
static DEVICE_ATTR(ftsfwupdate, S_IRUGO|S_IWUSR, shtps_fts_fwupdate_show, shtps_fts_fwupdate_store);
/*read and write register
*read example: echo 88 > ftstprwreg ---read register 0x88
*write example:echo 8807 > ftstprwreg ---write 0x07 into register 0x88
*
*note:the number of input must be 2 or 4.if it not enough,please fill in the 0.
*/
static DEVICE_ATTR(ftstprwreg, S_IRUGO|S_IWUSR, shtps_fts_tprwreg_show, shtps_fts_tprwreg_store);
/*upgrade from app.bin
*example:echo "*_app.bin" > ftsfwupgradeapp
*/
static DEVICE_ATTR(ftsfwupgradeapp, S_IRUGO|S_IWUSR, shtps_fts_fwupgradeapp_show, shtps_fts_fwupgradeapp_store);
static DEVICE_ATTR(ftsgetprojectcode, S_IRUGO|S_IWUSR, shtps_fts_getprojectcode_show, shtps_fts_getprojectcode_store);



/*add your attr in here*/
static struct attribute *shtps_fts_attributes[] = {
	&dev_attr_ftstpfwver.attr,
	&dev_attr_ftstpdriverver.attr,
	&dev_attr_ftsfwupdate.attr,
	&dev_attr_ftstprwreg.attr,
	&dev_attr_ftsfwupgradeapp.attr,
	&dev_attr_ftsgetprojectcode.attr,
	NULL
};

static struct attribute_group shtps_fts_attribute_group = {
	.attrs = shtps_fts_attributes
};

/************************************************************************
* Name: shtps_fts_create_sysfs
* Brief:  create sysfs for debug
* Input: i2c info
* Output: no
* Return: success =0
***********************************************************************/
int shtps_fts_create_sysfs(struct shtps_fts *ts)
{
	int err;
	struct i2c_client * client;

	shtps_fts_i2c_client = ts->fwctl_p->tps_ctrl_p;
	client = shtps_fts_i2c_client;

	err = sysfs_create_group(&client->dev.kobj, &shtps_fts_attribute_group);
	if (0 != err) 
	{
		dev_err(&client->dev, "%s() - ERROR: sysfs_create_group() failed.\n", __func__);
		sysfs_remove_group(&client->dev.kobj, &shtps_fts_attribute_group);
		return -EIO;
	} 
	else 
	{
		pr_info("fts:%s() - sysfs_create_group() succeeded.\n",__func__);
	}
	return err;
}
/************************************************************************
* Name: shtps_fts_remove_sysfs
* Brief:  remove sys
* Input: i2c info
* Output: no
* Return: no
***********************************************************************/
int shtps_fts_remove_sysfs(struct shtps_fts *ts)
{
	struct i2c_client * client = ts->fwctl_p->tps_ctrl_p;
	sysfs_remove_group(&client->dev.kobj, &shtps_fts_attribute_group);
	return 0;
}
/* -----------------------------------------------------------------------------------
*/
MODULE_DESCRIPTION("FocalTech fts TouchScreen driver");
MODULE_LICENSE("GPLv2");
