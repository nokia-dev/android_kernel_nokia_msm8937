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

#ifndef __SHTPS_FTS_DEVCTL_H__
#define __SHTPS_FTS_DEVCTL_H__
/* -----------------------------------------------------------------------------------
 */
#define SHTPS_VREG_ID_VDDH				"shtps_fts,vddh"
#define SHTPS_VREG_ID_VBUS				"shtps_fts,vbus"

/* -----------------------------------------------------------------------------------
 */
int shtps_device_setup(struct device* dev, int rst);
void shtps_device_teardown(struct device* dev, int rst);
void shtps_device_reset(int rst);
void shtps_device_sleep(struct device* dev);
void shtps_device_wakeup(struct device* dev);
void shtps_device_poweroff_reset(int rst);

#endif /* __SHTPS_FTS_DEVCTL_H__ */
