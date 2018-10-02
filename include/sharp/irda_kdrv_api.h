/* irda_kdrv_api.h (Infrared driver module)
 *
 * Copyright (C) 2009-2010 SHARP CORPORATION All rights reserved.
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
#ifndef	_IRDA_KDRV_API_H
#define	_IRDA_KDRV_API_H

#include <linux/ioctl.h>
#include <linux/tty.h>

#include "irda_common.h"

#undef	SHIRDA_TEST_DRIVER
#define	SHIRDA_OVERRIDE_IRDALDISC

#define	SHIRDA_DEVFILE_NAME		"msm_shirda"
#define	SHIRDA_DEVFILE			"/dev/"SHIRDA_DEVFILE_NAME


#define SHIRDA_LDISC_DRIVER_NAME	"shirda"

#define	SHIRB_LDISC_NAME		"shirb_ldisc"

#ifdef	SHIRDA_TEST_DRIVER
#define	SHIRDA_TEST_DEVFILE_NAME	"msm_shirtest"
#define	SHIRDA_TEST_DEVFILE		"/dev/"SHIRDA_TEST_DEVFILE_NAME
#endif

#ifdef	__KERNEL__
#ifdef	SHIRDA_OVERRIDE_IRDALDISC
#define	N_SHIRDA	N_IRDA
#else
#ifndef	N_SHIRDA
#ERROR	エラー N_SHIRDAが定義されていません
#endif
#endif

#else
#include	<utils/Log.h>
#include	<ctype.h>
#include	<dirent.h>
#include	<string.h>
#include	<stdio.h>

#define	SHIRDA_PARA_PATH	"/sys/module/shirda_msm_kdrv/parameters/"
#define	SHIRDA_NODE_PATH	SHIRDA_PARA_PATH"node"
#define	SHIRDA_TTYDEV_PATH	"/sys/bus/platform/devices/%s/tty"

inline static char *get_tty_devname() {
	int		fd;
	int		i, n;
	static char	tty_device_name[64];
	static char	devname_path[64];
	char		node[64];
	DIR		*dir;
	struct dirent	*ent;

	fd = open(SHIRDA_NODE_PATH, O_RDONLY);
	if (fd < 0) {
		ALOGE("NODE = %s open error (%d)", SHIRDA_NODE_PATH, errno);
		return NULL;
	}
	n = read(fd, node, 63);
	close(fd);

	for (i = 0; i < n; i++) {
		if (iscntrl((int)node[i]) != 0) {
			n = i;
			ALOGE("%dth charactor = 0x%02x", i, node[i]);
			break;
		}
	}
	if (n < 1) {
		ALOGE("Charactor deleate error");
		return NULL;
	}
	node[n] = (char)0x00;

	sprintf(devname_path, SHIRDA_TTYDEV_PATH, node);

	if ((dir = opendir(devname_path)) == NULL) {
		ALOGE("DIR %s open error (%d)", devname_path, errno);
		return NULL;
	}
	do {
		if ((ent = readdir(dir)) == NULL) {
			ALOGE("DIR read error (%d)", errno);
			closedir(dir);
			return NULL;
		}
#ifdef	SHIRDA_DEBUG
		ALOGE("read device name is %s", ent->d_name);
#endif
	} while (strstr(ent->d_name, "tty") == NULL);
	closedir(dir);

	sprintf(tty_device_name, "/dev/%s", ent->d_name);
#ifdef	SHIRDA_DEBUG
	ALOGE("tty device name is %s", tty_device_name);
#endif
	return tty_device_name;
}

#define	TTYHS_DEVFILE ((const char *)(get_tty_devname()))

#define	SHIRDA_PROC_TTY_LDISCS_PATH	"/proc/tty/ldiscs"

inline static int get_shirda_ldisc_id(const char *ldisc_name)
{
	FILE	*fd;
	int	num_param;
	int	id;
	char	sbuf[64];
	char	name[64];

	fd = fopen(SHIRDA_PROC_TTY_LDISCS_PATH, "r");
	if (fd == NULL) {
		ALOGE("tty ldisc info file %s open error (%d)",
					SHIRDA_PROC_TTY_LDISCS_PATH, errno);
		return -1;
	}

	while (fgets(sbuf, 64, fd) != NULL) {
#ifdef	SHIRDA_DEBUG
		ALOGE("fgets = %s", sbuf);
#endif
		num_param = sscanf(sbuf, "%s%d", name, &id);
#ifdef	SHIRDA_DEBUG
		ALOGE("sscan : name = %s, id = %d", name, id);
#endif
		if (num_param != 2) {
			ALOGE("ldisc info param error (%d)", num_param);
			fclose(fd);
			return -1;
		}

		if (strcmp(name, ldisc_name) == 0) {
#ifdef	SHIRDA_DEBUG
			ALOGE("found ldisc(%s) alias = %d", ldisc_name, id);
#endif
			fclose(fd);
			return id;
		}
	}

	ALOGE("fgets() return value is NULL (%d)", errno);
	fclose(fd);
	return -1;
}

#define	N_SHIRDA	(get_shirda_ldisc_id(SHIRDA_LDISC_DRIVER_NAME))
#define	N_SHIRB		(get_shirda_ldisc_id(SHIRB_LDISC_NAME))

#define	SHIRDA_KDRV_MODULE	"shirda_msm_kdrv"
#define	SHIRDA_LDISC_MODULE	"shirda_ldisc"
#define	SHIRB_LDISC_MODULE	"shirb_ldisc"

#define	SHIRDA_KDRV_VER_PATH	"/sys/module/"SHIRDA_KDRV_MODULE"/version"
#define	SHIRDA_LDISC_VER_PATH	"/sys/module/"SHIRDA_LDISC_MODULE"/version"
#define	SHIRB_LDISC_VER_PATH	"/sys/module/"SHIRB_LDISC_MODULE"/version"

#define	SHIRDA_VERSION_PATH	SHIRDA_PARA_PATH"ver"

inline static char *get_shirda_version(const char *ver_file)
{
	int	fd;
	static char	version[64];
	int	i, n;

	fd = open(ver_file, O_RDONLY);
	if (fd < 0) {
		ALOGE("%s open error", ver_file);
		version[0] = 0x00;
		return version;
	}
	n = read(fd, version, 63);
	close(fd);

	for (i = 0; i < n; i++) {
		if (iscntrl((int)version[i]) != 0) {
			n = i;
			ALOGE("%dth charactor = 0x%02x", i, version[i]);
			break;
		}
	}
	if (n < 1) {
		ALOGE("Charactor deleate error");
		n = 0;
	}
	version[n] = (char)0x00;

	return version;
}

#define	SHIRDA_VERSION		(get_shirda_version(SHIRDA_VERSION_PATH))
#define	SHIRDA_KDRV_VERSION	(get_shirda_version(SHIRDA_KDRV_VER_PATH))
#define	SHIRDA_LDISC_VERSION	(get_shirda_version(SHIRDA_LDISC_VER_PATH))
#define	SHIRB_LDISC_VERSION	(get_shirda_version(SHIRB_LDISC_VER_PATH))

#endif

#define	IRDA_DRV_IOCTL_MAGIC	'i'
#define	IRDA_DRV_IOCTL_SET_QOS		_IOW(IRDA_DRV_IOCTL_MAGIC, 1, \
							irda_qos_info)
#define	IRDA_DRV_IOCTL_GET_QOS		_IOR(IRDA_DRV_IOCTL_MAGIC, 2, \
							irda_qos_info)
#define	IRDA_DRV_IOCTL_READ_WAKEUP	_IO (IRDA_DRV_IOCTL_MAGIC, 3)
#define	IRDA_DRV_IOCTL_GET_ERR		_IOR(IRDA_DRV_IOCTL_MAGIC, 4, int)
#define	IRDA_DRV_IOCTL_LOOPBACK 	_IO (IRDA_DRV_IOCTL_MAGIC, 5)
#define	IRDA_DRV_IOCTL_GET_CAPABILITY 	_IOR(IRDA_DRV_IOCTL_MAGIC, 6, \
							irda_kdrv_capa_notify)
#define	IRDA_DRV_IOCTL_GET_MEDIABUSY	_IOR(IRDA_DRV_IOCTL_MAGIC, 7, int)
#define	IRDA_DRV_IOCTL_CLR_MEDIABUSY	_IO(IRDA_DRV_IOCTL_MAGIC,  8)
#define	IRDA_DRV_IOCTL_FTM_RECEIVER	_IO(IRDA_DRV_IOCTL_MAGIC,  9)

#define	IRDA_LDISC_NO_ERR		(1)
#define	IRDA_LDISC_READ_CANCELED	(2)
#define	IRDA_LDISC_LOGICAL_ERR		(3)
#define	IRDA_LDISC_PERMISSION_ERR	(4)
#define	IRDA_LDISC_TX_TIMEOUT		(5)
#define	IRDA_LDISC_RX_TIMEOUT		(6)
#define	IRDA_LDISC_CLOSED		(7)
#define	IRDA_LDISC_RX_BUFFER_OVERFLOW	(8)
#define	IRDA_LDISC_TX_SEND_ERR		(9)

#define	IRDA_LDISC_MEDIA_FREE		(0)
#define	IRDA_LDISC_MEDIA_BUSY		(1)

#endif
