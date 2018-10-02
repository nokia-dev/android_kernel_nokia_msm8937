/* Copyright (c) 2017, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/miscdevice.h>
#include <linux/mutex.h>
#include <linux/mm.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/sysctl.h>
#include <linux/regulator/consumer.h>
#include <linux/input.h>
#include <linux/regmap.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/interrupt.h>
#include <linux/sensors.h>
#include <linux/pm_wakeup.h>
#include <linux/uaccess.h>
#include <linux/atomic.h>

#define LTR557_I2C_NAME				"sharp,ltr557"
#define LTR557_LIGHT_INPUT_NAME		"ltr557-light"
#define LTR557_PROXIMITY_INPUT_NAME	"ltr557-proximity"
#define LTR557_E2P_PARTITION_DEVI	"/dev/block/bootdevice/by-name/deviceinfo"

#define LTR557_REG_MAIN_CTL			0x00
#define LTR557_REG_PS_LED			0x01
#define LTR557_REG_PS_N_PULSES		0x02
#define LTR557_REG_PS_MEAS_RATE		0x03
#define LTR557_REG_ALS_MEAS_RATE	0x04
#define LTR557_REG_ALS_GAIN			0x05
#define LTR557_REG_PART_ID			0x06
#define LTR557_REG_MAIN_STATUS		0x07
#define LTR557_REG_PS_DATA_0		0x08
#define LTR557_REG_PS_DATA_1		0x09
#define LTR557_REG_ALS_DATA_0		0x0D
#define LTR557_REG_ALS_DATA_1		0x0E
#define LTR557_REG_ALS_DATA_2		0x0F
#define LTR557_REG_INTERRUPT		0x19
#define LTR557_REG_INTERRUPT_PERSIST 0x1A
#define LTR557_REG_PS_THRES_UP_0	0x1B
#define LTR557_REG_PS_THRES_UP_1	0x1C
#define LTR557_REG_PS_THRES_LOW_0	0x1D
#define LTR557_REG_PS_THRES_LOW_1	0x1E
#define LTR557_REG_PS_OFFSET_0		0x1F
#define LTR557_REG_PS_OFFSET_1		0x20
#define LTR557_REG_ALS_THRES_UP_0	0x21
#define LTR557_REG_ALS_THRES_UP_1	0x22
#define LTR557_REG_ALS_THRES_UP_2	0x23
#define LTR557_REG_ALS_THRES_LOW_0	0x24
#define LTR557_REG_ALS_THRES_LOW_1	0x25
#define LTR557_REG_ALS_THRES_LOW_2	0x26

#define LTR557_REG_PS_MAGIC			0xFD
#define LTR557_REG_ALS_MAGIC		0xFE
#define LTR557_REG_MAGIC			0xFF

#define LTR557_PART_ID				0xC2

#define LTR557_ALS_SENSITIVITY		70

#define LTR557_BOOT_TIME_MS			120
#define LTR557_WAKE_TIME_MS			10

#define LTR557_PS_SATURATE_MASK		0x0800
#define LTR557_ALS_INT_MASK			0x10
#define LTR557_PS_INT_MASK			0x02

#define LTR557_ALS_GAIN_MASK		0x07

/* default als measurement rate is 100 ms */
#define LTR557_ALS_DEFAULT_MEASURE_RATE		0x02
#define LTR557_PS_MEASUREMENT_RATE_25MS		0x1B
#define LTR557_PS_MEASUREMENT_RATE_6_25MS	0x19

#define LTR557_CALIBRATE_SAMPLES	15

/* LTR557 ALS data is 20 bit */
#define ALS_DATA_MASK			0xfffff
#define ALS_LOW_BYTE(data)		((data) & 0xff)
#define ALS_MID_BYTE(data)		(((data) >> 8) & 0xff)
#define ALS_HIGH_BYTE(data)		(((data) >> 16) & 0x0f)

/* LTR557 PS data is 11 bit */
#define PS_DATA_MASK			0x7ff
#define PS_LOW_BYTE(data)		((data) & 0xff)
#define PS_HIGH_BYTE(data)		(((data) >> 8) & 0x7)

/* Calculated by 10% transmittance */
#define LTR557_MAX_LUX			(ALS_DATA_MASK * 10)
#define WINFAC					1

/* both als and ps interrupt are enabled */
#define LTR557_INTERRUPT_SETTING	0x15

/* Any proximity distance change will wakeup SoC */
#define LTR557_WAKEUP_ANY_CHANGE	0xff

#define CAL_BUF_LEN			16

#define E2P_OFFSET_DEVI             0x0007C000

enum {
	CMD_WRITE = 0,
	CMD_READ = 1,
};

struct regulator_map {
	struct regulator	*regulator;
	int			min_uv;
	int			max_uv;
	char			*supply;
};

struct pinctrl_config {
	struct pinctrl		*pinctrl;
	struct pinctrl_state	*state[2];
	char			*name[2];
};

struct stflib_s_als_calibinfo {
	unsigned char  sensor_id;
	unsigned short slope;     /* x1,000 */
	unsigned short intercept; /* x1 */
};

struct ltr557_data {
	struct i2c_client	*i2c;
	struct regmap		*regmap;
	struct regulator	*config;
	struct input_dev	*input_light;
	struct input_dev	*input_proximity;
	struct workqueue_struct	*workqueue;

	struct sensors_classdev	als_cdev;
	struct sensors_classdev	ps_cdev;
	struct mutex		ops_lock;
	ktime_t			last_als_ts;
	ktime_t			last_ps_ts;
	struct work_struct	report_work;
	struct work_struct	als_enable_work;
	struct work_struct	als_disable_work;
	struct work_struct	ps_enable_work;
	struct work_struct	ps_disable_work;
	atomic_t		wake_count;

	int			irq_gpio;
	int			irq;
	bool		als_enabled;
	bool		ps_enabled;
	u32			irq_flags;
	int			als_delay;
	int			ps_delay;
	int			als_cal;
	int			ps_cal;
	int			als_gain;
	int			als_persist;
	int			als_reso;
	int			als_measure_rate;
	int			ps_led;
	int			ps_pulses;
	int			ps_measure_rate;
	int			als_ps_persist;
	int			ps_wakeup_threshold;
	int			ps_thres_up;
	int			ps_thres_low;

	int			last_als;
	int			last_ps;
	int			flush_count;
	int			power_enabled;

	unsigned int		reg_addr;
	char			calibrate_buf[CAL_BUF_LEN];
	unsigned int		bias;

	struct stflib_s_als_calibinfo calibinfo;
};

static struct regulator_map power_config[] = {
	{.supply = "pm8937_l11", .min_uv = 2000000, .max_uv = 3300000, },
	{.supply = "pm8937_l5" , .min_uv = 1800000, .max_uv = 1800000, },
};

static struct pinctrl_config pin_config = {
	.name = { "prox_int_active", "prox_int_suspend" },
};

/* ALS gain table */
static int als_gain_table[] = {1, 3, 6, 9, 18};
/* ALS measurement repeat rate in ms */
static int als_mrr_table[] = { 25, 50, 100, 500, 1000, 2000, 2000};
/* PS measurement repeat rate in ms */
static int ps_mrr_table[] = { 0, 7, 13, 25, 50, 100, 200, 400};

#define LTR557_ALS_REPEAT_RATE_DEFAULT 100
#define LTR557_PS_REPEAT_RATE_DEFAULT 100

/* Tuned for devices with rubber */
static int ps_distance_table[] =  { 790, 337, 195, 114, 78, 62, 50 };

static struct sensors_classdev als_cdev = {
	.name = "ltr557-light",
	.vendor = "Lite-On Technology Corp",
	.version = 1,
	.handle = SENSORS_LIGHT_HANDLE,
	.type = SENSOR_TYPE_LIGHT,
	.max_range = "1048575",
	.resolution = "1.0",
	.sensor_power = "0.25",
	.min_delay = 100000,
	.max_delay = 2000,
	.fifo_reserved_event_count = 0,
	.fifo_max_event_count = 0,
	.flags = 2,
	.enabled = 0,
	.delay_msec = 50,
	.sensors_enable = NULL,
	.sensors_poll_delay = NULL,
};

static struct sensors_classdev ps_cdev = {
	.name = "ltr557-proximity",
	.vendor = "Lite-On Technology Corp",
	.version = 1,
	.handle = SENSORS_PROXIMITY_HANDLE,
	.type = SENSOR_TYPE_PROXIMITY,
	.max_range = "7",
	.resolution = "1.0",
	.sensor_power = "0.25",
	.min_delay = 6250,
	.max_delay = 400,
	.fifo_reserved_event_count = 0,
	.fifo_max_event_count = 0,
	.flags = 3,
	.enabled = 0,
	.delay_msec = 50,
	.sensors_enable = NULL,
	.sensors_poll_delay = NULL,
};

static int sensor_power_init(struct device *dev, struct regulator_map *map,
		int size)
{
	int rc = 0;
	int i;

	for (i = 0; i < size; i++) {
		map[i].regulator = regulator_get(dev, map[i].supply);
		if (IS_ERR(map[i].regulator)) {
			rc = PTR_ERR(map[i].regulator);
			dev_err(dev, "Regualtor get failed vdd rc=%d\n", rc);
		}
	}
	
	return rc;
}

