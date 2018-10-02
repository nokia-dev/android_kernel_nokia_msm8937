#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/io.h>
#include <fih/share/e2p.h>
#include "fih_ramtable.h"

#define FIH_E2P_DATA_ST  FIH_CSP_E2P_V02

#define FIH_E2P_ST_ADDR  FIH_E2P_BASE
#define FIH_E2P_ST_SIZE  FIH_E2P_SIZE

static int fih_e2p_proc_read_pid_show(struct seq_file *m, void *v)
{
	FIH_E2P_DATA_ST *e2p;

	e2p = (FIH_E2P_DATA_ST *)ioremap(FIH_E2P_ST_ADDR, sizeof(FIH_E2P_DATA_ST));
	if (e2p == NULL) {
		pr_err("%s: ioremap fail\n", __func__);
		return (0);
	}

	seq_printf(m, "%s\n", e2p->pid);

	iounmap(e2p);

	return 0;
}

static int productid_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, fih_e2p_proc_read_pid_show, NULL);
};

static struct file_operations productid_file_ops = {
	.owner   = THIS_MODULE,
	.open    = productid_proc_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release
};

static int fih_e2p_proc_read_bt_mac_show(struct seq_file *m, void *v)
{
	FIH_E2P_DATA_ST *e2p;

	e2p = (FIH_E2P_DATA_ST *)ioremap(FIH_E2P_ST_ADDR, sizeof(FIH_E2P_DATA_ST));
	if (e2p == NULL) {
		pr_err("%s: ioremap fail\n", __func__);
		return (0);
	}

	seq_printf(m, "%02X:%02X:%02X:%02X:%02X:%02X\n",
		e2p->bt_mac[0], e2p->bt_mac[1], e2p->bt_mac[2], e2p->bt_mac[3], e2p->bt_mac[4], e2p->bt_mac[5]);

	iounmap(e2p);

	return 0;
}

static int bt_mac_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, fih_e2p_proc_read_bt_mac_show, NULL);
};

static struct file_operations bt_mac_file_ops = {
	.owner   = THIS_MODULE,
	.open    = bt_mac_proc_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release
};

static int fih_e2p_proc_read_wifi_mac_show(struct seq_file *m, void *v)
{
	FIH_E2P_DATA_ST *e2p;

	e2p = (FIH_E2P_DATA_ST *)ioremap(FIH_E2P_ST_ADDR, sizeof(FIH_E2P_DATA_ST));
	if (e2p == NULL) {
		pr_err("%s: ioremap fail\n", __func__);
		return (0);
	}

	seq_printf(m, "%02X:%02X:%02X:%02X:%02X:%02X\n",
		e2p->wifi_mac[0], e2p->wifi_mac[1], e2p->wifi_mac[2], e2p->wifi_mac[3], e2p->wifi_mac[4], e2p->wifi_mac[5]);

	iounmap(e2p);

	return 0;
}

static int fih_e2p_proc_read_sim_mode_show(struct seq_file *m, void *v)
{
	FIH_E2P_DATA_ST *e2p;

	e2p = (FIH_E2P_DATA_ST *)ioremap(FIH_E2P_ST_ADDR, sizeof(FIH_E2P_DATA_ST));
	if (e2p == NULL) {
		pr_err("%s: ioremap fail\n", __func__);
		return (0);
	}

	seq_printf(m, "%c%c%c%c\n",
		e2p->simconfig[0], e2p->simconfig[1], e2p->simconfig[2], e2p->simconfig[3]);

	iounmap(e2p);

	return 0;
}

static int wifi_mac_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, fih_e2p_proc_read_wifi_mac_show, NULL);
};

static int sim_mode_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, fih_e2p_proc_read_sim_mode_show, NULL);
};

static struct file_operations wifi_mac_file_ops = {
	.owner   = THIS_MODULE,
	.open    = wifi_mac_proc_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release
};

static struct file_operations sim_mode_file_ops = {
	.owner   = THIS_MODULE,
	.open    = sim_mode_proc_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release
};

static struct {
		char *name;
		struct file_operations *ops;
} *p, fih_e2p[] = {
	{"productid", &productid_file_ops},
	{"bt_mac", &bt_mac_file_ops},
	{"wifi_mac", &wifi_mac_file_ops},
	{"simmode", &sim_mode_file_ops},
	{NULL}, };

static int __init fih_e2p_init(void)
{
	if (FIH_E2P_ST_SIZE < sizeof(FIH_E2P_DATA_ST)) {
		pr_err("%s: WARNNING!! FIH_E2P_ST_SIZE (%d) < sizeof(FIH_E2P_DATA_ST) (%lu)",
			__func__, FIH_E2P_ST_SIZE, sizeof(FIH_E2P_DATA_ST));
		return (1);
	}

	for (p = fih_e2p; p->name; p++) {
		if (proc_create(p->name, 0, NULL, p->ops) == NULL) {
			pr_err("fail to create proc/%s\n", p->name);
		}
	}

	return (0);
}

static void __exit fih_e2p_exit(void)
{
	for (p = fih_e2p; p->name; p++) {
		remove_proc_entry(p->name, NULL);
	}
}

module_init(fih_e2p_init);
module_exit(fih_e2p_exit);
