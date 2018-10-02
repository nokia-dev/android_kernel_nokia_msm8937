#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/io.h>
#include <linux/mm.h>
#include "fih_cpu.h"

#define FIH_PROC_DIR   "AllHWList"
#define FIH_PROC_PATH  "AllHWList/cpuinfo"

static unsigned int fih_msm_rd_jtag_id(void)
{
	unsigned int *temp;
	unsigned int value;

	temp = (unsigned int *)ioremap(FIH_MSM_HW_REV_NUM, sizeof(unsigned int));
	if (temp == NULL) {
		pr_err("%s: ioremap fail\n", __func__);
		return (0);
	}

	value = *temp;

	iounmap(temp);
	return value;
}

static unsigned int fih_msm_rd_feature_id(void)
{
	unsigned int *temp;
	unsigned int value;

	temp = (unsigned int *)ioremap(FIH_MSM_HW_FEATURE_ID, sizeof(unsigned int));
	if (temp == NULL) {
		pr_err("%s: ioremap fail\n", __func__);
		return (0);
	}

	value = (*temp&0xFF00000)>>20;

	iounmap(temp);
	return value;
}

static int fih_cpu_read_show(struct seq_file *m, void *v)
{
	char msg[32];
	char buf[4];
	unsigned int device = fih_msm_rd_jtag_id() & 0x00FFFFFF;
	unsigned int sample_type = (fih_msm_rd_jtag_id() & 0xFF000000) >> 24;
	unsigned int featureid = fih_msm_rd_feature_id();

	switch (device) {
		case HW_REV_NUM_MSM8937: strcpy(msg, "MSM8937, "); break;
		case HW_REV_NUM_MSM8940: strcpy(msg, "MSM8940, "); break;
		case HW_REV_NUM_MSM8917: strcpy(msg, "MSM8917, "); break;
		default: strcpy(msg, "Unknown"); break;
	}

	switch (sample_type) {
		case HW_REV_ES: strcat(msg, "ES/CS, "); break;
		default: strcpy(msg, "Unknown"); break;
	}
	sprintf(buf, "0x%d", featureid);
	strcat(msg, buf);

	seq_printf(m, "%s\n", msg);
	return 0;
}

static int cpuinfo_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, fih_cpu_read_show, NULL);
};

static struct file_operations cpu_info_file_ops = {
	.owner   = THIS_MODULE,
	.open    = cpuinfo_proc_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release
};

static int __init fih_cpu_init(void)
{

	if (proc_create(FIH_PROC_PATH, 0, NULL, &cpu_info_file_ops) == NULL) {
		pr_err("fail to create proc/%s\n", FIH_PROC_PATH);
		return (1);
	}
	return (0);
}

static void __exit fih_cpu_exit(void)
{
	remove_proc_entry(FIH_PROC_PATH, NULL);
}

module_init(fih_cpu_init);
module_exit(fih_cpu_exit);
