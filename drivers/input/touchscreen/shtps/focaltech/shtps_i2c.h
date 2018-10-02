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

#ifndef __SHTPS_I2C_H__
#define __SHTPS_I2C_H__
#include <linux/i2c.h>

typedef int (*tps_write_t)(void *, u16, u8);
typedef int (*tps_read_t)( void *, u16, u8 *, u32);
typedef int (*tps_write_packet_t)(void *, u16, u8 *, u32);
typedef int (*tps_read_packet_t)( void *, u16, u8 *, u32);
typedef int (*tps_direct_write_t)(void *, u8 *, u32);
typedef int (*tps_direct_read_t)( void *, u8 *, u32, u8 *, u32);

struct shtps_ctrl_functbl{
	tps_write_t				write_f;
	tps_read_t				read_f;
	tps_write_packet_t		packet_write_f;
	tps_read_packet_t		packet_read_f;
	tps_direct_write_t		direct_write_f;
	tps_direct_read_t		direct_read_f;
};

#define M_WRITE_FUNC(A, B, C)				(A)->devctrl_func_p->write_f((A)->tps_ctrl_p, B, C)
#define M_READ_FUNC(A, B, C, D)				(A)->devctrl_func_p->read_f((A)->tps_ctrl_p, B, C, D)
#define M_WRITE_PACKET_FUNC(A, B, C, D)		(A)->devctrl_func_p->packet_write_f((A)->tps_ctrl_p, B, C, D)
#define M_READ_PACKET_FUNC(A, B, C, D)		(A)->devctrl_func_p->packet_read_f((A)->tps_ctrl_p, B, C, D)
#define M_DIRECT_WRITE_FUNC(A, B, C)		(A)->devctrl_func_p->direct_write_f((A)->tps_ctrl_p, B, C)
#define M_DIRECT_READ_FUNC(A, B, C, D, E)	(A)->devctrl_func_p->direct_read_f((A)->tps_ctrl_p, B, C, D, E)

void shtps_set_i2c_transfer_log_enable(int para);
int shtps_get_i2c_transfer_log_enable(void);
#endif /* __SHTPS_I2C_H__ */
