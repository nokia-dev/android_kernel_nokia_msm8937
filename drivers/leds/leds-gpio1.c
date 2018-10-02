
/* Copyright (c) 2012-2014, The Linux Foundation. All rights reserved.
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
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/leds.h>
#include <linux/err.h>
#include <linux/spinlock.h>
#include <linux/of_platform.h>
#include <linux/of_device.h>
#include <linux/spmi.h>
#include <linux/qpnp/pwm.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/regulator/consumer.h>
#include <linux/delay.h>
#include <linux/gpio1.h>

#define LED_GPIO_MODE_MASK		0x3F
#define LED_GPIO_VIN_MASK		0x0F
#define LED_GPIO_PULL_MASK		0x0F
#define LED_GPIO_SRC_SEL_MASK		0xFF
#define LED_GPIO_DRV_OUT_MASK		0xFF
#define LED_GPIO_EANBLE_MASK		0xFF

#define LED_GPIO_MODE_CTRL(base)	(base + 0x40)
#define LED_GPIO_VIN_CTRL(base)		(base + 0x41)
#define LED_GPIO_PULL_CTRL(base)		(base + 0x42)
#define LED_GPIO_SRC_SEL_CTRL(base)		(base + 0x44)
#define LED_GPIO_DRV_OUT_CTRL(base)		(base + 0x45)
#define LED_GPIO_ENABLE_CTRL(base)		(base + 0x46)

#define CHANGE_MS_TO_US			1000

#define BBOX_GPIO1_PROBE_NODE_FAIL do {printk("BBox::UEC;23::0\n");} while (0);
#define BBOX_GPIO1_SET_BRIGHTNESS_FAIL do {printk("BBox::UEC;23::1\n");} while (0);

struct qpnp_led_data *led_g = NULL;

struct gpio_config_data {
	u8	source_sel;
	u8	mode_ctrl;
	u8	vin_ctrl;
	bool	enable;
};

struct qpnp_led_data {
	struct led_classdev	cdev;
	struct spmi_device	*spmi_dev;
	struct gpio_config_data	*gpio_cfg;
	struct pwm_device *pwm_bl;
	struct mutex		lock;
	u16			base;
	u8			reg;
};

static int
qpnp_led_masked_write(struct qpnp_led_data *led, u16 addr, u8 mask, u8 val)
{
	int rc;
	u8 reg;

	rc = spmi_ext_register_readl(led->spmi_dev->ctrl, led->spmi_dev->sid,
		addr, &reg, 1);
	if (rc) {
		BBOX_GPIO1_SET_BRIGHTNESS_FAIL
		dev_err(&led->spmi_dev->dev,
			"Unable to read from addr=%x, rc(%d)\n", addr, rc);
	}

	reg &= ~mask;
	reg |= val;

	rc = spmi_ext_register_writel(led->spmi_dev->ctrl, led->spmi_dev->sid,
		addr, &reg, 1);
	if (rc)
	{
		BBOX_GPIO1_SET_BRIGHTNESS_FAIL
		dev_err(&led->spmi_dev->dev,
			"Unable to write to addr=%x, rc(%d)\n", addr, rc);
	}
	return rc;
}

void init_GPIO1_behavior(struct qpnp_led_data *led)
{
	int rc=0;
	int val=0;
	printk(KERN_DEBUG "init_GPIO1_behavior base address => 0x%x\n",led->base);
	/////////////////set mode 0x40
	val=0x1;
	rc = qpnp_led_masked_write(led,LED_GPIO_MODE_CTRL(led->base),LED_GPIO_MODE_MASK,val);
	/////////////////set vin ctrl 0x41
	val=0x0;
	rc = qpnp_led_masked_write(led,LED_GPIO_VIN_CTRL(led->base),LED_GPIO_VIN_MASK,val);
	/////////////////set vin ctrl 0x42
	val=0x5;
	rc = qpnp_led_masked_write(led,LED_GPIO_PULL_CTRL(led->base),LED_GPIO_PULL_MASK,val);
	/////////////////set src ctrl 0x44
	val=0x0;
	rc = qpnp_led_masked_write(led,LED_GPIO_SRC_SEL_CTRL(led->base),LED_GPIO_SRC_SEL_MASK,val);
	/////////////////set drv out 0x45
	val=0x2;
	rc = qpnp_led_masked_write(led,LED_GPIO_DRV_OUT_CTRL(led->base),LED_GPIO_DRV_OUT_MASK,val);
	/////////////////set src ctrl 0x46
	val=0x80;
	rc = qpnp_led_masked_write(led,LED_GPIO_ENABLE_CTRL(led->base),LED_GPIO_EANBLE_MASK,val);
	///////////////////////////////////////////
	///////////////////////////////////////////
	pwm_disable(led->pwm_bl);
}

void disable_gpio1_solid(struct qpnp_led_data *led)
{
	int rc=0;
	int val=0;

	/////////////////set mode 0x40
	val=0x1;
	rc = qpnp_led_masked_write(led,LED_GPIO_MODE_CTRL(led->base),LED_GPIO_MODE_MASK,val);
	/////////////////set vin ctrl 0x41
	val=0x0;
	rc = qpnp_led_masked_write(led,LED_GPIO_VIN_CTRL(led->base),LED_GPIO_VIN_MASK,val);
	/////////////////set vin ctrl 0x42
	val=0x5;
	rc = qpnp_led_masked_write(led,LED_GPIO_PULL_CTRL(led->base),LED_GPIO_PULL_MASK,val);
	/////////////////set src ctrl 0x44
	val=0x0;
	rc = qpnp_led_masked_write(led,LED_GPIO_SRC_SEL_CTRL(led->base),LED_GPIO_SRC_SEL_MASK,val);
	/////////////////set drv out 0x45
	val=0x2;
	rc = qpnp_led_masked_write(led,LED_GPIO_DRV_OUT_CTRL(led->base),LED_GPIO_DRV_OUT_MASK,val);
	/////////////////set src ctrl 0x46
	val=0x80;
	rc = qpnp_led_masked_write(led,LED_GPIO_ENABLE_CTRL(led->base),LED_GPIO_EANBLE_MASK,val);

}
void enable_gpio1_solid(struct qpnp_led_data *led)
{
	int rc=0;
	int val=0;

	/////////////////set mode 0x40
	val=0x1;
	rc = qpnp_led_masked_write(led,LED_GPIO_MODE_CTRL(led->base),LED_GPIO_MODE_MASK,val);
	/////////////////set vin ctrl 0x41
	val=0x0;
	rc = qpnp_led_masked_write(led,LED_GPIO_VIN_CTRL(led->base),LED_GPIO_VIN_MASK,val);
	/////////////////set vin ctrl 0x42
	val=0x5;
	rc = qpnp_led_masked_write(led,LED_GPIO_PULL_CTRL(led->base),LED_GPIO_PULL_MASK,val);
	/////////////////set src ctrl 0x44
	val=0x80;
	rc = qpnp_led_masked_write(led,LED_GPIO_SRC_SEL_CTRL(led->base),LED_GPIO_SRC_SEL_MASK,val);
	/////////////////set drv out 0x45
	val=0x2;
	rc = qpnp_led_masked_write(led,LED_GPIO_DRV_OUT_CTRL(led->base),LED_GPIO_DRV_OUT_MASK,val);
	/////////////////set src ctrl 0x46
	val=0x80;
	rc = qpnp_led_masked_write(led,LED_GPIO_ENABLE_CTRL(led->base),LED_GPIO_EANBLE_MASK,val);

}
void disable_pwm(struct qpnp_led_data *led)
{
	pwm_disable(led->pwm_bl);
}
void enable_pwm(struct qpnp_led_data *led)
{
	int duty_us, period_us;
	int rc=0;

	duty_us = 64000;
	period_us = 4032000;
	rc = pwm_config_us(led->pwm_bl,duty_us,period_us);
	if (rc < 0)
	{
		printk(KERN_ERR "Failed to configure pwm for new values\n");
	}
	pwm_enable(led->pwm_bl);
}
void enable_pwm_fast(struct qpnp_led_data *led)
{
	int duty_us, period_us;
	int rc=0;

	duty_us = 100000;
	period_us = 1000000;
	rc = pwm_config_us(led->pwm_bl,duty_us,period_us);
	if (rc < 0)
	{
		printk(KERN_ERR "Failed to configure pwm for new values\n");
	}
	pwm_enable(led->pwm_bl);
}
void set_pwm_duty(struct qpnp_led_data *led , int blinkOnUs , int blinkOffUs)
{
	int duty_us, period_us;
	int rc=0;

	duty_us = blinkOnUs;
	period_us = blinkOnUs+blinkOffUs;
	rc = pwm_config_us(led->pwm_bl,duty_us,period_us);
	if (rc < 0)
	{
		printk(KERN_ERR "Failed to configure pwm for new values\n");
	}
	else
		pwm_enable(led->pwm_bl);
}

void enable_gpio1_pwm(struct qpnp_led_data *led)
{
	int rc=0;
	int val=0;

	/////////////////set mode 0x40
	val=0x1;
	rc = qpnp_led_masked_write(led,LED_GPIO_MODE_CTRL(led->base),LED_GPIO_MODE_MASK,val);
	/////////////////set vin ctrl 0x41
	val=0x0;
	rc = qpnp_led_masked_write(led,LED_GPIO_VIN_CTRL(led->base),LED_GPIO_VIN_MASK,val);
	/////////////////set vin ctrl 0x42
	val=0x5;
	rc = qpnp_led_masked_write(led,LED_GPIO_PULL_CTRL(led->base),LED_GPIO_PULL_MASK,val);
	/////////////////set src ctrl 0x44
	val=0x2;
	rc = qpnp_led_masked_write(led,LED_GPIO_SRC_SEL_CTRL(led->base),LED_GPIO_SRC_SEL_MASK,val);
	/////////////////set drv out 0x45
	val=0x2;
	rc = qpnp_led_masked_write(led,LED_GPIO_DRV_OUT_CTRL(led->base),LED_GPIO_DRV_OUT_MASK,val);
	/////////////////set src ctrl 0x46
	val=0x80;
	rc = qpnp_led_masked_write(led,LED_GPIO_ENABLE_CTRL(led->base),LED_GPIO_EANBLE_MASK,val);

}

void solid_on_off(bool status)
{
	//printk(KERN_DEBUG "solid_on_off\n");
	if(led_g!=NULL)
	{
		if(status==true)
		{
			enable_gpio1_solid(led_g);
			disable_pwm(led_g);
		}
		else
		{
			disable_gpio1_solid(led_g);
			disable_pwm(led_g);
		}
	}
}
EXPORT_SYMBOL_GPL(solid_on_off);

static ssize_t blink_mode_store(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{
	unsigned long mode;
	int ret;
	struct qpnp_led_data *led = NULL;
	led = dev_get_drvdata(dev);

	if(led ==NULL)
	{
		printk(KERN_ERR "led is NULL\n");
		return count;
	}
	else
		printk(KERN_DEBUG "blink_store LED base address => 0x%x\n",led->base);

	ret = kstrtoul(buf, 0, &mode);
	if (ret)
	{
		BBOX_GPIO1_SET_BRIGHTNESS_FAIL
		printk(KERN_ERR "led get mode fail\n");
		return count;
	}
	mutex_lock(&led->lock);
	if(mode==0) //GPIO1 disable
	{
		disable_gpio1_solid(led);
		disable_pwm(led);
	}
	else if(mode==1) //GPIO1 always on
	{
		enable_gpio1_solid(led);
		disable_pwm(led);
	}
	else if(mode==2)//GPIO1 blink
	{
		enable_gpio1_pwm(led);
	}
	else
	{
		printk(KERN_DEBUG "gpio 1 invalid mode\n");
	}
	mutex_unlock(&led->lock);
	return count;
}

static ssize_t pwm_duty_store(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{
	int blink_on;
	int blink_off;
	struct qpnp_led_data *led = NULL;
	led = dev_get_drvdata(dev);

	if(led ==NULL)
	{
		BBOX_GPIO1_SET_BRIGHTNESS_FAIL
		printk(KERN_ERR "led is NULL\n");
		return count;
	}
	else
		printk(KERN_DEBUG "blink_store LED base address => 0x%x\n",led->base);

	sscanf(buf, "%d %d\n", &blink_on, &blink_off);

	blink_on = blink_on*CHANGE_MS_TO_US;
	blink_off = blink_off*CHANGE_MS_TO_US;
	printk(KERN_DEBUG "pwm_duty_store LED blink On/Off => %d/%d\n",blink_on,blink_off);
	//printk("int size is => %ld\n",sizeof(int));
	mutex_lock(&led->lock);
	set_pwm_duty(led , blink_on,blink_off);
	mutex_unlock(&led->lock);
	return count;
}

static ssize_t pwm_powerOffChar_duty_store(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{
	int blink_on;
	int blink_off;
	struct qpnp_led_data *led = NULL;
	led = dev_get_drvdata(dev);

	if(led ==NULL)
	{
		printk(KERN_ERR "led is NULL\n");
		return count;
	}
	else
		printk(KERN_DEBUG "pwm_powerOffChar_duty_store LED base address => 0x%x\n",led->base);

	sscanf(buf, "%d %d\n", &blink_on, &blink_off);

	if(blink_on==0 && blink_off==0)
	{
		disable_gpio1_solid(led);
		disable_pwm(led);
	}
	else
	{
		enable_gpio1_pwm(led);
		blink_on = blink_on*CHANGE_MS_TO_US;
		blink_off = blink_off*CHANGE_MS_TO_US;
		printk(KERN_DEBUG "pwm_powerOffChar_duty_store LED blink On/Off => %d/%d\n",blink_on,blink_off);
		mutex_lock(&led->lock);
		set_pwm_duty(led , blink_on,blink_off);
		mutex_unlock(&led->lock);
	}
	return count;
}

static DEVICE_ATTR(blink, 0664, NULL, blink_mode_store);
static DEVICE_ATTR(pwm_duty, 0664, NULL, pwm_duty_store);
static DEVICE_ATTR(pwm_powerOffChar_duty, 0664, NULL, pwm_powerOffChar_duty_store);

static struct attribute *led_gpio1_attrs[] = {
	&dev_attr_blink.attr,
	&dev_attr_pwm_duty.attr,
	&dev_attr_pwm_powerOffChar_duty.attr,
	NULL
};

static const struct attribute_group led_gpio1_attr_group = {
	.attrs = led_gpio1_attrs,
};

static int gpio1_leds_probe(struct spmi_device *spmi)
{
	struct device_node *node , *temp;
	struct resource *led_resource;
	struct qpnp_led_data *led;
	int rc=0;
	int num_leds = 0;

	printk(KERN_DEBUG "gpio1_leds_probe\n");
	node = spmi->dev.of_node;
	if (node == NULL)
	{
		printk(KERN_ERR "gpio1_leds_probe node is NULL\n");
		BBOX_GPIO1_PROBE_NODE_FAIL
		return -1;
	}
	temp = NULL;
	while ((temp = of_get_next_child(node, temp)))
		num_leds++;
	printk(KERN_DEBUG "led num =>%d\n",num_leds);
	led = devm_kzalloc(&spmi->dev, sizeof(struct qpnp_led_data),GFP_KERNEL);
	if (!led) {
		printk(KERN_ERR "Unable to allocate memory for flash LED\n");
		BBOX_GPIO1_PROBE_NODE_FAIL
		return -1;
	}

	led->spmi_dev = spmi;
	led_resource = spmi_get_resource(spmi, spmi->dev_node, IORESOURCE_MEM, 0);
	if(!led_resource)
	{
		printk(KERN_ERR "Unable to get LED base address\n");
		BBOX_GPIO1_PROBE_NODE_FAIL
		return -1;
	}

	led->base = led_resource->start;
	printk(KERN_DEBUG "LED base address => 0x%x\n",led->base);
/*
	rc = sysfs_create_group(&led->cdev.dev->kobj,&led_gpio1_attrs);
	if (rc)
	{
		printk("fail to create gpio1 virtual file\n");
		return -1;
	}
*/

	for_each_child_of_node(node, temp)
	{
		led->pwm_bl = of_pwm_get(temp, NULL);

		if (IS_ERR(led->pwm_bl))
		{
			printk(KERN_ERR "Error, pwm device\n");
			BBOX_GPIO1_PROBE_NODE_FAIL
			led->pwm_bl = NULL;
			return -1;
		}

		rc = of_property_read_string(temp, "qcom,led-name",
						&led->cdev.name);
		if (rc < 0) {
			printk(KERN_ERR "Unable to read led name\n");
			BBOX_GPIO1_PROBE_NODE_FAIL
			return -1;
		}

		rc = led_classdev_register(&spmi->dev, &led->cdev);
		if (rc) {
			printk(KERN_ERR "Error, led_classdev_register\n");
			BBOX_GPIO1_PROBE_NODE_FAIL
			goto fail_id_check;
		}
		rc = sysfs_create_group(&led->cdev.dev->kobj,&led_gpio1_attr_group);
		if (rc)
		{
			printk(KERN_ERR "fail to create gpio1 virtual file\n");
			BBOX_GPIO1_PROBE_NODE_FAIL
			return -1;
		}
		mutex_init(&led->lock);
	}
	dev_set_drvdata(&spmi->dev, led);
	init_GPIO1_behavior(led);
	led_g=led;
	return rc;

