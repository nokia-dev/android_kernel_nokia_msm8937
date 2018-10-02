/* include/linux/input/shtps_dev_focaltech.h
 *
 * Copyright (C) 2016 SHARP CORPORATION
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
#ifndef __SHTPS_DEV_FOCALTECH_H__
#define __SHTPS_DEV_FOCALTECH_H__

/* -----------------------------------------------------------------------------------
 */
#define SH_TOUCH_DEVNAME		"shtps_fts"
#define SH_TOUCH_IF_DEVNAME 	"shtpsif"
#define SH_TOUCH_IF_DEVPATH 	"/dev/shtpsif"

#if defined( CONFIG_TOUCHSCREEN_SHTPS_FTS_FT8607 )
	#define SHTPS_TM_TXNUM_MAX		27
	#define SHTPS_TM_RXNUM_MAX		16
#else
	#define SHTPS_TM_TXNUM_MAX		27
	#define SHTPS_TM_RXNUM_MAX		16
#endif

#define SHTPS_FINGER_MAX		5

#define TPS_IOC_MAGIC					0xE0

#define TPSDEV_ENABLE					_IO  ( TPS_IOC_MAGIC,  1)
#define TPSDEV_DISABLE					_IO  ( TPS_IOC_MAGIC,  2)
#define TPSDEV_RESET					_IO  ( TPS_IOC_MAGIC,  3)
#define TPSDEV_SOFT_RESET				_IO  ( TPS_IOC_MAGIC,  4)
#define TPSDEV_GET_FW_VERSION			_IOR ( TPS_IOC_MAGIC,  5, unsigned short)
#define TPSDEV_ENTER_BOOTLOADER			_IOR ( TPS_IOC_MAGIC,  6, struct shtps_bootloader_info)
#define TPSDEV_LOCKDOWN_BOOTLOADER		_IOW ( TPS_IOC_MAGIC,  7, unsigned char*)
#define TPSDEV_ERASE_FLASE				_IO  ( TPS_IOC_MAGIC,  8)
#define TPSDEV_WRITE_IMAGE				_IOW ( TPS_IOC_MAGIC,  9, unsigned char*)
#define TPSDEV_WRITE_CONFIG				_IOW ( TPS_IOC_MAGIC, 10, unsigned char*)
#define TPSDEV_GET_TOUCHINFO			_IOR ( TPS_IOC_MAGIC, 11, struct shtps_touch_info)
#define TPSDEV_GET_TOUCHINFO_UNTRANS	_IOR ( TPS_IOC_MAGIC, 12, struct shtps_touch_info)
#define TPSDEV_SET_TOUCHMONITOR_MODE	_IOW ( TPS_IOC_MAGIC, 13, unsigned char)
#define TPSDEV_READ_REG					_IOWR( TPS_IOC_MAGIC, 14, struct shtps_ioctl_param)
#define TPSDEV_READ_ALL_REG				_IOR ( TPS_IOC_MAGIC, 15, struct shtps_ioctl_param)
#define TPSDEV_WRITE_REG				_IOW ( TPS_IOC_MAGIC, 16, struct shtps_ioctl_param)
#define TPSDEV_START_TM					_IOW ( TPS_IOC_MAGIC, 17, int)
#define TPSDEV_STOP_TM					_IO  ( TPS_IOC_MAGIC, 18)
#define TPSDEV_GET_BASELINE				_IOR ( TPS_IOC_MAGIC, 19, unsigned short*)
#define TPSDEV_GET_FRAMELINE			_IOR ( TPS_IOC_MAGIC, 20, unsigned char*)
#define TPSDEV_START_FACETOUCHMODE		_IO  ( TPS_IOC_MAGIC, 21)
#define TPSDEV_STOP_FACETOUCHMODE		_IO  ( TPS_IOC_MAGIC, 22)
#define TPSDEV_POLL_FACETOUCHOFF		_IO  ( TPS_IOC_MAGIC, 23)
#define TPSDEV_GET_FWSTATUS				_IOR ( TPS_IOC_MAGIC, 24, unsigned char)
#define TPSDEV_GET_FWDATE				_IOR ( TPS_IOC_MAGIC, 25, unsigned short)
#define TPSDEV_CALIBRATION_PARAM		_IOW ( TPS_IOC_MAGIC, 26, struct shtps_ioctl_param)
#define TPSDEV_DEBUG_REQEVENT			_IOW ( TPS_IOC_MAGIC, 27, int)
#define TPSDEV_SET_DRAGSTEP				_IOW ( TPS_IOC_MAGIC, 28, int)
#define TPSDEV_SET_POLLINGINTERVAL		_IOW ( TPS_IOC_MAGIC, 29, int)
#define TPSDEV_SET_FINGERFIXTIME		_IOW ( TPS_IOC_MAGIC, 30, int)
#define TPSDEV_REZERO					_IO  ( TPS_IOC_MAGIC, 31)
#define TPSDEV_ACK_FACETOUCHOFF			_IO  ( TPS_IOC_MAGIC, 32)
#define TPSDEV_START_TM_F05				_IOW ( TPS_IOC_MAGIC, 33, int)
#define TPSDEV_SET_DRAGSTEP_X			_IOW ( TPS_IOC_MAGIC, 34, int)
#define TPSDEV_SET_DRAGSTEP_Y			_IOW ( TPS_IOC_MAGIC, 35, int)
#define TPSDEV_LOGOUTPUT_ENABLE			_IOW ( TPS_IOC_MAGIC, 36, int)
#define TPSDEV_GET_TOUCHKEYINFO			_IOR ( TPS_IOC_MAGIC, 37, struct shtps_touch_key_info)
#define TPSDEV_GET_FW_VERSION_BUILTIN	_IOR ( TPS_IOC_MAGIC, 38, unsigned short)
#define TPSDEV_GET_SMEM_BASELINE		_IOR ( TPS_IOC_MAGIC, 39, unsigned short*)
#define TPSDEV_SET_LOWPOWER_MODE		_IOW ( TPS_IOC_MAGIC, 40, int)
#define TPSDEV_SET_CONT_LOWPOWER_MODE	_IOW ( TPS_IOC_MAGIC, 41, int)
#define TPSDEV_SET_INVALID_AREA			_IOW ( TPS_IOC_MAGIC, 42, int)
#define TPSDEV_SET_CHARGER_ARMOR			_IOW(TPS_IOC_MAGIC, 43, int)
#define TPSDEV_SET_WIRELESS_CHARGER_ARMOR	_IOW(TPS_IOC_MAGIC, 44, int)
#define TPSDEV_LPWG_ENABLE	            _IOW(TPS_IOC_MAGIC, 45, int)
#define TPSDEV_SET_VEILVIEW_STATE		_IOW ( TPS_IOC_MAGIC, 46, int)
#define TPSDEV_READ_REG_BLOCK			_IOWR( TPS_IOC_MAGIC, 47, struct shtps_ioctl_param)
#define TPSDEV_WRITE_REG_BLOCK			_IOW ( TPS_IOC_MAGIC, 48, struct shtps_ioctl_param)
#define TPSDEV_GET_VEILVIEW_PATTERN		_IOR ( TPS_IOC_MAGIC, 49, int)
#define TPSDEV_READ_REG_PACKET			_IOWR( TPS_IOC_MAGIC, 50, struct shtps_ioctl_param)
#define TPSDEV_WRITE_REG_PACKET			_IOW ( TPS_IOC_MAGIC, 51, struct shtps_ioctl_param)
#define TPSDEV_HOVER_ENABLE				_IOW ( TPS_IOC_MAGIC, 52, int)
#define TPSDEV_GET_BASELINE_RAW			_IOR ( TPS_IOC_MAGIC, 53, unsigned short*)
#define TPSDEV_CALIBRATION_PEN_PARAM	_IOW ( TPS_IOC_MAGIC, 54, struct shtps_ioctl_param)
#define TPSDEV_SET_NARROW_FRAME_MODE	_IOW ( TPS_IOC_MAGIC, 55, int)
#define TPSDEV_SET_LCD_LOWPOWER_MODE	_IOW ( TPS_IOC_MAGIC, 56, int)
#define TPSDEV_BASELINE_OFFSET_DISABLE	_IOW ( TPS_IOC_MAGIC, 57, int)
#define TPSDEV_CHECK_CRC_ERROR	        _IOW ( TPS_IOC_MAGIC, 58, int)
#define TPSDEV_SET_PEN_ENABLE			_IOW ( TPS_IOC_MAGIC, 59, int)
#define TPSDEV_GET_PEN_ENABLE			_IOR ( TPS_IOC_MAGIC, 60, int)
#define TPSDEV_SET_LOW_REPORTRATE_MODE	_IOR ( TPS_IOC_MAGIC, 61, int)
#define TPSDEV_GET_SERIAL_NUMBER		_IOR ( TPS_IOC_MAGIC, 62, unsigned char*)
#define TPSDEV_GET_SERIAL_NUMBER_SIZE	_IOR ( TPS_IOC_MAGIC, 63, int)
#define TPSDEV_LOCKDOWN_BOOTLOADER_BUILTIN	_IOW ( TPS_IOC_MAGIC, 64, int)
#define TPSDEV_WRITE_IMAGE_BUILTIN			_IOW ( TPS_IOC_MAGIC, 65, int)
#define TPSDEV_WRITE_CONFIG_BUILTIN				_IOW ( TPS_IOC_MAGIC, 66, int)
#define TPSDEV_GET_HYBRID_ADC			_IOR ( TPS_IOC_MAGIC, 67, unsigned int*)
#define TPSDEV_GET_ADC_RANGE			_IOR ( TPS_IOC_MAGIC, 68, unsigned short*)
#define TPSDEV_LPWG_DOUBLE_TAP_ENABLE	_IOW ( TPS_IOC_MAGIC, 69, int)
#define TPSDEV_SET_GLOVE_MODE			_IOW ( TPS_IOC_MAGIC, 70, int)
#define TPSDEV_SET_HIGH_REPORT_MODE		_IOW ( TPS_IOC_MAGIC, 71, int)
#define TPSDEV_GET_CBDATA				_IOR ( TPS_IOC_MAGIC, 72, unsigned short*)
#define TPSDEV_FW_UPGRADE				_IOW ( TPS_IOC_MAGIC, 73, struct shtps_ioctl_param)
#define TPSDEV_BOOT_FW_UPDATE_REQ		_IO  ( TPS_IOC_MAGIC, 74)
#define TPSDEV_GET_DOUBLE_TAP_ENABLE	_IOR ( TPS_IOC_MAGIC, 75, int)
#define TPSDEV_GET_CALIBRATION_PARAM	_IOR ( TPS_IOC_MAGIC, 76, struct shtps_ioctl_param)


