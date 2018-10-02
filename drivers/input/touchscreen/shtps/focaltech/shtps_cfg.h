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

#ifndef __SHTPS_CFG_H__
#define __SHTPS_CFG_H__
/* --------------------------------------------------------------------------- */

#include <linux/input/shtps_dev.h>
/* --------------------------------------------------------------------------- */
#if defined(CONFIG_TOUCHSCREEN_SHTPS_FTS_FT8607)
	#include "ft8607/shtps_cfg_ft8607.h"

#else
	#include "ft8607/shtps_cfg_ft8607.h"

#endif

/* --------------------------------------------------------------------------- */
#endif	/* __SHTPS_CFG_H__ */

