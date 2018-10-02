#ifndef __FIH_HWID_2_H
#define __FIH_HWID_2_H

/* special for modem to link this file
 * to get the project, phase, and band enum */

enum {
        FIH_PRJ_QCT = 0,
        FIH_PRJ_MS3 = 1,
        FIH_PRJ_MA3 = 2,
        FIH_PRJ_MF3 = 3,
        FIH_PRJ_D1C = 6,
        FIH_PRJ_D1 =7,
	FIH_PRJ_VZ1 = 8,
        FIH_PRJ_PLE = 10,
        FIH_PRJ_OC6 = 11,
        FIH_PRJ_MAX
};

enum {
	FIH_REV_DEFAULT = 0,
	FIH_REV_EVB = 1,
	FIH_REV_EVB2,
	FIH_REV_EVB3,
	FIH_REV_EVB4,
	FIH_REV_EVB5,
	FIH_REV_PRE_EVT = 10,
	FIH_REV_PRE_EVT2,
	FIH_REV_PRE_EVT3,
	FIH_REV_PRE_EVT4,
	FIH_REV_PRE_EVT5,
	FIH_REV_PRE_EVT6,
	FIH_REV_PRE_EVT7,
	FIH_REV_PRE_EVT8,
	FIH_REV_PRE_EVT9,
	FIH_REV_EVT = 20,
	FIH_REV_EVT2,
	FIH_REV_EVT3,
	FIH_REV_EVT4,
	FIH_REV_EVT5,
	FIH_REV_EVT6,
	FIH_REV_EVT7,
	FIH_REV_EVT8,
	FIH_REV_EVT9,
	FIH_REV_DVT = 30,
	FIH_REV_DVT2,
	FIH_REV_DVT3,
	FIH_REV_DVT4,
	FIH_REV_DVT5,
	FIH_REV_DVT6,
	FIH_REV_DVT7,
	FIH_REV_DVT8,
	FIH_REV_DVT9,
	FIH_REV_PVT = 40,
	FIH_REV_PVT2,
	FIH_REV_PVT3,
	FIH_REV_PVT4,
	FIH_REV_PVT5,
	FIH_REV_PVT6,
	FIH_REV_PVT7,
	FIH_REV_PVT8,
	FIH_REV_PVT9,
	FIH_REV_MP = 50,
	FIH_REV_MP2,
	FIH_REV_MP3,    
	FIH_REV_MP4,
	FIH_REV_MP5,
	FIH_REV_MAX = 255,
};

enum {
	FIH_BAND_DEFAULT = 0,


	
	 //Gary-MS3
        FIH_RF_G_900_1800_1900_W_1_5_8_L_1_3_5_8_41 = 1, /* JP */
        FIH_RF_G_850_900_1800_1900_W_1_5_8_L_1_3_5_7_8_40 = 2, /* SE-ASIA */
				FIH_BAND_G_850_900_1800_1900_W_1_8_L_1_3_7_8_28_38 = 3,/* Taiwan CA */
				//D1-8937
        FIH_RF_G_900_1800_W_1_8_L_3_5_20_40 = 10, /* India */
        FIH_RF_G_900_1800_W_1_5_8_L_1_3_5_8_40 = 11, /* Indoneisa */
				//D1-8940
				FIH_RF_G_850_900_1800_1900_W_1_5_8_19_L_1_3_7_8_20_28_38_40 = 12, /* Asia */
				FIH_RF_G_850_900_1800_1900_W_1_8_L_1_3_7_8_20_38_40 = 13, /* EU */
				FIH_RF_G_850_900_1800_1900_W_1_2_4_5_8_L_2_3_4_7_12_17_28_38 =14, /* LATAM */
				//D1C-8937
				FIH_RF_G_850_900_1800_1900_W_1_2_5_8_C_0_T_34_39_L_1_3_5_38_39_40_41 = 21, /*China*/
				FIH_RF_G_850_900_1800_1900_W_1_2_5_8_L_1_3_5_7_8_28_38_40_41 = 22, /* Asia */
				//D1C-8940

        FIH_RF_G_850_900_1800_W_1_5_8_L_1_3_5_7_8_28_40 = 30,/* VZ1-Sharp oversea Taiwan/SEA */
        FIH_RF_G_850_900_1800_W_1_8_L_1_3_5_8_40 = 31,/* VZ1-Indoneisa */

        FIH_RF_8937_G_850_900_1800_1900_W_1_2_5_8_L_1_3_5_7_8_20_28_38_40 = 91, /*Plate APAC */
        FIH_RF_8937_G_850_900_1800_1900_W_1_2_4_5_8_L_2_3_4_7_12_17_28_38 = 92, /*Plate LATAM */
        FIH_RF_G_900_1800_1900_W_1_2_4_8_L_1_2_3_4_7_8_20_41=40, //G1 end- Greens

        /* NO BAND */
        FIH_RF_NONE = 240, /* RF_BAND-ID = 0 */
        FIH_BAND_NONE = 240,
        /* WIFI_ONLY */
        FIH_BAND_WIFI_ONLY = 240,

        FIH_RF_MAX = 255,
	FIH_BAND_MAX = 255,
};

#endif /* __FIH_HWID_2_H */
