/*
 *  ml630q790.h - Linux kernel modules for Sensor Hub 
 *
 *  Copyright (C) 2012-2014 LAPIS SEMICONDUCTOR CO., LTD.
 *
 *  This file is based on :
 *    ml610q792.h - Linux kernel modules for acceleration sensor
 *    http://android-dev.kyocera.co.jp/source/versionSelect_URBANO.html
 *    (C) 2012 KYOCERA Corporation
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef _ML630Q790_H_
#define _ML630Q790_H_
#ifndef NO_LINUX
#include <linux/spi/spi.h>
#include <linux/i2c.h>
#include <linux/poll.h>
// SHMDS_HUB_0701_05 add S
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
// SHMDS_HUB_0701_05 add E
#include <linux/input.h> /* SHMDS_HUB_0308_01 add */
#endif
#include "shub_io.h"

// SOFT SW
#define SHUB_SW_GYRO_ENABLE         	/* SHMDS_HUB_2902_01 add */
//#define SHUB_SW_GPIO_PMIC           	/* SHMDS_HUB_0104_03 add */
#define SHUB_SW_PINCTRL             	/* SHMDS_HUB_1501_01 add */
#define SHUB_SW_INPUT_SYNC_MOD      	/* SHMDS_HUB_1501_01 add */
//#define SHUB_SW_TIME_API            	/* SHMDS_HUB_1801_01 add */
//#define SHUB_SW_GPIO_INT1           	/* SHMDS_HUB_0115_01 add */
#define SHUB_SW_PICKUP_DETECT       	/* SHMDS_HUB_1701_10 add */
#define SHUB_SW_PICKUP_ALGO_03      	/* SHMDS_HUB_1702_01 add */
#define SHUB_SW_TWIST_DETECT        	/* SHMDS_HUB_2301_01 add */
//#define SHUB_SW_PWM_ENABLE          	/* SHMDS_HUB_3001_01 add  SHMDS_HUB_3001_03 del*/
//#define SHUB_SW_MAG_INT             	/* SHMDS_HUB_0103_20 add */
#define SHUB_SW_SHTERM              	/* SHMDS_HUB_3703_01 add */

#define CONFIG_HOSTIF_SPI
//#define CONFIG_HOSTIF_I2C
//#define CONFIG_ML630Q790_DEBUG
    #define CONFIG_ACC_SENSOR
//#define CONFIG_ACC_U2DH
    #define CONFIG_GYRO_SENSOR
    #define CONFIG_MAG_SENSOR
    #define CONFIG_ORI_SENSOR
    #define CONFIG_LINEACC_SENSOR
    #define CONFIG_GAMEROT_SENSOR
    #define CONFIG_GRAV_SENSOR
    #define CONFIG_GYROUNC_SENSOR
    #define CONFIG_MAGROT_SENSOR
    #define CONFIG_MAGUNC_SENSOR
    #define CONFIG_ROTVEC_SENSOR
    #define CONFIG_PEDO_SENSOR
    #define CONFIG_PEDODEC_SENSOR
    #define CONFIG_SIGNIFICANT_MOTION
//#define CONFIG_PICKUP_PROX
//#define CONFIG_BARO_SENSOR
//#define CONFIG_PWM_LED
//#define CONFIG_BKL_PWM_LED
#define CONFIG_ANDROID_ENGINEERING
#define SHUB_SW_FACTORY_MODE

#define SENOSR_HUB_I2C_SLAVE_ADDRESS (0x2f)
#define SENOSR_HUB_DRIVER_NAME    "sensorhub"

#define SHUB_ACTIVE_ACC                   (0x00000010)
#define SHUB_ACTIVE_PEDOM                 (0x00000020)
#define SHUB_ACTIVE_PEDOM_NO_NOTIFY       (0x00000040)
#define SHUB_ACTIVE_EXT_PEDOM             (0x00000080)
#define SHUB_ACTIVE_EXT_TOTAL_STATUS      (0x00000100)
#define SHUB_ACTIVE_DEVICE_ORI            (0x00000200) /* SHMDS_HUB_0132_01 add */
#define SHUB_ACTIVE_TWIST                 (0x00000400) /* SHMDS_HUB_2301_01 add */
#define SHUB_ACTIVE_SHEX_ACC              (0x00000800) /* SHMDS_HUB_0201_01 add */

#define SHUB_ACTIVE_GYRO                  (0x00001000)
#define SHUB_ACTIVE_MAG                   (0x00002000)
#define SHUB_ACTIVE_BARO                  (0x00004000)  /* SHMDS_HUB_0120_01 add */

