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

/* -------------------------------------------------------------------------- */
#include "shtps_fts.h"
#include "shtps_param_extern.h"
#include "shtps_log.h"
#include "shtps_filter.h"

/* -------------------------------------------------------------------------- */
void shtps_filter_main(struct shtps_fts *ts, struct shtps_touch_info *info)
{
}

/* -------------------------------------------------------------------------- */
void shtps_filter_init(struct shtps_fts *ts)
{
}

/* -------------------------------------------------------------------------- */
void shtps_filter_deinit(struct shtps_fts *ts)
{
}
/* -------------------------------------------------------------------------- */
void shtps_filter_sleep_enter(struct shtps_fts *ts)
{
}

/* -------------------------------------------------------------------------- */
void shtps_filter_force_touchup(struct shtps_fts *ts)
{
}
/* -------------------------------------------------------------------------- */
MODULE_DESCRIPTION("FocalTech fts TouchScreen driver");
MODULE_LICENSE("GPLv2");
