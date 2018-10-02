/* irda_common.h (Infrared driver module)
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
/****************************************************************************/
#ifndef _IRDA_COMMON_H
#define _IRDA_COMMON_H

#include "ir_types.h"

/****************************************************************************/
typedef enum {
	IRDA_BAUD_9600 = 1,
	IRDA_BAUD_19200,
	IRDA_BAUD_38400,
	IRDA_BAUD_57600,
	IRDA_BAUD_115200,
	IRDA_BAUD_4000000
} irda_boud_enum;

typedef struct {
	irda_boud_enum baud_rate;
	uint16 connection_address;
	uint16 add_bof;
	uint16 mpi;
	uint16 mtt;
} irda_qos_info;

#define IRDA_DRV_SET_ADD_BOF_48	(48)
#define IRDA_DRV_SET_ADD_BOF_32	(32)
#define IRDA_DRV_SET_ADD_BOF_24	(24)
#define IRDA_DRV_SET_ADD_BOF_20	(20)
#define IRDA_DRV_SET_ADD_BOF_16	(16)
#define IRDA_DRV_SET_ADD_BOF_14	(14)
#define IRDA_DRV_SET_ADD_BOF_12	(12)
#define IRDA_DRV_SET_ADD_BOF_10	(10)
#define IRDA_DRV_SET_ADD_BOF_8	(8)
#define IRDA_DRV_SET_ADD_BOF_6	(6)
#define IRDA_DRV_SET_ADD_BOF_5	(5)
#define IRDA_DRV_SET_ADD_BOF_4	(4)
#define IRDA_DRV_SET_ADD_BOF_3	(3)
#define IRDA_DRV_SET_ADD_BOF_2	(2)
#define IRDA_DRV_SET_ADD_BOF_1	(1)
#define IRDA_DRV_SET_ADD_BOF_0	(0)

#define IRDA_DRV_SET_MPI_MAX	(1600)
#define IRDA_DRV_SET_MPI_MIN	(10)

#define IRDA_DRV_SET_MTT_MAX	(1000)
#define IRDA_DRV_SET_MTT_MIN	(1)
#define IRDA_DRV_SET_MTT_ZERO	(0)

#define IRDA_KDRV_DEF_CA	(uint16)0xFF
#define IRDA_KDRV_DEF_ABOF	(10)
#define IRDA_KDRV_DEF_MTT	(500)

typedef struct {
	uint32 mpi_min;
	uint32 mpi_max;
} irda_capa_mpi_info;

typedef struct {
	uint32 max_size;
	uint32 hw_buff_cnt;
	uint32 sw_tx_buff_cnt;
	uint32 sw_rx_buff_cnt;
} irda_capa_buff_info;

typedef struct {
	uint32			baud_rate;
	uint32			add_bof;
	uint32			min_tat;
	uint32			window_size;
	irda_capa_mpi_info	mpi;
	irda_capa_buff_info	buff;
} irda_capa_info;

typedef struct {
	uint8			baud_rate;
	uint8			baud_rate_ext;
	uint8			add_bof;
	uint8			min_tat;
	uint8			window_size;
	uint16			mpi;
	uint8			data_size;
} irda_request_capa_info;

typedef struct {
	irda_qos_info	qos_init;
	irda_capa_info	capability;
	irda_request_capa_info	request_capability;
} irda_kdrv_capa_notify;

#define	IRDA_DRV_CAPA_WINDOW_SIZE_1	0x00000001
#define	IRDA_DRV_CAPA_WINDOW_SIZE_2	0x00000002
#define	IRDA_DRV_CAPA_WINDOW_SIZE_3	0x00000004
#define	IRDA_DRV_CAPA_WINDOW_SIZE_4	0x00000008
#define	IRDA_DRV_CAPA_WINDOW_SIZE_5	0x00000010
#define	IRDA_DRV_CAPA_WINDOW_SIZE_6	0x00000020
#define	IRDA_DRV_CAPA_WINDOW_SIZE_7	0x00000040

#define	IRDA_DRV_CAPA_BAUD_2400		0x00000001
#define	IRDA_DRV_CAPA_BAUD_9600		0x00000002
#define	IRDA_DRV_CAPA_BAUD_19200	0x00000004
#define	IRDA_DRV_CAPA_BAUD_38400	0x00000008
#define	IRDA_DRV_CAPA_BAUD_57600	0x00000010
#define	IRDA_DRV_CAPA_BAUD_115200	0x00000020
#define	IRDA_DRV_CAPA_BAUD_576000	0x00000040
#define	IRDA_DRV_CAPA_BAUD_1152000	0x00000080
#define	IRDA_DRV_CAPA_BAUD_4000000	0x00000100

#define	IRDA_DRV_CAPA_ADD_BOF_48	0x00000001
#define	IRDA_DRV_CAPA_ADD_BOF_32	0x00000002
#define	IRDA_DRV_CAPA_ADD_BOF_24	0x00000004
#define	IRDA_DRV_CAPA_ADD_BOF_20	0x00000008
#define	IRDA_DRV_CAPA_ADD_BOF_16	0x00000010
#define	IRDA_DRV_CAPA_ADD_BOF_14	0x00000020
#define	IRDA_DRV_CAPA_ADD_BOF_12	0x00000040
#define	IRDA_DRV_CAPA_ADD_BOF_10	0x00000080
#define	IRDA_DRV_CAPA_ADD_BOF_8		0x00000100
#define	IRDA_DRV_CAPA_ADD_BOF_6		0x00000200
#define	IRDA_DRV_CAPA_ADD_BOF_5		0x00000400
#define	IRDA_DRV_CAPA_ADD_BOF_4		0x00000800
#define	IRDA_DRV_CAPA_ADD_BOF_3		0x00001000
#define	IRDA_DRV_CAPA_ADD_BOF_2		0x00002000
#define	IRDA_DRV_CAPA_ADD_BOF_1		0x00004000
#define	IRDA_DRV_CAPA_ADD_BOF_0		0x00008000

#define	IRDA_DRV_CAPA_MINTAT_10000	0x00000001
#define	IRDA_DRV_CAPA_MINTAT_5000	0x00000002
#define	IRDA_DRV_CAPA_MINTAT_1000	0x00000004
#define	IRDA_DRV_CAPA_MINTAT_500	0x00000008
#define	IRDA_DRV_CAPA_MINTAT_100	0x00000010
#define	IRDA_DRV_CAPA_MINTAT_50		0x00000020
#define	IRDA_DRV_CAPA_MINTAT_10		0x00000040

#define	IRDA_DRV_CAPA_DATA_SIZE_64	0x01
#define	IRDA_DRV_CAPA_DATA_SIZE_128	0x02
#define	IRDA_DRV_CAPA_DATA_SIZE_256	0x04
#define	IRDA_DRV_CAPA_DATA_SIZE_512	0x08
#define	IRDA_DRV_CAPA_DATA_SIZE_1024	0x10
#define	IRDA_DRV_CAPA_DATA_SIZE_2048	0x20

typedef struct {
	signed long int	irda_err;
	unsigned long	rx_packets;
	unsigned long	tx_packets;
	unsigned long	rx_bytes;
	unsigned long	tx_bytes;
	unsigned long	rx_errors;
	unsigned long	tx_errors;
	unsigned long	rx_dropped;
	unsigned long	tx_dropped;
	unsigned long	rx_length_errors;
	unsigned long	rx_crc_errors;
	unsigned long	rx_missed_errors;
	unsigned long	tx_carrier_errors;
	unsigned long	reserve_1;
	unsigned long	reserve_2;
	unsigned long	rx_mask_packets;
} shirda_stats;

#endif