#define SHUB_ACTIVE_ORI                   (0x00008000)
#define SHUB_ACTIVE_GRAVITY               (0x00010000)
#define SHUB_ACTIVE_LACC                  (0x00020000)
#define SHUB_ACTIVE_RV                    (0x00040000)
#define SHUB_ACTIVE_RV_NONMAG             (0x00080000)
#define SHUB_ACTIVE_RV_NONGYRO            (0x00100000)

#define SHUB_ACTIVE_GYROUNC               (0x00400000)
#define SHUB_ACTIVE_MAGUNC                (0x00800000)
#define SHUB_ACTIVE_PEDODEC               (0x01000000)
#define SHUB_ACTIVE_SIGNIFICANT           (0x02000000)

#define SHUB_ACTIVE_GDEC                  (0x08000000)
#define SHUB_ACTIVE_MOTIONDEC             (0x10000000)
#define SHUB_ACTIVE_PEDODEC_NO_NOTIFY     (0x20000000)
#define SHUB_ACTIVE_PICKUP                (0x40000000) /* SHMDS_HUB_1701_01 add */

#define SHUB_ACTIVE_ERROR                 (0x80000000)

/* SHMDS_HUB_0201_01 SHMDS_HUB_0206_02 SHMDS_HUB_0120_01 SHMDS_HUB_2301_01 mod S */
// Sensor All
#define ACTIVE_FUNC_MASK (\
        SHUB_ACTIVE_ACC                   | \
        SHUB_ACTIVE_PEDOM                 | \
        SHUB_ACTIVE_PEDOM_NO_NOTIFY       | \
        SHUB_ACTIVE_SHEX_ACC              | \
        SHUB_ACTIVE_SIGNIFICANT           | \
        SHUB_ACTIVE_GYRO                  | \
        SHUB_ACTIVE_MAG                   | \
        SHUB_ACTIVE_BARO                  | \
        SHUB_ACTIVE_ORI                   | \
        SHUB_ACTIVE_GRAVITY               | \
        SHUB_ACTIVE_LACC                  | \
        SHUB_ACTIVE_RV                    | \
        SHUB_ACTIVE_RV_NONMAG             | \
        SHUB_ACTIVE_RV_NONGYRO            | \
        SHUB_ACTIVE_GYROUNC               | \
        SHUB_ACTIVE_MAGUNC                | \
        SHUB_ACTIVE_MOTIONDEC             | \
        SHUB_ACTIVE_PICKUP                | \
        SHUB_ACTIVE_DEVICE_ORI            | \
        SHUB_ACTIVE_GDEC                  | \
        SHUB_ACTIVE_PEDODEC_NO_NOTIFY     | \
        SHUB_ACTIVE_EXT_PEDOM             | \
        SHUB_ACTIVE_EXT_TOTAL_STATUS      | \
        SHUB_ACTIVE_PEDODEC)

#define ACC_GROUP_MASK  (SHUB_ACTIVE_ACC  | SHUB_ACTIVE_SHEX_ACC )
#define GYRO_GROUP_MASK (SHUB_ACTIVE_GYRO | SHUB_ACTIVE_GYROUNC )
#define MAG_GROUP_MASK  (SHUB_ACTIVE_MAG  | SHUB_ACTIVE_MAGUNC )
#define BARO_GROUP_MASK (SHUB_ACTIVE_BARO)  /* SHMDS_HUB_0120_01 add */

// App Task : ACC Enable
#define APPTASK_GROUP_ACC_MASK (\
        SHUB_ACTIVE_PEDOM                 | \
        SHUB_ACTIVE_PEDOM_NO_NOTIFY       | \
        SHUB_ACTIVE_SIGNIFICANT           | \
        SHUB_ACTIVE_MOTIONDEC             | \
        SHUB_ACTIVE_PICKUP                | \
        SHUB_ACTIVE_DEVICE_ORI            | \
        SHUB_ACTIVE_EXT_PEDOM             | \
        SHUB_ACTIVE_PEDODEC_NO_NOTIFY     | \
        SHUB_ACTIVE_PEDODEC)

// App Task : GYRO Enable
#define APPTASK_GROUP_GYRO_MASK (\
        SHUB_ACTIVE_TWIST)

// App Task : MAG Enable
#define APPTASK_GROUP_MAG_MASK            0

// App Task : ALL
#define APPTASK_GROUP_MASK                ( \
        APPTASK_GROUP_ACC_MASK            | \
        APPTASK_GROUP_GYRO_MASK           | \
        APPTASK_GROUP_MAG_MASK)