static int sensor_power_deinit(struct device *dev, struct regulator_map *map,
		int size)
{
	int i;

	for (i = 0; i < size; i++) {
		regulator_put( map[i].regulator );
		map[i].regulator = NULL;
	}

	return 0;
}

static int sensor_power_config(struct device *dev, struct regulator_map *map,
		int size, bool enable)
{
	int i;
	int rc = 0;

	if (enable) {
		for (i = 0; i < size; i++) {
			rc = regulator_enable(map[i].regulator);
			if (rc) {
				dev_err(dev, "enable %s failed.\n",
						map[i].supply);
				goto exit_enable;
			}
		}
	} else {
		for (i = 0; i < size; i++) {
			rc = regulator_disable(map[i].regulator);
			if (rc) {
				dev_err(dev, "disable %s failed.\n",
						map[i].supply);
				goto exit_disable;
			}
		}
	}

	return 0;

exit_enable:
	for (i = i - 1; i >= 0; i--)
		regulator_disable(map[i].regulator);

	return rc;

exit_disable:
	for (i = i - 1; i >= 0; i--)
		if (regulator_enable(map[i].regulator))
			dev_err(dev, "enable %s failed\n", map[i].supply);

	return rc;
}

static int sensor_pinctrl_init(struct device *dev,
		struct pinctrl_config *config)
{
	config->pinctrl = devm_pinctrl_get(dev);
	if (IS_ERR_OR_NULL(config->pinctrl)) {
		dev_err(dev, "Failed to get pinctrl\n");
		return PTR_ERR(config->pinctrl);
	}

	config->state[0] =
		pinctrl_lookup_state(config->pinctrl, config->name[0]);
	if (IS_ERR_OR_NULL(config->state[0])) {
		dev_err(dev, "Failed to look up %s\n", config->name[0]);
		return PTR_ERR(config->state[0]);
	}

	config->state[1] =
		pinctrl_lookup_state(config->pinctrl, config->name[1]);
	if (IS_ERR_OR_NULL(config->state[1])) {
		dev_err(dev, "Failed to look up %s\n", config->name[1]);
		return PTR_ERR(config->state[1]);
	}

	pinctrl_select_state(config->pinctrl, config->state[0]);

	return 0;
}

static int ltr557_get_calibinfo(struct ltr557_data *ltr)
{
	struct file *fp = NULL;
	char *buf = NULL;
	int len = 0;
	int fsize = 0;

	fp = filp_open(LTR557_E2P_PARTITION_DEVI, O_RDWR|O_NONBLOCK|O_SYNC, 0644);
	if (IS_ERR(fp)) {
		dev_err(&ltr->i2c->dev, "Cannot open file: %s\n", LTR557_E2P_PARTITION_DEVI);
		return -ENODATA;
	}

	fp->f_op->llseek(fp, E2P_OFFSET_DEVI, SEEK_SET);
	fsize = sizeof(struct stflib_s_als_calibinfo);
	buf = kzalloc(fsize, GFP_KERNEL);
	if (buf == NULL) {
		dev_err(&ltr->i2c->dev, "Cannot allocate memory\n");
		filp_close(fp, NULL);
		return -ENOMEM;
	}
	
	len = fp->f_op->read(fp, buf, sizeof(struct stflib_s_als_calibinfo), &fp->f_pos);
	if (len != sizeof(struct stflib_s_als_calibinfo)) {
		filp_close(fp, NULL);
		kfree(buf);
		dev_err(&ltr->i2c->dev, "Cannot read length\n");
		return -EFAULT;
	}

	filp_close(fp, NULL);

	memcpy(&ltr->calibinfo, buf, len);
	kfree(buf);
	
	dev_dbg(&ltr->i2c->dev, "id:0x%02X, slope:0x%04X, intercept:0x%04X\n", 
					ltr->calibinfo.sensor_id, ltr->calibinfo.slope, ltr->calibinfo.intercept);
	return 0;
}

static void ltr557_init_calibinfo(struct stflib_s_als_calibinfo *info)
{
	info->sensor_id = 0;
	info->slope     = 1000;
	info->intercept = 0;
}

static int ltr557_parse_dt(struct device *dev, struct ltr557_data *ltr)
{
	struct device_node *dp = dev->of_node;
	u32 value;
	int rc;
	int i;

	rc = of_get_named_gpio_flags(dp, "qcom,irq-gpio", 0,
			&ltr->irq_flags);
	if (rc < 0) {
		dev_err(dev, "unable to read irq gpio\n");
		return rc;
	}
	ltr->irq_gpio = rc;

	/* als ps persist */
	rc = of_property_read_u32(dp, "qcom,als-ps-persist", &value);
	if (rc) {
		dev_err(dev, "read qcom,als-ps-persist failed\n");
		return rc;
	}
	ltr->als_ps_persist = value;

	/* ps led */
	rc = of_property_read_u32(dp, "qcom,ps-led", &value);
	if (rc) {
		dev_err(dev, "read qcom,ps-led failed\n");
		return rc;
	}
	ltr->ps_led = value;

	/* ps pulses */
	rc = of_property_read_u32(dp, "qcom,ps-pulses", &value);
	if (rc) {
		dev_err(dev, "read qcom,ps-pulses failed\n");
		return rc;
	}
	if (value > 0x40) {
		dev_err(dev, "qcom,ps-pulses out of range\n");
		return -EINVAL;
	}
	ltr->ps_pulses = value;

	/* als resolution */
	rc = of_property_read_u32(dp, "qcom,als-resolution", &value);
	if (rc) {
		dev_err(dev, "read qcom,als-resolution failed\n");
		return rc;
	}
	if (value > 0x4) {
		dev_err(dev, "qcom,als-resolution out of range\n");
		return -EINVAL;
	}

	ltr->als_reso = value;

	/* ps wakeup threshold */
	rc = of_property_read_u32(dp, "qcom,wakeup-threshold", &value);
	if (rc) {
		dev_err(dev, "qcom,wakeup-threshold incorrect, drop to default\n");
		value = LTR557_WAKEUP_ANY_CHANGE;
	}
	if ((value >= ARRAY_SIZE(ps_distance_table)) &&
			(value != LTR557_WAKEUP_ANY_CHANGE)) {
		dev_err(dev, "wakeup threshold too big\n");
		return -EINVAL;
	}
	ltr->ps_wakeup_threshold = value;

	/* ps distance table */
	rc = of_property_read_u32_array(dp, "qcom,ps-distance-table",
			ps_distance_table, ARRAY_SIZE(ps_distance_table));
	if ((rc == -ENODATA) || (rc == -EOVERFLOW)) {
		dev_warn(dev, "qcom,ps-distance-table not correctly set\n");
		return rc;
	}

	for (i = 1; i < ARRAY_SIZE(ps_distance_table); i++) {
		if (ps_distance_table[i - 1] < ps_distance_table[i]) {
			dev_err(dev, "ps distance table should in descend order\n");
			return -EINVAL;
		}
	}

	if (ps_distance_table[0] > PS_DATA_MASK) {
		dev_err(dev, "distance table out of range\n");
		return -EINVAL;
	}

	/* als gain */
	rc = of_property_read_u32(dp, "qcom,als-gain", &value);
	if (rc) {
		dev_err(dev, "read qcom,als-gain failed. Drop to default\n");
		value = 0;
	}	
	if (value > 0x4) {
		dev_err(dev, "qcom,als-gain invalid\n");
		return -EINVAL;
	}
	ltr->als_gain = value;

	return 0;
}

static int ltr557_dynamic_calibrate(struct ltr557_data *ltr)
{
	int i = 0;
	int data;
	int data_total = 0;
	int noise = 0;
	int count = 5;
	int ps_thd_val_low, ps_thd_val_high;	
	u8 ps_data[4];
	int rc;
	int tmp;
	
	tmp = ps_mrr_table[ltr->ps_measure_rate & 0x07] + LTR557_WAKE_TIME_MS;

	for (i = 0; i < count; i++) {
		// wait for ps value be stable
		msleep(tmp);

		rc = regmap_bulk_read(ltr->regmap, LTR557_REG_PS_DATA_0,
				ps_data, 2);
		if (rc) {
			dev_err(&ltr->i2c->dev, "read PS data failed\n");
			continue;
		}

		data = (ps_data[1] << 8) | ps_data[0];
		if (data & LTR557_PS_SATURATE_MASK)
		{
			return data;
		}		

		data_total += data;
	}

	noise = data_total / count;

	if (ltr->ps_cal == 0) {
		if (noise < 100) {
			ps_thd_val_high = noise + 100;
			ps_thd_val_low  = noise + 50;
		}
		else if (noise < 200) {
			ps_thd_val_high = noise + 150;
			ps_thd_val_low  = noise + 60;
		}
		else if (noise < 300) {
			ps_thd_val_high = noise + 150;
			ps_thd_val_low  = noise + 60;
		}
		else if (noise < 400) {
			ps_thd_val_high = noise + 150;
			ps_thd_val_low  = noise + 60;
		}
		else if (noise < 600) {
			ps_thd_val_high = noise + 180;
			ps_thd_val_low  = noise + 90;
		}
		else if (noise < 1000) {
			ps_thd_val_high = noise + 300;
			ps_thd_val_low  = noise + 180;
		}
		else {
			ps_thd_val_high = 1500;
			ps_thd_val_low  = 1300;
		}

		ps_data[0] = PS_LOW_BYTE(ps_thd_val_high);
		ps_data[1] = PS_HIGH_BYTE(ps_thd_val_high);
		ps_data[2] = PS_LOW_BYTE(ps_thd_val_low);
		ps_data[3] = PS_HIGH_BYTE(ps_thd_val_low);
	
		rc = regmap_bulk_write(ltr->regmap,
				LTR557_REG_PS_THRES_UP_0, ps_data, 4);
		if (rc) {
			dev_err(&ltr->i2c->dev, "set up threshold failed\n");	
		}

		ltr->ps_thres_up = ps_thd_val_high;
		ltr->ps_thres_low = ps_thd_val_low;

		ltr->ps_cal = 1;
	} else {
		ps_data[0] = PS_LOW_BYTE(ltr->ps_thres_up);
		ps_data[1] = PS_HIGH_BYTE(ltr->ps_thres_up);
		ps_data[2] = PS_LOW_BYTE(ltr->ps_thres_low);
		ps_data[3] = PS_HIGH_BYTE(ltr->ps_thres_low);
	
		rc = regmap_bulk_write(ltr->regmap,
				LTR557_REG_PS_THRES_UP_0, ps_data, 4);
		if (rc) {
			dev_err(&ltr->i2c->dev, "set up threshold failed\n");	
		}
	}
	
	return noise;
}

