/*
 *  shub-spi.c - Linux kernel modules for Sensor Hub 
 *
 *  Copyright (C) 2014 LAPIS SEMICONDUCTOR CO., LTD.
 *
 *  This file is based on :
 *    ml610q792.c - Linux kernel modules for acceleration sensor
 *    http://android-dev.kyocera.co.jp/source/versionSelect_URBANO.html
 *    (C) 2012 KYOCERA Corporation
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
#include <linux/moduleparam.h>
#include <linux/input-polldev.h>

#include <asm/uaccess.h> 
#include <linux/fs.h>
#include <linux/ioctl.h>
#include <linux/delay.h>
#include <linux/wakelock.h>

#include <linux/module.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/hrtimer.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/poll.h>
//#include <asm/gpio.h>            /* SHMDS_HUB_0111_01 del */
#include <linux/types.h>
#include "ml630q790.h"
#include <linux/sched.h>
//#include <linux/earlysuspend.h>  /* SHMDS_HUB_0111_01 del */
#include <linux/miscdevice.h>

#define SSIO_MASK_WRITE              (0x7f)
#define SSIO_MASK_READ               (0x80)

static int32_t spi_probe( struct spi_device *client );
static int32_t spi_remove( struct spi_device *client );
/* SHMDS_HUB_0130_01 del S */
//static int32_t spi_suspend( struct spi_device *client, pm_message_t mesg );
//static int32_t spi_resume( struct spi_device *client );
/* SHMDS_HUB_0130_01 del E */
/* SHMDS_HUB_0130_01 mod S */
static int32_t pm_suspend( struct device *dev );
static int32_t pm_resume( struct device *dev );
/* SHMDS_HUB_0130_01 mod E */
static void    spi_shutdown( struct spi_device *client );

static struct spi_device *client_mcu;

/* SHMDS_HUB_0109_02 add S */
static int shub_acc_axis_val = 0;
static int shub_gyro_axis_val = 0;
static int shub_mag_axis_val = 0;
/* SHMDS_HUB_0109_02 add E */

#if 1  // SHMDS_HUB_0104_05 add
#ifdef CONFIG_OF
static const struct of_device_id shshub_dev_dt_match[] = {
	{ .compatible = "sharp,sensorhub",},
	{}
};
#else
#define shshub_dev_dt_match NULL;
#endif /* CONFIG_OF */
#endif

// SHMDS_HUB_0130_01 add 
static const struct dev_pm_ops shub_pm_ops = {
    .suspend     = pm_suspend,
    .resume      = pm_resume,
};
// SHMDS_HUB_0130_01 add 

static struct spi_driver interface_driver = {
    .probe       = spi_probe,
    .driver = {
        .name    = SENOSR_HUB_DRIVER_NAME,
        .bus     = &spi_bus_type,
        .owner   = THIS_MODULE,
        .pm      = &shub_pm_ops,               // SHMDS_HUB_0130_01 add
        .of_match_table = shshub_dev_dt_match, // SHMDS_HUB_0104_05 add
    },
    .remove      = spi_remove,
//    .suspend     = spi_suspend,              // SHMDS_HUB_0130_01 del
//    .resume      = spi_resume,               // SHMDS_HUB_0130_01 del
    .shutdown    = spi_shutdown,
};

static int32_t spi_remove( struct spi_device *client )
{
    return shub_remove();
}

/* SHMDS_HUB_0130_01 del S */
//static int32_t spi_suspend( struct spi_device *client, pm_message_t mesg )
//{
//    shub_suspend(client, mesg);
//    return 0;
//}

//static int32_t spi_resume( struct spi_device *client )
//{
//    shub_resume(client);
//    return 0;
//}
/* SHMDS_HUB_0130_01 del E */
/* SHMDS_HUB_0130_01 add S */
static int32_t pm_suspend( struct device *dev )
{
    pm_message_t mesg;
    shub_suspend(dev, mesg);
    return 0;
}

static int32_t pm_resume( struct device *dev )
{
    shub_resume(dev);
    return 0;
}
/* SHMDS_HUB_0130_01 add E */

static void spi_shutdown( struct spi_device *client )
{
}

static int32_t spi_probe( struct spi_device *client )
{
/* SHMDS_HUB_0109_02 add S */
    int rc;
    struct device_node *np = client->dev.of_node;
    u32 temp_val;
    rc = of_property_read_u32(np, "shub,shub_acc_axis_val", &temp_val);
    if (rc) {
        printk("[shub]Unable to read. shub,shub_acc_axis_val\n");
        shub_acc_axis_val = 0;
    }
    else {
        shub_acc_axis_val = temp_val;
    }

    rc = of_property_read_u32(np, "shub,shub_gyro_axis_val", &temp_val);
    if (rc) {
        printk("[shub]Unable to read. shub,shub_gyro_axis_val\n");
        shub_gyro_axis_val = 0;
    }
    else {
        shub_gyro_axis_val = temp_val;
    }

    rc = of_property_read_u32(np, "shub,shub_mag_axis_val", &temp_val);
    if (rc) {
        printk("[shub]Unable to read. shub,shub_mag_axis_val\n");
        shub_mag_axis_val = 0;
    }
    else {
        shub_mag_axis_val = temp_val;
    }
/* SHMDS_HUB_0109_02 add E */

    shub_set_gpio_no(client); /* SHMDS_HUB_0110_01 add */

    client_mcu = client;
    client_mcu->bits_per_word = 8;

    client_mcu->max_speed_hz = 4*1000*1000;

    return shub_probe();
}

