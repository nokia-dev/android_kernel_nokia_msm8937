#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <fih/hwid.h>
#include <linux/uaccess.h>


extern int CE_enable;
extern int CT_enable;
extern int CABC_enable;
#if 0
extern int vddio_enable;	//Display-PowerPinControlPinAndInitCodeAPI-00+_20150519
extern int avdd_enable;		//Display-PowerPinControlPinAndInitCodeAPI-00+_20150519
extern int avee_enable;		//Display-PowerPinControlPinAndInitCodeAPI-00+_20150519
#endif

extern int fih_get_ce (void);
extern int fih_set_ce (int ce);
extern int fih_get_ct (void);
extern int fih_set_ct (int ct);
extern int fih_get_cabc (void);
extern int fih_set_cabc (int cabc);
extern void fih_get_read_reg (char *reg_val);	//Display-DynamicReadWriteRegister-00+_20160729
extern void fih_set_read_reg (unsigned int reg, unsigned int reg_len);	//Display-DynamicReadWriteRegister-00+_20160729
extern void fih_set_write_reg (unsigned int len, char *data);	//Display-DynamicReadWriteRegister-00+_20160729

#if 0
extern int fih_get_vddio (void);		//Display-PowerPinControlPinAndInitCodeAPI-00+_20150519
extern int fih_set_vddio (int vddio);	//Display-PowerPinControlPinAndInitCodeAPI-00+_20150519
extern int fih_get_avdd (void);		//Display-PowerPinControlPinAndInitCodeAPI-00+_20150519
extern int fih_set_avdd (int avdd);	//Display-PowerPinControlPinAndInitCodeAPI-00+_20150519
extern int fih_get_avee (void);		//Display-PowerPinControlPinAndInitCodeAPI-00+_20150519
extern int fih_set_avee (int avee);	//Display-PowerPinControlPinAndInitCodeAPI-00+_20150519
extern int fih_get_reset (void);		//Display-PowerPinControlPinAndInitCodeAPI-00+_20150519
extern int fih_set_reset (int reset);	//Display-PowerPinControlPinAndInitCodeAPI-00+_20150519
extern int fih_get_init (void);			//Display-PowerPinControlPinAndInitCodeAPI-00+_20150519
extern int fih_set_init (int reset);		//Display-PowerPinControlPinAndInitCodeAPI-00+_20150519
extern int fih_get_ldos (void);		//Display-PowerPinControlPinAndInitCodeAPI-00+_20150519
extern int fih_set_ldos (int reset);	//Display-PowerPinControlPinAndInitCodeAPI-00+_20150519
#endif

static int fih_lcm_read_color_mode(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", fih_get_ce());

	return 0;
}

static ssize_t fih_lcm_write_color_mode(struct file *file, const char __user *buffer,
	size_t count, loff_t *ppos)
{
	unsigned char tmp[2];
	unsigned int size;
	unsigned int res;

	size = (count > sizeof(tmp))? sizeof(tmp):count;

	if (copy_from_user(tmp, buffer, size))
	{
		pr_err("%s: copy_from_user fail\n", __func__);
		return -EFAULT;
	}

	//Display-CE&CTWillNotBeExecutedUntilPanelInitIsDone-01*{_20150428
	if(strstr(saved_command_line, "androidboot.fihmode=0")!=NULL)  //Display-FixCABCCanNotWorkInAndroidO-00*_20171020
	{
		res = fih_set_ce(simple_strtoull(tmp, NULL, 0));
		if (res < 0)
		{
			return res;
		}
	}
	//Display-CE&CTWillNotBeExecutedUntilPanelInitIsDone-01*}_20150428

	return size;
}

static int fih_lcm_color_mode_open(struct inode *inode, struct file *file)
{
	return single_open(file, fih_lcm_read_color_mode, NULL);
};

static struct file_operations cm_file_ops = {
	.owner   = THIS_MODULE,
	.open    = fih_lcm_color_mode_open,
	.read    = seq_read,
	.write	 = fih_lcm_write_color_mode,
	.llseek  = seq_lseek,
	.release = single_release
};