static int ltr557_check_device(struct ltr557_data *ltr)
{
	unsigned int part_id;
	int rc;

	rc = regmap_read(ltr->regmap, LTR557_REG_PART_ID, &part_id);
	if (rc) {
		dev_err(&ltr->i2c->dev, "read reg %d failed.(%d)\n",
				LTR557_REG_PART_ID, rc);
		return rc;
	}

	if (part_id != LTR557_PART_ID)
		return -ENODEV;

	return 0;
}

static int ltr557_init_input(struct ltr557_data *ltr)
{
	struct input_dev *input;
	int status;

	input = devm_input_allocate_device(&ltr->i2c->dev);
	if (!input) {
		dev_err(&ltr->i2c->dev, "allocate light input device failed\n");
		return -ENOMEM;
	}

	input->name = LTR557_LIGHT_INPUT_NAME;
	input->phys = "ltr557/input0";
	input->id.bustype = BUS_I2C;

	input_set_capability(input, EV_ABS, ABS_MISC);
	input_set_abs_params(input, ABS_MISC, 0, LTR557_MAX_LUX, 0, 0);

	status = input_register_device(input);
	if (status) {
		dev_err(&ltr->i2c->dev, "register light input device failed.\n");
		return status;
	}

	ltr->input_light = input;

	input = devm_input_allocate_device(&ltr->i2c->dev);
	if (!input) {
		dev_err(&ltr->i2c->dev, "allocate proximity input device failed\n");
		return -ENOMEM;
	}

	input->name = LTR557_PROXIMITY_INPUT_NAME;
	input->phys = "ltr557/input1";
	input->id.bustype = BUS_I2C;

	input_set_capability(input, EV_ABS, ABS_DISTANCE);
	input_set_abs_params(input, ABS_DISTANCE, 0,
			ARRAY_SIZE(ps_distance_table), 0, 0);

	status = input_register_device(input);
	if (status) {
		dev_err(&ltr->i2c->dev, "register proxmity input device failed.\n");
		return status;
	}

	ltr->input_proximity = input;

	return 0;
}

static int ltr557_init_device(struct ltr557_data *ltr)
{
	int rc;
	
	/* Enable als/ps interrupt */
	rc = regmap_write(ltr->regmap, LTR557_REG_INTERRUPT,
			LTR557_INTERRUPT_SETTING);
	if (rc) {
		dev_err(&ltr->i2c->dev, "write %d register failed\n",
				LTR557_REG_INTERRUPT);
		return rc;
	}

	rc = regmap_write(ltr->regmap, LTR557_REG_INTERRUPT_PERSIST,
			ltr->als_ps_persist);
	if (rc) {
		dev_err(&ltr->i2c->dev, "write %d register failed\n",
				LTR557_REG_INTERRUPT_PERSIST);
		return rc;
	}

	rc = regmap_write(ltr->regmap, LTR557_REG_PS_N_PULSES, ltr->ps_pulses);
	if (rc) {
		dev_err(&ltr->i2c->dev, "write %d register failed\n",
				LTR557_REG_PS_N_PULSES);
		return rc;
	}

	rc = regmap_write(ltr->regmap, LTR557_REG_ALS_MEAS_RATE,
		(ltr->als_reso << 4) | (ltr->als_measure_rate));
	if (rc) {
		dev_err(&ltr->i2c->dev, "write %d failed\n",
				LTR557_REG_ALS_MEAS_RATE);
		return rc;
	}

	rc = regmap_write(ltr->regmap, LTR557_REG_PS_MEAS_RATE,
			ltr->ps_measure_rate);
	if (rc) {
		dev_err(&ltr->i2c->dev, "write %d failed\n",
				LTR557_REG_PS_MEAS_RATE);
		return rc;
	}

	/* Set calibration parameter low byte */
	rc = regmap_write(ltr->regmap, LTR557_REG_PS_OFFSET_0,
			PS_LOW_BYTE(ltr->bias));
	if (rc) {
		dev_err(&ltr->i2c->dev, "write %d register failed\n",
				LTR557_REG_PS_OFFSET_0);
		return rc;
	}

	/* Set calibration parameter high byte */
	rc = regmap_write(ltr->regmap, LTR557_REG_PS_OFFSET_1,
			PS_HIGH_BYTE(ltr->bias));
	if (rc) {
		dev_err(&ltr->i2c->dev, "write %d register failed\n",
				LTR557_REG_PS_OFFSET_1);
		return rc;
	}

	/* set up als gain */
	rc = regmap_write(ltr->regmap, LTR557_REG_ALS_GAIN,	ltr->als_gain);
	if (rc) {
		dev_err(&ltr->i2c->dev, "write %d register failed\n",
				LTR557_REG_ALS_GAIN);
		return rc;
	}

	return 0;
}

/* Calculate the lux value based on ADC data */
static int ltr557_calc_lux(struct ltr557_data *ltr, int alsdata, int gain, int als_int_fac)
{
	//int ratio;
	int lux;

	lux = alsdata * 8 * WINFAC / (gain * als_int_fac * 10);
	// lux = ltr->calibinfo.slope * lux / 1000 - ltr->calibinfo.intercept;
    lux =  ( lux - ltr->calibinfo.intercept ) * 1000 / ltr->calibinfo.slope;
	
    if ( lux < 0 ) {
        lux = 0;
    }

	return lux;
}

