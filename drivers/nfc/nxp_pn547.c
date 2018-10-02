/*
 * Copyright (C) 2010 Trusted Logic S.A.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/jiffies.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/miscdevice.h>
#include <linux/spinlock.h>
#include <linux/of_gpio.h>
#include "nxp_pn547.h"
#include <linux/gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/wakelock.h>
#include <linux/pinctrl/consumer.h>

#define MAX_BUFFER_SIZE	512
#define PN547_WAKE_LOCK_TIMEOUT	(HZ)
#undef NFC_DBG

#define BBSLOG
#ifdef BBSLOG
#define NFC_READ_ERROR do {printk("BBox;%s: NCI cmd transfer failure\n", __func__); printk("BBox::UEC;16::0");} while(0)
#endif

unsigned char nci_reset_nci[] = {0x20, 0x00, 0x01, 0x01};//Send U-RSET frame
unsigned char nci_reset_nci_resp[] = {0x40, 0x00, 0x03, 0x00, 0x11, 0x01};//Send U-RSET frame use for PN548

struct pn547_dev	{
	wait_queue_head_t	read_wq;
	struct mutex		read_mutex;
	struct i2c_client	*client;
	struct miscdevice	pn547_device;
	unsigned int 		ven_gpio;
	unsigned int 		firm_gpio;
	unsigned int		irq_gpio;
	bool			irq_enabled;
	spinlock_t		irq_enabled_lock;
	struct pinctrl *pinctrl;
	struct pinctrl_state    *pins_active;
	struct pinctrl_state    *pins_sleep;
	struct wake_lock	pn547_wake_lock;
	bool			suspended;
};

static ssize_t pn547_ven_on_off(struct device *dev,
                          struct device_attribute *attr, const char *buf, size_t size)
{
        struct i2c_client *client = to_i2c_client(dev);
        struct pn547_dev *pn547_dev = i2c_get_clientdata(client);

	if (*buf == '0'){
		gpio_set_value(pn547_dev->firm_gpio, 0);
		gpio_set_value(pn547_dev->ven_gpio, 0);
	}
	if (*buf == '1'){
		gpio_set_value(pn547_dev->firm_gpio, 0);
		gpio_set_value(pn547_dev->ven_gpio, 0);
		msleep(10);
		gpio_set_value(pn547_dev->firm_gpio, 0);
		gpio_set_value(pn547_dev->ven_gpio, 1);
        }

	printk("%s : ven_gpio = %d\n", __func__, gpio_get_value(pn547_dev->ven_gpio));
	printk("%s : firm_gpio = %d\n", __func__, gpio_get_value(pn547_dev->firm_gpio));

	return size;
}

static ssize_t pn547_irq(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct pn547_dev *pn547_dev = i2c_get_clientdata(client);
	int irq_status = 0;

	irq_status = gpio_get_value(pn547_dev->irq_gpio);
	printk("%s : irq_gpio = %d\n", __func__, irq_status);
	sprintf(buf, "%d\n", irq_status);

	return strlen(buf);
}

static DEVICE_ATTR(nfc_ven, 0644, NULL, pn547_ven_on_off);
static DEVICE_ATTR(nfc_irq, 0644, pn547_irq, NULL);

static struct attribute *pn547_attributes[] = {
        &dev_attr_nfc_ven.attr,
        &dev_attr_nfc_irq.attr,
        NULL
};

static struct attribute_group pn547_attribute_group = {
	.attrs = pn547_attributes
};

static void pn547_disable_irq(struct pn547_dev *pn547_dev)
{
	unsigned long flags;

	spin_lock_irqsave(&pn547_dev->irq_enabled_lock, flags);
	if (pn547_dev->irq_enabled) {
		disable_irq_nosync(pn547_dev->client->irq);
		pn547_dev->irq_enabled = false;
	}
	spin_unlock_irqrestore(&pn547_dev->irq_enabled_lock, flags);
}

static irqreturn_t pn547_dev_irq_handler(int irq, void *dev_id)
{
	struct pn547_dev *pn547_dev = dev_id;

	pn547_disable_irq(pn547_dev);

	/* Wake up waiting readers */
	wake_up(&pn547_dev->read_wq);
	if (pn547_dev->suspended)
	{
#ifdef NFC_DBG
		pr_debug("%s, suspended, .\n", __func__);
#endif
		wake_lock_timeout(&pn547_dev->pn547_wake_lock, PN547_WAKE_LOCK_TIMEOUT);
	}
	return IRQ_HANDLED;
}