static int fih_lcm_read_color_temperature(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", fih_get_ct());

	return 0;
}

static ssize_t fih_lcm_write_color_temperature(struct file *file, const char __user *buffer,
	size_t count, loff_t *ppos)
{
	unsigned char tmp[5];
	unsigned int size;
	unsigned int res;

	size = (count > sizeof(tmp))? sizeof(tmp):count;

	if (copy_from_user(tmp, buffer, size))
	{
		pr_err("%s: copy_from_user fail\n", __func__);
		return -EFAULT;
	}

	//CE&CTWillNotBeExecutedUntilPanelInitIsDone-01*{_20150428
	if(strstr(saved_command_line, "androidboot.fihmode=0")!=NULL)	//FixCABCCanNotWorkInAndroidO-00*_20171020
	{
		res = fih_set_ct(simple_strtoull(tmp, NULL, 0));
		if (res < 0)
		{
			return res;
		}
	}
	//Display-CE&CTWillNotBeExecutedUntilPanelInitIsDone-01*}_20150428

	return size;
}

static int fih_lcm_color_temperature_open(struct inode *inode, struct file *file)
{
	return single_open(file, fih_lcm_read_color_temperature, NULL);
};

static struct file_operations ct_file_ops = {
	.owner   = THIS_MODULE,
	.open    = fih_lcm_color_temperature_open,
	.read    = seq_read,
	.write	 = fih_lcm_write_color_temperature,
	.llseek  = seq_lseek,
	.release = single_release
};


static int fih_lcm_read_cabc_settings(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", fih_get_cabc());

	return 0;
}

static ssize_t fih_lcm_write_cabc_settings(struct file *file, const char __user *buffer,
	size_t count, loff_t *ppos)
{
	unsigned char tmp[2];
	unsigned int size;
	unsigned int res;

	size = (count > sizeof(tmp))? sizeof(tmp):count;

	if (copy_from_user(tmp, buffer, size))
	{
		pr_err("%s: copy_from_user fail\n", __func__);
		return -EFAULT;
	}

	//CE&CTWillNotBeExecutedUntilPanelInitIsDone-01*{_20150428
	if(strstr(saved_command_line, "androidboot.fihmode=0")!=NULL)	//FixCABCCanNotWorkInAndroidO-00*_20171020
	{
		res = fih_set_cabc(simple_strtoull(tmp, NULL, 0));
		if (res < 0)
		{
			return res;
		}
	}
	//Display-CE&CTWillNotBeExecutedUntilPanelInitIsDone-01*}_20150428

	return size;
}

static int fih_lcm_cabc_settings_open(struct inode *inode, struct file *file)
{
	return single_open(file, fih_lcm_read_cabc_settings, NULL);
};

static struct file_operations cabc_file_ops = {
	.owner   = THIS_MODULE,
	.open    = fih_lcm_cabc_settings_open,
	.read    = seq_read,
	.write	 = fih_lcm_write_cabc_settings,
	.llseek  = seq_lseek,
	.release = single_release
};

//Display-DynamicReadWriteRegister-00+{_20160729
static int fih_lcm_read_reg(struct seq_file *m, void *v)
{
	char reg_value[20]={0};

	fih_get_read_reg(reg_value);

	seq_printf(m, "%s", reg_value);

	return 0;
}

static ssize_t fih_lcm_write_reg(struct file *file, const char __user *buffer,
	size_t count, loff_t *ppos)
{
	unsigned char tmp[4];
	unsigned int size;
	unsigned int reg;
	unsigned int len;

	if (count < 4)
	{
		//pr_err("%s: Length of input parameter is less than 4\n", __func__);
		return -EFAULT;
	}

	size = (count > sizeof(tmp))? sizeof(tmp):count;

	if (copy_from_user(tmp, buffer, size))
	{
		pr_err("%s: copy_from_user fail\n", __func__);
		return -EFAULT;
	}
	pr_err("[HL]%s: tmp = (%s)\n", __func__, tmp);

	sscanf(tmp, "%x,%d", &reg, &len);

	pr_err("[HL]%s: reg = 0x%x, len = %d\n", __func__, reg, len);

	fih_set_read_reg(reg, len);

	return size;
}