static int ltr557_process_data(struct ltr557_data *ltr, int als_ps, int ps_cali)
{
	int als_int_fac;
	int rc = 0;

	unsigned int tmp;
	u8 als_data[6];
	int lux;
	int alsdata;	

	u8 ps_data[4];
	int i;
	int distance;

	if (als_ps) { /* process als data */
		/* Read data */
		rc = regmap_bulk_read(ltr->regmap, LTR557_REG_ALS_DATA_0,
				als_data, 3);
		if (rc) {
			dev_err(&ltr->i2c->dev, "read %d failed.(%d)\n",
					LTR557_REG_ALS_DATA_0, rc);
			goto exit;
		}
		alsdata = als_data[0] | (als_data[1] << 8) | (als_data[2] << 16);
		
		als_int_fac = 1;
		lux = ltr557_calc_lux(ltr, alsdata, als_gain_table[ltr->als_gain], als_int_fac);

		dev_dbg(&ltr->i2c->dev, "lux:%d als_data:0x%x-0x%x-0x%x\n",
				lux, als_data[0], als_data[1],	als_data[2]);
		
		if (lux != ltr->last_als) {
			input_report_abs(ltr->input_light, ABS_MISC, lux);
			input_sync(ltr->input_light);
		}

		ltr->last_als = lux;

		/* lower threshold */
//		tmp = lux - lux / 10;
		tmp = alsdata - alsdata / 10;
		als_data[3] = ALS_LOW_BYTE(tmp);
		als_data[4] = ALS_MID_BYTE(tmp);
		als_data[5] = ALS_HIGH_BYTE(tmp);

		/* upper threshold */
//		tmp = lux + lux / 10;
		tmp = alsdata + alsdata / 10;
		als_data[0] = ALS_LOW_BYTE(tmp);
		als_data[1] = ALS_MID_BYTE(tmp);
		als_data[2] = ALS_HIGH_BYTE(tmp);		

		rc = regmap_bulk_write(ltr->regmap, LTR557_REG_ALS_THRES_UP_0,
				als_data, 6);
		if (rc) {
			dev_err(&ltr->i2c->dev, "read %d failed.(%d)\n",
				LTR557_REG_ALS_THRES_UP_0, rc);
			goto exit;
		}
	}
	else { /* process ps value */
		if (ps_cali)
		{
			tmp = ltr557_dynamic_calibrate(ltr);
		}
		else
		{
			rc = regmap_bulk_read(ltr->regmap, LTR557_REG_PS_DATA_0,
					ps_data, 2);
			if (rc) {
				dev_err(&ltr->i2c->dev, "read %d failed.(%d)\n",
						LTR557_REG_PS_DATA_0, rc);
				goto exit;
			}

			dev_dbg(&ltr->i2c->dev, "ps data: 0x%x 0x%x\n",
					ps_data[0], ps_data[1]);

			tmp = (ps_data[1] << 8) | ps_data[0];
		}

		if (tmp & LTR557_PS_SATURATE_MASK)
			distance = 0;
		else {
			for (i = 0; i < ARRAY_SIZE(ps_distance_table); i++) {
				if (tmp > ps_distance_table[i]) {
					distance = i;
					break;
				}
			}
			distance = i;
		}
#if 1
		ltr->last_ps = distance;

		if (tmp > ltr->ps_thres_up)
			distance = 0;	// near
		else if (tmp < ltr->ps_thres_low)
			distance = 7;	// far
		else
			distance = 7;

		if (ps_cali)
			distance = 7;

		input_report_abs(ltr->input_proximity, ABS_DISTANCE,
			distance);
		input_sync(ltr->input_proximity);
#else
		if (distance != ltr->last_ps) {
			input_report_abs(ltr->input_proximity, ABS_DISTANCE,
					distance);
			input_event(ltr->input_proximity, EV_SYN, SYN_TIME_SEC,
					ktime_to_timespec(timestamp).tv_sec);
			input_event(ltr->input_proximity, EV_SYN, SYN_TIME_NSEC,
					ktime_to_timespec(timestamp).tv_nsec);
			input_sync(ltr->input_proximity);

			ltr->last_ps_ts = timestamp;
		}

		ltr->last_ps = distance;

		/* lower threshold */
		if (distance < ARRAY_SIZE(ps_distance_table))
			tmp = ps_distance_table[distance];
		else
			tmp = 0;

		ps_data[2] = PS_LOW_BYTE(tmp);
		ps_data[3] = PS_HIGH_BYTE(tmp);

		/* upper threshold */
		if (distance > 0)
			tmp = ps_distance_table[distance - 1];
		else
			tmp = PS_DATA_MASK;

		ps_data[0] = PS_LOW_BYTE(tmp);
		ps_data[1] = PS_HIGH_BYTE(tmp);

		dev_dbg(&ltr->i2c->dev, "ps threshold: 0x%x 0x%x 0x%x 0x%x\n",
				ps_data[0], ps_data[1], ps_data[2], ps_data[3]);

		rc = regmap_bulk_write(ltr->regmap, LTR557_REG_PS_THRES_UP_0,
				ps_data, 4);
		if (rc) {
			dev_err(&ltr->i2c->dev, "write %d failed.(%d)\n",
					LTR557_REG_PS_THRES_UP_0, rc);
			goto exit;
		}
#endif
	}
exit:
	return rc;
}

static irqreturn_t ltr557_irq_handler(int irq, void *data)
{
	struct ltr557_data *ltr = data;
	bool rc;

	if (ltr->workqueue != NULL) {
		rc = queue_work(ltr->workqueue, &ltr->report_work);
		/* wake up event should hold a wake lock until reported */
		if (rc && (atomic_inc_return(&ltr->wake_count) == 1))
			pm_stay_awake(&ltr->i2c->dev);
	}

	return IRQ_HANDLED;
}

static void ltr557_report_work(struct work_struct *work)
{
	struct ltr557_data *ltr = container_of(work, struct ltr557_data,
			report_work);
	int rc;
	unsigned int status;
	
	mutex_lock(&ltr->ops_lock);

	/* avoid fake interrupt */
	if (!ltr->power_enabled) {
		dev_dbg(&ltr->i2c->dev, "fake interrupt triggered\n");
		goto exit;
	}

	/* read status */
	rc = regmap_read(ltr->regmap, LTR557_REG_MAIN_STATUS, &status);
	if (rc) {
		dev_err(&ltr->i2c->dev, "read %d failed.(%d)\n",
				LTR557_REG_MAIN_STATUS, rc);
		status |= LTR557_PS_INT_MASK;
		goto exit;
	}

	dev_dbg(&ltr->i2c->dev, "interrupt issued status=0x%x.\n", status);

	/* als interrupt issueed */
	if ((status & LTR557_ALS_INT_MASK) && (ltr->als_enabled)) {
		rc = ltr557_process_data(ltr, 1, 0);
		if (rc)
			goto exit;
		dev_dbg(&ltr->i2c->dev, "process als done!\n");
	}

	if ((status & LTR557_PS_INT_MASK) && (ltr->ps_enabled)) {
		rc = ltr557_process_data(ltr, 0, 0);
		if (rc)
			goto exit;
		dev_dbg(&ltr->i2c->dev, "process ps data done!\n");
		pm_wakeup_event(&ltr->input_proximity->dev, 200);
	}

exit:
	if (atomic_dec_and_test(&ltr->wake_count)) {
		pm_relax(&ltr->i2c->dev);
		dev_dbg(&ltr->i2c->dev, "wake lock released\n");
	}

	mutex_unlock(&ltr->ops_lock);
}

static int ltr557_enable_ps(struct ltr557_data *ltr, int enable)
{
	unsigned int config;
	unsigned int tmp;
	int rc = 0;
	
	rc = regmap_read(ltr->regmap, LTR557_REG_MAIN_CTL, &config);
	if (rc) {
		dev_err(&ltr->i2c->dev, "read %d failed.(%d)\n",
				LTR557_REG_MAIN_CTL, rc);
		return rc;
	}

	if (enable) {
		/* Enable ps sensor */
		rc = regmap_write(ltr->regmap, LTR557_REG_MAIN_CTL,
				config | 0x01);
		if (rc) {
			dev_err(&ltr->i2c->dev, "write %d failed.(%d)\n",
					LTR557_REG_MAIN_CTL, rc);
			goto exit;
		}

		rc = regmap_read(ltr->regmap, LTR557_REG_PS_MEAS_RATE, &tmp);
		if (rc) {
			dev_err(&ltr->i2c->dev, "read %d failed.(%d)\n",
					LTR557_REG_PS_MEAS_RATE, rc);
			goto exit;
		}

		/* Wait for data ready */
		msleep(ps_mrr_table[tmp & 0x7] + LTR557_WAKE_TIME_MS);

		/* clear last ps value */
		ltr->last_ps = -1;

		input_report_abs(ltr->input_proximity, ABS_DISTANCE,
			ltr->last_ps);
		input_sync(ltr->input_proximity);

		rc = ltr557_process_data(ltr, 0, 1);
		if (rc) {
			dev_err(&ltr->i2c->dev, "process ps data failed\n");
			goto exit;
		}

		/* clear interrupt */
		rc = regmap_read(ltr->regmap, LTR557_REG_MAIN_STATUS, &tmp);
		if (rc) {
			dev_err(&ltr->i2c->dev, "clear interrupt failed\n");
			goto exit;
		}

		ltr->ps_enabled = true;

	} else {
		/* disable ps_sensor */
		rc = regmap_write(ltr->regmap, LTR557_REG_MAIN_CTL,
				config & (~0x01));
		if (rc) {
			dev_err(&ltr->i2c->dev, "write %d failed.(%d)\n",
					LTR557_REG_MAIN_CTL, rc);
			goto exit;
		}

		ltr->ps_enabled = false;
	}
exit:
	return rc;
}

static int ltr557_enable_als(struct ltr557_data *ltr, int enable)
{
	int rc = 0;
	unsigned int config;
	unsigned int tmp;
	
	rc = regmap_read(ltr->regmap, LTR557_REG_MAIN_CTL, &config);
	if (rc) {
		dev_err(&ltr->i2c->dev, "read %d failed.(%d)\n",
				LTR557_REG_MAIN_CTL, rc);
		goto exit;
	}

	if (enable) {
		/* get calibration info */
		rc = ltr557_get_calibinfo(ltr);
		if (rc) {
			dev_err(&ltr->i2c->dev, "calibinfo cannot read\n");
			ltr557_init_calibinfo(&ltr->calibinfo);
		} else {
			if ((ltr->calibinfo.sensor_id == LTR557_PART_ID) && (ltr->calibinfo.slope != 0)) {
				dev_dbg(&ltr->i2c->dev, "calibinfo read success\n");
			} else {
				dev_err(&ltr->i2c->dev, "calibinfo default\n");
				ltr557_init_calibinfo(&ltr->calibinfo);
			}
		}

		/* enable als_sensor */
		rc = regmap_write(ltr->regmap, LTR557_REG_MAIN_CTL,
				config | 0x2);
		if (rc) {
			dev_err(&ltr->i2c->dev, "write %d failed.(%d)\n",
					LTR557_REG_MAIN_CTL, rc);
			goto exit;
		}

		rc = regmap_read(ltr->regmap, LTR557_REG_ALS_MEAS_RATE, &tmp);
		if (rc) {
			dev_err(&ltr->i2c->dev, "write %d failed.(%d)\n",
					LTR557_REG_ALS_MEAS_RATE, rc);
			goto exit;
		}

		/* Wait for data ready */
		msleep(als_mrr_table[tmp & 0x7] + LTR557_WAKE_TIME_MS);

		/* Clear last value and report even not change. */
		ltr->last_als = -1;

		input_report_abs(ltr->input_light, ABS_MISC, ltr->last_als);
		input_sync(ltr->input_light);

		rc = ltr557_process_data(ltr, 1, 0);
		if (rc) {
			dev_err(&ltr->i2c->dev, "process als data failed\n");
			goto exit;
		}

		/* clear interrupt */
		rc = regmap_read(ltr->regmap, LTR557_REG_MAIN_STATUS, &tmp);
		if (rc) {
			dev_err(&ltr->i2c->dev, "clear interrupt failed\n");
			goto exit;
		}

		ltr->als_enabled = true;
	} else {
		/* disable als sensor */
		rc = regmap_write(ltr->regmap, LTR557_REG_MAIN_CTL,
				config & (~0x2));
		if (rc) {
			dev_err(&ltr->i2c->dev, "write %d failed.(%d)\n",
					LTR557_REG_MAIN_CTL, rc);
			goto exit;
		}

		ltr->als_enabled = false;
	}

exit:
	return rc;
}

