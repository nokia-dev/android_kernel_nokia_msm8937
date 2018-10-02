#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/io.h>
#include <linux/file.h>
#include <linux/uaccess.h>
/*#include <mach/msm_iomap.h>*/
#include "fih_pon.h"
#include "fih_rere.h"
#include "fih_ramtable.h"

#define FIH_APR_ST_HEAD  0x5F415052  /* _APR */
#define FIH_APR_ST_TAIL  0x5250415F  /* RPA_ */

#define FIH_APR_MEM_ADDR  FIH_APR_BASE
#define FIH_APR_MEM_SIZE  FIH_APR_SIZE /* 32 Byte */

struct st_fih_apr {
	unsigned int head;
	unsigned int pon;
	unsigned int rere;
	unsigned int poff;
	unsigned int tail;
};

static struct st_fih_apr apr;

#define FIH_APR_PROC_PON_NAME   "poweroncause"
#define FIH_APR_PROC_RERE_NAME  "rebootreason"
#define FIH_APR_PROC_POFF_NAME  "poweroffcause"

static struct proc_dir_entry *entry_apr_pon;
static struct proc_dir_entry *entry_apr_rere;
static struct proc_dir_entry *entry_apr_poff;

//VNA-3504, add modem failure reason++
#define FIH_APR_PROC_MFR_NAME  "modemfailurereason"
#define MAX_SSR_REASON_LEN 81U
extern char fih_failure_reason[MAX_SSR_REASON_LEN];
static struct proc_dir_entry *entry_apr_mfr;
//VNA-3504, add modem failure reason--

#ifdef CONFIG_FIH_IPO
void fih_ipo_pon_to_pwrkey(void)
{
	pr_info("[%s] pon from 0x%08X to power_key reason 0x00000080\n", __func__, apr.pon);
	apr.pon = 0x00000080;
}
EXPORT_SYMBOL(fih_ipo_pon_to_pwrkey);

void fih_ipo_pon_to_rtc(void)
{
	pr_info("[%s] pon from 0x%08X to rtc_alrm reason 0x00000004\n", __func__, apr.pon);
	apr.pon = 0x00000004;
}
EXPORT_SYMBOL(fih_ipo_pon_to_rtc);

void fih_ipo_pon_to_usb(void)
{
	pr_info("[%s] pon from 0x%08X to usb_chg reason 0x00000010\n", __func__, apr.pon);
	apr.pon = 0x00000010;
}
EXPORT_SYMBOL(fih_ipo_pon_to_usb);
#endif

int fih_apr_get_rere (void)
{
	return apr.rere;
}
EXPORT_SYMBOL(fih_apr_get_rere);

static int fih_apr_proc_rd_pon(struct seq_file *m, void *v)
{
	seq_printf(m, "0x%08x\n", apr.pon);
	return 0;
}

/*int fih_apr_proc_wt_pon(struct file *file, const char __user *buffer,
	size_t count, loff_t *ppos)*/
ssize_t fih_apr_proc_wt_pon(struct file *file, const char __user *buffer,
	size_t count, loff_t *ppos)
{
	unsigned char tmp[16];
	unsigned int size;

	size = (count > sizeof(tmp))? sizeof(tmp):count;

	if ( copy_from_user(tmp, buffer, size) ) {
		pr_err("%s: copy_from_user fail\n", __func__);
		return -EFAULT;
	}

	apr.pon = simple_strtoull(tmp, NULL, size);

	return size;
}

static int poweroncause_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, fih_apr_proc_rd_pon, NULL);
};

static struct file_operations poweroncause_file_ops = {
	.owner   = THIS_MODULE,
	.open    = poweroncause_proc_open,
	.read    = seq_read,
	.write   = fih_apr_proc_wt_pon,
	.llseek  = seq_lseek,
	.release = single_release
};

static int fih_apr_proc_rd_rere(struct seq_file *m, void *v)
{
	seq_printf(m, "0x%08x\n", apr.rere);
	return 0;
}

static int rebootreason_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, fih_apr_proc_rd_rere, NULL);
};

static struct file_operations rebootreason_file_ops = {
	.owner   = THIS_MODULE,
	.open    = rebootreason_proc_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release
};

static int fih_apr_proc_rd_poff(struct seq_file *m, void *v)
{
	seq_printf(m, "0x%08x\n", apr.poff);
	return 0;
}

static int poweroffcause_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, fih_apr_proc_rd_poff, NULL);
};

static struct file_operations poweroffcause_file_ops = {
	.owner   = THIS_MODULE,
	.open    = poweroffcause_proc_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release
};

//VNA-3504, add modem failure reason++
/******************************************************************************
*     Modem_Failure_Reason
*****************************************************************************/
static int fih_apr_proc_rd_mfr(struct seq_file *m, void *v)
{
	seq_printf(m, "%s\n", fih_failure_reason);
	//RFY-2133, format the failure reasen buffer after reading it every time
	fih_failure_reason[0]='\0';

	return 0;
}
//VNA-3504, add modem failure reason--

static int modemfailurereason_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, fih_apr_proc_rd_mfr, NULL);
};

static struct file_operations modemfailurereason_file_ops = {
	.owner   = THIS_MODULE,
	.open    = modemfailurereason_proc_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release
};

