/*
 * FPC1020 Fingerprint sensor device driver
 *
 * This driver will control the platform resources that the FPC fingerprint
 * sensor needs to operate. The major things are probing the sensor to check
 * that it is actually connected and let the Kernel know this and with that also
 * enabling and disabling of regulators, enabling and disabling of platform
 * clocks, controlling GPIOs such as SPI chip select, sensor reset line, sensor
 * IRQ line, MISO and MOSI lines.
 *
 * The driver will expose most of its available functionality in sysfs which
 * enables dynamic control of these features from eg. a user space process.
 *
 * The sensor's IRQ events will be pushed to Kernel's event handling system and
 * are exposed in the drivers event node. This makes it possible for a user
 * space process to poll the input node and receive IRQ events easily. Usually
 * this node is available under /dev/input/eventX where 'X' is a number given by
 * the event system. A user space process will need to traverse all the event
 * nodes and ask for its parent's name (through EVIOCGNAME) which should match
 * the value in device tree named input-device-name.
 *
 * This driver will NOT send any SPI commands to the sensor it only controls the
 * electrical parts.
 *
 *
 * Copyright (c) 2015 Fingerprint Cards AB <tech@fingerprints.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License Version 2
 * as published by the Free Software Foundation.
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/wakelock.h>
#include <soc/qcom/scm.h>

#define FPC1020_RESET_LOW_US 1000
#define FPC1020_RESET_HIGH1_US 100
#define FPC1020_RESET_HIGH2_US 1250
#define FPC_TTW_HOLD_TIME 1000

#define BBOX_FPC_PROB_FAIL do {printk("BBox::UEC;39::0\n");} while (0);
#define BBOX_FPC_RESET_FAIL do {printk("BBox::UEC;39::1\n");} while (0);
#define BBOX_FPC_RESET_CHIP_FAIL do {printk("BBox::UEC;39::2\n");} while (0);
#define BBOX_FPC_RESET_CHIP_HOLD_LOW_FAIL do {printk("BBox::UEC;39::3\n");} while (0);

static struct class *fp_class;
static atomic_t fp_atomic;

struct fpc1020_data {
	struct device *dev;
	int irq_gpio;
	int rst_gpio;
	int fpid_gpio;
	int irq_num;
	struct mutex lock;
	bool prepared;
	bool wakeup_enabled;

	struct pinctrl *ts_pinctrl;
	struct pinctrl_state *gpio_state_active;
	struct pinctrl_state *gpio_state_suspend;
	struct wake_lock ttw_wl;
	struct regulator *pwr_reg;
};

static int fpc1020_request_named_gpio(struct fpc1020_data *fpc1020,
		const char *label, int *gpio)
{
	struct device *dev = fpc1020->dev;
	struct device_node *np = dev->of_node;
	int rc = of_get_named_gpio(np, label, 0);
	if (rc < 0) {
		dev_err(dev, "failed to get '%s'\n", label);
		printk("BBox;failed to get '%s'\n", label);
		BBOX_FPC_PROB_FAIL;
		return rc;
	}
	*gpio = rc;
	rc = devm_gpio_request(dev, *gpio, label);
	if (rc) {
		dev_err(dev, "failed to request gpio %d\n", *gpio);
		printk("BBox;failed to request gpio %d\n", *gpio);
		BBOX_FPC_PROB_FAIL;
		return rc;
	}
	return rc;
}

/* -------------------------------------------------------------------- */
static int fpc1020_pinctrl_init(struct fpc1020_data *fpc1020)
{
	int ret = 0;
	struct device *dev = fpc1020->dev;

	fpc1020->ts_pinctrl = devm_pinctrl_get(dev);
	if (IS_ERR_OR_NULL(fpc1020->ts_pinctrl)) {
		dev_err(dev, "Target does not use pinctrl\n");
		printk("BBox;Target does not use pinctrl\n");
		BBOX_FPC_PROB_FAIL;
		ret = PTR_ERR(fpc1020->ts_pinctrl);
		goto err;
	}

	fpc1020->gpio_state_active =
		pinctrl_lookup_state(fpc1020->ts_pinctrl, "pmx_fp_active");
	if (IS_ERR_OR_NULL(fpc1020->gpio_state_active)) {
		dev_err(dev, "Cannot get active pinstate\n");
		printk("BBox;Cannot get active pinstate\n");
		BBOX_FPC_PROB_FAIL;
		ret = PTR_ERR(fpc1020->gpio_state_active);
		goto err;
	}

	fpc1020->gpio_state_suspend =
		pinctrl_lookup_state(fpc1020->ts_pinctrl, "pmx_fp_suspend");
	if (IS_ERR_OR_NULL(fpc1020->gpio_state_suspend)) {
		dev_err(dev, "Cannot get sleep pinstate\n");
		printk("BBox;Cannot get sleep pinstate\n");
		BBOX_FPC_PROB_FAIL;
		ret = PTR_ERR(fpc1020->gpio_state_suspend);
		goto err;
	}

	return 0;
err:
	fpc1020->ts_pinctrl = NULL;
	fpc1020->gpio_state_active = NULL;
	fpc1020->gpio_state_suspend = NULL;
	return ret;
}

