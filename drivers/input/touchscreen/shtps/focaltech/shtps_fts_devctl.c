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
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/regulator/consumer.h>

#include <linux/input/shtps_dev.h>
#include "shtps_fts.h"
#include "shtps_log.h"
#include "shtps_param_extern.h"
#include "shtps_fts_devctl.h"

/* -----------------------------------------------------------------------------------
 */

/* -----------------------------------------------------------------------------------
 */
int shtps_device_setup(struct device* dev, int rst)
{
	int ret = 0;
#if defined(SHTPS_POWER_ON_CONTROL_ENABLE)
	struct regulator *reg_vddh = NULL;
#if 0
	struct regulator *reg_vbus = NULL;
#endif
#endif /* SHTPS_POWER_ON_CONTROL_ENABLE */

	ret = gpio_request(rst, "shtps_rst");
	if (ret) {
		SHTPS_LOG_ERR_PRINT("%s() request gpio failed (rst)\n", __func__);
//		return ret;
	}

#if defined(SHTPS_POWER_ON_CONTROL_ENABLE)
	reg_vddh = devm_regulator_get(dev, SHTPS_VREG_ID_VDDH);

	if (IS_ERR(reg_vddh)) {
		SHTPS_LOG_ERR_PRINT("%s() regulator_init vddh Err\n", __func__);
		reg_vddh = NULL;
//		return -1;
	    return 0;
	}

	ret = regulator_is_enabled(reg_vddh);
	if(ret == 0){
		ret = regulator_enable(reg_vddh);
		if (ret){
			SHTPS_LOG_ERR_PRINT("%s() regulator_enable vddh Err[%d]\n", __func__, ret);
		}
	}

	regulator_put(reg_vddh);
	reg_vddh = NULL;

	udelay(SHTPS_POWER_VDDH_WAIT_US);

#if 0
	reg_vbus = devm_regulator_get(dev, SHTPS_VREG_ID_VBUS);

	if (IS_ERR(reg_vbus)) {
		SHTPS_LOG_ERR_PRINT("%s() regulator_init vbus Err\n", __func__);
		reg_vbus = NULL;
		return -1;
	}

	ret = regulator_is_enabled(reg_vbus);
	if(ret == 0){
		ret = regulator_enable(reg_vbus);
		if (ret){
			SHTPS_LOG_ERR_PRINT("%s() regulator_enable vbus Err[%d]\n", __func__, ret);
		}
	}

	ret = regulator_set_mode(reg_vbus, REGULATOR_MODE_NORMAL);
	if (ret != 0) {
		SHTPS_LOG_ERR_PRINT("%s() regulator_set_mode vbus Err[%d]\n", __func__, ret);
	}

	regulator_put(reg_vbus);
	reg_vbus = NULL;

	udelay(SHTPS_POWER_VBUS_WAIT_US);
#endif
#endif /* SHTPS_POWER_ON_CONTROL_ENABLE */

    return 0;
}

void shtps_device_teardown(struct device* dev, int rst)
{
    gpio_free(rst);
}

void shtps_device_reset(int rst)
{
	gpio_set_value(rst, 0);
	mb();
	mdelay(SHTPS_HWRESET_TIME_MS);

	gpio_set_value(rst, 1);
	mb();
	mdelay(SHTPS_HWRESET_AFTER_TIME_MS);
}

void shtps_device_poweroff_reset(int rst)
{
	gpio_set_value(rst, 0);
	mb();
	mdelay(SHTPS_HWRESET_TIME_MS);
}
EXPORT_SYMBOL(shtps_device_poweroff_reset);

void shtps_device_sleep(struct device* dev)
{
}
EXPORT_SYMBOL(shtps_device_sleep);

void shtps_device_wakeup(struct device* dev)
{
}
EXPORT_SYMBOL(shtps_device_wakeup);
/* -----------------------------------------------------------------------------------
*/
MODULE_DESCRIPTION("FocalTech fts TouchScreen driver");
MODULE_LICENSE("GPLv2");
