#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/io.h>

#include <fih/hwid.h>
#include "fih_ramtable.h"
#include "fih_hwcfg.h"

/* FIH_HWID_DEVELOP is used to condition the load hwid_table
 * 0 = load hwid table from memory which lk write
 * 1 = load default hwid table for development use */
#define FIH_HWID_DEVELOP  (0)

/* conserve current hwid table in kernel use */
struct st_hwid_table fih_hwid_table;

/* default coded hwid table for development use */
struct st_hwid_table def_hwid_table = {
	/* mpp */
	.r1 = 0,
	.r2 = 0,
	.r3 = 0,
	/* info */
	.prj = FIH_PRJ_D1C,
	.rev = FIH_REV_EVB,
	.rf  = FIH_RF_G_850_900_1800_1900_W_1_2_5_8_C_0_T_34_39_L_1_3_5_38_39_40_41,
	/* device tree */
	.dtm = 1,
	.dtn = 1,
};

void fih_hwid_setup(void)
{
#if (FIH_HWID_DEVELOP)
	pr_info("%s: setup hwid table by default coded for development\n", __func__);
	memcpy(&fih_hwid_table, &def_hwid_table, sizeof(struct st_hwid_table));
#else
	struct st_hwid_table *mem_hwid_table = (struct st_hwid_table *)ioremap(FIH_HWID_ADDR, sizeof(struct st_hwid_table));
	if (mem_hwid_table == NULL) {
		pr_err("%s: setup hwid table by default coded because load fail\n", __func__);
		memcpy(&fih_hwid_table, &def_hwid_table, sizeof(struct st_hwid_table));
		return;
	}

	pr_info("%s: read hwid table from memory\n", __func__);
	memcpy(&fih_hwid_table, mem_hwid_table, sizeof(struct st_hwid_table));
#endif

	fih_hwid_print(&fih_hwid_table);
}

void fih_hwid_print(struct st_hwid_table *tb)
{
	pr_info("fih_hwid_print: begin\n");

	if (NULL == tb) {
		pr_err("fih_hwid_print: tb = NULL\n");
		return;
	}

	/* mpp */
	pr_info("R1 = %d\n", tb->r1);
	pr_info("R2 = %d\n", tb->r2);
	pr_info("R3 = %d\n", tb->r3);
	/* info */
	pr_info("PROJECT = %d\n", tb->prj);
	pr_info("HW_REV = %d\n", tb->rev);
	pr_info("RF_BAND = %d\n", tb->rf);
	/* device tree */
	pr_info("DT_MAJOR = %d\n", tb->dtm);
	pr_info("DT_MINOR = %d\n", tb->dtn);

	pr_info("fih_hwid_print: end\n");
}

void fih_hwid_read(struct st_hwid_table *tb)
{
	memcpy(tb, &fih_hwid_table, sizeof(struct st_hwid_table));
}

int fih_hwid_fetch(int idx)
{
	struct st_hwid_table *tb = &fih_hwid_table;
	int ret = (-1);

	switch (idx) {
		/* mpp */
		case FIH_HWID_R1: ret = tb->r1; break;
		case FIH_HWID_R2: ret = tb->r2; break;
		case FIH_HWID_R3: ret = tb->r3; break;
		/* info */
		case FIH_HWID_PRJ: ret = tb->prj; break;
		case FIH_HWID_REV: ret = tb->rev; break;
		case FIH_HWID_RF: ret = tb->rf; break;
		/* device tree */
		case FIH_HWID_DTM: ret = tb->dtm; break;
		case FIH_HWID_DTN: ret = tb->dtn; break;
		default:
			pr_warn("fih_hwid_fetch: unknown idx = %d\n", idx);
			ret = (-1);
			break;
	}

	return ret;
}

int fih_hwcfg_match_pcba_description(char* key)
{
  int ret = 0;
  struct st_hwcfg_info *buf = (struct st_hwcfg_info *)ioremap(FIH_HWCFG_MEM_ADDR, FIH_HWCFG_MEM_SIZE);

  if (buf == NULL) {
    pr_warn("fih_hwcfg_match_pcba_description: ipremap fail\n");
    return ret;
  }else {
    //Make sure pcba_description have data to parser
    if(strlen(buf->pcba_description) > 0){
      char * pch = NULL;
      pch = strstr(buf->pcba_description, key);
      if(pch){
        pr_warn("fih_hwcfg_match_pcba_description: Search keyword in HWCFG\n");
        ret= 1;
      }
    }
    iounmap(buf);
  }

  return ret;
}