static int pn547_dev_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct pn547_dev *pn547_dev = i2c_get_clientdata(client);

	if (gpio_get_value(pn547_dev->irq_gpio)) {
		pr_info("%s : IRQ is high, abort suspend. \n", __func__);
		return -EBUSY;
	}

	mutex_lock(&pn547_dev->read_mutex);
	if (gpio_get_value(pn547_dev->ven_gpio))
	{
//#ifdef NFC_DBG
//		pr_debug("%s : VEN is on, enable wake up interrupt. \n", __func__);
//#endif
		irq_set_irq_wake(client->irq, 1);
		pn547_dev->suspended = true;
	}
	mutex_unlock(&pn547_dev->read_mutex);

	return 0;
}

static int pn547_dev_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct pn547_dev *pn547_dev = i2c_get_clientdata(client);

	mutex_lock(&pn547_dev->read_mutex);
        if (gpio_get_value(pn547_dev->ven_gpio))
        {
//#ifdef NFC_DBG
//                pr_debug("%s : VEN is on, disable wake up interrupt. \n", __func__);
//#endif
                irq_set_irq_wake(client->irq, 0);
		pn547_dev->suspended = false;
	}
	mutex_unlock(&pn547_dev->read_mutex);

	return 0;
}

static SIMPLE_DEV_PM_OPS(pn547_dev_pm_ops, pn547_dev_suspend, pn547_dev_resume);

static ssize_t pn547_dev_read(struct file *filp, char __user *buf,
		size_t count, loff_t *offset)
{
	struct pn547_dev *pn547_dev = filp->private_data;
	char tmp[MAX_BUFFER_SIZE];
	int ret;
#ifdef NFC_DBG
	int i;
#endif
	int retry;

	if (count > MAX_BUFFER_SIZE)
		count = MAX_BUFFER_SIZE;

	pr_debug("%s : reading %zu bytes.\n", __func__, count);

	mutex_lock(&pn547_dev->read_mutex);

	if (!gpio_get_value(pn547_dev->irq_gpio)) {
		if (filp->f_flags & O_NONBLOCK) {
			ret = -EAGAIN;
			goto fail;
		}

		while (1) {
            retry = 5;
irq_read_retry:
			pn547_dev->irq_enabled = true;
			enable_irq(pn547_dev->client->irq);
			ret = wait_event_interruptible(
					pn547_dev->read_wq,
					!pn547_dev->irq_enabled);

			pn547_disable_irq(pn547_dev);
			if (ret) {
				if(retry > 0) {
					retry--;
					printk("%s: irq wait_event_interruptible ret=%d retry.\n", __func__, ret);
					goto irq_read_retry;
				} else {
					printk("%s: irq wait_event_interruptible ret=%d fail!\n", __func__, ret);
					goto fail;
				}
			}
			if (gpio_get_value(pn547_dev->irq_gpio))
				break;

			pr_warning("%s: spurious interrupt detected\n", __func__);
		}
	}

	/* Read data */
	ret = i2c_master_recv(pn547_dev->client, tmp, count);

	mutex_unlock(&pn547_dev->read_mutex);

	/* pn547 seems to be slow in handling I2C read requests
	 * so add 1ms delay after recv operation */
	udelay(1000);

	if (ret < 0) {
		//pr_err("%s: i2c_master_recv returned %d\n", __func__, ret);  //Chuck mark for remove log.
#ifdef BBSLOG
		NFC_READ_ERROR;
#endif
		return ret;
	}
	if (ret > count) {
		pr_err("%s: received too many bytes from i2c (%d)\n",
			__func__, ret);
		return -EIO;
	}
	if (copy_to_user(buf, tmp, ret)) {
		pr_warning("%s : failed to copy to user space\n", __func__);
		return -EFAULT;
	}

#ifdef NFC_DBG
	pr_debug("IFD->PC:");
	for(i = 0; i < ret; i++){
		pr_debug("%02X", tmp[i]);
	}
	pr_debug("\n");
#endif

	return ret;

fail:
	mutex_unlock(&pn547_dev->read_mutex);
	return ret;
}