// SHMDS_HUB_0701_05 add S
void shub_sensor_rep_spi(struct seq_file *s)
{
    seq_printf(s, "[spi       ]");
    seq_printf(s, "shub_acc_axis_val=%d, ",shub_acc_axis_val);
    seq_printf(s, "shub_gyro_axis_val=%d, ",shub_gyro_axis_val);
    seq_printf(s, "shub_mag_axis_val=%d\n",shub_mag_axis_val);
}
// SHMDS_HUB_0701_05 add E

/* SHMDS_HUB_3701_01 add S */
int shub_spi_init(void)
{
    int ret;
    ret = spi_register_driver(&interface_driver);
    if(ret != 0){
        printk("[shub] can't regist spi driver ret=%d\n", ret);
    }
    return ret;
}

void shub_spi_exit(void)
{
    spi_unregister_driver(&interface_driver);
}
/* SHMDS_HUB_3701_01 add E */

int32_t hostif_write_proc(uint8_t adr, const uint8_t *data, uint16_t size)
{
    int32_t ret = 0;
    struct spi_message  message;
    struct spi_transfer transfer;
    uint8_t send_data[512+128];

    if((data == NULL) || (size == 0)){
        printk("[shub] SPI write input error(data=%p, size=%d)\n", data, size);
        return -1;
    }

    send_data[0] = adr;
    memcpy(&send_data[1], data, size);
    memset(&transfer, 0, sizeof(transfer));

    adr &= SSIO_MASK_WRITE;

/* SHMDS_HUB_0110_02 del S */
#ifndef SHUB_SW_PINCTRL
    ret = spi_setup(client_mcu);
    if(ret < 0) {
        printk("[shub] init SPI failed. ret=%d\n", ret);
        return ret;
    }
#endif
/* SHMDS_HUB_0110_02 del E */
    spi_message_init(&message);

    transfer.tx_buf = send_data;
    transfer.rx_buf = NULL;
    transfer.len    = 1 + size;
    transfer.speed_hz = 2*1000*1000; // SHMDS_HUB_0104_10 add
    spi_message_add_tail(&transfer, &message);

    ret = spi_sync(client_mcu, &message);
    if(ret < 0){
        printk("[shub] SPI write error(ret=%d, adr=0x%02x, size=%d)\n", ret, adr, size);
    }   

    return ret;
}
EXPORT_SYMBOL(hostif_write_proc);

int32_t hostif_read_proc(uint8_t adr, uint8_t *data, uint16_t size)
{
    int32_t ret = 0;
    struct spi_message  message;
/* SHMDS_HUB_0347_01 mod S */
    struct spi_transfer transfer[1];
    int i;
    uint16_t r_len = size;
    uint8_t *txbuf_p;
    uint8_t *rxbuf_p;
    uint8_t r_buff[FIFO_SIZE+2]; // maxbuff + addr + null

    if( (data == NULL) || (size == 0)){
        printk("[shub] SPI read input error(data=%p, size=%d)\n", data, size);
        return -1;
    }

    if(r_len > FIFO_SIZE){
        printk("[shub] SPI read size error(size=%d)\n", size);
        r_len = FIFO_SIZE;
    }

    memset(&r_buff[0], 0, sizeof(r_buff));
    memset(&transfer, 0, sizeof(transfer));

    adr |= SSIO_MASK_READ;
    r_buff[0] = adr;
    txbuf_p = &r_buff[0];
    rxbuf_p = &r_buff[1];
/* SHMDS_HUB_0347_01 mod E */

/* SHMDS_HUB_0110_02 del S */
#ifndef SHUB_SW_PINCTRL
    ret = spi_setup(client_mcu);
    if(ret < 0){
        printk("[shub] init SPI failed. ret=%d\n", ret);
        return ret;
    }
#endif
/* SHMDS_HUB_0110_02 del E */
    spi_message_init(&message);

/* SHMDS_HUB_0347_01 mod S */
    transfer[0].tx_buf = txbuf_p;
    transfer[0].rx_buf = rxbuf_p;
    transfer[0].len    = r_len + 1;
    transfer[0].speed_hz = 2*1000*1000; // SHMDS_HUB_0104_10 add
    spi_message_add_tail(&transfer[0], &message);
/* SHMDS_HUB_0347_01 mod E */

    ret = spi_sync(client_mcu, &message);
    if(ret < 0){
        printk("[shub] read error(ret=%d, adr=0x%02x, size=%d)\n", ret, adr, (r_len+1));
    }

/* SHMDS_HUB_0347_01 add S */
    rxbuf_p += 1;
    for(i = 0;i < r_len;i++){
        *data++ = *rxbuf_p++;
    }
/* SHMDS_HUB_0347_01 add E */

    return ret;
}

/* SHMDS_HUB_0109_02 add S */
int shub_get_acc_axis_val(void)
{
    printk("[shub]acc_axis_val=%d\n", shub_acc_axis_val);
    return shub_acc_axis_val;
}

int shub_get_gyro_axis_val(void)
{
    printk("[shub]gyro_axis_val=%d\n", shub_gyro_axis_val);
    return shub_gyro_axis_val;
}

int shub_get_mag_axis_val(void)
{
    printk("[shub]mag_axis_val=%d\n", shub_mag_axis_val);
    return shub_mag_axis_val;
}
/* SHMDS_HUB_0109_02 add E */

EXPORT_SYMBOL(hostif_read_proc);

// module_spi_driver(interface_driver); /* SHMDS_HUB_3701_01 del */

MODULE_DESCRIPTION("SensorHub SPI Driver");
MODULE_AUTHOR("LAPIS SEMICONDUCTOR");
MODULE_LICENSE("GPL v2");