// Fusion Task : ACC + GYRO + MAG Enable
#define FUSION_9AXIS_GROUP_MASK (\
        SHUB_ACTIVE_ORI                   |\
        SHUB_ACTIVE_GRAVITY               |\
        SHUB_ACTIVE_LACC                  |\
        SHUB_ACTIVE_RV                    |\
        SHUB_ACTIVE_RV_NONMAG)

// Fusion Task : ACC + GYRO Enable
#define FUSION_6AXIS_ACC_GYRO_MASK        0

// Fusion Task : ACC + MAG Enable
#define FUSION_6AXIS_ACC_MAG_MASK (\
        SHUB_ACTIVE_RV_NONGYRO )

// Fusion Task : GYRO + MAG Enable
#define FUSION_6AXIS_GYRO_MAG_MASK (\
        SHUB_ACTIVE_TWIST)

// Fusion Task : ALL
#define FUSION_GROUP_MASK (\
        FUSION_9AXIS_GROUP_MASK           | \
        FUSION_6AXIS_ACC_GYRO_MASK        | \
        FUSION_6AXIS_ACC_MAG_MASK         | \
        FUSION_6AXIS_GYRO_MAG_MASK)

// Fusion Task : ACC Sensor
#define FUSION_GROUP_ACC_MASK (\
        FUSION_9AXIS_GROUP_MASK           | \
        FUSION_6AXIS_ACC_GYRO_MASK        | \
        FUSION_6AXIS_ACC_MAG_MASK)

// Fusion Task : GYRO Sensor
#define FUSION_GROUP_GYRO_MASK (\
        FUSION_9AXIS_GROUP_MASK           | \
        FUSION_6AXIS_ACC_GYRO_MASK        | \
        FUSION_6AXIS_GYRO_MAG_MASK)

// Fusion Task : MAG Sensor
#define FUSION_GROUP_MAG_MASK (\
        FUSION_9AXIS_GROUP_MASK           | \
        FUSION_6AXIS_ACC_MAG_MASK         | \
        FUSION_6AXIS_GYRO_MAG_MASK)

// ACC Sensor
#define ACTIVE_ACC_GROUP_MASK (\
        FUSION_9AXIS_GROUP_MASK           | \
        FUSION_6AXIS_ACC_GYRO_MASK        | \
        FUSION_6AXIS_ACC_MAG_MASK         | \
        ACC_GROUP_MASK                    | \
        APPTASK_GROUP_ACC_MASK            | \
        SHUB_ACTIVE_GDEC)

// GYRO Sensor
#define ACTIVE_GYRO_GROUP_MASK (\
        GYRO_GROUP_MASK                   | \
        FUSION_9AXIS_GROUP_MASK           | \
        FUSION_6AXIS_ACC_GYRO_MASK        | \
        FUSION_6AXIS_GYRO_MAG_MASK)

// MAG Sensor
#define ACTIVE_MAG_GROUP_MASK  ( \
        GYRO_GROUP_MASK                   | \
        MAG_GROUP_MASK                    | \
        FUSION_9AXIS_GROUP_MASK           | \
        FUSION_6AXIS_ACC_MAG_MASK         | \
        FUSION_6AXIS_GYRO_MAG_MASK)
/* SHMDS_HUB_0201_01 mod E */

#define PEDOM_GROUP_MASK (\
        SHUB_ACTIVE_SIGNIFICANT           | \
        SHUB_ACTIVE_PEDOM                 | \
        SHUB_ACTIVE_PEDOM_NO_NOTIFY       | \
        SHUB_ACTIVE_PEDODEC_NO_NOTIFY     | \
        SHUB_ACTIVE_PEDODEC)

#define STEPCOUNT_GROUP_MASK (\
        SHUB_ACTIVE_PEDOM                 | \
        SHUB_ACTIVE_PEDOM_NO_NOTIFY)

#define STEPDETECT_GROUP_MASK (\
        SHUB_ACTIVE_PEDODEC_NO_NOTIFY     | \
        SHUB_ACTIVE_PEDODEC)

/* 30min */
#define SHUB_TIMER_MAX            (30*60*1000)

/*
   Logging (batch mode)
   */
#define LOGGING_RAM_SIZE          (3*1024)
#define FIFO_SIZE                 (512)
#define PRM_SIZE                  (16)