static ssize_t pn547_dev_write(struct file *filp, const char __user *buf,
		size_t count, loff_t *offset)
{
	struct pn547_dev  *pn547_dev;
	char tmp[MAX_BUFFER_SIZE];
	int ret;
#ifdef NFC_DBG
	int i;
#endif

	pn547_dev = filp->private_data;

	if (count > MAX_BUFFER_SIZE)
		count = MAX_BUFFER_SIZE;

	if (copy_from_user(tmp, buf, count)) {
		pr_err("%s : failed to copy from user space\n", __func__);
		return -EFAULT;
	}

	pr_debug("%s : writing %zu bytes.\n", __func__, count);
	/* Write data */
	ret = i2c_master_send(pn547_dev->client, tmp, count);
	if (ret != count) {
		//pr_err("%s : i2c_master_send returned %d\n", __func__, ret); //Chuck mark for remove log.
		ret = -EIO;
	}

	/* pn547 seems to be slow in handling I2C write requests
	 * so add 1ms delay after I2C send oparation */
	udelay(1000);

#ifdef NFC_DBG
	pr_debug("PC->IFD:");
	for(i = 0; i < count; i++){
		pr_debug("%02X", tmp[i]);
	}
	pr_debug("\n");
#endif

	return ret;
}

static int pn547_dev_open(struct inode *inode, struct file *filp)
{
	struct pn547_dev *pn547_dev = container_of(filp->private_data,
						struct pn547_dev,
						pn547_device);

	filp->private_data = pn547_dev;

	pr_debug("%s : %d,%d\n", __func__, imajor(inode), iminor(inode));

	return 0;
}

static long  pn547_dev_ioctl(struct file *filp, unsigned int cmd,
				unsigned long arg)
{
	struct pn547_dev *pn547_dev = filp->private_data;

	switch (cmd) {
	case PN547_SET_PWR:
		if (arg == 2) {
			/* power on with firmware download (requires hw reset)
			 */
			printk("%s power on with firmware\n", __func__);
			gpio_set_value(pn547_dev->ven_gpio, 1);
			msleep(20);
			if (pn547_dev->firm_gpio)
				gpio_set_value(pn547_dev->firm_gpio, 1);
			msleep(20);
			gpio_set_value(pn547_dev->ven_gpio, 0);
			msleep(100);
			gpio_set_value(pn547_dev->ven_gpio, 1);
			msleep(20);
		} else if (arg == 1) {
			/* power on */
			//printk("%s power on\n", __func__);
			if (pn547_dev->firm_gpio)
				gpio_set_value(pn547_dev->firm_gpio, 0);
			gpio_set_value(pn547_dev->ven_gpio, 1);
			msleep(100);
		} else  if (arg == 0) {
			/* power off */
			//printk("%s power off\n", __func__);
			if (pn547_dev->firm_gpio)
				gpio_set_value(pn547_dev->firm_gpio, 0);
			gpio_set_value(pn547_dev->ven_gpio, 0);
			msleep(100);
		} else {
			pr_err("%s bad arg %lu\n", __func__, arg);
			return -EINVAL;
		}
		break;
	default:
		pr_err("%s bad ioctl %u\n", __func__, cmd);
		return -EINVAL;
	}

	return 0;
}
#ifdef CONFIG_COMPAT
static long  pn547_dev_compat_ioctl(struct file *filp, unsigned int cmd,
				unsigned long arg)
{
	struct pn547_dev *pn547_dev = filp->private_data;

	switch (cmd) {
	case PN547_SET_PWR:
		if (arg == 2) {
			/* power on with firmware download (requires hw reset)
			 */
			printk("%s power on with firmware\n", __func__);
			gpio_set_value(pn547_dev->ven_gpio, 1);
			msleep(20);
			if (pn547_dev->firm_gpio)
				gpio_set_value(pn547_dev->firm_gpio, 1);
			msleep(20);
			gpio_set_value(pn547_dev->ven_gpio, 0);
			msleep(100);
			gpio_set_value(pn547_dev->ven_gpio, 1);
			msleep(20);
		} else if (arg == 1) {
			/* power on */
			printk("%s power on\n", __func__);
			if (pn547_dev->firm_gpio)
				gpio_set_value(pn547_dev->firm_gpio, 0);
			gpio_set_value(pn547_dev->ven_gpio, 1);
			msleep(100);
		} else  if (arg == 0) {
			/* power off */
			printk("%s power off\n", __func__);
			if (pn547_dev->firm_gpio)
				gpio_set_value(pn547_dev->firm_gpio, 0);
			gpio_set_value(pn547_dev->ven_gpio, 0);
			msleep(100);
		} else {
			pr_err("%s bad arg %lu\n", __func__, arg);
			return -EINVAL;
		}
		break;
	default:
		pr_err("%s bad ioctl %u\n", __func__, cmd);
		return -EINVAL;
	}

	return 0;
}
#endif	
static const struct file_operations pn547_dev_fops = {
	.owner	= THIS_MODULE,
	.llseek	= no_llseek,
	.read	= pn547_dev_read,
	.write	= pn547_dev_write,
	.open	= pn547_dev_open,
	.unlocked_ioctl  = pn547_dev_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = pn547_dev_compat_ioctl
#endif	
};

