/*
 *  shub-input_motion_detect.c - Linux kernel modules for interface of ML630Q790
 *
 *  Copyright (C) 2014 LAPIS SEMICONDUCTOR CO., LTD.
 *
 *  This file is based on :
 *    alps-input.c - Linux kernel modules for interface of ML610Q792
 *    http://android-dev.kyocera.co.jp/source/versionSelect_URBANO.html
 *    (C) 2012 KYOCERA Corporation
 *    Copyright (C) 2010 ALPS
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/ioctl.h>
#include <linux/delay.h>
#include <linux/poll.h>
#include <asm/uaccess.h> 

#include "shub_io.h"
#include "ml630q790.h"

#ifdef CONFIG_ANDROID_ENGINEERING
static int shub_devori_log = 0;
module_param(shub_devori_log, int, 0600);
#define DBG_DEVORI_IO(msg, ...) {                      \
    if(shub_devori_log & 0x01)                         \
        printk("[shub][devori] " msg, ##__VA_ARGS__);  \
}
#define DBG_DEVORI_DATA(msg, ...) {                    \
    if(shub_devori_log & 0x02)                         \
        printk("[shub][devori] " msg, ##__VA_ARGS__);  \
}
#else
#define DBG_DEVORI_IO(msg, ...)
#define DBG_DEVORI_DATA(msg, ...)
#endif

#define INPUT_DEV_NAME "shub_devori"
#define INPUT_DEV_PHYS "shub_devori/input0"
#define MISC_DEV_NAME  "shub_io_devori"

#define INDEX_D1           0
#define INDEX_D2           1
#define INDEX_TM           2
#define INDEX_TMNS         3
#define INDEX_SUM          4

#define APP_DEVORI_ENABLE           0x27
#define APP_STADET_ENABLE           0x28
#define APP_MOTDET_ENABLE           0x29

static DEFINE_MUTEX(shub_lock);
static int32_t power_state = 0;

static struct platform_device *pdev;
static struct input_dev *shub_idev;

static int32_t shub_probe_devori(struct platform_device *pfdev);
static int32_t shub_remove_devori(struct platform_device *pfdev);

#ifdef CONFIG_OF
    static struct of_device_id shub_of_match_tb_devori[] = {
        { .compatible = "sharp,shub_dev_orien" ,},
        {}
    };
#else
    #define shub_of_match_tb_devori NULL
#endif

static long shub_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    void __user *argp = (void __user *)arg;
    int32_t ret = -1, tmpval = 0;
    IoCtlParam param;
    
    switch (cmd) {
    case SHUBIO_DEVORI_ACTIVATE:
        ret = copy_from_user(&tmpval, argp, sizeof(tmpval));
        if (ret) {
            printk("error : shub_ioctl(cmd = SHUBIO_DEVORI_ACTIVATE)\n");
            return -EFAULT;
        }
        DBG_DEVORI_IO("ioctl(cmd = Set_Active) : val=%d\n", tmpval);
        
        if(power_state == tmpval) {
            DBG_DEVORI_IO("ioctl(cmd = Set_Active) : no chenge stat");
            return 0;
        }
        
        mutex_lock(&shub_lock);
        memset(&param, 0, sizeof(IoCtlParam));
        param.m_iType = APP_DEVORI_ENABLE;
        ret = shub_get_param(param.m_iType, param.m_iParam);
        if(ret != 0) {
            printk( "[shub]shub_get_param : APP_DEVORI_ENABLE error. ret=%d\n", ret);
            mutex_unlock(&shub_lock);
            return ret;
        }
        if (tmpval == 0) {
            // disable
            param.m_iType = APP_DEVORI_ENABLE;
            param.m_iParam[0] = 0;
            ret = shub_set_param(param.m_iType, param.m_iParam);
            if(ret != 0) {
                printk( "[shub]shub_set_param : APP_DEVORI_ENABLE error. ret=%d\n", ret);
                mutex_unlock(&shub_lock);
                return ret;
            }
        } else {
            // enable
            param.m_iType = APP_DEVORI_ENABLE;
            param.m_iParam[0] = 1;
            ret = shub_set_param(param.m_iType, param.m_iParam);
            if(ret != 0) {
                printk( "[shub]shub_set_param : APP_DEVORI_ENABLE error. ret=%d\n", ret);
                mutex_unlock(&shub_lock);
                return ret;
            }
        }
        power_state = tmpval;
        mutex_unlock(&shub_lock);
        break;
    default:
        return -ENOTTY;
    }
    return 0;
}

static long shub_ioctl_wrapper(struct file *filp, unsigned int cmd, unsigned long arg)
{
    SHUB_DBG_TIME_INIT
    long ret = 0;
    
    shub_qos_start();
    SHUB_DBG_TIME_START
    ret = shub_ioctl(filp, cmd , arg);
    SHUB_DBG_TIME_END(cmd)
    shub_qos_end();
    return ret;
}

void shub_input_report_devoriect(int32_t *data)
{
    int32_t val = 0;
    int32_t d1, d2;
    
    if(data == NULL) {
        return;
    }
    
    d1 = data[INDEX_D1];
    d2 = data[INDEX_D2];
    
    if((d1 == 2) && (d2 == 1)) {
        val = 0;
    }else if((d1 == 1) && (d2 == 1)) {
        val = 1;
    }else if((d1 == 2) && (d2 == 2)) {
        val = 2;
    }else if((d1 == 1) && (d2 == 2)) {
        val = 3;
    }
    
    DBG_DEVORI_DATA("data d=%d, t(s)=%d, t(ns)=%d\n", val, data[INDEX_TM],data[INDEX_TMNS]);
    
    SHUB_INPUT_VAL_CLEAR(shub_idev, ABS_Y, val);
    input_report_abs(shub_idev, ABS_Y, val);
    input_report_abs(shub_idev, ABS_RY, data[INDEX_TM]);
    input_report_abs(shub_idev, ABS_RZ, data[INDEX_TMNS]);
    shub_input_sync_init(shub_idev);
    input_event(shub_idev, EV_SYN, SYN_REPORT, SHUB_INPUT_DEVICE_ORI);
}

static struct file_operations shub_fops = {
    .owner   = THIS_MODULE,
    .unlocked_ioctl = shub_ioctl_wrapper,
#ifdef CONFIG_COMPAT
    .compat_ioctl   = shub_ioctl_wrapper,
#endif
};

static struct miscdevice shub_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name  = MISC_DEV_NAME,
    .fops  = &shub_fops,
};

static int32_t shub_probe_devori(struct platform_device *pfdev)
{
    int32_t ret = 0;
    
    if(!shub_connect_check()){
        printk(KERN_INFO "Device Orientation Connect Error!!\n");
        ret = -ENODEV;
        goto out_driver;
    }
    pdev = platform_device_register_simple(INPUT_DEV_NAME, -1, NULL, 0);
    if (IS_ERR(pdev)) {
        ret = PTR_ERR(pdev);
        goto out_driver;
    }
    shub_idev = shub_com_allocate_device(SHUB_INPUT_DEVICE_ORI, &pdev->dev);
    if (!shub_idev) {
        ret = -ENOMEM;
        goto out_device;
    }
    ret = misc_register(&shub_device);
    if (ret) {
        printk("shub-init: shub_io_device register failed\n");
        goto exit_misc_device_register_failed;
    }
    return 0;
    
exit_misc_device_register_failed:
out_device:
    platform_device_unregister(pdev);
out_driver:
    return ret;
}

static int32_t shub_remove_devori(struct platform_device *pfdev)
{
    misc_deregister(&shub_device);
    shub_com_unregister_device(SHUB_INPUT_DEVICE_ORI);
    platform_device_unregister(pdev);
    
    return 0;
}

static struct platform_driver shub_devori_driver = {
    .probe = shub_probe_devori,
    .remove = shub_remove_devori,
    .shutdown = NULL,
    .driver = {
        .name = "shub_dev_devori",
        .of_match_table = shub_of_match_tb_devori,
    },
};

int shub_devori_init(void)          /* SHMDS_HUB_3701_01 mod */
{
    int32_t ret = 0;
    ret = platform_driver_register(&shub_devori_driver);
    return ret;
}

void shub_devori_exit(void)         /* SHMDS_HUB_3701_01 mod */
{
    platform_driver_unregister(&shub_devori_driver);
}

// late_initcall(shub_devori_init); /* SHMDS_HUB_3701_01 del */
// module_exit(shub_devori_exit);   /* SHMDS_HUB_3701_01 del */

MODULE_DESCRIPTION("SensorHub Input Device (Device Orientation)");
MODULE_AUTHOR("LAPIS SEMICOMDUCTOR");
MODULE_LICENSE("GPL v2");
