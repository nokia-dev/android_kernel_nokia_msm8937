#ifndef __FIH_HWID_1_H
#define __FIH_HWID_1_H

struct st_hwid_table {
	char r1; /* r1 is hundreds digit */
	char r2; /* r2 is tens digit     */
	char r3; /* r3 is units digit    */ /* NOT USED IN FAO */
	/* info */
	char prj; /* project */
	char rev; /* hw_rev */
	char rf;  /* rf_band */
	/* device tree */
	char dtm; /* Major number */
	char dtn; /* minor Number */
};

enum {
	/* hwid_value is equal: r1*100 + r2*10 + r3 */
	FIH_HWID_R1 = 0,	/* r1 is hundreds digit */
	FIH_HWID_R2,		/* r2 is tens digit     */
	FIH_HWID_R3,		/* r3 is units digit    */
	/* info */
	FIH_HWID_PRJ,
	FIH_HWID_REV,
	FIH_HWID_RF,
	/* device tree */
	FIH_HWID_DTM,
	FIH_HWID_DTN,
	/* max */
	FIH_HWID_MAX
};

/******************************************************************************
 *  device tree
 *****************************************************************************/

enum {
	FIH_DTM_DEFAULT = 0,
	FIH_DTM_MAX = 255,
};

enum {
	FIH_DTN_DEFAULT = 0,
	FIH_DTN_MAX = 255,
};

/******************************************************************************
 *  driver
 *****************************************************************************/

#endif /* __FIH_HWID_1_H */
