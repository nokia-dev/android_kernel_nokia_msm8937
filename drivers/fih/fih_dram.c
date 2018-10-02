#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/io.h>
#include <linux/mm.h>
#include "fih_ramtable.h"

#define FIH_PROC_DIR   "AllHWList"
#define FIH_PROC_PATH  "AllHWList/draminfo"
#define FIH_PROC_TESTRESULT_PATH  "AllHWList/dramtest_result"// add for memory test in RUNIN
#define FIH_PROC_SIZE  32

static char fih_proc_data[FIH_PROC_SIZE] = "";
static char fih_proc_test_result[FIH_PROC_SIZE] = {0};// add for memory test in RUNIN

#if 0
/* for draminfo Christine@ modify */
#define MSM_IMEM_BASE_PHY 0x08600000
#define SHARED_IMEM_DDR_OFFSET 0x19C

/* ddr_device_params_common_short: must be same with ddr_device_params_common
(boot_images:core/boot/ddr/common/params/v1/ddr_params.h)*/
typedef struct
{
  unsigned int device_name;
  unsigned int manufacture_name;
  unsigned int device_type;
} ddr_device_params_common_short;

static void fih_dram_setup(void)
{
    char *buf = fih_proc_data;
    ddr_device_params_common_short ddr_device_info;
    void __iomem *imem_base;

    imem_base = ioremap(MSM_IMEM_BASE_PHY, 4*1024);
    memcpy(&ddr_device_info, imem_base + SHARED_IMEM_DDR_OFFSET, sizeof(ddr_device_params_common_short));

    pr_info("%s: max_mapnr %lu pages %lu MB, imem_base 0x%p\n", __func__, max_mapnr, (max_mapnr >> (20-PAGE_SHIFT)), imem_base);
    sprintf(buf, "%lu MB", (max_mapnr >> (20-PAGE_SHIFT))); //page -> MB

    if(ddr_device_info.device_type == 0x2)
    {
        pr_info("fih_dram_init DDR2\n");
        strcat(buf, " DDR2");
    }
    else if(ddr_device_info.device_type == 0x5)
    {
        pr_info("fih_dram_init DDR3\n");
        strcat(buf, " DDR3");
    }
    else
    {
        strcat(buf, " Unknown");
    }
    pr_info("DDR : manufacture_name = 0x%x\n", ddr_device_info.manufacture_name);
    if(ddr_device_info.manufacture_name == 0x3)
    {
        strcat(buf, " Elpida");
    }
    else if(ddr_device_info.manufacture_name == 0x6)
    {
        strcat(buf, " Hynix");
    }
    else if(ddr_device_info.manufacture_name == 0xff)
    {
        strcat(buf, " Micron");
    }
    else if(ddr_device_info.manufacture_name == 0x1)
    {
        strcat(buf, " Samsung");
    }
    else
    {
        strcat(buf, " Unknown");
    }
    iounmap(imem_base);
}
#endif

/*-------------------------------------------------------------*/
#define FIH_MEM_ST_HEAD  0x48464d45  /* HFME */
#define FIH_MEM_ST_TAIL  0x454d4654  /* EMFT */

struct st_fih_mem {
	unsigned int head;
	unsigned int mfr_id;
	unsigned int ddr_type;
	unsigned int size_mb;
	unsigned int test_result;// add for memory test in RUNIN
	unsigned int tail;
};
struct st_fih_mem dram;

static void fih_dram_setup_MEM(void)
{
	char *buf = fih_proc_data;
	struct st_fih_mem *p;
	char size[16];

	/* get dram in mem which lk write */
	p = (struct st_fih_mem *)ioremap(FIH_MEM_MEM_ADDR, sizeof(struct st_fih_mem));
	if (p == NULL) {
		pr_err("%s: ioremap fail\n", __func__);
		dram.head = FIH_MEM_ST_HEAD;
		dram.mfr_id = 0;
		dram.ddr_type = 0;
		dram.size_mb = 0;
		dram.test_result = 0;// add for memory test in RUNIN
		dram.tail = FIH_MEM_ST_TAIL;
	} else {
		memcpy(&dram, p, sizeof(struct st_fih_mem));
		iounmap(p);
	}

	/* check magic of dram */
	if ((dram.head != FIH_MEM_ST_HEAD)||(dram.tail != FIH_MEM_ST_TAIL)) {
		pr_err("%s: bad magic\n", __func__);
		dram.head = FIH_MEM_ST_HEAD;
		dram.mfr_id = 0;
		dram.ddr_type = 0;
		dram.size_mb = 0;
		dram.test_result = 0;// add for memory test in RUNIN
		dram.tail = FIH_MEM_ST_TAIL;
	}

	switch (dram.mfr_id) {
		case 0x01: strcat(buf, "SAMSUNG"); break;
		case 0x03: strcat(buf, "MICRON"); break;
		case 0x06: strcat(buf, "SK-HYNIX"); break;
		case 0xFF: strcat(buf, "MICRON"); break;
		default: strcat(buf, "UNKNOWN"); break;
	}

	switch (dram.ddr_type) {
		case 2: strcat(buf, " LPDDR2"); break;
		case 5: strcat(buf, " LPDDR3"); break;
		default: strcat(buf, " LPDDRX"); break;
	}

	snprintf(size, sizeof(size), " %dMB", dram.size_mb);
	strcat(buf, size); 

	// add for memory test in RUNIN START
	//To-Do: use switch case to identify PASS/FAIL
	snprintf(fih_proc_test_result, sizeof(fih_proc_test_result), "%d", dram.test_result);
	// add for memory test in RUNIN END

}


// add for memory test in RUNIN START
/*-------------------------------------------------------------*/
static int fih_proc_test_result_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%s\n", fih_proc_test_result);
	return 0;
}

static int draminfo_test_result_open(struct inode *inode, struct file *file)
{
	return single_open(file, fih_proc_test_result_show, NULL);
};

static struct file_operations draminfo_test_result_ops = {
	.owner   = THIS_MODULE,
	.open    = draminfo_test_result_open,	
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release
};
// add for memory test in RUNIN END

/*-------------------------------------------------------------*/

static int fih_proc_read_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%s\n", fih_proc_data);
	return 0;
}

static int draminfo_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, fih_proc_read_show, NULL);
};

static struct file_operations draminfo_file_ops = {
	.owner   = THIS_MODULE,
	.open    = draminfo_proc_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release
};

static int __init fih_proc_init(void)
{
	//fih_dram_setup();
	fih_dram_setup_MEM();

	if (proc_create(FIH_PROC_PATH, 0, NULL, &draminfo_file_ops) == NULL) {
		pr_err("fail to create proc/%s\n", FIH_PROC_PATH);
		return (1);
	}
	// add for memory test in RUNIN START
	if (proc_create(FIH_PROC_TESTRESULT_PATH, 0, NULL, &draminfo_test_result_ops) == NULL) {
		pr_err("fail to create proc/%s\n", FIH_PROC_PATH);
		return (1);
	}
	// add for memory test in RUNIN END
	return (0);
}

static void __exit fih_proc_exit(void)
{
	remove_proc_entry(FIH_PROC_PATH, NULL);
}

module_init(fih_proc_init);
module_exit(fih_proc_exit);
