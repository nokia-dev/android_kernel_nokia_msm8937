#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include "fih_emmc.h"

#define FIH_PROC_DIR   "AllHWList"
#define FIH_PROC_PATH  "emmcinfo"
#define FIH_PROC_SIZE  FIH_EMMC_SIZE

static char fih_proc_data[FIH_PROC_SIZE] = "unknown";

void fih_emmc_setup(char *info)
{
	snprintf(fih_proc_data, sizeof(fih_proc_data), "%s", info);
}

static int fih_proc_read_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%s\n", fih_proc_data);
	return 0;
}

static int emmcinfo_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, fih_proc_read_show, NULL);
};

static struct file_operations emmcinfo_file_ops = {
	.owner   = THIS_MODULE,
	.open    = emmcinfo_proc_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release
};

static int __init fih_proc_init(void)
{
  struct proc_dir_entry *proc_parent;
  proc_parent = proc_mkdir(FIH_PROC_DIR, NULL);
  if (NULL == proc_create(FIH_PROC_PATH, S_IRUGO, proc_parent, &emmcinfo_file_ops)) {
    pr_err("fail to create proc/%s/%s\n", FIH_PROC_DIR, FIH_PROC_PATH);
    return (1);
  }

	return (0);
}

static void __exit fih_proc_exit(void)
{
	remove_proc_entry(FIH_PROC_PATH, NULL);
}

module_init(fih_proc_init);
module_exit(fih_proc_exit);