fail_id_check:
	led_classdev_unregister(&led->cdev);
	return rc;
}

static int gpio1_leds_remove(struct spmi_device *spmi)
{
	struct qpnp_led_data *led  = dev_get_drvdata(&spmi->dev);
	if(led ==NULL)
	{
		printk(KERN_ERR "gpio1_leds_remove led is NULL\n");
	}
	else
		printk(KERN_DEBUG "gpio1_leds_remove blink_store LED base address => 0x%x\n",led->base);

	led_classdev_unregister(&led->cdev);
	mutex_destroy(&led->lock);
	return 0;
}

#ifdef CONFIG_OF
static struct of_device_id spmi_match_table[] = {
	{ .compatible = "qcom,leds-gpio1",},
	{ },
};
#else
#define spmi_match_table NULL
#endif

static struct spmi_driver leds_driver = {
	.driver		= {
		.name	= "qcom,leds-gpio1",
		.of_match_table = spmi_match_table,
	},
	.probe		= gpio1_leds_probe,
	.remove		= gpio1_leds_remove,
};

static int __init led_init(void)
{
	return spmi_driver_register(&leds_driver);
}
late_initcall(led_init);

static void __exit led_exit(void)
{
	spmi_driver_unregister(&leds_driver);
}
module_exit(led_exit);

MODULE_DESCRIPTION("GPIO1 LEDs control driver");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("leds:leds-gpio1");