// data size
#define DATA_SIZE_ACC                7
#define DATA_SIZE_MAG                8
#define DATA_SIZE_GYRO               7
#define DATA_SIZE_MAG_CAL_OFFSET     6
#define DATA_SIZE_GYRO_CAL_OFFSET    6
#define DATA_SIZE_ORI                8
#define DATA_SIZE_GRAVITY            7
#define DATA_SIZE_LINEARACC          7
#define DATA_SIZE_RVECT              9
#define DATA_SIZE_GEORV              10
#define DATA_SIZE_GAMERV             9
#define DATA_SIZE_PEDOCNT            10
#define DATA_SIZE_TOTALSTATUS        9 
/* SHMDS_HUB_0120_01 add S */
#ifdef CONFIG_BARO_SENSOR               /* SHMDS_HUB_0120_09 mod */
#define DATA_SIZE_BARO                4 /* SHMDS_HUB_0120_02 mod (5->4) */
#endif
/* SHMDS_HUB_0120_01 add E */

#define TIMER_RESO 5

/* SHMDS_HUB_0604_01 add S */
/* SHMDS_HUB_2203_01 mod S */
#define SHUB_MAXG_A                  8 /* 8G (2G,4G,8G,16G) */
#if (SHUB_MAXG_A == 2)
#define SHUB_SET_RANG_VAL            0
#define ACC_CMN_MAX               2048
#define ACC_CMN_MIN              -2048
#elif (SHUB_MAXG_A == 4)
#define SHUB_SET_RANG_VAL            1
#define ACC_CMN_MAX               4096
#define ACC_CMN_MIN              -4096
#elif (SHUB_MAXG_A == 8)
#define SHUB_SET_RANG_VAL            2
#define ACC_CMN_MAX               8192
#define ACC_CMN_MIN              -8192
#else // (SHUB_MAXG_A == 16)
#define SHUB_SET_RANG_VAL            3
#define ACC_CMN_MAX              16384
#define ACC_CMN_MIN             -16384
#endif
/* SHMDS_HUB_2203_01 mod E */

#define MAG_CMN_MAX              20000
#define MAG_CMN_MIN             -20000
/* SHMDS_HUB_0604_01 add E */
/* SHMDS_HUB_0120_01 add S */
//#define BARO_CMN_MASK         0x01FFFF /* SHMDS_HUB_0120_02 add SHMDS_HUB_0120_03 del*/
#define BARO_CMN_MAX            (1260*4096) /* SHMDS_HUB_0120_02 SHMDS_HUB_0120_03 mod (0x01FFFF -> 0xFFFFFF -> 1260*4096) */
#define BARO_CMN_MIN            (260*4096)  /* SHMDS_HUB_0120_01 mod (0 -> 260*4096) */
/* SHMDS_HUB_0120_01 add E */

// SHMDS_HUB_0601_01 add S
enum{
    SHUB_INPUT_ACC,                 // [----] Accelerometer
    SHUB_INPUT_MAG,                 // [COM1] Magnetic Filed sensor
    SHUB_INPUT_GYRO,                // [COM2] Gyroscope
    SHUB_INPUT_GRAVITY,             // [COM3] Gravity
    SHUB_INPUT_LINEARACC,           // [COM3] Linear Acceleration
    SHUB_INPUT_ORI,                 // [----] Orientation
    SHUB_INPUT_RVECT,               // [----] Rotation Vector
    SHUB_INPUT_SIGNIFICANT,         // [COM5] Significant Motion
    SHUB_INPUT_GYROUNC,             // [COM2] Gyroscope Uncalibrated
    SHUB_INPUT_MAGUNC,              // [COM1] Magnetic Field sensor Uncalibrated
    SHUB_INPUT_GAMEROT,             // [COM4] Game Rotation Vector
    SHUB_INPUT_PEDO,                // [COM5] Step Counter
    SHUB_INPUT_PEDODET,             // [COM5] Step Detector
    SHUB_INPUT_GEORV,               // [COM4] Geomagnetic Rotation Vector
    SHUB_INPUT_DEVICE_ORI,          // [COM5] Device orientation   /* SHMDS_HUB_0132_01 add */
    SHUB_INPUT_BARO,                // [COM3] Barometric Pressure  /* SHMDS_HUB_0120_01 add */
    SHUB_INPUT_MAXNUM,              // max number 
    SHUB_INPUT_META_DATA = 0x0100   // METADATA SHMDS_HUB_0601_05
};
// SHMDS_HUB_0601_01 add E

