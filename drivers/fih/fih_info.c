#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <fih/hwid.h>
#include "fih_ramtable.h"
#include "fih_hwcfg.h"

static unsigned int my_proc_addr = FIH_HWCFG_MEM_ADDR;
static unsigned int my_proc_size = FIH_HWCFG_MEM_SIZE;


static int fih_info_proc_read_project_show(struct seq_file *m, void *v)
{
	struct st_hwcfg_info *buf = (struct st_hwcfg_info *)ioremap(my_proc_addr, my_proc_size);

	if (buf == NULL) {
	  seq_printf(m, "%s\n", "N/A");
	}else {
  	seq_printf(m, "%s\n", buf->project_name);
  	iounmap(buf);
  }

	return 0;
}

static int devmodel_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, fih_info_proc_read_project_show, NULL);
};

static struct file_operations devmodel_file_ops = {
	.owner   = THIS_MODULE,
	.open    = devmodel_proc_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release
};

static int fih_info_proc_read_hw_rev_show(struct seq_file *m, void *v)
{
	struct st_hwcfg_info *buf = (struct st_hwcfg_info *)ioremap(my_proc_addr, my_proc_size);

	if (buf == NULL) {
		seq_printf(m, "%s\n", "N/A");
	}else {
		if(strncmp(buf->project_name,"VZ1",3)==0) {
			seq_printf(m, "%s\n", buf->phase_hw);
		}
		else{
			seq_printf(m, "%s\n", buf->phase_sw);
		}
	  iounmap(buf);
  }

	return 0;
}
static int baseband_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, fih_info_proc_read_hw_rev_show, NULL);
};

static struct file_operations baseband_file_ops = {
	.owner   = THIS_MODULE,
	.open    = baseband_proc_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release
};

static int fih_info_proc_read_rf_band_show(struct seq_file *m, void *v)
{
	struct st_hwcfg_info *buf = (struct st_hwcfg_info *)ioremap(my_proc_addr, my_proc_size);

	if (buf == NULL) {
		seq_printf(m, "%s\n", "UNKNOWN\n");
	}else {
  	seq_printf(m, "%s\n", buf->rf_band);
  	iounmap(buf);
  }

	return 0;
}
static int bandinfo_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, fih_info_proc_read_rf_band_show, NULL);
};

static struct file_operations bandinfo_file_ops = {
	.owner   = THIS_MODULE,
	.open    = bandinfo_proc_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release
};

static int fih_info_proc_read_hwcfg_show(struct seq_file *m, void *v)
{
	struct st_hwid_table tb;

	fih_hwid_read(&tb);
	/* mpp */
	seq_printf(m, "r1=%d\n", tb.r1);
	seq_printf(m, "r2=%d\n", tb.r2);
	seq_printf(m, "r3=%d\n", tb.r3);
	/* info */
	seq_printf(m, "prj=%d\n", tb.prj);
	seq_printf(m, "rev=%d\n", tb.rev);
	seq_printf(m, "rf=%d\n", tb.rf);
	/* device tree */
	seq_printf(m, "dtm=%d\n", tb.dtm);
	seq_printf(m, "dtn=%d\n", tb.dtn);

	return 0;
}

static int hwcfg_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, fih_info_proc_read_hwcfg_show, NULL);
};

static struct file_operations hwcfg_file_ops = {
	.owner   = THIS_MODULE,
	.open    = hwcfg_proc_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release
};

static int fih_info_proc_read_module_show(struct seq_file *m, void *v)
{
	struct st_hwcfg_info *buf = (struct st_hwcfg_info *)ioremap(my_proc_addr, my_proc_size);

	if (buf == NULL) {
		seq_printf(m, "%s\n", "UNKNOWN\n");
	}else {
  	seq_printf(m, "%s\n", buf->sku_name);
  	iounmap(buf);
  }

	return 0;
}

static int module_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, fih_info_proc_read_module_show, NULL);
};

static struct file_operations module_file_ops = {
	.owner   = THIS_MODULE,
	.open    = module_proc_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release
};

static int fih_info_proc_read_hwmodel_show(struct seq_file *m, void *v)
{
	struct st_hwcfg_info *buf = (struct st_hwcfg_info *)ioremap(my_proc_addr, my_proc_size);

	if (buf == NULL) {
		seq_printf(m, "%s\n", "N/A");
	}else {
		seq_printf(m, "%s\n", buf->factory_name);
		iounmap(buf);
  }

	return 0;
}

static int hwmodel_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, fih_info_proc_read_hwmodel_show, NULL);
};

static struct file_operations hwmodel_file_ops = {
	.owner   = THIS_MODULE,
	.open    = hwmodel_proc_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release
};

static int fih_info_proc_read_fqc_xml_show(struct seq_file *m, void *v)
{
	struct st_hwcfg_info *buf = (struct st_hwcfg_info *)ioremap(my_proc_addr, my_proc_size);

	if (buf == NULL) {
		seq_printf(m, "%s\n", "N/A");
	}else {
		if(strncmp(buf->project_name,"ND1",3)==0) {
			seq_printf(m, "system/etc/fqc_%s_%s.xml\n",fih_hwcfg_match_pcba_description("dualsim")?"ds":"ss" ,buf->factory_name);
		}else if(strncmp(buf->project_name,"PLE",3) == 0) {
                    seq_printf(m, "system/etc/fqc_%s_%s.xml\n",fih_hwcfg_match_pcba_description("dualsim")?"ds":"ss","PLE");
                 }else {
	  		seq_printf(m, "system/etc/fqc_%s.xml\n", buf->factory_name);
		}
  	iounmap(buf);
  }

	return 0;
}

static int fqc_xml_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, fih_info_proc_read_fqc_xml_show, NULL);
};

static struct file_operations fqc_xml_file_ops = {
	.owner   = THIS_MODULE,
	.open    = fqc_xml_proc_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release
};

static int fih_info_proc_simslot_show(struct seq_file *m, void *v)
{
  int slot = 1;
  if(fih_hwcfg_match_pcba_description("dualsim"))
    slot=2;
  else
    slot=1;

  seq_printf(m, "%d\n", slot);
	return 0;
}

static int simslot_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, fih_info_proc_simslot_show, NULL);
};

static struct file_operations simslot_file_ops = {
	.owner   = THIS_MODULE,
	.open    = simslot_proc_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release
};

static struct {
		char *name;
		struct file_operations *ops;
} *p, fih_info[] = {
	{"devmodel", &devmodel_file_ops},
	{"baseband", &baseband_file_ops},
	{"bandinfo", &bandinfo_file_ops},
	{"hwcfg", &hwcfg_file_ops},
	{"MODULE", &module_file_ops},
	{"hwmodel", &hwmodel_file_ops},
	{"fqc_xml", &fqc_xml_file_ops},
	{"SIMSlot", &simslot_file_ops},
	{NULL}, };

static int __init fih_info_init(void)
{
	for (p = fih_info; p->name; p++) {
		if (proc_create(p->name, 0, NULL, p->ops) == NULL) {
			pr_err("fail to create proc/%s\n", p->name);
		}
	}

	return (0);
}

static void __exit fih_info_exit(void)
{
	for (p = fih_info; p->name; p++) {
		remove_proc_entry(p->name, NULL);
	}
}

module_init(fih_info_init);
module_exit(fih_info_exit);