#ifdef CONFIG_COMPAT
#define COMPAT_TPSDEV_ENTER_BOOTLOADER			_IOR ( TPS_IOC_MAGIC,  6, struct shtps_compat_bootloader_info)
#define COMPAT_TPSDEV_LOCKDOWN_BOOTLOADER		_IOW ( TPS_IOC_MAGIC,  7, shtps_ioctl_compat_pointer_param)
#define COMPAT_TPSDEV_WRITE_IMAGE				_IOW ( TPS_IOC_MAGIC,  9, shtps_ioctl_compat_pointer_param)
#define COMPAT_TPSDEV_WRITE_CONFIG				_IOW ( TPS_IOC_MAGIC, 10, shtps_ioctl_compat_pointer_param)
#define COMPAT_TPSDEV_GET_TOUCHINFO				_IOR ( TPS_IOC_MAGIC, 11, struct shtps_compat_touch_info)
#define COMPAT_TPSDEV_GET_TOUCHINFO_UNTRANS		_IOR ( TPS_IOC_MAGIC, 12, struct shtps_compat_touch_info)
#define COMPAT_TPSDEV_READ_REG					_IOWR( TPS_IOC_MAGIC, 14, struct shtps_compat_ioctl_param)
#define COMPAT_TPSDEV_READ_ALL_REG				_IOR ( TPS_IOC_MAGIC, 15, struct shtps_compat_ioctl_param)
#define COMPAT_TPSDEV_WRITE_REG					_IOW ( TPS_IOC_MAGIC, 16, struct shtps_compat_ioctl_param)
#define COMPAT_TPSDEV_GET_BASELINE				_IOR ( TPS_IOC_MAGIC, 19, shtps_ioctl_compat_pointer_param)
#define COMPAT_TPSDEV_GET_FRAMELINE				_IOR ( TPS_IOC_MAGIC, 20, shtps_ioctl_compat_pointer_param)
#define COMPAT_TPSDEV_CALIBRATION_PARAM			_IOW ( TPS_IOC_MAGIC, 26, struct shtps_compat_ioctl_param)
#define COMPAT_TPSDEV_GET_TOUCHKEYINFO			_IOR ( TPS_IOC_MAGIC, 37, struct shtps_compat_touch_key_info)
#define COMPAT_TPSDEV_GET_SMEM_BASELINE			_IOR ( TPS_IOC_MAGIC, 39, shtps_ioctl_compat_pointer_param)
#define COMPAT_TPSDEV_READ_REG_BLOCK			_IOWR( TPS_IOC_MAGIC, 47, struct shtps_compat_ioctl_param)
#define COMPAT_TPSDEV_WRITE_REG_BLOCK			_IOW ( TPS_IOC_MAGIC, 48, struct shtps_compat_ioctl_param)
#define COMPAT_TPSDEV_READ_REG_PACKET			_IOWR( TPS_IOC_MAGIC, 50, struct shtps_compat_ioctl_param)
#define COMPAT_TPSDEV_WRITE_REG_PACKET			_IOW ( TPS_IOC_MAGIC, 51, struct shtps_compat_ioctl_param)
#define COMPAT_TPSDEV_GET_BASELINE_RAW			_IOR ( TPS_IOC_MAGIC, 53, shtps_ioctl_compat_pointer_param)
#define COMPAT_TPSDEV_CALIBRATION_PEN_PARAM		_IOW ( TPS_IOC_MAGIC, 54, struct shtps_compat_ioctl_param)
#define COMPAT_TPSDEV_GET_SERIAL_NUMBER			_IOR ( TPS_IOC_MAGIC, 62, shtps_ioctl_compat_pointer_param)
#define COMPAT_TPSDEV_GET_HYBRID_ADC			_IOR ( TPS_IOC_MAGIC, 67, shtps_ioctl_compat_pointer_param)
#define COMPAT_TPSDEV_GET_ADC_RANGE				_IOR ( TPS_IOC_MAGIC, 68, shtps_ioctl_compat_pointer_param)
#define COMPAT_TPSDEV_GET_CBDATA				_IOR ( TPS_IOC_MAGIC, 72, shtps_ioctl_compat_pointer_param)
#define COMPAT_TPSDEV_FW_UPGRADE				_IOW ( TPS_IOC_MAGIC, 73, struct shtps_compat_ioctl_param)
#define COMPAT_TPSDEV_GET_CALIBRATION_PARAM		_IOR ( TPS_IOC_MAGIC, 76, struct shtps_compat_ioctl_param)