static int fih_lcm_reg_open(struct inode *inode, struct file *file)
{
	return single_open(file, fih_lcm_read_reg, NULL);
};

static struct file_operations reg_read_file_ops = {
	.owner   = THIS_MODULE,
	.open    = fih_lcm_reg_open,
	.read    = seq_read,
	.write	 = fih_lcm_write_reg,
	.llseek  = seq_lseek,
	.release = single_release
};

static int fih_lcm_write_reg_read(struct seq_file *m, void *v)
{
	pr_err("[HL]%s <-- START\n", __func__);
	return 0;
}

static ssize_t fih_lcm_write_reg_write(struct file *file, const char __user *buffer,
	size_t count, loff_t *ppos)
{
	unsigned char tmp[512];
	unsigned int size;
	unsigned int len;
	unsigned char data[512];

	pr_err("\n\n*** [HL] %s <-- START ***\n\n", __func__);

	if (count < 8)
	{
		pr_err("%s: Length of input parameter is less than 8\n", __func__);
		return -EFAULT;
	}

	size = (count > sizeof(tmp))? sizeof(tmp):count;

	if (copy_from_user(tmp, buffer, size))
	{
		pr_err("%s: copy_from_user fail\n", __func__);
		return -EFAULT;
	}
	pr_err("[HL]%s: tmp = (%s)\n", __func__, tmp);

	sscanf(tmp, "%d,%s,", &len, data);

	pr_err("[HL]%s: len = %d, data = (%s)\n", __func__, len, data);

	fih_set_write_reg(len, data);

	pr_err("\n\n*** [HL] %s <-- END ***\n\n", __func__);

	return count;
}

static int fih_lcm_write_reg_open(struct inode *inode, struct file *file)
{
	return single_open(file, fih_lcm_write_reg_read, NULL);
};

static struct file_operations reg_write_file_ops = {
	.owner   = THIS_MODULE,
	.open    = fih_lcm_write_reg_open,
	.read    = seq_read,
	.write	 = fih_lcm_write_reg_write,
	.llseek  = seq_lseek,
	.release = single_release
};
//Display-DynamicReadWriteRegister-00+}_20160729

//Display-AckErrCountAndStatus-00+{_20161014
char fih_awer_cnt[32] = "unknown";
void fih_awer_cnt_set(char *info)
{
	strcpy(fih_awer_cnt, info);
}

static int fih_awer_cnt_read_proc(struct seq_file *m, void *v)
{
	seq_printf(m, "%s\n", fih_awer_cnt);
	return 0;
}

static int fih_awer_cnt_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, fih_awer_cnt_read_proc, NULL);
}

static struct file_operations awer_cnt_operations = {
	.open		= fih_awer_cnt_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= seq_release,
};

char fih_awer_status[32] = "unknown";
void fih_awer_status_set(char *info)
{
	strcpy(fih_awer_status, info);
}

static int fih_awer_status_read_proc(struct seq_file *m, void *v)
{
	seq_printf(m, "%s\n", fih_awer_status);
	return 0;
}

static int fih_awer_status_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, fih_awer_status_read_proc, NULL);
}

static struct file_operations awer_status_operations = {
	.open		= fih_awer_status_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= seq_release,
};
//Display-AckErrCountAndStatus-00+}_20161014

#if 0
//Display-PowerPinControlPinAndInitCodeAPI-00+{_20150519
static int fih_lcm_read_vddio(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", fih_get_vddio());

	return 0;
}

static ssize_t fih_lcm_write_vddio(struct file *file, const char __user *buffer,
	size_t count, loff_t *ppos)
{
	unsigned char tmp[2];
	unsigned int size;
	unsigned int res;

	size = (count > sizeof(tmp))? sizeof(tmp):count;

	if (copy_from_user(tmp, buffer, size))
	{
		pr_err("%s: copy_from_user fail\n", __func__);
		return -EFAULT;
	}

	res = fih_set_vddio(simple_strtoull(tmp, NULL, 0));
	if (res < 0)
	{
		return res;
	}

	return size;
}