/* -------------------------------------------------------------------- */
static int fpc1020_pinctrl_select(struct fpc1020_data *fpc1020, bool on)
{
	int ret = 0;
	struct pinctrl_state *pins_state;
	struct device *dev = fpc1020->dev;

	pins_state = on ? fpc1020->gpio_state_active : fpc1020->gpio_state_suspend;
	if (IS_ERR_OR_NULL(pins_state)) {
		dev_err(dev, "not a valid '%s' pinstate\n",
			on ? "pmx_fp_active" : "pmx_fp_suspend");
		printk("BBox;not a valid '%s' pinstate\n", on ? "pmx_fp_active" : "pmx_fp_suspend");
		BBOX_FPC_PROB_FAIL;
		return -1;
	}

	ret = pinctrl_select_state(fpc1020->ts_pinctrl, pins_state);
	if (ret) {
		dev_err(dev, "can not set %s pins\n",
			on ? "pmx_fp_active" : "pmx_fp_suspend");
		printk("BBox;can not set %s pins\n", on ? "pmx_fp_active" : "pmx_fp_suspend");
		BBOX_FPC_PROB_FAIL;
	}

	return ret;
}

/**
 * sysfs node nodify FP vendor information
 */
static ssize_t fp_vendor_get(struct device *device,
			     struct device_attribute *attribute,
			     char* buffer)
{
	struct  fpc1020_data *fpc1020 = dev_get_drvdata(device);
	int fpid_status = gpio_get_value(fpc1020->fpid_gpio);

	return scnprintf(buffer, PAGE_SIZE, "%s\n", fpid_status? "FPC SUNTEL":"FPC MCNEX");
}
static DEVICE_ATTR(fp_vendor, S_IRUSR | S_IRGRP | S_IROTH, fp_vendor_get, NULL);

/**
 * sysfs node for controlling whether the driver is allowed
 * to wake up the platform on interrupt.
 */
static ssize_t wakeup_enable_set(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct  fpc1020_data *fpc1020 = dev_get_drvdata(dev);

	if (!strncmp(buf, "enable", strlen("enable"))) {
		fpc1020->wakeup_enabled = true;
	} else if (!strncmp(buf, "disable", strlen("disable"))) {
		fpc1020->wakeup_enabled = false;
	} else
		return -EINVAL;

	return count;
}
static DEVICE_ATTR(wakeup_enable, S_IWUSR, NULL, wakeup_enable_set);

/**
 * sysf node to check the interrupt status of the sensor, the interrupt
 * handler should perform sysf_notify to allow userland to poll the node.
 */
static ssize_t irq_get(struct device *device,
			     struct device_attribute *attribute,
			     char* buffer)
{
	struct fpc1020_data *fpc1020 = dev_get_drvdata(device);
	int irq = gpio_get_value(fpc1020->irq_gpio);
	return scnprintf(buffer, PAGE_SIZE, "%i\n", irq);
}


/**
 * writing to the irq node will just drop a printk message
 * and return success, used for latency measurement.
 */
static ssize_t irq_ack(struct device *device,
			     struct device_attribute *attribute,
			     const char *buffer, size_t count)
{
	struct fpc1020_data *fpc1020 = dev_get_drvdata(device);
	dev_dbg(fpc1020->dev, "%s\n", __func__);
	return count;
}
static DEVICE_ATTR(irq, S_IRUSR | S_IWUSR, irq_get, irq_ack);

