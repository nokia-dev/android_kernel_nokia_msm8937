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
#include <linux/fs.h>
#include <linux/poll.h>
#include <linux/jiffies.h>
#include <linux/slab.h>
#include <linux/delay.h>

#include <linux/input/shtps_dev.h>

#include "shtps_fts.h"
#include "shtps_fwctl.h"
#include "shtps_log.h"
#include "shtps_param_extern.h"

/* -------------------------------------------------------------------------- */
extern struct shtps_fwctl_functbl		shtps_fwctl_focaltech_function_table;

struct shtps_fwctl_functbl	*shtps_fwctl_function[]={
	&shtps_fwctl_focaltech_function_table,
	NULL,
};

/* -------------------------------------------------------------------------- */
int shtps_fwctl_init(struct shtps_fts *ts_p, void *tps_ctrl_p, struct shtps_ctrl_functbl *func_p)
{
	ts_p->fwctl_p = kzalloc(sizeof(struct shtps_fwctl_info), GFP_KERNEL);
	if(ts_p->fwctl_p == NULL){
		PR_ERROR("memory allocation error:%s()\n", __func__);
		return -ENOMEM;
	}
	ts_p->fwctl_p->fwctl_ic_type = 0;
	ts_p->fwctl_p->fwctl_func_p = shtps_fwctl_function[ ts_p->fwctl_p->fwctl_ic_type ];
	ts_p->fwctl_p->devctrl_func_p = func_p;	/* func.table i2c */
	ts_p->fwctl_p->tps_ctrl_p = tps_ctrl_p;	/* struct device */

	shtps_fwctl_ic_init(ts_p);

	return 0;
}

/* -------------------------------------------------------------------------- */
void shtps_fwctl_deinit(struct shtps_fts *ts_p)
{
	if(ts_p->fwctl_p)	kfree(ts_p->fwctl_p);
	ts_p->fwctl_p = NULL;
}

/* -----------------------------------------------------------------------------------
*/
MODULE_DESCRIPTION("FocalTech fts TouchScreen driver");
MODULE_LICENSE("GPLv2");