static int ltr557_als_sync_delay(struct ltr557_data *ltr,
		unsigned int als_delay)
{
	int index = 0;
	int i;
	unsigned int val;
	int rc = 0;
	int min;

	if (!ltr->power_enabled) {
		dev_dbg(&ltr->i2c->dev, "power is not enabled\n");
		return 0;
	}

	min = abs(als_delay - als_mrr_table[0]);
	for (i = 0; i < ARRAY_SIZE(als_mrr_table); i++) {
		if (als_delay == als_mrr_table[i]) {
			index = i;
			break;
		}
		if (min > abs(als_delay - als_mrr_table[i])) {
			index = i;
			min = abs(als_delay - als_mrr_table[i]);
		}		
	}

	dev_dbg(&ltr->i2c->dev, "als delay %d ms\n", als_mrr_table[index]);

	rc = regmap_read(ltr->regmap, LTR557_REG_ALS_MEAS_RATE, &val);
	if (rc) {
		dev_err(&ltr->i2c->dev, "read %d failed\n",
				LTR557_REG_ALS_MEAS_RATE);
		goto exit;
	}
	val &= ~0x7;

	ltr->als_measure_rate = index;
	rc = regmap_write(ltr->regmap, LTR557_REG_ALS_MEAS_RATE, val | index);
	if (rc) {
		dev_err(&ltr->i2c->dev, "write %d failed\n",
				LTR557_REG_ALS_MEAS_RATE);
		goto exit;
	}

exit:
	return rc;
}


static int ltr557_ps_sync_delay(struct ltr557_data *ltr, unsigned int ps_delay)
{
	int index = 0;
	int i;
	int rc = 0;
	int min;

	if (!ltr->power_enabled) {
		dev_dbg(&ltr->i2c->dev, "power is not enabled\n");
		return 0;
	}

	min = abs(ps_delay - ps_mrr_table[1]);
	for (i = 1; i < ARRAY_SIZE(ps_mrr_table); i++) {
		if (ps_delay == ps_mrr_table[i]) {
			index = i;
			break;
		}
		if (min > abs(ps_delay - ps_mrr_table[i])) {
			min = abs(ps_delay - ps_mrr_table[i]);
			index = i;
		}
	}

	ltr->ps_measure_rate = index | 0x18;
	dev_dbg(&ltr->i2c->dev, "ps delay %d ms\n", ps_mrr_table[index]);

	rc = regmap_write(ltr->regmap, LTR557_REG_PS_MEAS_RATE, ltr->ps_measure_rate);
	if (rc) {
		dev_err(&ltr->i2c->dev, "write %d failed\n",
				LTR557_REG_PS_MEAS_RATE);
		goto exit;
	}

exit:
	return rc;
}

static void ltr557_als_enable_work(struct work_struct *work)
{
	struct ltr557_data *ltr = container_of(work, struct ltr557_data,
			als_enable_work);

	mutex_lock(&ltr->ops_lock);
	if (!ltr->power_enabled) { /* new HAL? */
		if (sensor_power_config(&ltr->i2c->dev, power_config,
					ARRAY_SIZE(power_config), true)) {
			dev_err(&ltr->i2c->dev, "power up sensor failed.\n");
			goto exit;
		}

		msleep(LTR557_BOOT_TIME_MS);
		ltr->power_enabled = true;
		if (ltr557_init_device(ltr)) {
			dev_err(&ltr->i2c->dev, "init device failed\n");
			goto exit_power_off;
		}

		ltr557_als_sync_delay(ltr, ltr->als_delay);
	}

	if (ltr557_enable_als(ltr, 1)) {
		dev_err(&ltr->i2c->dev, "enable als failed\n");
		goto exit_power_off;
	}

exit_power_off:
	if ((!ltr->als_enabled) && (!ltr->ps_enabled) &&
			ltr->power_enabled) {
		if (sensor_power_config(&ltr->i2c->dev, power_config,
					ARRAY_SIZE(power_config), false)) {
			dev_err(&ltr->i2c->dev, "power up sensor failed.\n");
			goto exit;
		}
		ltr->power_enabled = false;
	}
exit:
	mutex_unlock(&ltr->ops_lock);
}


static void ltr557_als_disable_work(struct work_struct *work)
{
	struct ltr557_data *ltr = container_of(work, struct ltr557_data,
			als_disable_work);

	mutex_lock(&ltr->ops_lock);

	if (ltr557_enable_als(ltr, 0)) {
		dev_err(&ltr->i2c->dev, "disable als failed\n");
		goto exit;
	}

	if ((!ltr->als_enabled) && (!ltr->ps_enabled) && ltr->power_enabled) {
		if (sensor_power_config(&ltr->i2c->dev, power_config,
					ARRAY_SIZE(power_config), false)) {
			dev_err(&ltr->i2c->dev, "power up sensor failed.\n");
			goto exit;
		}

		ltr->power_enabled = false;
	}

exit:
	mutex_unlock(&ltr->ops_lock);
}

static void ltr557_ps_enable_work(struct work_struct *work)
{
	struct ltr557_data *ltr = container_of(work, struct ltr557_data,
			ps_enable_work);

	mutex_lock(&ltr->ops_lock);
	if (!ltr->power_enabled) {
		if (sensor_power_config(&ltr->i2c->dev, power_config,
					ARRAY_SIZE(power_config), true)) {
			dev_err(&ltr->i2c->dev, "power up sensor failed.\n");
			goto exit;
		}

		msleep(LTR557_BOOT_TIME_MS);
		ltr->power_enabled = true;

		if (ltr557_init_device(ltr)) {
			dev_err(&ltr->i2c->dev, "init device failed\n");
			goto exit_power_off;
		}

		ltr557_ps_sync_delay(ltr, ltr->ps_delay);
	}

	if (ltr557_enable_ps(ltr, 1)) {
		dev_err(&ltr->i2c->dev, "enable ps failed\n");
		goto exit_power_off;
	}

exit_power_off:
	if ((!ltr->als_enabled) && (!ltr->ps_enabled) &&
			ltr->power_enabled) {
		if (sensor_power_config(&ltr->i2c->dev, power_config,
					ARRAY_SIZE(power_config), false)) {
			dev_err(&ltr->i2c->dev, "power up sensor failed.\n");
			goto exit;
		}
		ltr->power_enabled = false;
	}

exit:
	mutex_unlock(&ltr->ops_lock);
}

static void ltr557_ps_disable_work(struct work_struct *work)
{
	struct ltr557_data *ltr = container_of(work, struct ltr557_data,
			ps_disable_work);

	mutex_lock(&ltr->ops_lock);

	if (ltr557_enable_ps(ltr, 0)) {
		dev_err(&ltr->i2c->dev, "ltrsable ps failed\n");
		goto exit;
	}

	if ((!ltr->als_enabled) && (!ltr->ps_enabled) && ltr->power_enabled) {
		if (sensor_power_config(&ltr->i2c->dev, power_config,
					ARRAY_SIZE(power_config), false)) {
			dev_err(&ltr->i2c->dev, "power up sensor failed.\n");
			goto exit;
		}

		ltr->power_enabled = false;
	}
exit:
	mutex_unlock(&ltr->ops_lock);
}


static struct regmap_config ltr557_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
};

static int ltr557_cdev_enable_als(struct sensors_classdev *sensors_cdev,
		unsigned int enable)
{
	struct ltr557_data *ltr = container_of(sensors_cdev,
			struct ltr557_data, als_cdev);

	dev_dbg(&ltr->i2c->dev, "enable:%d\n", enable);

	mutex_lock(&ltr->ops_lock);

	if (enable)
		queue_work(ltr->workqueue, &ltr->als_enable_work);
	else
		queue_work(ltr->workqueue, &ltr->als_disable_work);

	mutex_unlock(&ltr->ops_lock);

	return 0;
}

static int ltr557_cdev_enable_ps(struct sensors_classdev *sensors_cdev,
		unsigned int enable)
{
	struct ltr557_data *ltr = container_of(sensors_cdev,
			struct ltr557_data, ps_cdev);

	dev_dbg(&ltr->i2c->dev, "enable:%d\n", enable);

	mutex_lock(&ltr->ops_lock);

	if (enable)
		queue_work(ltr->workqueue, &ltr->ps_enable_work);
	else
		queue_work(ltr->workqueue, &ltr->ps_disable_work);

	mutex_unlock(&ltr->ops_lock);

	return 0;
}

