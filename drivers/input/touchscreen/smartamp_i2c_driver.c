/*
 * Smart AMP I2C virtual driver
 *
 * Copyright (C) 2012 Google, Inc.
 *
 * Author: BobIHLee <bobihlee@fih.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/completion.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/input/mt.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/slab.h>
#include <linux/gpio.h>

/* Each client has this additional data */
struct i2c_test_data {
	struct i2c_client *client;
	struct input_dev *input_dev;
};

int smartampi2cdatagpio;
int smartampi2cclkgpio;

static int i2c_test_test_transfer(struct i2c_client *client)
{
	struct i2c_msg xfer; //I2C transfer structure
	u8 *buf = kmalloc(1, GFP_ATOMIC); //allocate buffer from Heap since i2c_transfer() is non-blocking call
	buf[0] = 0x55; //data to transfer
	xfer.addr = client->addr;
	xfer.flags = 0;
	xfer.len = 1;
	xfer.buf = buf;
	return i2c_transfer(client->adapter, &xfer, 1);
}


static int i2c_test_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	//struct i2c_test_data *data = i2c_get_clientdata(client);
	#if 1
	if (gpio_request(smartampi2cdatagpio, "smartampi2cdata")) {
		pr_err("%s:request smartampi2cdata failed\n", __func__);
		//BBOX_LCM_GPIO_FAIL
		gpio_free(smartampi2cdatagpio);
		return -ENODEV;
	}
	gpio_set_value(smartampi2cdatagpio, 1);
	gpio_free(smartampi2cdatagpio);

	if (gpio_request(smartampi2cclkgpio, "smartampi2cclk")) {
		pr_err("%s:request smartampi2cclk failed\n", __func__);
		//BBOX_LCM_GPIO_FAIL
		gpio_free(smartampi2cclkgpio);
		return -ENODEV;
	}
	gpio_set_value(smartampi2cclkgpio, 1);
	gpio_free(smartampi2cclkgpio);
	#endif
	dev_err(&client->dev, "GPIO22 and GPIO23 set to high in suspend \n");

	return 0;
}

static int i2c_test_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	//struct i2c_test_data *data = i2c_get_clientdata(client);
	#if 1
	if (gpio_request(smartampi2cdatagpio, "smartampi2cdata")) {
		pr_err("%s:request smartampi2cdata failed\n", __func__);
		//BBOX_LCM_GPIO_FAIL
		gpio_free(smartampi2cdatagpio);
		return -ENODEV;
	}
	gpio_set_value(smartampi2cdatagpio, 1);
	gpio_free(smartampi2cdatagpio);

	if (gpio_request(smartampi2cclkgpio, "smartampi2cclk")) {
		pr_err("%s:request smartampi2cclk failed\n", __func__);
		//BBOX_LCM_GPIO_FAIL
		gpio_free(smartampi2cclkgpio);
		return -ENODEV;
	}
	gpio_set_value(smartampi2cclkgpio, 1);
	gpio_free(smartampi2cclkgpio);
	#endif
	dev_err(&client->dev, "GPIO22 and GPIO23 set to high in resume \n");
	
	return 0;
}


static SIMPLE_DEV_PM_OPS(i2c_test_pm_ops, i2c_test_suspend, i2c_test_resume);

static int i2c_test_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	//int irq_gpio = -1;
	//int irq;
	int addr;
	struct i2c_test_data *data;
	
	data = kzalloc(sizeof(struct i2c_test_data), GFP_KERNEL);
	if (!data) {
		dev_err(&client->dev, "Failed to allocate memory\n");
		return -ENOMEM;
	}
	
	data->client = client;
	i2c_set_clientdata(client, data);
	
	//Parse data using dt.
	if(client->dev.of_node){
		smartampi2cdatagpio = of_get_named_gpio(client->dev.of_node, "qcom_i2c_test,smartamp-i2c-data-gpio", 0);
		smartampi2cclkgpio = of_get_named_gpio(client->dev.of_node, "qcom_i2c_test,smartamp-i2c-clk-gpio", 0);
		//irq_gpio = of_get_named_gpio_flags(client->dev.of_node, "qcom_i2c_test,irq-gpio", 0, NULL);
	}
	//irq = client->irq; //GPIO irq #. already converted to gpio_to_irq
	addr = client->addr; //Slave Addr
	//dev_err(&client->dev, "gpio [%d] irq [%d] gpio_irq [%d] Slaveaddr [%x] \n", irq_gpio, irq, gpio_to_irq(irq_gpio), addr);
	dev_err(&client->dev, "gpio 22 an 23 Slaveaddr [%x] \n", addr);
	//You can initiate a I2C transfer anytime
	//using i2c_client *client structure
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("%s : need I2C_FUNC_I2C\n", __func__);
		return  -ENODEV;
	}
	#if 1
	if (!gpio_is_valid(smartampi2cdatagpio))
		pr_err("%s:%d, tp reset gpio not specified\n", __func__, __LINE__);
	
	gpio_set_value(smartampi2cdatagpio, 1);
	gpio_free(smartampi2cdatagpio);
	
	if (!gpio_is_valid(smartampi2cclkgpio))
		pr_err("%s:%d, tp reset gpio not specified\n", __func__, __LINE__);
	
	gpio_set_value(smartampi2cclkgpio, 1);
	gpio_free(smartampi2cclkgpio);
	#endif
	i2c_test_test_transfer(client);

	return 0;

}

static const struct of_device_id qcom_i2c_test_table[] = {
	{ .compatible = "qcom,i2c-test", },
	{},
};
MODULE_DEVICE_TABLE(of, qcom_i2c_test_table);

static const struct i2c_device_id qcom_id[] = {
	{ "qcom_i2c_test", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, qcom_id);

static struct i2c_driver i2c_test_driver = {
	.driver = {
		.name	= "qcom_i2c_test",
		.owner	= THIS_MODULE,
		.of_match_table = qcom_i2c_test_table,
		.pm	= &i2c_test_pm_ops,
	},
	.probe		= i2c_test_probe,
	.id_table	= qcom_id,
};

module_i2c_driver(i2c_test_driver);

/* Module information */
MODULE_AUTHOR("BobihLee <bobihlee@fih.com>");
MODULE_DESCRIPTION("Smart AMP I2C virtual driver");
MODULE_LICENSE("GPL");