typedef __u32 shtps_ioctl_compat_pointer_param;
struct shtps_compat_ioctl_param {
	int				size;
	__u32			data;
};

struct shtps_compat_bootloader_info {
	__u32	block_size;
	__u32	program_block_num;
	__u32	config_block_num;
};

struct compat_fingers{
	unsigned short	x;
	unsigned short	y;
	unsigned char	state;
	unsigned char	wx;
	unsigned char	wy;
	unsigned char	z;
};

struct shtps_compat_touch_info {
	struct compat_fingers	fingers[SHTPS_FINGER_MAX];

	unsigned char		gs1;
	unsigned char		gs2;
	unsigned char		flick_x;
	unsigned char		flick_y;
	unsigned char		flick_time;
	unsigned char		finger_num;
};

struct shtps_compat_touch_key_info {
	unsigned char		menu_key_state;
	unsigned char		home_key_state;
	unsigned char		back_key_state;
	unsigned char		down_key_state;
	unsigned char		up_key_state;
};
#endif /* CONFIG_COMPAT */


#define TPSDEV_FACETOUCH_NOCHG		0x00
#define TPSDEV_FACETOUCH_DETECT		0x01
#define TPSDEV_FACETOUCH_OFF_DETECT	0x02

#define TPSDEV_TOUCHINFO_MODE_LCDSIZE	0
#define TPSDEV_TOUCHINFO_MODE_DEVSIZE	1

