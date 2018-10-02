#ifndef __FIH_PON_H
#define __FIH_PON_H

/* power on cause is define by qc pmic */
#define FIH_PON_PMIC_C_HARD_RESET  0x00000001
#define FIH_PON_PMIC_C_SMPL        0x00000002
#define FIH_PON_PMIC_C_RTC         0x00000004
#define FIH_PON_PMIC_C_DC_CHG      0x00000008
#define FIH_PON_PMIC_C_USB_CHG     0x00000010
#define FIH_PON_PMIC_C_PON1        0x00000020
#define FIH_PON_PMIC_C_CBLPWR      0x00000040
#define FIH_PON_PMIC_C_KPDPWR      0x00000080

#define FIH_PON_PMIC_W_SOFT        0x00000001
#define FIH_PON_PMIC_W_PS_HOLD     0x00000002
#define FIH_PON_PMIC_W_PMIC_WD     0x00000004
#define FIH_PON_PMIC_W_GP1         0x00000008  /* Keypad_Reset1 */
#define FIH_PON_PMIC_W_GP2         0x00000010  /* Keypad_Reset2 */
#define FIH_PON_PMIC_W_SIMULT_N    0x00000020  /* simultaneous KPDPWR_N and RESIN_N */
#define FIH_PON_PMIC_W_RESIN_N     0x00000040
#define FIH_PON_PMIC_W_KPDPWR_N    0x00000080

/* power on cause is define by fih apr */
#define FIH_PON_APR_MODEM_FATAL           0x10000000
#define FIH_PON_APR_KERNEL_PANIC          0x20000000
#define FIH_PON_APR_UNKNOWN_RESET         0x40000000
#define FIH_PON_APR_HOST_RAMDUMP          0x60000000
#define FIH_PON_APR_FRAMEWORK_EXCEPTION   0x80000000
#define FIH_PON_APR_ABNORMAL_POWER_DOWN   0x01000000
#define FIH_PON_APR_ABNORMAL_POWER_RESET  0x02000000

/* power on cause is define for customer */
#define FIH_PON_CUST_SKT_RESTART          0x00100000

/* power on cause is define for fih ramdump */
#define FIH_PON_RAMDUMP_SD_SUCCESS        0x00010000
#define FIH_PON_RAMDUMP_SD_FAIL           0x00020000
#define FIH_PON_RAMDUMP_USB_SUCCESS       0x00040000
#define FIH_PON_RAMDUMP_USB_FAIL          0x00080000

#define FIH_PON_QC_PMIC_MASK              0x0000FFFF
#define FIH_PON_FIH_APR_MASK              0xFF000000
#define FIH_PON_CUSTOMZ_MASK              0x00F00000
#define FIH_PON_RAMDUMP_MASK              0x000F0000

#endif /* __FIH_PON_H */