/* SHMDS_HUB_0110_01 add S */
enum{
    SHUB_GPIO_PIN_RESET,            // RESET
    SHUB_GPIO_PIN_BRMP,             // BRMP
    SHUB_GPIO_PIN_INT0,             // INT0
#ifdef SHUB_SW_GPIO_INT1            // SHMDS_HUB_0115_01 mod
    SHUB_GPIO_PIN_INT1,             // INT1
#endif
    SHUB_GPIO_PIN_MAXNUM            // max number
};

enum{
    SHUB_GPIO_PULL_DOWN,            // RESET
    SHUB_GPIO_PULL_UP,              // BRMP
    SHUB_GPIO_PULL_MAXNUM           // max number
};
/* SHMDS_HUB_0110_01 add E */

/* SHMDS_HUB_0311_01 add S */
enum{
    SHUB_SAME_NOTIFY_GYRO,
    SHUB_SAME_NOTIFY_MAG,
    NUM_SHUB_SAME_NOTIFY_KIND
};
/* SHMDS_HUB_0311_01 add E */

/* SHMDS_HUB_0322_01 add S */
enum{
   SHUB_FUP_NO_COMMAND,
   SHUB_FUP_ERR_WRITE = 0xFFFF0001,
   SHUB_FUP_ERR_CHECK_ACCESS,
   SHUB_FUP_ERR_INIT_COMMON,
   SHUB_FUP_ERR_SET_PARAM
};
/* SHMDS_HUB_0322_01 add E */

/* SHMDS_HUB_1701_03 add S */
#ifdef CONFIG_PICKUP_PROX
#define SHUB_PICKUP_ENABLE_PARAM     (0x17) /* SHMDS_HUB_2701_01 mod (7 -> 0x17) */
#else
#define SHUB_PICKUP_ENABLE_PARAM     (0x26) /* SHMDS_HUB_1701_09 SHMDS_HUB_1701_13(0x04->0x26) */
#endif
/* SHMDS_HUB_1701_03 add E */

/* SHMDS_HUB_1801_01 add S */
#ifdef SHUB_SW_TIME_API
void shub_dbg_timer_start(struct timespec *tv);
void shub_dbg_timer_end(struct timespec tv, unsigned int cmd);
#define SHUB_DBG_TIME_INIT      struct timespec shub_timeval;
#define SHUB_DBG_TIME_START     shub_dbg_timer_start(&shub_timeval);
#define SHUB_DBG_TIME_END(cmd)  shub_dbg_timer_end(shub_timeval, cmd);
#else
#define SHUB_DBG_TIME_INIT
#define SHUB_DBG_TIME_START
#define SHUB_DBG_TIME_END(cmd)
#endif
/* SHMDS_HUB_1801_01 add E */

#define usleep(x)                 usleep_range(x,x+1)
#define monotonic_to_bootbased(x) get_monotonic_boottime(x)

/* SHMDS_HUB_0308_01 add S */
static inline void shub_input_set_value(struct input_dev *dev, unsigned code, int value)
{
/* SHMDS_HUB_0603_02 add S */
	if(value == 0){
		dev->absinfo[code].value = 1;
	}else{
		dev->absinfo[code].value = 0;
	}	
/* SHMDS_HUB_0603_02 add E */
}
/* SHMDS_HUB_0308_01 end E */

/* SHMDS_HUB_0603_01 add S */
#define SHUB_INPUT_VAL_CLEAR(dev, val ,value)      shub_input_set_value(dev, val, value)  /* SHMDS_HUB_0603_02 add (value) */
/* SHMDS_HUB_0603_01 add S */

/* SHMDS_HUB_0602_01 add S */
static inline void shub_input_sync_init(struct input_dev *dev)
{
#ifndef SHUB_SW_INPUT_SYNC_MOD
    dev->sync = 0; 
#endif
}
/* SHMDS_HUB_0602_01 end E */

/* SHMDS_HUB_0308_01 add S */
static inline void shub_input_first_report(struct input_dev *dev, int value)
{
#ifdef SHUB_SW_INPUT_SYNC_MOD
    dev->absinfo[ABS_RUDDER].value = 0;
    input_report_abs(dev, ABS_RUDDER, value);
#endif
}

static inline void shub_set_param_first(struct input_dev *dev)
{
#ifdef SHUB_SW_INPUT_SYNC_MOD
    input_set_abs_params(dev, ABS_RUDDER, 0, 0x1, 0, 0);
#endif
}
/* SHMDS_HUB_0308_01 end E */

