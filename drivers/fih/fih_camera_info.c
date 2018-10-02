/*
 * Copyright (c) 2011-2013, The Linux Foundation. All rights reserved.
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
#include <linux/debugfs.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#include <linux/of.h>

//,derek, add for debug
#undef pr_info
#define pr_info(fmt, args...)  pr_err("%s:%d" fmt"\n", __func__, __LINE__, ##args)

u32 fih_msm8976_camera_use_external_isp=0;

static char *my_proc_name = "camera_info";

static int fih_camera_info_show(struct seq_file *m, void *v)
{
	pr_info("fih_camera_info_show use ISP = %d",fih_msm8976_camera_use_external_isp);
	seq_printf(m, "%d\n", fih_msm8976_camera_use_external_isp);
	return 0;
}

static int fih_camera_info_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, fih_camera_info_show, NULL);
};

static struct file_operations my_file_ops = {
	.owner   = THIS_MODULE,
	.open    = fih_camera_info_proc_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release
};

static int __init fih_camera_info_init(void)
{
	struct proc_dir_entry *entry;
	struct device_node *fih_dt_node = NULL;

	pr_info("fih_camera_info_init in");

	entry = proc_create(my_proc_name, 0444, NULL, &my_file_ops);
	if (!entry) {
		pr_err("%s: fail create %s proc\n", my_proc_name, my_proc_name);
	}

	fih_dt_node = of_find_node_by_name(of_find_node_by_path("/"), "altek");
	if(fih_dt_node == NULL)
	{
		pr_info("fih_camera_info_init is NULL");
		return -ENOMEM;
	}
	if (of_device_is_compatible(fih_dt_node, "mini_isp")){
		pr_info("fih_camera_info_init sucess");
		fih_msm8976_camera_use_external_isp=1;
	}else{
		pr_info("fih_camera_info_init fail");
		fih_msm8976_camera_use_external_isp=0;
	}

	return 0;
}

static void __exit fih_camera_info_exit(void)
{
	pr_info("dt_info module is unloaded\n");
}

arch_initcall(fih_camera_info_init);
module_exit(fih_camera_info_exit);
MODULE_AUTHOR("IdaChiang");
MODULE_DESCRIPTION("FIH Device Tree Information");