struct shtps_ioctl_param {
	int				size;
	unsigned char*	data;
};

struct shtps_bootloader_info {
	unsigned int	block_size;
	unsigned int	program_block_num;
	unsigned int	config_block_num;
};

struct fingers{
	unsigned short	x;
	unsigned short	y;
	unsigned char	state;
	unsigned char	wx;
	unsigned char	wy;
	unsigned char	z;
};

struct shtps_touch_info {
	struct fingers		fingers[SHTPS_FINGER_MAX];

	unsigned char		gs1;
	unsigned char		gs2;
	unsigned char		flick_x;
	unsigned char		flick_y;
	unsigned char		flick_time;
	unsigned char		finger_num;
};

struct shtps_touch_key_info {
	unsigned char		menu_key_state;
	unsigned char		home_key_state;
	unsigned char		back_key_state;
	unsigned char		down_key_state;
	unsigned char		up_key_state;
};

enum shtps_proc_mode {
	SHTPS_IN_IDLE,
	SHTPS_IN_NORMAL,
	SHTPS_IN_BOOTLOADER,
};

enum shtps_diag_mode {
	SHTPS_DIAGMODE_NONE,
	SHTPS_DIAGMODE_TOUCHINFO,
	SHTPS_DIAGMODE_TM,
};

enum shtps_diag_tm_mode {
	SHTPS_TMMODE_NONE,
	SHTPS_TMMODE_FRAMELINE,
	SHTPS_TMMODE_BASELINE,
	SHTPS_TMMODE_BASELINE_RAW,
	SHTPS_TMMODE_CBDATA,
};