static int ltr557_cdev_set_als_delay(struct sensors_classdev *sensors_cdev,
		unsigned int delay_msec)
{
	struct ltr557_data *ltr = container_of(sensors_cdev,
			struct ltr557_data, als_cdev);
	int rc;

	mutex_lock(&ltr->ops_lock);

	ltr->als_delay = delay_msec;
	rc = ltr557_als_sync_delay(ltr, delay_msec);

	mutex_unlock(&ltr->ops_lock);

	return rc;
}

static int ltr557_cdev_set_ps_delay(struct sensors_classdev *sensors_cdev,
		unsigned int delay_msec)
{
	struct ltr557_data *ltr = container_of(sensors_cdev,
			struct ltr557_data, ps_cdev);
	int rc;

	mutex_lock(&ltr->ops_lock);

	ltr->ps_delay = delay_msec;
	rc = ltr557_ps_sync_delay(ltr, delay_msec);

	mutex_unlock(&ltr->ops_lock);

	return 0;
}

static int ltr557_cdev_ps_flush(struct sensors_classdev *sensors_cdev)
{
	struct ltr557_data *ltr = container_of(sensors_cdev,
			struct ltr557_data, ps_cdev);

	input_event(ltr->input_proximity, EV_SYN, SYN_CONFIG,
			ltr->flush_count++);
	input_sync(ltr->input_proximity);

	return 0;
}

static int ltr557_cdev_als_flush(struct sensors_classdev *sensors_cdev)
{
	struct ltr557_data *ltr = container_of(sensors_cdev,
			struct ltr557_data, als_cdev);

	input_event(ltr->input_light, EV_SYN, SYN_CONFIG, ltr->flush_count++);
	input_sync(ltr->input_light);

	return 0;
}

/* This function should be called when sensor is disabled */
static int ltr557_cdev_ps_calibrate(struct sensors_classdev *sensors_cdev,
		int axis, int apply_now)
{
	int rc;
	int power;
	unsigned int config;
	unsigned int interrupt;
	u16 min = PS_DATA_MASK;
	u8 ps_data[2];
	int count = LTR557_CALIBRATE_SAMPLES;
	struct ltr557_data *ltr = container_of(sensors_cdev,
			struct ltr557_data, ps_cdev);


	if (axis != AXIS_BIAS)
		return 0;

	mutex_lock(&ltr->ops_lock);

	/* Ensure only be called when sensors in standy mode */
	if (ltr->als_enabled || ltr->ps_enabled) {
		rc = -EPERM;
		goto exit;
	}

	power = ltr->power_enabled;
	if (!power) {
		rc = sensor_power_config(&ltr->i2c->dev, power_config,
					ARRAY_SIZE(power_config), true);
		if (rc) {
			dev_err(&ltr->i2c->dev, "power up sensor failed.\n");
			goto exit;
		}

		msleep(LTR557_BOOT_TIME_MS);

		rc = ltr557_init_device(ltr);
		if (rc) {
			dev_err(&ltr->i2c->dev, "init ltr557 failed\n");
			goto exit;
		}
	}

	rc = regmap_read(ltr->regmap, LTR557_REG_INTERRUPT, &interrupt);
	if (rc) {
		dev_err(&ltr->i2c->dev, "read interrupt configuration failed\n");
		goto exit_power_off;
	}

	/* disable interrupt */
	rc = regmap_write(ltr->regmap, LTR557_REG_INTERRUPT, 0x10);
	if (rc) {
		dev_err(&ltr->i2c->dev, "disable interrupt failed\n");
		goto exit_power_off;
	}

	rc = regmap_read(ltr->regmap, LTR557_REG_MAIN_CTL, &config);
	if (rc) {
		dev_err(&ltr->i2c->dev, "read %d failed.(%d)\n",
				LTR557_REG_MAIN_CTL, rc);
		goto exit_enable_interrupt;
	}

	/* clear offset */
	ps_data[0] = 0;
	ps_data[1] = 0;
	rc = regmap_bulk_write(ltr->regmap, LTR557_REG_PS_OFFSET_0,
			ps_data, 2);
	if (rc) {
		dev_err(&ltr->i2c->dev, "write %d failed.(%d)\n",
				LTR557_REG_PS_OFFSET_0, rc);
		goto exit_enable_interrupt;
	}

	/* enable ps sensor */
	rc = regmap_write(ltr->regmap, LTR557_REG_MAIN_CTL, config | 0x01);
	if (rc) {
		dev_err(&ltr->i2c->dev, "write %d failed.(%d)\n",
				LTR557_REG_MAIN_CTL, rc);
		goto exit_enable_interrupt;
	}

	/* ps measurement rate set to fastest rate */
	rc = regmap_write(ltr->regmap, LTR557_REG_PS_MEAS_RATE,
			LTR557_PS_MEASUREMENT_RATE_6_25MS);
	if (rc) {
		dev_err(&ltr->i2c->dev, "read %d failed.(%d)\n",
				LTR557_REG_PS_MEAS_RATE, rc);
		goto exit_enable_interrupt;
	}

	msleep(LTR557_WAKE_TIME_MS);

	while (--count) {
		/* the measurement rate is 6.25 ms */
		usleep_range(9000, 10000);
		rc = regmap_bulk_read(ltr->regmap, LTR557_REG_PS_DATA_0,
				ps_data, 2);
		if (rc) {
			dev_err(&ltr->i2c->dev, "read PS data failed\n");
			break;
		}
		if (min > ((ps_data[1] << 8) | ps_data[0]))
			min = (ps_data[1] << 8) | ps_data[0];
	}

	/* disable ps sensor */
	rc = regmap_write(ltr->regmap, LTR557_REG_MAIN_CTL, config);
	if (rc) {
		dev_err(&ltr->i2c->dev, "write %d failed.(%d)\n",
				LTR557_REG_MAIN_CTL, rc);
		goto exit_enable_interrupt;
	}

	if (!count) {
		if (min > (PS_DATA_MASK >> 1)) {
			dev_err(&ltr->i2c->dev, "ps data out of range, check if shield\n");
			rc = -EINVAL;
			goto exit_enable_interrupt;
		}

		if (apply_now) {
			ps_data[1] = PS_LOW_BYTE(min);
			ps_data[0] = PS_HIGH_BYTE(min);
			rc = regmap_bulk_write(ltr->regmap,
					LTR557_REG_PS_OFFSET_0, ps_data, 2);
			if (rc) {
				dev_err(&ltr->i2c->dev, "write %d failed.(%d)\n",
						LTR557_REG_PS_OFFSET_0, rc);
				goto exit_enable_interrupt;
			}
			ltr->bias = min;
		}

		snprintf(ltr->calibrate_buf, sizeof(ltr->calibrate_buf),
				"0,0,%d", min);
		dev_dbg(&ltr->i2c->dev, "result: %s\n", ltr->calibrate_buf);
	} else {
		dev_err(&ltr->i2c->dev, "calibration failed\n");
		rc = -EINVAL;
	}

exit_enable_interrupt:
	if (regmap_write(ltr->regmap, LTR557_REG_INTERRUPT, interrupt)) {
		dev_err(&ltr->i2c->dev, "enable interrupt failed\n");
		goto exit_power_off;
	}

exit_power_off:
	if (!power) {
		if (sensor_power_config(&ltr->i2c->dev, power_config,
					ARRAY_SIZE(power_config), false)) {
			dev_err(&ltr->i2c->dev, "power off sensor failed.\n");
			goto exit;
		}
	}
exit:
	mutex_unlock(&ltr->ops_lock);
	return rc;
}

static int ltr557_cdev_ps_write_cal(struct sensors_classdev *sensors_cdev,
		struct cal_result_t *cal_result)
{
	int power;
	u8 ps_data[2];
	int rc = 0;
	struct ltr557_data *ltr = container_of(sensors_cdev,
			struct ltr557_data, ps_cdev);

	mutex_lock(&ltr->ops_lock);
	power = ltr->power_enabled;
	if (!power) {
		rc = sensor_power_config(&ltr->i2c->dev, power_config,
					ARRAY_SIZE(power_config), true);
		if (rc) {
			dev_err(&ltr->i2c->dev, "power up sensor failed.\n");
			goto exit;
		}
	}

	ltr->bias = cal_result->bias;
	ps_data[1] = PS_LOW_BYTE(cal_result->bias);
	ps_data[0] = PS_HIGH_BYTE(cal_result->bias);
	rc = regmap_bulk_write(ltr->regmap, LTR557_REG_PS_OFFSET_0,
			ps_data, 2);
	if (rc) {
		dev_err(&ltr->i2c->dev, "write %d failed.(%d)\n",
				LTR557_REG_PS_OFFSET_0, rc);
		goto exit_power_off;
	}

	snprintf(ltr->calibrate_buf, sizeof(ltr->calibrate_buf), "0,0,%d",
			ltr->bias);

exit_power_off:
	if (!power) {
		if (sensor_power_config(&ltr->i2c->dev, power_config,
					ARRAY_SIZE(power_config), false)) {
			dev_err(&ltr->i2c->dev, "power off sensor failed.\n");
			goto exit;
		}
	}
exit:

	mutex_unlock(&ltr->ops_lock);
	return rc;
};