static struct attribute *attributes[] = {
	&dev_attr_irq.attr,
	&dev_attr_wakeup_enable.attr,
	&dev_attr_fp_vendor.attr,
	NULL
};

static const struct attribute_group attribute_group = {
	.attrs = attributes,
};

static irqreturn_t fpc1020_irq_handler(int irq, void *handle)
{
	struct fpc1020_data *fpc1020 = handle;
	dev_dbg(fpc1020->dev, "%s\n", __func__);

	if (fpc1020->wakeup_enabled) {
		wake_lock_timeout(&fpc1020->ttw_wl,
					msecs_to_jiffies(FPC_TTW_HOLD_TIME));
	}

	sysfs_notify(&fpc1020->dev->kobj, NULL, dev_attr_irq.attr.name);

	return IRQ_HANDLED;
}

static int fpc1020_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device *fp_dev;
	int rc = 0;
	int irqf;
	struct device_node *np = dev->of_node;
	struct fpc1020_data *fpc1020 = devm_kzalloc(dev, sizeof(*fpc1020),
			GFP_KERNEL);

	pr_info("%s start.\n",__func__);
	//Win add for check FPM vendor
	pr_info("%s start check FPM vendor.\n",__func__);
	if(strstr(saved_command_line, "androidboot.fp=fpc") == NULL)
	{
		rc = -1;
		goto exit;
	}
	pr_info("%s end check FPM vendor.\n",__func__);
	//Win add for check FPM vendor

	if (!fpc1020) {
		dev_err(dev,
			"failed to allocate memory for struct fpc1020_data\n");
		printk("BBox;failed to allocate memory for struct fpc1020_data\n");
		BBOX_FPC_PROB_FAIL;
		rc = -ENOMEM;
		goto exit;
	}

	fpc1020->dev = dev;
	dev_set_drvdata(dev, fpc1020);

	if (!np) {
		dev_err(dev, "no of node found\n");
		printk("BBox;no of node found\n");
		BBOX_FPC_PROB_FAIL;
		rc = -EINVAL;
		goto exit;
	}

	rc = fpc1020_request_named_gpio(fpc1020, "fpc,irq-gpio",
			&fpc1020->irq_gpio);
	if (rc)
		goto exit;

	rc = gpio_direction_input(fpc1020->irq_gpio);

	if (rc) {
		dev_err(fpc1020->dev,
			"gpio_direction_input (irq) failed.\n");
		printk("BBox;gpio_direction_input (irq) failed.\n");
		BBOX_FPC_PROB_FAIL;
		goto exit;
	}

	rc = fpc1020_request_named_gpio(fpc1020, "fpc,reset-gpio",
			&fpc1020->rst_gpio);
	if (rc)
		goto exit;

	rc = fpc1020_request_named_gpio(fpc1020, "fpc,fpid-gpio",
			&fpc1020->fpid_gpio);
	if (rc)
		goto exit;

	rc = gpio_direction_input(fpc1020->fpid_gpio);

	if (rc) {
		dev_err(fpc1020->dev,
			"gpio_direction_input (fpid) failed.\n");
		printk("BBox;gpio_direction_input (fpid) failed.\n");
		BBOX_FPC_PROB_FAIL;
		goto exit;
	}

	rc = fpc1020_pinctrl_init(fpc1020);
	if (rc)
		goto exit;

	rc = fpc1020_pinctrl_select(fpc1020, true);
	if (rc)
		goto exit;

	pr_info("F@FP fpc1020->rst_gpio =%d fpc1020->irq_gpio =%d fpc1020->fpid_gpio =%d\n",fpc1020->rst_gpio,fpc1020->irq_gpio, fpc1020->fpid_gpio);

	fpc1020->pwr_reg = regulator_get(fpc1020->dev,"vdd_io");
	if (IS_ERR(fpc1020->pwr_reg)) {
			dev_err(fpc1020->dev,
					"%s: Failed to get power regulator\n",
					__func__);
			printk("BBox;%s: Failed to get power regulator\n", __func__);
			BBOX_FPC_PROB_FAIL;
			rc = PTR_ERR(fpc1020->pwr_reg);
			goto exit;
	}

	if (fpc1020->pwr_reg) {
		rc = regulator_enable(fpc1020->pwr_reg);
		if (rc < 0) {
			dev_err(fpc1020->dev,
					"%s: Failed to enable power regulator\n",
					__func__);
			printk("BBox;%s: Failed to enable power regulator\n", __func__);
			BBOX_FPC_PROB_FAIL;
			goto regulator_put;
		}
	}
	pr_info("%s regulator_enable.\n",__func__);
	fpc1020->wakeup_enabled = false;

	irqf = IRQF_TRIGGER_RISING | IRQF_ONESHOT;
	mutex_init(&fpc1020->lock);
	rc = devm_request_threaded_irq(dev, gpio_to_irq(fpc1020->irq_gpio),
			NULL, fpc1020_irq_handler, irqf,
			dev_name(dev), fpc1020);
	if (rc) {
		dev_err(dev, "could not request irq %d\n",
				gpio_to_irq(fpc1020->irq_gpio));
		printk("BBox;could not request irq %d\n", gpio_to_irq(fpc1020->irq_gpio));
		BBOX_FPC_PROB_FAIL;
		goto exit;
	}
	dev_dbg(dev, "requested irq %d\n", gpio_to_irq(fpc1020->irq_gpio));
	pr_info("F@FP requested irq %d\n", gpio_to_irq(fpc1020->irq_gpio));
	/* Request that the interrupt should be wakeable */
	enable_irq_wake(gpio_to_irq(fpc1020->irq_gpio));

	wake_lock_init(&fpc1020->ttw_wl, WAKE_LOCK_SUSPEND, "fpc_ttw_wl");

	rc = sysfs_create_group(&dev->kobj, &attribute_group);
	if (rc) {
		dev_err(dev, "could not create sysfs\n");
		printk("BBox;could not create sysfs\n");
		BBOX_FPC_PROB_FAIL;
		goto exit;
	}
	rc = sysfs_create_link(dev->kobj.parent,
							&dev->kobj,
							"fpc1020");
	if (rc < 0) {
		pr_info("F@FP %s Can't create soft link in hall driver\n", __func__);
	}
	fp_class = class_create(THIS_MODULE, "fp");
	fp_dev = device_create(fp_class, NULL, atomic_inc_return(&fp_atomic),
						fpc1020, "fpc1020");
	rc = sysfs_create_group(&fp_dev->kobj, &attribute_group);
	if (rc < 0) {
		pr_info("F@FP %s Can't create soft link in hall driver\n", __func__);
	}
	rc = gpio_direction_output(fpc1020->rst_gpio, 1);

	if (rc) {
		dev_err(fpc1020->dev,
			"gpio_direction_output (reset) failed.\n");
		printk("BBox;gpio_direction_output (reset) failed.\n");
		BBOX_FPC_PROB_FAIL;
		goto exit;
	}
	gpio_set_value(fpc1020->rst_gpio, 1);
	udelay(FPC1020_RESET_HIGH1_US);
	pr_info("F@FP RESET GPIO %d \n",gpio_get_value(fpc1020->rst_gpio));
	gpio_set_value(fpc1020->rst_gpio, 0);
	udelay(FPC1020_RESET_LOW_US);
	pr_info("F@FP RESET GPIO %d \n",gpio_get_value(fpc1020->rst_gpio));
	gpio_set_value(fpc1020->rst_gpio, 1);
	udelay(FPC1020_RESET_HIGH2_US);
	pr_info("F@FP RESET GPIO %d \n",gpio_get_value(fpc1020->rst_gpio));
	dev_info(dev, "%s: ok\n", __func__);

exit:
	return rc;

regulator_put:
	if (fpc1020->pwr_reg) {
		regulator_put(fpc1020->pwr_reg);
		fpc1020->pwr_reg = NULL;
	}
	return rc;
}

static struct of_device_id fpc1020_of_match[] = {
	{ .compatible = "fpc,fpc1020", },
	{}
};
MODULE_DEVICE_TABLE(of, fpc1020_of_match);

static struct platform_driver fpc1020_driver = {
	.driver = {
		.name		= "fpc1020",
		.owner		= THIS_MODULE,
		.of_match_table = fpc1020_of_match,
	},
	.probe = fpc1020_probe,
};
module_platform_driver(fpc1020_driver);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Aleksej Makarov");
MODULE_AUTHOR("Henrik Tillman <henrik.tillman@fingerprints.com>");
MODULE_AUTHOR("Martin Trulsson <martin.trulsson@fingerprints.com>");
MODULE_DESCRIPTION("FPC1020 Fingerprint sensor device driver.");