/* SHMDS_HUB_0110_01 add S */
int shub_get_gpio_no(int gpio);
int shub_gpio_request(int gpio);
int shub_gpio_free(int gpio);
int shub_gpio_direction_output(int gpio, int data);
int shub_gpio_direction_input(int gpio);
int shub_gpio_set_value(int gpio, int data);
int shub_gpio_get_value(int gpio);
int shub_gpio_tlmm_config(int gpio, int data);
int shub_gpio_to_irq(int gpio);
/* SHMDS_HUB_0110_01 add E */

// SHMDS_HUB_1101_01 add S
void shub_qos_start(void);
void shub_qos_end(void);
// SHMDS_HUB_1101_01 add E

#ifdef CONFIG_HOSTIF_I2C
int shub_set_gpio_no( struct i2c_client *client ); /* SHMDS_HUB_0110_01 add */
int32_t shub_suspend( struct i2c_client *client, pm_message_t mesg );
int32_t shub_resume( struct i2c_client *client );
#endif
#ifdef CONFIG_HOSTIF_SPI
int shub_set_gpio_no( struct spi_device *client ); /* SHMDS_HUB_0110_01 add */
int shub_spi_init(void);                           /* SHMDS_HUB_3701_01 add */
void shub_spi_exit(void);                          /* SHMDS_HUB_3701_01 add */
/* SHMDS_HUB_0130_01 del S */ 
//int32_t shub_suspend( struct spi_device *client, pm_message_t mesg );
//int32_t shub_resume( struct spi_device *client );
/* SHMDS_HUB_0130_01 del E */
/* SHMDS_HUB_0130_01 add S */ 
int32_t shub_suspend( struct device *dev, pm_message_t mesg );
int32_t shub_resume( struct device *dev );
/* SHMDS_HUB_0130_01 add E */ 
#endif
void shub_user_reset(void);                        /* SHMDS_HUB_0304_02 add */
int32_t shub_initialize( void );
int32_t shub_update_fw(bool boot, uint8_t *arg_iDataPage1, uint8_t *arg_iDataPage2, uint32_t arg_iLen);
int32_t shub_get_fw_version(uint8_t *arg_iData);
int32_t shub_get_sensors_data(int32_t type, int32_t* data);
int32_t shub_activate(int32_t arg_iSensType, int32_t arg_iEnable);
int32_t shub_activate_logging(int32_t arg_iSensType, int32_t arg_iEnable);
int32_t shub_set_delay(int32_t arg_iSensType, int32_t delay);
int32_t shub_set_delay_logging(int32_t arg_iSensType, int32_t delay);
int32_t shub_get_current_active(void);
int32_t shub_get_current_active_logging(void);
int32_t shub_direct_hostcmd(uint16_t cmd, const uint8_t *prm, uint8_t *rslt);
int32_t shub_logging_flush(void);
void shub_logging_clear(void);
int32_t shub_get_pedo_event( void );
int32_t shub_probe(void);
int32_t shub_remove(void);
void shub_debug_level_chg(int32_t lv);

int32_t shub_set_param(int32_t type,int32_t* data);
int32_t shub_get_param(int32_t type,int32_t* data);

int32_t shub_init_app(int32_t type);
int32_t shub_clear_data(int32_t type);
int32_t shub_get_data_pedometer(int32_t* data);
int32_t shub_get_data_normal_pedometer(int32_t* data);
int32_t shub_get_data_act_detection(int32_t* data);
int32_t shub_get_data_low_power(int32_t* data);
int32_t shub_get_data_motion(int32_t* data);

int32_t shub_read_sensor(uint8_t type, uint8_t addr ,uint8_t *data);
int32_t shub_write_sensor(uint8_t type, uint8_t addr ,uint8_t data);
int32_t shub_set_acc_offset(int32_t* offsets);
int32_t shub_get_acc_offset(int32_t* offsets);
int32_t shub_set_acc_position(int32_t type);
int32_t shub_set_mag_offset(int32_t* offsets);
int32_t shub_get_mag_offset(int32_t* offsets);
int32_t shub_set_mag_position(int32_t type);
int32_t shub_cal_mag_axis_interfrence(int32_t* mat);
int32_t shub_set_gyro_offset(int32_t* offsets);
int32_t shub_get_gyro_offset(int32_t* offsets);
int32_t shub_set_gyro_position(int32_t type);
int32_t shub_adjust_value(int32_t min,int32_t max,int32_t value);
int32_t shub_set_baro_offset(int32_t offset);   /* SHMDS_HUB_0120_01 add */
int32_t shub_get_baro_offset(int32_t* offset);  /* SHMDS_HUB_0120_01 add */
/* SHMDS_HUB_0103_01 add S */
int32_t shub_direct_sendcmd(uint16_t cmd, const uint8_t *prm);
int32_t shub_noreturn_sendcmd(uint16_t cmd, const uint8_t *prm);
int32_t shub_direct_get_error(uint16_t *error);
int32_t shub_direct_get_result(uint8_t *result);
int32_t shub_direct_multi_get_result(uint8_t *result);
int32_t shub_hostif_write(uint8_t adr, const uint8_t *data, uint16_t size);
int32_t shub_hostif_read(uint8_t adr, uint8_t *data, uint16_t size);
int32_t shub_cmd_wite(struct IoctlDiagCmdReq *arg_cmd);
int32_t shub_cmd_read(struct IoctlDiagCmdReq *arg_cmd);
int32_t shub_get_int_state(int *state);
/* SHMDS_HUB_0103_01 add E */

