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

#ifndef __SHTPS_FILTER_H__
#define __SHTPS_FILTER_H__

/* -------------------------------------------------------------------------- */
struct shtps_fts;
struct shtps_touch_info;

/* -------------------------------------------------------------------------- */
typedef void (*shtps_filter_check_f)(struct shtps_fts *, struct shtps_touch_info *);
typedef void (*shtps_filter_init_f)(struct shtps_fts *);
typedef void (*shtps_filter_deinit_f)(struct shtps_fts *);

typedef void (*shtps_filter_sleep_f)(struct shtps_fts *);
typedef void (*shtps_filter_touchup_f)(struct shtps_fts *);

/* -------------------------------------------------------------------------- */
void shtps_filter_main(struct shtps_fts *ts, struct shtps_touch_info *info);
void shtps_filter_init(struct shtps_fts *ts);
void shtps_filter_deinit(struct shtps_fts *ts);
void shtps_filter_sleep_enter(struct shtps_fts *ts);
void shtps_filter_force_touchup(struct shtps_fts *ts);


/* -------------------------------------------------------------------------- */
#endif	/* __SHTPS_FILTER_H__ */