static int fih_lcm_vddio_open(struct inode *inode, struct file *file)
{
	return single_open(file, fih_lcm_read_vddio, NULL);
};

static struct file_operations vddio_file_ops = {
	.owner   = THIS_MODULE,
	.open    = fih_lcm_vddio_open,
	.read    = seq_read,
	.write	 = fih_lcm_write_vddio,
	.llseek  = seq_lseek,
	.release = single_release
};

static int fih_lcm_read_avdd(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", fih_get_avdd());

	return 0;
}

static ssize_t fih_lcm_write_avdd(struct file *file, const char __user *buffer,
	size_t count, loff_t *ppos)
{
	unsigned char tmp[2];
	unsigned int size;
	unsigned int res;

	size = (count > sizeof(tmp))? sizeof(tmp):count;

	if (copy_from_user(tmp, buffer, size))
	{
		pr_err("%s: copy_from_user fail\n", __func__);
		return -EFAULT;
	}

	res = fih_set_avdd(simple_strtoull(tmp, NULL, 0));
	if (res < 0)
	{
		return res;
	}

	return size;
}

static int fih_lcm_avdd_open(struct inode *inode, struct file *file)
{
	return single_open(file, fih_lcm_read_avdd, NULL);
};

static struct file_operations avdd_file_ops = {
	.owner   = THIS_MODULE,
	.open    = fih_lcm_avdd_open,
	.read    = seq_read,
	.write	 = fih_lcm_write_avdd,
	.llseek  = seq_lseek,
	.release = single_release
};

static int fih_lcm_read_avee(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", fih_get_avee());

	return 0;
}

static ssize_t fih_lcm_write_avee(struct file *file, const char __user *buffer,
	size_t count, loff_t *ppos)
{
	unsigned char tmp[2];
	unsigned int size;
	unsigned int res;

	size = (count > sizeof(tmp))? sizeof(tmp):count;

	if (copy_from_user(tmp, buffer, size))
	{
		pr_err("%s: copy_from_user fail\n", __func__);
		return -EFAULT;
	}

	res = fih_set_avee(simple_strtoull(tmp, NULL, 0));
	if (res < 0)
	{
		return res;
	}

	return size;
}

static int fih_lcm_avee_open(struct inode *inode, struct file *file)
{
	return single_open(file, fih_lcm_read_avee, NULL);
};

static struct file_operations avee_file_ops = {
	.owner   = THIS_MODULE,
	.open    = fih_lcm_avee_open,
	.read    = seq_read,
	.write	 = fih_lcm_write_avee,
	.llseek  = seq_lseek,
	.release = single_release
};

static int fih_lcm_read_reset(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", fih_get_reset());

	return 0;
}

static ssize_t fih_lcm_write_reset(struct file *file, const char __user *buffer,
	size_t count, loff_t *ppos)
{
	unsigned char tmp[2];
	unsigned int size;
	unsigned int res;

	size = (count > sizeof(tmp))? sizeof(tmp):count;

	if (copy_from_user(tmp, buffer, size))
	{
		pr_err("%s: copy_from_user fail\n", __func__);
		return -EFAULT;
	}

	res = fih_set_reset(simple_strtoull(tmp, NULL, 0));
	if (res < 0)
	{
		return res;
	}

	return size;
}

static int fih_lcm_reset_open(struct inode *inode, struct file *file)
{
	return single_open(file, fih_lcm_read_reset, NULL);
};

static struct file_operations reset_file_ops = {
	.owner   = THIS_MODULE,
	.open    = fih_lcm_reset_open,
	.read    = seq_read,
	.write	 = fih_lcm_write_reset,
	.llseek  = seq_lseek,
	.release = single_release
};

static int fih_lcm_read_init(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", fih_get_init());

	return 0;
}