/* Input Subsystem */
void shub_input_report_acc(int32_t *data);
void shub_input_report_mag(int32_t *data);
void shub_input_report_gyro(int32_t *data);
void shub_input_report_gyro_uncal(int32_t *data);
void shub_input_report_mag_uncal(int32_t *data);
void shub_input_report_orien(int32_t *data);
void shub_input_report_grav(int32_t *data);
void shub_input_report_linear(int32_t *data);
void shub_input_report_rot(int32_t *data);
void shub_input_report_rot_gyro(int32_t *data);
void shub_input_report_rot_mag(int32_t *data);
void shub_input_report_stepdetect(int32_t *data);
void shub_input_report_stepcnt(int32_t *data);
void shub_input_report_significant(int32_t *data);
void shub_input_report_baro(int32_t *data);  /* SHMDS_HUB_0120_01 add */
bool shub_fw_update_check(void);
bool shub_connect_check(void);
/* SHMDS_HUB_0201_01 add S */
void shub_input_report_exif_grav_det(bool send);
void shub_input_report_exif_ride_pause_det(bool send, int32_t info);    // SHMDS_HUB_0209_02 add
void shub_input_report_exif_pickup_det(void);                           /* SHMDS_HUB_1701_01 */
void shub_input_report_exif_twist_det(unsigned char info);              /* SHMDS_HUB_2301_01 */
void shub_input_report_exif_mot_det(unsigned char det_info);
void shub_input_report_exif_shex_acc(int32_t *data);
void shub_input_report_exif_judge(void);
int shub_vibe_notify_check(int kind);                   /* SHMDS_HUB_0213_01 add */
int shub_set_default_parameter(void);
/* SHMDS_HUB_0201_01 add E */
int shub_set_default_ped_parameter(void);               /* SHMDS_HUB_0204_19 add */
/* SHMDS_HUB_0202_01 add S */
void shub_set_already_md_flg(int data);
void shub_clr_already_md_flg(int data);
int shub_get_already_md_flg(void);
/* SHMDS_HUB_0202_01 add E */
/* SHMDS_HUB_0109_02 add S */
int shub_get_acc_axis_val(void);
int shub_get_gyro_axis_val(void);
int shub_get_mag_axis_val(void);
/* SHMDS_HUB_0109_02 add E */
/* SHMDS_HUB_0601_01 add S */
struct input_dev *shub_com_allocate_device(int inp_type, struct device *dev);
void shub_com_unregister_device(int inp_type);
/* SHMDS_HUB_0601_01 add E */
int shub_get_exif_delay_ms(void);   // SHMDS_HUB_0206_03 add
void shub_set_param_check_exif(int type, int *data);    // SHMDS_HUB_0207_01 add
void shub_get_param_check_exif(int type, int *data);    // SHMDS_HUB_0207_01 add
void shub_set_enable_ped_exif_flg(int en);              // SHMDS_HUB_0207_01 add
int shub_get_mcu_ped_enable(void);                      // SHMDS_HUB_0206_06 add
void shub_exif_input_val_init(void);                    // SHMDS_HUB_0304_01 add
/* SHMDS_HUB_0211_01 add S */
void shub_set_param_exif_md_mode(int32_t *param);
void shub_set_exif_md_mode_flg(int32_t mode);
int32_t shub_get_exif_md_mode_flg(void);
/* SHMDS_HUB_0211_01 add E */

