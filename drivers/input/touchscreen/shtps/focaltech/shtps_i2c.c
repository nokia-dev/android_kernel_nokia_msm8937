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
#include <linux/i2c.h>
#include <linux/slab.h>

/* -------------------------------------------------------------------------- */
#include <linux/input/shtps_dev.h>

#include "shtps_param_extern.h"
#include "shtps_log.h"
#include "shtps_i2c.h"

/* -------------------------------------------------------------------------- */
#if defined( SHTPS_LOG_ERROR_ENABLE )
	#define SHTPS_LOG_I2C_FUNC_CALL()	// SHTPS_LOG_FUNC_CALL()
#else
	#define SHTPS_LOG_I2C_FUNC_CALL()
#endif
/* -------------------------------------------------------------------------- */
struct shtps_fts;

static DEFINE_MUTEX(i2c_rw_access);

static int shtps_i2c_write_main(void *client, u16 addr, u8 *writebuf, u32 size);
static int shtps_i2c_write(void *client, u16 addr, u8 data);
static int shtps_i2c_write_packet(void *client, u16 addr, u8 *data, u32 size);

static int shtps_i2c_read(void *client, u16 addr, u8 *buf, u32 size);
static int shtps_i2c_read_packet (void *client, u16 addr, u8 *buf,  u32 size);

#if defined( SHTPS_DEVICE_ACCESS_LOG_ENABLE )
#define I2C_IO_WRITE	0
#define I2C_IO_READ		1
#define I2C_IO_READ_W	2
#define I2C_IO_READ_R	3

static int shtps_i2c_transfer_log_enable = 0;

void shtps_set_i2c_transfer_log_enable(int para)
{
	shtps_i2c_transfer_log_enable = para;
}

int shtps_get_i2c_transfer_log_enable(void)
{
	return shtps_i2c_transfer_log_enable;
}

static void shtps_i2c_transfer_log(int io, u16 addr, u8 *buf, int size)
{
	if(shtps_i2c_transfer_log_enable == 1){
		u8 *strbuf = NULL;
		u8 datastrbuf[10];
		int allocSize = 0;
		int i;

		allocSize = 100 + (3 * size);
		strbuf = (u8 *)kzalloc(allocSize, GFP_KERNEL);
		if(strbuf == NULL){
			SHTPS_LOG_DBG_PRINT("shtps_i2c_transfer_log() alloc error. size = %d\n", allocSize);
			return;
		}

		if(addr == 0x7FFF){
			sprintf(strbuf, "[I2C] %s ---- :", (io == I2C_IO_WRITE ? "W " : (io == I2C_IO_READ ? " R" : (io == I2C_IO_READ_W ? "W>" : "<R"))));
		}
		else{
			sprintf(strbuf, "[I2C] %s %04X :", (io == I2C_IO_WRITE ? "W " : (io == I2C_IO_READ ? " R" : (io == I2C_IO_READ_W ? "W>" : "<R"))), addr);
		}
		for(i = 0; i < size; i++){
			sprintf(datastrbuf, " %02X", buf[i]);
			strcat(strbuf, datastrbuf);
		}

		DBG_PRINTK("%s\n", strbuf);

		if(strbuf != NULL){
			kfree(strbuf);
		}
	}
}
#endif /* #if defined( SHTPS_DEVICE_ACCESS_LOG_ENABLE ) */

/* -----------------------------------------------------------------------------------
 */
static int shtps_i2c_transfer(struct i2c_client *client, char *writebuf, int writelen, char *readbuf, int readlen)
{
	int ret;

	mutex_lock(&i2c_rw_access);

	if(writelen > 0){
		if(readlen > 0){
			struct i2c_msg msgs[] = {
				{
					.addr = client->addr,
					.flags = 0,
					.len = writelen,
					.buf = writebuf,
				},
				{
					.addr = client->addr,
					.flags = I2C_M_RD,
					.len = readlen,
					.buf = readbuf,
				 },
			};
			ret = i2c_transfer(client->adapter, msgs, 2);
			if(ret < 0){
				SHTPS_LOG_ERR_PRINT("%s: i2c read error.\n", __func__);
			}
		}
		else{
			struct i2c_msg msgs[] = {
				{
					.addr = client->addr,
					.flags = 0,
					.len = writelen,
					.buf = writebuf,
				},
			};
			ret = i2c_transfer(client->adapter, msgs, 1);
			if(ret < 0){
				SHTPS_LOG_ERR_PRINT("%s: i2c write error.\n", __func__);
			}
		}
	}
	else{
		struct i2c_msg msgs[] = {
			{
				.addr = client->addr,
				.flags = I2C_M_RD,
				.len = readlen,
				.buf = readbuf,
			},
		};
		ret = i2c_transfer(client->adapter, msgs, 1);
		if(ret < 0){
			SHTPS_LOG_ERR_PRINT("%s:i2c read error.\n", __func__);
		}
	}

	mutex_unlock(&i2c_rw_access);

	return ret;
}
/* -----------------------------------------------------------------------------------
 */