static ssize_t fih_lcm_write_init(struct file *file, const char __user *buffer,
	size_t count, loff_t *ppos)
{
	unsigned char tmp[2];
	unsigned int size;
	unsigned int res;

	size = (count > sizeof(tmp))? sizeof(tmp):count;

	if (copy_from_user(tmp, buffer, size))
	{
		pr_err("%s: copy_from_user fail\n", __func__);
		return -EFAULT;
	}

	res = fih_set_init(simple_strtoull(tmp, NULL, 0));
	if (res < 0)
	{
		return res;
	}

	return size;
}

static int fih_lcm_init_open(struct inode *inode, struct file *file)
{
	return single_open(file, fih_lcm_read_init, NULL);
};

static struct file_operations init_file_ops = {
	.owner   = THIS_MODULE,
	.open    = fih_lcm_init_open,
	.read    = seq_read,
	.write	 = fih_lcm_write_init,
	.llseek  = seq_lseek,
	.release = single_release
};

static int fih_lcm_read_ldos(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", fih_get_ldos());

	return 0;
}

static ssize_t fih_lcm_write_ldos(struct file *file, const char __user *buffer,
	size_t count, loff_t *ppos)
{
	unsigned char tmp[2];
	unsigned int size;
	unsigned int res;

	size = (count > sizeof(tmp))? sizeof(tmp):count;

	if (copy_from_user(tmp, buffer, size))
	{
		pr_err("%s: copy_from_user fail\n", __func__);
		return -EFAULT;
	}

	res = fih_set_ldos(simple_strtoull(tmp, NULL, 0));
	if (res < 0)
	{
		return res;
	}

	return size;
}

static int fih_lcm_ldos_open(struct inode *inode, struct file *file)
{
	return single_open(file, fih_lcm_read_ldos, NULL);
};

static struct file_operations ldos_file_ops = {
	.owner   = THIS_MODULE,
	.open    = fih_lcm_ldos_open,
	.read    = seq_read,
	.write	 = fih_lcm_write_ldos,
	.llseek  = seq_lseek,
	.release = single_release
};
//Display-PowerPinControlPinAndInitCodeAPI-00+}_20150519
#endif

struct {
	char *name;
	struct file_operations *ops;
} *p, LCM0_cm[] = {
	{"AllHWList/LCM0/color_mode", &cm_file_ops},
	{NULL}, },
	LCM0_ct[] = {
	{"AllHWList/LCM0/color_temperature", &ct_file_ops},
	{NULL}, },
	LCM0_cabc[] = {
	{"AllHWList/LCM0/CABC_settings", &cabc_file_ops},
	{NULL}, },
	LCM0_reg_read[] = {									//Display-DynamicReadWriteRegister-00+_20160729
	{"AllHWList/LCM0/reg_read", &reg_read_file_ops},
	{NULL}, },
	LCM0_reg_write[] = {								//Display-DynamicReadWriteRegister-00+_20160729
	{"AllHWList/LCM0/reg_write", &reg_write_file_ops},
	{NULL}, },
	LCM0_awer_cnt[] = {
	{"AllHWList/LCM0/awer_cnt", &awer_cnt_operations},
	{NULL}, },
	LCM0_awer_status[] = {
	{"AllHWList/LCM0/awer_status", &awer_status_operations},
	{NULL},
	#if 0
	{NULL}, },
	//Display-PowerPinControlPinAndInitCodeAPI-00+{_20150519
	LCM0_vddio[] = {
	{"AllHWList/LCM0/vddio", &vddio_file_ops},
	{NULL}, },
	LCM0_avdd[] = {
	{"AllHWList/LCM0/avdd", &avdd_file_ops},
	{NULL}, },
	LCM0_avee[] = {
	{"AllHWList/LCM0/avee", &avee_file_ops},
	{NULL}, },
	LCM0_reset[] = {
	{"AllHWList/LCM0/reset", &reset_file_ops},
	{NULL}, },
	LCM0_init[] = {
	{"AllHWList/LCM0/init", &init_file_ops},
	{NULL}, },
	LCM0_ldos[] = {
	{"AllHWList/LCM0/ldos", &ldos_file_ops},
	{NULL},
	};
	//Display-PowerPinControlPinAndInitCodeAPI-00+}_20150519
	#else
	};
	#endif