/* SHMDS_HUB_3701_01 add S */
int shub_mcu_init(void);
int shub_exif_init(void);
int shub_acc_init(void);
int shub_gyro_init(void);
int shub_mag_init(void);
int shub_orien_init(void);
int shub_linearacc_init(void);
#ifdef CONFIG_BARO_SENSOR
int shub_baro_init(void);
#endif
int shub_rot_gyro_init(void);
int shub_grav_init(void);
int shub_gyrounc_init(void);
int shub_rot_mag_init(void);
int shub_mag_unc_init(void);
int shub_rot_init(void);
int shub_pedo_init(void);
int shub_pedodet_init(void);
int shub_signif_init(void);
int shub_devori_init(void);
int shub_diag_init(void);

void shub_diag_exit(void);
void shub_devori_exit(void);
void shub_signif_exit(void);
void shub_pedodet_exit(void);
void shub_pedo_exit(void);
void shub_rot_exit(void);
void shub_mag_unc_exit(void);
void shub_rot_mag_exit(void);
void shub_gyrounc_exit(void);
void shub_grav_exit(void);
void shub_rot_gyro_exit(void);
#ifdef CONFIG_BARO_SENSOR
void shub_baro_exit(void);
#endif
void shub_linearacc_exit(void);
void shub_orien_exit(void);
void shub_mag_exit(void);
void shub_gyro_exit(void);
void shub_acc_exit(void);
void shub_exif_exit(void);
void shub_mcu_exit(void);
/* SHMDS_HUB_3701_01 add E */

void shub_suspend_acc(void);
void shub_suspend_mag(void);
void shub_suspend_mag_uncal(void);
void shub_suspend_gyro(void);
void shub_suspend_gyro_uncal(void);
void shub_suspend_orien(void);
void shub_suspend_grav(void);
void shub_suspend_linear(void);
void shub_suspend_rot(void);
void shub_suspend_rot_gyro(void);
void shub_suspend_rot_mag(void);
void shub_suspend_exif(void);   // SHMDS_HUB_0203_01 add
void shub_suspend_baro(void);   /* SHMDS_HUB_0120_01 add */

void shub_resume_acc(void);
void shub_resume_mag(void);
void shub_resume_mag_uncal(void);
void shub_resume_gyro(void);
void shub_resume_gyro_uncal(void);
void shub_resume_orien(void);
void shub_resume_grav(void);
void shub_resume_linear(void);
void shub_resume_rot(void);
void shub_resume_rot_gyro(void);
void shub_resume_rot_mag(void);
void shub_resume_exif(void);    // SHMDS_HUB_0203_01 add
void shub_resume_baro(void);    /* SHMDS_HUB_0120_01 add */

/* SHMDS_HUB_0311_01 add S */
int32_t shub_get_sensor_activate_info(int kind);
int32_t shub_get_sensor_first_measure_info(int kind);
int shub_get_sensor_same_delay_flg(int kind);
/* SHMDS_HUB_0311_01 add E */

// SHMDS_HUB_0701_05 add S
void shub_sensor_rep_input_exif(struct seq_file *s);
void shub_sensor_rep_input_acc(struct seq_file *s);
void shub_sensor_rep_input_gyro(struct seq_file *s);
void shub_sensor_rep_input_gyro_uncal(struct seq_file *s);
void shub_sensor_rep_input_mag(struct seq_file *s);
void shub_sensor_rep_input_mag_uncal(struct seq_file *s);
void shub_sensor_rep_input_orien(struct seq_file *s);
void shub_sensor_rep_input_grav(struct seq_file *s);
void shub_sensor_rep_input_linear(struct seq_file *s);
void shub_sensor_rep_input_rot(struct seq_file *s);
void shub_sensor_rep_input_game_rot_gyro(struct seq_file *s);
void shub_sensor_rep_input_rot_mag(struct seq_file *s);
void shub_sensor_rep_input_pedo(struct seq_file *s);
void shub_sensor_rep_input_pedodect(struct seq_file *s);
void shub_sensor_rep_input_baro(struct seq_file *s);    /* SHMDS_HUB_0120_01 add */

void shub_sensor_rep_input_mcu(struct seq_file *s);
void shub_sensor_rep_spi(struct seq_file *s);
// SHMDS_HUB_0701_05 add E

int32_t shub_get_acc_delay_ms(void);		/* SHMDS_HUB_0313_01 add */

void shub_set_error_code(uint32_t mode);    /* SHMDS_HUB_0322_02 add */
uint32_t shub_get_error_code(void);         /* SHMDS_HUB_0322_01 add */

void shub_pedo_timer_stop(void);            /* SHMDS_HUB_0304_04 add */

void shub_input_report_devoriect(int32_t *data);    /* SHMDS_HUB_0132_01 add */

#endif /* _ML630Q790_H_ */