static ssize_t ltr557_register_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct ltr557_data *ltr = dev_get_drvdata(dev);
	unsigned int val;
	int rc;
	ssize_t count = 0;
	int i;
	bool pwrflg = false;
	
	u8 ps_data[3] = {};
	u8 als_data[3] = {};
	
	mutex_lock(&ltr->ops_lock);
	
	if (!ltr->power_enabled) {
		sensor_power_config(&ltr->i2c->dev, power_config, ARRAY_SIZE(power_config), true);
		msleep(LTR557_BOOT_TIME_MS);
		pwrflg = true;
	}
	
	if (ltr->reg_addr == LTR557_REG_MAGIC) {
		for (i = 0; i <= 0x26; i++) {
			rc = regmap_read(ltr->regmap, LTR557_REG_MAIN_CTL + i,
					&val);
			if (rc) {
				dev_err(&ltr->i2c->dev, "read %d failed\n",
						LTR557_REG_MAIN_CTL + i);
				//break;
				continue;
			}
			count += snprintf(&buf[count], PAGE_SIZE, "%02x\n", val);
		}
	} else if (ltr->reg_addr == LTR557_REG_PS_MAGIC) {
		rc = regmap_bulk_read(ltr->regmap, LTR557_REG_MAIN_STATUS, ps_data, 3);
		if (rc) {
			dev_err(&ltr->i2c->dev, "Ps read failed:%d\n", rc);
			goto out;
		}
		for (i = 0; i < 3; i++) {
			dev_dbg(&ltr->i2c->dev, "addr:0x%02X, data:0x%02X\n", LTR557_REG_MAIN_STATUS + i, ps_data[i]);
			count += snprintf(&buf[count], PAGE_SIZE, "%02x\n", ps_data[i]);
		}
	} else if (ltr->reg_addr == LTR557_REG_ALS_MAGIC) {
		rc = regmap_bulk_read(ltr->regmap, LTR557_REG_ALS_DATA_0, als_data, 3);
		if (rc) {
			dev_err(&ltr->i2c->dev, "Als read failed:%d\n", rc);
			goto out;
		}
		for (i = 0; i < 3; i++) {
			dev_dbg(&ltr->i2c->dev, "addr:0x%02X, data:0x%02X\n", LTR557_REG_ALS_DATA_0 + i, als_data[i]);
			count += snprintf(&buf[count], PAGE_SIZE, "%02x\n", als_data[i]);
		}
	} else {
		rc = regmap_read(ltr->regmap, ltr->reg_addr, &val);
		if (rc) {
			dev_err(&ltr->i2c->dev, "read %d failed\n",
					ltr->reg_addr);
			goto out;
		}
		count += snprintf(&buf[count], PAGE_SIZE, "%02x\n", val);
	}

	if (pwrflg == true) {
		sensor_power_config(&ltr->i2c->dev, power_config, ARRAY_SIZE(power_config), false);
	}
	mutex_unlock(&ltr->ops_lock);
	return count;

out:
	if (pwrflg == true) {
		sensor_power_config(&ltr->i2c->dev, power_config, ARRAY_SIZE(power_config), false);
	}
	mutex_unlock(&ltr->ops_lock);
	return rc;
}

static ssize_t ltr557_register_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct ltr557_data *ltr = dev_get_drvdata(dev);
	unsigned int reg;
	unsigned int val;
	unsigned int cmd;
	int rc;
	bool pwrflg = false;

	if (sscanf(buf, "%u %u %u\n", &cmd, &reg, &val) < 2) {
		dev_err(&ltr->i2c->dev, "argument error\n");
		return -EINVAL;
	}

	mutex_lock(&ltr->ops_lock);
	
	if (cmd == CMD_WRITE) {
		if (!ltr->power_enabled) {
			sensor_power_config(&ltr->i2c->dev, power_config, ARRAY_SIZE(power_config), true);
			msleep(LTR557_BOOT_TIME_MS);
			pwrflg = true;
		}

		rc = regmap_write(ltr->regmap, reg, val);
		if (rc) {
			dev_err(&ltr->i2c->dev, "write %d failed\n", reg);
			goto out;
		}

		if (pwrflg == true) {
			sensor_power_config(&ltr->i2c->dev, power_config, ARRAY_SIZE(power_config), false);
		}
	} else if (cmd == CMD_READ) {
		ltr->reg_addr = reg;
		dev_dbg(&ltr->i2c->dev, "register address set to 0x%x\n", reg);
	}

	mutex_unlock(&ltr->ops_lock);
	return size;

out:
	if (pwrflg == true) {
		sensor_power_config(&ltr->i2c->dev, power_config, ARRAY_SIZE(power_config), false);
	}
	mutex_unlock(&ltr->ops_lock);
	return rc;
}

static DEVICE_ATTR(register, S_IWUSR | S_IRUGO,
		ltr557_register_show,
		ltr557_register_store);

static struct attribute *ltr557_attr[] = {
	&dev_attr_register.attr,
	NULL
};

static const struct attribute_group ltr557_attr_group = {
	.attrs = ltr557_attr,
};

static int ltr557_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct ltr557_data *ltr;
	int res = 0;

	printk("probling ltr557...\n");

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		printk("ltr557 i2c check failed.\n");
		return -ENODEV;
	}

	ltr = devm_kzalloc(&client->dev, sizeof(struct ltr557_data),
			GFP_KERNEL);
	if (!ltr) {
		printk("memory allocation failed,\n");
		return -ENOMEM;
	}

	ltr->i2c = client;

	if (client->dev.of_node) {
		res = ltr557_parse_dt(&client->dev, ltr);
		if (res) {
			printk("unable to parse device tree.(%d)\n", res);
			goto out;
		}
	} else {
		printk("device tree not found.\n");
		res = -ENODEV;
		goto out;
	}

	dev_set_drvdata(&client->dev, ltr);
	mutex_init(&ltr->ops_lock);

	ltr->regmap = devm_regmap_init_i2c(client, &ltr557_regmap_config);
	if (IS_ERR(ltr->regmap)) {
		printk("init regmap failed.(%ld)\n", PTR_ERR(ltr->regmap));
		res = PTR_ERR(ltr->regmap);
		goto out;
	}

	res = sensor_power_init(&client->dev, power_config,
			ARRAY_SIZE(power_config));
	if (res) {
		printk("init power failed.\n");
		goto out;
	}

	res = sensor_power_config(&client->dev, power_config,
			ARRAY_SIZE(power_config), true);
	if (res) {
		printk("power up sensor failed.\n");
		goto err_power_config;
	}

	res = sensor_pinctrl_init(&client->dev, &pin_config);
	if (res) {
		printk("init pinctrl failed.\n");
		goto err_pinctrl_init;
	}

	msleep(LTR557_BOOT_TIME_MS);

	res = ltr557_check_device(ltr);
	if (res) {
		printk("check device failed.\n");
		goto err_check_device;
	}

	ltr->als_measure_rate = LTR557_ALS_DEFAULT_MEASURE_RATE;
	ltr->ps_measure_rate = LTR557_PS_MEASUREMENT_RATE_25MS;

	res = ltr557_init_device(ltr);
	if (res) {
		printk("check device failed.\n");
		goto err_init_device;
	}

	/* configure interrupt */
	if (gpio_is_valid(ltr->irq_gpio)) {
		res = gpio_request(ltr->irq_gpio, "ltr557_interrupt");
		if (res) {
			printk("unable to request interrupt gpio %d\n", ltr->irq_gpio);
			goto err_request_gpio;
		}

		res = gpio_direction_input(ltr->irq_gpio);
		if (res) {
			printk("unable to set direction for gpio %d\n", ltr->irq_gpio);
			goto err_set_direction;
		}

		ltr->irq = gpio_to_irq(ltr->irq_gpio);

		res = devm_request_irq(&client->dev, ltr->irq,
				ltr557_irq_handler,
				ltr->irq_flags | IRQF_ONESHOT,
				"ltr557", ltr);

		if (res) {
			printk("request irq %d failed(%d),\n", ltr->irq, res);
			goto err_request_irq;
		}

		/* device wakeup initialization */
		device_init_wakeup(&client->dev, 1);

		ltr->workqueue = alloc_workqueue("ltr557_workqueue", WQ_FREEZABLE, 0);
		INIT_WORK(&ltr->report_work, ltr557_report_work);
		INIT_WORK(&ltr->als_enable_work, ltr557_als_enable_work);
		INIT_WORK(&ltr->als_disable_work, ltr557_als_disable_work);
		INIT_WORK(&ltr->ps_enable_work, ltr557_ps_enable_work);
		INIT_WORK(&ltr->ps_disable_work, ltr557_ps_disable_work);
	} else {
		res = -ENODEV;
		goto err_init_device;
	}

	res = sysfs_create_group(&client->dev.kobj, &ltr557_attr_group);
	if (res) {
		printk("sysfs create group failed\n");
		goto err_create_group;
	}

	res = ltr557_init_input(ltr);
	if (res) {
		printk("init input failed.\n");
		goto err_init_input;
	}

	ltr557_init_calibinfo(&ltr->calibinfo);

	ltr->als_cdev = als_cdev;
	ltr->als_cdev.sensors_enable = ltr557_cdev_enable_als;
	ltr->als_cdev.sensors_poll_delay = ltr557_cdev_set_als_delay;
	ltr->als_cdev.sensors_flush = ltr557_cdev_als_flush;
	res = sensors_classdev_register(&ltr->input_light->dev, &ltr->als_cdev);
	if (res) {
		printk("sensors class register failed.\n");
		goto err_register_als_cdev;
	}

	ltr->ps_cdev = ps_cdev;
	ltr->ps_cdev.sensors_enable = ltr557_cdev_enable_ps;
	ltr->ps_cdev.sensors_poll_delay = ltr557_cdev_set_ps_delay;
	ltr->ps_cdev.sensors_flush = ltr557_cdev_ps_flush;
	ltr->ps_cdev.sensors_calibrate = ltr557_cdev_ps_calibrate;
	ltr->ps_cdev.sensors_write_cal_params = ltr557_cdev_ps_write_cal;
	ltr->ps_cdev.params = ltr->calibrate_buf;
	res = sensors_classdev_register(&ltr->input_proximity->dev,
			&ltr->ps_cdev);
	if (res) {
		printk("sensors class register failed.\n");
		goto err_register_ps_cdev;
	}

	ltr->ps_cal = 0;
	ltr->als_delay = LTR557_ALS_REPEAT_RATE_DEFAULT;
	ltr->ps_delay  = LTR557_PS_REPEAT_RATE_DEFAULT;

	sensor_power_config(&client->dev, power_config,
			ARRAY_SIZE(power_config), false);

	printk("ltr557 successfully probed!\n");
	
	return 0;