static int __init fih_lcm_init(void)
{
	struct proc_dir_entry *lcm0_dir;
	struct proc_dir_entry *ent;

	pr_debug("\n\n*** [HL] %s, AAA CE_enable = %d ***\n\n", __func__, CE_enable);
	pr_debug("\n\n*** [HL] %s, AAA CT_enable = %d ***\n\n", __func__, CT_enable);
	pr_debug("\n\n*** [HL] %s, AAA CABC_enable = %d ***\n\n", __func__, CABC_enable);

	//Display-AddCTCPanelHX8394FInsideSupport-01*_20150522
	lcm0_dir = proc_mkdir("AllHWList/LCM0", NULL);

	if ((CE_enable) || (CT_enable) || (CABC_enable))
	{
		if (CE_enable)
		{
			ent = proc_create((LCM0_cm->name) + 15, 0, lcm0_dir, LCM0_cm->ops);
			if (ent == NULL)
			{
				pr_err("\n\nUnable to create /proc/%s", LCM0_cm->name);
			}
			pr_debug("\n\n*** [HL] %s, succeed to create proc/%s ***\n\n", __func__, LCM0_cm->name);
		}

		if (CT_enable)
		{
			ent = proc_create((LCM0_ct->name) + 15, 0, lcm0_dir, LCM0_ct->ops);
			if (ent == NULL)
			{
				pr_err("\n\nUnable to create /proc/%s", LCM0_ct->name);
			}
			pr_debug("\n\n*** [HL] %s, succeed to create proc/%s ***\n\n", __func__, LCM0_ct->name);
		}

		if (CABC_enable)
		{
			ent = proc_create((LCM0_cabc->name) + 15, 0, lcm0_dir, LCM0_cabc->ops);
			if (ent == NULL)
			{
				pr_err("\n\nUnable to create /proc/%s", LCM0_cabc->name);
			}
			pr_debug("\n\n*** [HL] %s, succeed to create proc/%s ***\n\n", __func__, LCM0_cabc->name);
		}
	}

	//Display-DynamicReadWriteRegister-00+{_20160729
	ent = proc_create((LCM0_reg_read->name) + 15, 0, lcm0_dir, LCM0_reg_read->ops);
	if (ent == NULL)
	{
		pr_err("\n\nUnable to create /proc/%s", LCM0_reg_read->name);
	}
	pr_debug("\n\n*** [HL] %s, succeed to create proc/%s ***\n\n", __func__, LCM0_reg_read->name);

	ent = proc_create((LCM0_reg_write->name) + 15, 0, lcm0_dir, LCM0_reg_write->ops);
	if (ent == NULL)
	{
		pr_err("\n\nUnable to create /proc/%s", LCM0_reg_write->name);
	}
	pr_debug("\n\n*** [HL] %s, succeed to create proc/%s ***\n\n", __func__, LCM0_reg_write->name);
	//Display-DynamicReadWriteRegister-00+}_20160729

	//Display-AckErrCountAndStatus-00+{_20161014
	ent = proc_create((LCM0_awer_cnt->name) + 15, 0, lcm0_dir, LCM0_awer_cnt->ops);
	if (ent == NULL)
	{
		pr_err("\n\nUnable to create /proc/%s", LCM0_awer_cnt->name);
	}
	pr_debug("\n\n*** [HL] %s, succeed to create proc/%s ***\n\n", __func__, LCM0_awer_cnt->name);

	ent = proc_create((LCM0_awer_status->name) + 15, 0, lcm0_dir, LCM0_awer_status->ops);
	if (ent == NULL)
	{
		pr_err("\n\nUnable to create /proc/%s", LCM0_awer_status->name);
	}
	pr_debug("\n\n*** [HL] %s, succeed to create proc/%s ***\n\n", __func__, LCM0_awer_status->name);
	//Display-AckErrCountAndStatus-00+}_20161014

	#if 0
	//Display-PowerPinControlPinAndInitCodeAPI-00+{_20150519
	if (vddio_enable)
	{
		ent = proc_create((LCM0_vddio->name) + 15, 0, lcm0_dir, LCM0_vddio->ops);
		if (ent == NULL)
		{
			pr_err("\n\nUnable to create /proc/%s", LCM0_vddio->name);
		}
		pr_debug("\n\n*** [HL] %s, succeed to create proc/%s ***\n\n", __func__, LCM0_vddio->name);
	}

	if (avdd_enable)
	{
		ent = proc_create((LCM0_avdd->name) + 15, 0, lcm0_dir, LCM0_avdd->ops);
		if (ent == NULL)
		{
			pr_err("\n\nUnable to create /proc/%s", LCM0_avdd->name);
		}
		pr_debug("\n\n*** [HL] %s, succeed to create proc/%s ***\n\n", __func__, LCM0_avdd->name);
	}

	if (avee_enable)
	{
		ent = proc_create((LCM0_avee->name) + 15, 0, lcm0_dir, LCM0_avee->ops);
		if (ent == NULL)
		{
			pr_err("\n\nUnable to create /proc/%s", LCM0_avee->name);
		}
		pr_debug("\n\n*** [HL] %s, succeed to create proc/%s ***\n\n", __func__, LCM0_avee->name);
	}

	ent = proc_create((LCM0_reset->name) + 15, 0, lcm0_dir, LCM0_reset->ops);
	if (ent == NULL)
	{
		pr_err("\n\nUnable to create /proc/%s", LCM0_reset->name);
	}
	pr_debug("\n\n*** [HL] %s, succeed to create proc/%s ***\n\n", __func__, LCM0_reset->name);

	ent = proc_create((LCM0_init->name) + 15, 0, lcm0_dir, LCM0_init->ops);
	if (ent == NULL)
	{
		pr_err("\n\nUnable to create /proc/%s", LCM0_init->name);
	}
	pr_debug("\n\n*** [HL] %s, succeed to create proc/%s ***\n\n", __func__, LCM0_init->name);

	ent = proc_create((LCM0_ldos->name) + 15, 0, lcm0_dir, LCM0_ldos->ops);
	if (ent == NULL)
	{
		pr_err("\n\nUnable to create /proc/%s", LCM0_ldos->name);
	}
	pr_debug("\n\n*** [HL] %s, succeed to create proc/%s ***\n\n", __func__, LCM0_ldos->name);
	//Display-PowerPinControlPinAndInitCodeAPI-00+}_20150519
	#endif

	return (0);
}