static int pn547_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	int ret;
	struct pn547_i2c_platform_data *platform_data;
	struct pn547_dev *pn547_dev;

	printk("%s pn547 probe. \n", __func__);
	//platform_data = client->dev.platform_data;
	//platform_data = kzalloc(sizeof(*platform_data), GFP_KERNEL);
		if (client->dev.of_node) {
		platform_data = devm_kzalloc(&client->dev,
			sizeof(struct pn547_i2c_platform_data), GFP_KERNEL);
		if (!platform_data) {
			dev_err(&client->dev,
			"nfc-pn547 probe: Failed to allocate memory\n");
			return -ENOMEM;
		}
	} else {
		platform_data = client->dev.platform_data;
	}
	if (!platform_data)
		return -EINVAL;
	printk("nfc-pn547 probe: %s, inside nfc-pn547 flags = %x\n",__func__, client->flags);

	if (platform_data == NULL) {
		pr_err("%s : nfc probe fail\n", __func__);
		return  -ENODEV;
	}

	pn547_dev = kzalloc(sizeof(*pn547_dev), GFP_KERNEL);
	if (pn547_dev == NULL) {
		dev_err(&client->dev,
				"failed to allocate memory for module data\n");
		ret = -ENOMEM;
		goto err_exit;
	}
	
    pn547_dev->client = client;
  
	//pn547
	if( client->dev.of_node ){
		platform_data->irq_gpio = of_get_named_gpio_flags(client->dev.of_node, "nxp,irq-gpio", 0, NULL);
		platform_data->ven_gpio = of_get_named_gpio_flags(client->dev.of_node, "nxp,ven-gpio", 0, NULL);
		platform_data->firm_gpio = of_get_named_gpio_flags(client->dev.of_node, "nxp,firm-gpio", 0, NULL);
	}
	printk("%s irq_gpio=%d, ven_gpio=%d, firm_gpio=%d\n", __func__,
		platform_data->irq_gpio, platform_data->ven_gpio, platform_data->firm_gpio);
	//pn547

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("%s : need I2C_FUNC_I2C\n", __func__);
		return  -ENODEV;
	}

	ret = gpio_request(platform_data->irq_gpio, "nfc_int");
	if (ret){
		pr_err("%s: gpio_request irq_gpio failed %d\n", __func__, ret);
		return  -ENODEV;
	}
	ret = gpio_request(platform_data->ven_gpio, "nfc_ven");
	if (ret){
		pr_err("%s: gpio_request ven_gpio failed %d\n", __func__, ret);
		goto err_ven;
	}

	if (platform_data->firm_gpio) {
		ret = gpio_request(platform_data->firm_gpio, "nfc_firm");
		if (ret){
			pr_err("%s: gpio_request firm_gpio failed %d\n", __func__, ret);
			goto err_firm;
		}
	}

	pn547_dev->irq_gpio = platform_data->irq_gpio;
	pn547_dev->ven_gpio  = platform_data->ven_gpio;
	pn547_dev->firm_gpio  = platform_data->firm_gpio;

	ret = gpio_direction_input(pn547_dev->irq_gpio);
	if (ret < 0) {
		pr_err("%s :not able to set irq_gpio as input\n", __func__);
		goto err_ven;
	}
	ret = gpio_direction_output(pn547_dev->ven_gpio, 0);
	if (ret < 0) {
		pr_err("%s : not able to set ven_gpio as output\n", __func__);
		goto err_firm;
	}
	if (platform_data->firm_gpio) {
		ret = gpio_direction_output(pn547_dev->firm_gpio, 0);
		if (ret < 0) {
			pr_err("%s : not able to set firm_gpio as output\n",
				 __func__);
			goto err_exit;
		}
	}

	ret = sysfs_create_group(&client->dev.kobj, &pn547_attribute_group);
	if (ret) {
		pr_err("%s : sysfs registration failed, error %d\n", __func__, ret);
		goto err_create_file;
	}

	ret = sysfs_create_link(client->dev.kobj.parent->parent->parent->parent, &client->dev.kobj, "pn547_attr");
	if (ret) {
		pr_err("%s : sysfs create link failed, error %d\n", __func__, ret);
		goto err_create_link;
	}

	/* init mutex and queues */
	init_waitqueue_head(&pn547_dev->read_wq);
	mutex_init(&pn547_dev->read_mutex);
	spin_lock_init(&pn547_dev->irq_enabled_lock);

	pn547_dev->pn547_device.minor = MISC_DYNAMIC_MINOR;
	pn547_dev->pn547_device.name = "pn544";
	pn547_dev->pn547_device.fops = &pn547_dev_fops;

	ret = misc_register(&pn547_dev->pn547_device);
	if (ret) {
		pr_err("%s : misc_register failed\n", __FILE__);
		goto err_misc_register;
	}

	/* request irq.  the irq is set whenever the chip has data available
	 * for reading.  it is cleared when all data has been read.
	 */
	printk("%s : requesting IRQ %d\n", __func__, client->irq);
	pn547_dev->irq_enabled = true;
	ret = request_irq(client->irq, pn547_dev_irq_handler,
			  IRQF_TRIGGER_HIGH, client->name, pn547_dev);

	wake_lock_init(&pn547_dev->pn547_wake_lock, WAKE_LOCK_SUSPEND, "nxp_pn547");

	if (ret) {
		dev_err(&client->dev, "request_irq failed\n");
		goto err_request_irq_failed;
	}

	pr_err("F@NFC %s Success.",__func__);
	pn547_disable_irq(pn547_dev);
	i2c_set_clientdata(client, pn547_dev);

	return 0;
	
