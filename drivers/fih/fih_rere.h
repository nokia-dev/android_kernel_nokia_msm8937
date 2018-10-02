#ifndef __FIH_RERE_H
#define __FIH_RERE_H

#define FIH_RERE_CMD_REBOOT_MODE      0x00000000
#define FIH_RERE_UNKNOWN_RESET        0x20389999  /* fih_apr */
#define FIH_RERE_SKT_RESTART          0x21534b54  /* !SKT */
#define FIH_RERE_MEMORY_TEST          0x33445566  /* Memory Test */
#define FIH_RERE_KERNEL_PANIC         0x46544443  /* fih_apr */
#define FIH_RERE_KERNEL_BUG           0x4B425547  /* fih_apr */
#define FIH_RERE_KERNEL_RESTART       0x52455354  /* fih_apr */
#define FIH_RERE_KERNEL_SHUTDOWN      0x5348444E  /* fih_apr */
#define FIH_RERE_KERNEL_WDOG          0x57444F47  /* fih_apr */
#define FIH_RERE_SECBOOT_UNLOCK       0x6C75636B  /* fih_lock */
#define FIH_RERE_FASTBOOT_MODE        0x77665500
#define FIH_RERE_REBOOT_MODE          0x77665501
#define FIH_RERE_RECOVERY_MODE        0x77665502
#define FIH_RERE_FRAMEWORK_EXCEPTION  0x77665503  /* fih_apr */
#define FIH_RERE_FACTORY_MODE         0x77665504  /* fih_apr */
#define FIH_RERE_MODEM_FATAL_ERR      0x77665506  /* fih_apr */
#define FIH_RERE_POFF_CHG_ALARM       0x77665507  /* poff chg alarm */

unsigned int fih_rere_rd_imem(void);
void fih_rere_wt_imem(unsigned int rere);

#endif /* __FIH_RERE_H */