err_register_ps_cdev:
	sensors_classdev_unregister(&ltr->als_cdev);
err_register_als_cdev:
err_init_input:
	sysfs_remove_group(&client->dev.kobj, &ltr557_attr_group);
err_create_group:
err_request_irq:
err_set_direction:
	gpio_free(ltr->irq_gpio);
err_request_gpio:
err_init_device:
	device_init_wakeup(&client->dev, 0);
err_check_device:
err_pinctrl_init:
	sensor_power_config(&client->dev, power_config,
			ARRAY_SIZE(power_config), false);
err_power_config:
	sensor_power_deinit(&client->dev, power_config,
			ARRAY_SIZE(power_config));
out:
	return res;
}

static int ltr557_remove(struct i2c_client *client)
{
	struct ltr557_data *ltr = dev_get_drvdata(&client->dev);

	sensors_classdev_unregister(&ltr->ps_cdev);
	sensors_classdev_unregister(&ltr->als_cdev);

	if (ltr->input_light)
		input_unregister_device(ltr->input_light);

	if (ltr->input_proximity)
		input_unregister_device(ltr->input_proximity);

	destroy_workqueue(ltr->workqueue);
	device_init_wakeup(&ltr->i2c->dev, 0);
	sensor_power_config(&client->dev, power_config,
			ARRAY_SIZE(power_config), false);
	sensor_power_deinit(&client->dev, power_config,
			ARRAY_SIZE(power_config));
	return 0;
}

static int ltr557_suspend(struct device *dev)
{
	int res = 0;
	struct ltr557_data *ltr = dev_get_drvdata(dev);
	u8 ps_data[4];
	unsigned int config;
	int idx = ltr->ps_wakeup_threshold;

	dev_dbg(dev, "suspending ltr557...");

	mutex_lock(&ltr->ops_lock);

	/* proximity is enabled */
	if (ltr->ps_enabled) {
		/* disable als sensor to avoid wake up by als interrupt */
		if (ltr->als_enabled) {
			res = regmap_read(ltr->regmap, LTR557_REG_MAIN_CTL,
					&config);
			if (res) {
				dev_err(&ltr->i2c->dev, "read %d failed.(%d)\n",
						LTR557_REG_MAIN_CTL, res);
				return res;
			}

			res = regmap_write(ltr->regmap, LTR557_REG_MAIN_CTL,
					config & (~0x2));
			if (res) {
				dev_err(&ltr->i2c->dev, "write %d failed.(%d)\n",
						LTR557_REG_MAIN_CTL, res);
				goto exit;
			}
		}

		/* Don't power off sensor because proximity is a
		 * wake up sensor.
		 */
		if (device_may_wakeup(&ltr->i2c->dev)) {
			dev_dbg(&ltr->i2c->dev, "enable irq wake\n");
			enable_irq_wake(ltr->irq);
		}

		/* Setup threshold to avoid frequent wakeup */
		if (device_may_wakeup(&ltr->i2c->dev) &&
				(idx != LTR557_WAKEUP_ANY_CHANGE)) {
			dev_dbg(&ltr->i2c->dev, "last ps: %d\n", ltr->last_ps);
			if (ltr->last_ps > idx) {
				ps_data[2] = 0x0;
				ps_data[3] = 0x0;
				ps_data[0] =
					PS_LOW_BYTE(ps_distance_table[idx]);
				ps_data[1] =
					PS_HIGH_BYTE(ps_distance_table[idx]);
			} else {
				ps_data[2] =
					PS_LOW_BYTE(ps_distance_table[idx]);
				ps_data[3] =
					PS_HIGH_BYTE(ps_distance_table[idx]);
				ps_data[0] = PS_LOW_BYTE(PS_DATA_MASK);
				ps_data[1] = PS_HIGH_BYTE(PS_DATA_MASK);
			}

			res = regmap_bulk_write(ltr->regmap,
					LTR557_REG_PS_THRES_UP_0, ps_data, 4);
			if (res) {
				dev_err(&ltr->i2c->dev, "set up threshold failed\n");
				goto exit;
			}
		}
	} else {
		/* power off */
		disable_irq(ltr->irq);
		if (ltr->power_enabled) {
			res = sensor_power_config(dev, power_config,
					ARRAY_SIZE(power_config), false);
			if (res) {
				dev_err(dev, "failed to suspend ltr557\n");
				enable_irq(ltr->irq);
				goto exit;
			}
		}
		pinctrl_select_state(pin_config.pinctrl, pin_config.state[1]);
	}
exit:
	mutex_unlock(&ltr->ops_lock);
	return res;
}

static int ltr557_resume(struct device *dev)
{
	int res = 0;
	struct ltr557_data *ltr = dev_get_drvdata(dev);
	unsigned int config;

	dev_dbg(dev, "resuming ltr557...");
	if (ltr->ps_enabled) {
		if (device_may_wakeup(&ltr->i2c->dev)) {
			dev_dbg(&ltr->i2c->dev, "disable irq wake\n");
			disable_irq_wake(ltr->irq);
		}

		if (ltr->als_enabled) {
			res = regmap_read(ltr->regmap, LTR557_REG_MAIN_CTL,
					&config);
			if (res) {
				dev_err(&ltr->i2c->dev, "read %d failed.(%d)\n",
						LTR557_REG_MAIN_CTL, res);
				goto exit;
			}

			res = regmap_write(ltr->regmap, LTR557_REG_MAIN_CTL,
					config | 0x2);
			if (res) {
				dev_err(&ltr->i2c->dev, "write %d failed.(%d)\n",
						LTR557_REG_MAIN_CTL, res);
				goto exit;
			}
		}
	} else {
		pinctrl_select_state(pin_config.pinctrl, pin_config.state[0]);
		/* Power up sensor */
		if (ltr->power_enabled) {
			res = sensor_power_config(dev, power_config,
					ARRAY_SIZE(power_config), true);
			if (res) {
				dev_err(dev, "failed to power up ltr557\n");
				goto exit;
			}
			msleep(LTR557_BOOT_TIME_MS);

			res = ltr557_init_device(ltr);
			if (res) {
				dev_err(dev, "failed to init ltr557\n");
				goto exit_power_off;
			}
		}

		if (ltr->als_enabled) {
			res = ltr557_enable_als(ltr, ltr->als_enabled);
			if (res) {
				dev_err(dev, "failed to enable ltr557\n");
				goto exit_power_off;
			}
		}

		enable_irq(ltr->irq);
	}

	return res;

exit_power_off:
	if ((!ltr->als_enabled) && (!ltr->ps_enabled) &&
			ltr->power_enabled) {
		if (sensor_power_config(&ltr->i2c->dev, power_config,
					ARRAY_SIZE(power_config), false)) {
			dev_err(&ltr->i2c->dev, "power up sensor failed.\n");
			goto exit;
		}
		ltr->power_enabled = false;
	}

exit:
	return res;
}

static const struct i2c_device_id ltr557_id[] = {
	{ LTR557_I2C_NAME, 0 },
	{ }
};

static struct of_device_id ltr557_match_table[] = {
	{ .compatible = "sharp,ltr557", },
	{ },
};
MODULE_DEVICE_TABLE(of, ltr557_match_table);

static const struct dev_pm_ops ltr557_pm_ops = {
	.suspend = ltr557_suspend,
	.resume = ltr557_resume,
};

static struct i2c_driver ltr557_driver = {
	.probe = ltr557_probe,
	.remove = ltr557_remove,
	.id_table = ltr557_id,
	.driver = {
		.owner = THIS_MODULE,
		.name = LTR557_I2C_NAME,
		.of_match_table = ltr557_match_table,
		.pm = &ltr557_pm_ops,
	},
};

module_i2c_driver(ltr557_driver);

MODULE_DESCRIPTION("LTR-557ALPS Driver");
MODULE_LICENSE("GPL v2");