static int shtps_i2c_write_main(void *client, u16 addr, u8 *writebuf, u32 size)
{
	int ret = 0;
	u8 *buf;
	u8 buf_local[1 + SHTPS_I2C_BLOCKWRITE_BUFSIZE];
	int allocSize = 0;

	if(size > SHTPS_I2C_BLOCKWRITE_BUFSIZE){
		allocSize = sizeof(u8) * (1 + size);
		buf = (u8 *)kzalloc(allocSize, GFP_KERNEL);
		if(buf == NULL){
			SHTPS_LOG_DBG_PRINT("%s alloc error. size = %d\n", __func__, allocSize);
			return -ENOMEM;
		}
	}
	else{
		buf = buf_local;
	}

	buf[0] = addr;
	memcpy(&buf[1], writebuf, size);

	ret = shtps_i2c_transfer((struct i2c_client *)client, buf, (1 + size), NULL, 0);
	if(ret < 0){
		SHTPS_LOG_ERR_PRINT("i2c write error. addr=0x%02x size=%d\n", addr, size);
	}
	else{
		ret = 0;
		#if defined( SHTPS_DEVICE_ACCESS_LOG_ENABLE )
			shtps_i2c_transfer_log(I2C_IO_WRITE, addr, writebuf, size);
		#endif /* #if defined( SHTPS_DEVICE_ACCESS_LOG_ENABLE ); */
	}

	if(allocSize > 0){
		kfree(buf);
	}
	return ret;
}

static int shtps_i2c_write(void *client, u16 addr, u8 data)
{
	SHTPS_LOG_I2C_FUNC_CALL();
	return shtps_i2c_write_packet(client, addr, &data, 1);
}

static int shtps_i2c_write_packet(void *client, u16 addr, u8 *data, u32 size)
{
	int status = 0;
	int retry = SHTPS_I2C_RETRY_COUNT;

	SHTPS_LOG_I2C_FUNC_CALL();

	do{
		status = shtps_i2c_write_main(client,
			addr,
			data,
			size);
		//
		if(status){
			struct timespec tu;
			tu.tv_sec = (time_t)0;
			tu.tv_nsec = SHTPS_I2C_RETRY_WAIT * 1000000;
			hrtimer_nanosleep(&tu, NULL, HRTIMER_MODE_REL, CLOCK_MONOTONIC);
		}
	}while(status != 0 && retry-- > 0);

	if(status){
		SHTPS_LOG_ERR_PRINT("i2c write error\n");
	}
	#if defined( SHTPS_LOG_DEBUG_ENABLE )
	else if(retry < (SHTPS_I2C_RETRY_COUNT)){
		SHTPS_LOG_DBG_PRINT("i2c retry num = %d\n", SHTPS_I2C_RETRY_COUNT - retry);
	}
	#endif /* SHTPS_LOG_DEBUG_ENABLE */

	return status;
}

static int shtps_i2c_read_main(void *client, u16 addr, u8 *readbuf, u32 size)
{
	int ret;

	if(addr != 0x7FFF){
		u8 writebuf[1];

		writebuf[0] = addr;

		ret = shtps_i2c_transfer((struct i2c_client *)client, writebuf, 1, readbuf, size);
		if(ret < 0){
			SHTPS_LOG_ERR_PRINT("i2c read error. addr=0x%02x size=%u\n", addr, size);
		}
		else{
			ret = 0;

			#if defined( SHTPS_DEVICE_ACCESS_LOG_ENABLE )
				shtps_i2c_transfer_log(I2C_IO_READ, addr, readbuf, size);
			#endif /* #if defined( SHTPS_DEVICE_ACCESS_LOG_ENABLE ); */
		}
	}
	else{
		ret = shtps_i2c_transfer((struct i2c_client *)client, NULL, 0, readbuf, size);
		if(ret < 0){
			SHTPS_LOG_ERR_PRINT("i2c read error. addr=0x%02x size=%u\n", addr, size);
		}
		else{
			ret = 0;

			#if defined( SHTPS_DEVICE_ACCESS_LOG_ENABLE )
				shtps_i2c_transfer_log(I2C_IO_READ, addr, readbuf, size);
			#endif /* #if defined( SHTPS_DEVICE_ACCESS_LOG_ENABLE ); */
		}
	}

	return ret;
}