err_request_irq_failed:
	misc_deregister(&pn547_dev->pn547_device);
	wake_lock_destroy(&pn547_dev->pn547_wake_lock);
err_misc_register:
	mutex_destroy(&pn547_dev->read_mutex);
	sysfs_remove_link(client->dev.kobj.parent->parent->parent, "pn547_attr");
err_create_link:
	sysfs_remove_group(&client->dev.kobj, &pn547_attribute_group);
err_create_file:
	kfree(pn547_dev);
err_exit:
	if (pn547_dev->firm_gpio)
		gpio_free(platform_data->firm_gpio);
err_firm:
	gpio_free(platform_data->ven_gpio);
err_ven:
	gpio_free(platform_data->irq_gpio);
	return ret;
}

static int pn547_remove(struct i2c_client *client)
{
	struct pn547_dev *pn547_dev;

	pn547_dev = i2c_get_clientdata(client);
	free_irq(client->irq, pn547_dev);
	misc_deregister(&pn547_dev->pn547_device);
	mutex_destroy(&pn547_dev->read_mutex);
	gpio_free(pn547_dev->irq_gpio);
	gpio_free(pn547_dev->ven_gpio);
	if (pn547_dev->firm_gpio)
		gpio_free(pn547_dev->firm_gpio);
        wake_lock_destroy(&pn547_dev->pn547_wake_lock);
	kfree(pn547_dev);

	return 0;
}

static const struct i2c_device_id pn547_id[] = {
	{ "pn547", 0 },
	{ }
};

static const struct of_device_id msm_match_table[] = {
	{.compatible = "nxp-nfc"},
	{}
};

MODULE_DEVICE_TABLE(of, msm_match_table);

static struct i2c_driver pn547_driver = {
	.id_table	= pn547_id,
	.probe		= pn547_probe,
	.remove		= pn547_remove,
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= "NXP_PN547_NFC",
		.of_match_table = msm_match_table,
		.pm = &pn547_dev_pm_ops,
	},
};

/*
 * module load/unload record keeping
 */

static int __init pn547_dev_init(void)
{
	printk("Loading pn547 driver\n");
	return i2c_add_driver(&pn547_driver);
}
module_init(pn547_dev_init);

static void __exit pn547_dev_exit(void)
{
	printk("Unloading pn547 driver\n");
	i2c_del_driver(&pn547_driver);
}
module_exit(pn547_dev_exit);

MODULE_AUTHOR("Sylvain Fonteneau");
MODULE_DESCRIPTION("NFC PN547 driver");
MODULE_LICENSE("GPL");

