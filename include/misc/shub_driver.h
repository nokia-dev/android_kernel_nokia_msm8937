/* include/sharp/shub_driver.h  (Shub Driver)
 *
 * Copyright (C) 2014 SHARP CORPORATION
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef SHUB_DRIVER_H
#define SHUB_DRIVER_H

/* ------------------------------------------------------------------------- */
/*   PWM ERROR CODE                                                          */
/* ------------------------------------------------------------------------- */
/* SHMDS_HUB_3001_02 add S */
#define SHUB_ERR_PWM_PARAM         (-1)
#define SHUB_ERR_PWM_DEVICE        (-2)
#define SHUB_ERR_PWM_WIDTH         (-3)
#define SHUB_ERR_PWM_STE_STA       (-4)
#define SHUB_ERR_PWM_START         (-5)
#define SHUB_ERR_PWM_SUSPEND       (-6)      /* SHMDS_HUB_3002_01 add */
#define SHUB_ERR_PWM_LOGIC         (-7)      /* SHMDS_HUB_3003_01 add */
#define SHUB_ERR_PWM_PULSE         (-8)      /* SHMDS_HUB_3003_01 add */
#define SHUB_ERR_PWM_PULSE2CNT     (-9)      /* SHMDS_HUB_3003_02 add */
/* SHMDS_HUB_3001_02 add E */

/* ------------------------------------------------------------------------- */
/* TYPES                                                                     */
/* ------------------------------------------------------------------------- */
enum {
    SHUB_STOP_PED_TYPE_VIB,
    SHUB_STOP_PED_TYPE_TPS,
    SHUB_STOP_PED_TYPE_SPE,     /* SHMDS_HUB_0207_02 add */
    NUM_SHUB_STOP_PED_TYPE
};

/* ------------------------------------------------------------------------- */
/* STRUCT                                                                    */
/* ------------------------------------------------------------------------- */
// SHMDS_HUB_3001_02 add S
struct shub_pwm_param {
    uint8_t high;
    uint8_t total;
    uint8_t defaultStat;
};
// SHMDS_HUB_3001_02 add E

// SHMDS_HUB_3003_01 add S
struct shub_bkl_pwm_param {
    uint8_t logic;
    uint8_t defaultStat;
    uint8_t pulseNum;
    uint16_t high1;
    uint16_t total1;
    uint16_t high2;
    uint16_t total2;
    uint8_t pulse2Cnt;     /* SHMDS_HUB_3003_02 add */
};
// SHMDS_HUB_3003_01 add E

// SHMDS_HUB_3201_01 add S
struct shub_input_acc_info {
    uint8_t nStat;
    int32_t nX;
    int32_t nY;
    int32_t nZ;
    int32_t nPrd;
};
// SHMDS_HUB_3201_01 add E

/* ------------------------------------------------------------------------- */
/* PROTOTYPES                                                                */
/* ------------------------------------------------------------------------- */
int shub_api_stop_pedometer_func (int type);
int shub_api_restart_pedometer_func (int type);
int shub_api_enable_pwm(int enable); /* SHMDS_HUB_3001_02 add */
int shub_api_set_param_pwm(struct shub_pwm_param *param); /* SHMDS_HUB_3001_02 add */
int shub_api_get_param_pwm(struct shub_pwm_param *param); /* SHMDS_HUB_3001_02 add */
int shub_api_enable_bkl_pwm(int enable);                           /* SHMDS_HUB_3003_01 add */
int shub_api_set_param_bkl_pwm(struct shub_bkl_pwm_param *param);  /* SHMDS_HUB_3003_01 add */
int shub_api_get_param_bkl_pwm(struct shub_bkl_pwm_param *param);  /* SHMDS_HUB_3003_01 add */
int shub_api_set_port_bkl_pwm(int port);                           /* SHMDS_HUB_3003_01 add */
int shub_api_get_port_bkl_pwm(int *port);                          /* SHMDS_HUB_3003_01 add */
int shub_api_get_acc_info(struct shub_input_acc_info *info);  /* SHMDS_HUB_3201_01 add */

#endif /* SHUB_DRIVER_H */