static int shtps_i2c_read(void *client, u16 addr, u8 *buf, u32 size)
{
	SHTPS_LOG_I2C_FUNC_CALL();
	return shtps_i2c_read_packet(client, addr, buf, size);
}

static int shtps_i2c_read_packet(void *client, u16 addr, u8 *buf, u32 size)
{
	int status = 0;
	int i;
	u32 s;
	int retry = SHTPS_I2C_RETRY_COUNT;

	SHTPS_LOG_I2C_FUNC_CALL();
	do{
		s = size;
		for(i = 0; i < size; i += SHTPS_I2C_BLOCKREAD_BUFSIZE){
			status = shtps_i2c_read_main(client,
				addr,
				buf + i,
				(s > SHTPS_I2C_BLOCKREAD_BUFSIZE) ? (SHTPS_I2C_BLOCKREAD_BUFSIZE) : (s));
			//
			if(status){
				struct timespec tu;
				tu.tv_sec = (time_t)0;
				tu.tv_nsec = SHTPS_I2C_RETRY_WAIT * 1000000;
				hrtimer_nanosleep(&tu, NULL, HRTIMER_MODE_REL, CLOCK_MONOTONIC);
				break;
			}
			s -= SHTPS_I2C_BLOCKREAD_BUFSIZE;
			addr = 0x7FFF;	/* specifying no address after first read */
		}
	}while(status != 0 && retry-- > 0);

	if(status){
		SHTPS_LOG_ERR_PRINT("i2c read error\n");
	}
	#if defined( SHTPS_LOG_DEBUG_ENABLE )
	else if(retry < (SHTPS_I2C_RETRY_COUNT)){
		SHTPS_LOG_DBG_PRINT("i2c retry num = %d\n", SHTPS_I2C_RETRY_COUNT - retry);
	}
	#endif /* SHTPS_LOG_DEBUG_ENABLE */
	
	return status;
}

static int shtps_i2c_direct_write(void *client, u8 *writebuf, u32 writelen)
{
	int ret = 0;

	if (writelen > 0) {
		ret = shtps_i2c_transfer((struct i2c_client *)client, writebuf, writelen, NULL, 0);
		if (ret < 0) {
			SHTPS_LOG_ERR_PRINT("%s: i2c write error.\n", __func__);
		}
		else {
			#if defined( SHTPS_DEVICE_ACCESS_LOG_ENABLE )
				shtps_i2c_transfer_log(I2C_IO_WRITE, 0x7FFF, writebuf, writelen);
			#endif /* #if defined( SHTPS_DEVICE_ACCESS_LOG_ENABLE ); */
		}
	}
	return ret;
}

static int shtps_i2c_direct_read(void *client, u8 *writebuf, u32 writelen, u8 *readbuf, u32 readlen)
{
	int ret;

	if (writelen > 0) {
		ret = shtps_i2c_transfer((struct i2c_client *)client, writebuf, writelen, readbuf, readlen);
		if (ret < 0) {
			SHTPS_LOG_ERR_PRINT("%s: i2c read error.\n", __func__);
		}
		else {
			#if defined( SHTPS_DEVICE_ACCESS_LOG_ENABLE )
				shtps_i2c_transfer_log(I2C_IO_READ_W, 0x7FFF, writebuf, writelen);
				shtps_i2c_transfer_log(I2C_IO_READ_R, 0x7FFF, readbuf, readlen);
			#endif /* #if defined( SHTPS_DEVICE_ACCESS_LOG_ENABLE ); */
		}
	} else {
		ret = shtps_i2c_transfer((struct i2c_client *)client, NULL, 0, readbuf, readlen);
		if (ret < 0) {
			SHTPS_LOG_ERR_PRINT("%s:i2c read error.\n", __func__);
		}
		else {
			#if defined( SHTPS_DEVICE_ACCESS_LOG_ENABLE )
				shtps_i2c_transfer_log(I2C_IO_READ, 0x7FFF, readbuf, readlen);
			#endif /* #if defined( SHTPS_DEVICE_ACCESS_LOG_ENABLE ); */
		}
	}

	return ret;
}

/* -----------------------------------------------------------------------------------
*/
static const struct shtps_ctrl_functbl TpsCtrl_I2cFuncTbl = {
	.write_f          = shtps_i2c_write,
	.read_f           = shtps_i2c_read,
	.packet_write_f   = shtps_i2c_write_packet,
	.packet_read_f    = shtps_i2c_read_packet,
	.direct_write_f   = shtps_i2c_direct_write,
	.direct_read_f    = shtps_i2c_direct_read,
};