enum{
	SHTPS_VEILVIEW_PATTERN_RGB_CHIDORI_1H = 0,
	SHTPS_VEILVIEW_PATTERN_RGB_CHIDORI_2H,
	SHTPS_VEILVIEW_PATTERN_MONOCHROME_1H,
	SHTPS_VEILVIEW_PATTERN_MONOCHROME_2H,
};

#ifdef CONFIG_OF
	; // nothing
#else /* CONFIG_OF */
struct shtps_platform_data {
	int (*setup)(struct device *);
	void (*teardown)(struct device *);
	int gpio_rst;
};
#endif /* CONFIG_OF */

struct shtps_touch_state {
	u8				numOfFingers;
	u8				fingerStatus[SHTPS_FINGER_MAX];
	u8				dragStep[SHTPS_FINGER_MAX][2];
	u8				rezeroRequest;
	unsigned long	drag_timeout[SHTPS_FINGER_MAX][2];
	u16				dragStepReturnTime[SHTPS_FINGER_MAX][2];
};

/* -----------------------------------------------------------------------------------
 */
extern void msm_tps_setsleep(int on);
extern int msm_tps_set_veilview_state_on(void);
extern int msm_tps_set_veilview_state_off(void);
extern int msm_tps_get_veilview_pattern(void);
extern void msm_tps_set_grip_state(int on);
extern void msm_tps_set_cover_state(int on);

#endif /* __SHTPS_DEV_FOCALTECH_H__ */