static int __init fih_apr_init(void)
{
	struct st_fih_apr *p;

	/* get apr in mem which lk write */
	p = (struct st_fih_apr *)ioremap(FIH_APR_MEM_ADDR, sizeof(struct st_fih_apr));
	if (p == NULL) {
		pr_err("%s: ioremap fail\n", __func__);
		return (1);
	}
	memcpy(&apr, p, sizeof(struct st_fih_apr));
	iounmap(p);

	pr_info("%s: apr.head = 0x%08x\n", __func__, apr.head);
	pr_info("%s: apr.pon  = 0x%08x\n", __func__, apr.pon);
	pr_info("%s: apr.rere = 0x%08x\n", __func__, apr.rere);
	pr_info("%s: apr.poff = 0x%08x\n", __func__, apr.poff);
	pr_info("%s: apr.tail = 0x%08x\n", __func__, apr.tail);

	/* check magic of apr */
	if ((FIH_APR_ST_HEAD != apr.head)||(FIH_APR_ST_TAIL != apr.tail)) {
		pr_err("%s: bad magic\n", __func__);
		apr.head = FIH_APR_ST_HEAD;
		apr.pon  = 0;
		apr.rere = 0;
		apr.poff = 0;
		apr.tail = FIH_APR_ST_TAIL;
	}

	// 20150326,  modify pon for MSM8909 +++
	/* notice bbs log */
	if (apr.pon & FIH_PON_PMIC_W_SIMULT_N) {
		if (apr.poff & 0x00000020)
			printk("BBox::UEC;40::21\n"); // HW RESET
	}
	if (apr.pon  & FIH_PON_APR_KERNEL_PANIC) printk("BBox::UEC;40::25\n"); // KP
	if (apr.pon  & FIH_PON_APR_UNKNOWN_RESET) printk("BBox::UEC;40::26\n"); // UN
	if (apr.pon  & FIH_PON_PMIC_C_SMPL) printk("BBox::UEC;40::22\n"); // SMPL
	if (apr.poff & 0x00002000) printk("BBox::UEC;40::23\n"); // UVLO
	if (apr.poff & 0x00000010) printk("BBox::UEC;40::24\n"); // GP2
	if (apr.poff & 0x00002000) printk("BBox::UEC;49::0\n");
	// 20150326,  modify pon for MSM8909 ---

	/* create proc virtual for pon */
	entry_apr_pon = proc_create(FIH_APR_PROC_PON_NAME, 0644, NULL, &poweroncause_file_ops);
	if (entry_apr_pon == NULL) {
		pr_err("%s: Fail to create %s\n", __func__, FIH_APR_PROC_PON_NAME);
		goto exit_pon;
	}

	/* create proc virtual for rere */
	entry_apr_rere = proc_create(FIH_APR_PROC_RERE_NAME, 0444, NULL, &rebootreason_file_ops);
	if (entry_apr_rere == NULL) {
		pr_err("%s: Fail to create %s\n", __func__, FIH_APR_PROC_RERE_NAME);
		goto exit_rere;
	}

	/* create proc virtual for poff */
	entry_apr_poff = proc_create(FIH_APR_PROC_POFF_NAME, 0444, NULL, &poweroffcause_file_ops);
	if (entry_apr_poff == NULL) {
		pr_err("%s: Fail to create %s\n", __func__, FIH_APR_PROC_POFF_NAME);
		goto exit_poff;
	}

	//VNA-3504, add modem failure reason++
	entry_apr_mfr = proc_create(FIH_APR_PROC_MFR_NAME, 0444, NULL, &modemfailurereason_file_ops);
	if (entry_apr_mfr == NULL) {
		pr_err("%s: Fail to create %s\n", __func__, FIH_APR_PROC_MFR_NAME);
		goto exit_mfr;
	}
	//VNA-3504, add modem failure reason--

	return (0);

//VNA-3504, add modem failure reason++
	remove_proc_entry(FIH_APR_PROC_MFR_NAME, NULL);
exit_mfr:
//VNA-3504, add modem failure reason--
	remove_proc_entry(FIH_APR_PROC_POFF_NAME, NULL);
exit_poff:
	remove_proc_entry(FIH_APR_PROC_RERE_NAME, NULL);
exit_rere:
	remove_proc_entry(FIH_APR_PROC_PON_NAME, NULL);
exit_pon:

	return (1);
}

static void __exit fih_apr_exit(void)
{
	struct st_fih_apr *p = (struct st_fih_apr *)ioremap(FIH_APR_MEM_ADDR, sizeof(struct st_fih_apr));
	if (p != NULL) {
		memset(p, 0x00, sizeof(struct st_fih_apr));
		iounmap(p);
	}

	//VNA-3504, add modem failure reason++
	remove_proc_entry(FIH_APR_PROC_MFR_NAME, NULL);
	//VNA-3504, add modem failure reason--
	remove_proc_entry(FIH_APR_PROC_POFF_NAME, NULL);
	remove_proc_entry(FIH_APR_PROC_RERE_NAME, NULL);
	remove_proc_entry(FIH_APR_PROC_PON_NAME, NULL);
}

static int __init fih_rere_wt_init(void)
{
	/* for detect unknown reset */
	fih_rere_wt_imem(FIH_RERE_UNKNOWN_RESET);
	return (0);
}

//late_initcall(fih_apr_init);
late_initcall(fih_rere_wt_init);
module_init(fih_apr_init);
module_exit(fih_apr_exit);