/* -----------------------------------------------------------------------------------
*/
static int shtps_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	extern	int shtps_fts_core_probe( struct device *ctrl_dev_p,
									struct shtps_ctrl_functbl *func_p,
									void *ctrl_p,
									char *modalias,
									int gpio_irq);
	int result;

	_log_msg_sync( LOGMSG_ID__PROBE, "");
	SHTPS_LOG_I2C_FUNC_CALL();

	/* ---------------------------------------------------------------- */
	result = i2c_check_functionality(client->adapter, I2C_FUNC_I2C);
	if (result == 0) {
		SHTPS_LOG_DBG_PRINT("I2C not supported\n");
		goto fail_err;
	}

	/* ---------------------------------------------------------------- */
	result = shtps_fts_core_probe(
							&client->dev,
							(struct shtps_ctrl_functbl*)&TpsCtrl_I2cFuncTbl,
							(void *)client,
							client->name,
							client->irq );
	if(result){
		goto fail_err;
	}

	/* ---------------------------------------------------------------- */

	_log_msg_sync( LOGMSG_ID__PROBE_DONE, "");
	return 0;
	/* ---------------------------------------------------------------- */


fail_err:
	_log_msg_sync( LOGMSG_ID__PROBE_FAIL, "");
	return result;
}

/* -----------------------------------------------------------------------------------
*/
static int shtps_i2c_remove(struct i2c_client *client)
{
	extern int shtps_fts_core_remove(struct shtps_fts *, struct device *, void *ctrl_p);
	struct shtps_fts *ts_p = i2c_get_clientdata(client);
	int ret = shtps_fts_core_remove(ts_p, &client->dev, (void *)client);

	_log_msg_sync( LOGMSG_ID__REMOVE_DONE, "");
	SHTPS_LOG_DBG_PRINT("shtps_i2c_remove() done\n");
	return ret;
}

/* -----------------------------------------------------------------------------------
*/
static int shtps_i2c_suspend(struct device *dev)
{
	extern int shtps_fts_core_suspend(struct shtps_fts *);
	struct shtps_fts *ts_p = dev_get_drvdata(dev);
	int ret = shtps_fts_core_suspend(ts_p);

	_log_msg_sync( LOGMSG_ID__SUSPEND, "");
	return ret;
}

/* -----------------------------------------------------------------------------------
*/
static int shtps_i2c_resume(struct device *dev)
{
	extern int shtps_fts_core_resume(struct shtps_fts *);
	struct shtps_fts *ts_p = dev_get_drvdata(dev);
	int ret = shtps_fts_core_resume(ts_p);

	_log_msg_sync( LOGMSG_ID__RESUME, "");
	return ret;
}

/* -----------------------------------------------------------------------------------
*/
static const struct dev_pm_ops shtps_fts_ts_pm_ops = {
	.suspend	= shtps_i2c_suspend,
	.resume		= shtps_i2c_resume,
};
static const struct i2c_device_id shtps_fts_ts_id[] = {
	{"fts_ts", 0},
	{},
};

MODULE_DEVICE_TABLE(i2c, shtps_fts_ts_id);

#ifdef CONFIG_OF // Open firmware must be defined for dts useage
static struct of_device_id shtps_fts_table[] = {
	{ . compatible = "sharp,shtps_fts" ,}, // Compatible node must match dts
	{ },
};
#endif

static struct i2c_driver shtps_i2c_driver = {
	.probe		= shtps_i2c_probe,
	.remove		= shtps_i2c_remove,
	.driver		= {
		#ifdef CONFIG_OF // Open firmware must be defined for dts useage
			.of_match_table = shtps_fts_table,
		#endif
		.name = "shtps_fts",	// SH_TOUCH_DEVNAME,
		.owner = THIS_MODULE,
#ifdef CONFIG_PM
		.pm = &shtps_fts_ts_pm_ops,
#endif
	},
	.id_table = shtps_fts_ts_id,
};

/* -----------------------------------------------------------------------------------
*/
static int __init shtps_i2c_init(void)
{
	SHTPS_LOG_DBG_PRINT("shtps_i2c_init() start\n");
	_log_msg_sync( LOGMSG_ID__INIT, "");

	return i2c_add_driver(&shtps_i2c_driver);
}
module_init(shtps_i2c_init);

/* -----------------------------------------------------------------------------------
*/
static void __exit shtps_i2c_exit(void)
{
	i2c_del_driver(&shtps_i2c_driver);

	SHTPS_LOG_DBG_PRINT("shtps_i2c_exit() done\n");
	_log_msg_sync( LOGMSG_ID__EXIT, "");
}
module_exit(shtps_i2c_exit);

/* -----------------------------------------------------------------------------------
*/
MODULE_DESCRIPTION("FocalTech fts TouchScreen driver");
MODULE_LICENSE("GPLv2");