static void __exit fih_lcm_exit(void)
{
	if (CE_enable)
	{
		remove_proc_entry(LCM0_cm->name, NULL);
	}

	if (CT_enable)
	{
		remove_proc_entry(LCM0_ct->name, NULL);
	}

	if (CABC_enable)
	{
		remove_proc_entry(LCM0_cabc->name, NULL);
	}

	//Display-DynamicReadWriteRegister-00+{_20160729
	remove_proc_entry(LCM0_reg_read->name, NULL);
	remove_proc_entry(LCM0_reg_write->name, NULL);
	//Display-DynamicReadWriteRegister-00+}_20160729

	//Display-AckErrCountAndStatus-00+{_20161014
	remove_proc_entry(LCM0_awer_cnt->name, NULL);
	remove_proc_entry(LCM0_awer_status->name, NULL);
	//Display-AckErrCountAndStatus-00+}_20161014

	#if 0
	//Display-AddCTCPanelHX8394FInsideSupport-01+{_20150522
	if (vddio_enable)
	{
		remove_proc_entry(LCM0_vddio->name, NULL);
	}

	if (avdd_enable)
	{
		remove_proc_entry(LCM0_avdd->name, NULL);
	}

	if (avee_enable)
	{
		remove_proc_entry(LCM0_avee->name, NULL);
	}

	remove_proc_entry(LCM0_reset->name, NULL);
	remove_proc_entry(LCM0_init->name, NULL);
	remove_proc_entry(LCM0_ldos->name, NULL);
	//Display-AddCTCPanelHX8394FInsideSupport-01+}_20150522
	#endif
}

late_initcall(fih_lcm_init);
module_exit(fih_lcm_exit);
