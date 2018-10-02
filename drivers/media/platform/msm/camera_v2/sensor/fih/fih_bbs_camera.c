/****************************************************************************
* File: fih_bbs_camera.c                                                    *
* Description: write camera error message to bbs log                        *
*                                                                           *
****************************************************************************/

/****************************************************************************
*                               Include File                                *
****************************************************************************/
#include <linux/types.h>
#include <linux/stddef.h>
#include <linux/string.h>
#include <linux/io.h>
#include "fih_bbs_camera.h"

#include <linux/ctype.h>

/****************************************************************************
*                               Module I2C ADDR                             *
****************************************************************************/
struct fih_bbs_camera_module_list {
	u8 i2c_addr;
	u8 type;
	char module_name[32];
};

/*JGR
//IMX298
#define FIH_I2C_ADDR_IMX298_CAMERA 0x20
#define FIH_I2C_ADDR_IMX298_EEPROM 0xA2
#define FIH_I2C_ADDR_IMX298_ACTUATOR 0xE4
#define FIH_I2C_ADDR_IMX298_OIS 0x7C

//IMX258
#define FIH_I2C_ADDR_IMX258_CAMERA 0x34
#define FIH_I2C_ADDR_IMX258_EEPROM 0xA0
#define FIH_I2C_ADDR_IMX258_ACTUATOR 0x18
*/
/*D1C component start*/
//S5K3P3
#define FIH_I2C_ADDR_S5K3P3_CAMERA 0x20
#define FIH_I2C_ADDR_S5K3P3_EEPROM 0xA2
#define FIH_I2C_ADDR_S5K3P3_ACTUATOR 0x18
//S5K4H8
#define FIH_I2C_ADDR_S5K4H8_CAMERA 0x5A
#define FIH_I2C_ADDR_S5K4H8_EEPROM 0x5A //FIH_I2C_ADDR_S5K4H8_EEPROM is the same
/*D1C component end*/

/*Mx3 component start*/
//IMX258
#define FIH_I2C_ADDR_IMX258_CAMERA 0x34
#define FIH_I2C_ADDR_IMX258_EEPROM 0xA0
#define FIH_I2C_ADDR_IMX258_ACTUATOR 0x18
//IMX219
#define FIH_I2C_ADDR_IMX219_CAMERA 0x20
/*Mx3 component end*/

/*ND1 component start*/
//IMX258
#define FIH_I2C_ADDR_IMX258_ND1_CAMERA 0x34
#define FIH_I2C_ADDR_IMX258_ND1_EEPROM 0xA0
#define FIH_I2C_ADDR_IMX258_ND1_ACTUATOR 0x18
//S5K4H8
#define FIH_I2C_ADDR_S5K4H8_ND1_EEPROM 0x20
#define FIH_I2C_ADDR_S5K4H8_ND1_CAMERA 0x20
#define FIH_I2C_ADDR_S5K4H8_ND1_ACTUATOR 0x18
/*N1C component end*/

/*VZ1 component start*/
//IMX258
#define FIH_I2C_ADDR_IMX258_VZ1_CAMERA 0x34
#define FIH_I2C_ADDR_IMX258_VZ1_EEPROM 0xA0
#define FIH_I2C_ADDR_IMX258_VZ1_ACTUATOR 0x18
//S5K5E8
#define FIH_I2C_ADDR_S5K5E8_VZ1_CAMERA 0x20
/*VZ1 component end*/

uint8_t count=0;
uint8_t tmp[8];
char aName[32];
char a_name[32];
extern const char *actuatorName;

/****************************************************************************
*                          Private Type Declaration                         *
****************************************************************************/

struct fih_bbs_camera_module_list fih_camera_list[] = {
	/*D1C start*/
	{
	.i2c_addr = FIH_I2C_ADDR_S5K3P3_CAMERA,		// 0
  .type = ((FIH_BBS_CAMERA_LOCATION_MAIN << 4) | FIH_BBS_CAMERA_MODULE_IC),
	.module_name = "Camera ic (S5K3P3)",
	},
	{
	.i2c_addr = FIH_I2C_ADDR_S5K3P3_EEPROM,		// 1
	.type = ((FIH_BBS_CAMERA_LOCATION_MAIN << 4) | FIH_BBS_CAMERA_MODULE_EEPROM),
	.module_name = "EEPROM (S5K3P3)",
	},
	{
	.i2c_addr = FIH_I2C_ADDR_S5K3P3_ACTUATOR, 		// 2
	.type = ((FIH_BBS_CAMERA_LOCATION_MAIN << 4) | FIH_BBS_CAMERA_MODULE_ACTUATOR),
	.module_name = "Actuator (DW9800_S5K3P3)",
	},
	{
	.i2c_addr = FIH_I2C_ADDR_S5K4H8_CAMERA, 		// 3
	.type = ((FIH_BBS_CAMERA_LOCATION_FRONT << 4) | FIH_BBS_CAMERA_MODULE_IC),
	.module_name = "Camera ic (S5K4H8)",
	},
	{
	.i2c_addr = FIH_I2C_ADDR_S5K4H8_EEPROM, 		// 4
	.type = ((FIH_BBS_CAMERA_LOCATION_FRONT << 4) | FIH_BBS_CAMERA_MODULE_EEPROM),
	.module_name = "EEPROM (S5K4H8)",
	},
	/*D1C end*/

	/*ND1 start*/
	{
	.i2c_addr = FIH_I2C_ADDR_IMX258_ND1_CAMERA, 		// 5
	.type = ((FIH_BBS_CAMERA_LOCATION_MAIN << 4) | FIH_BBS_CAMERA_MODULE_IC),
	.module_name = "Camera ic (IMX258)",
	},
	{
	.i2c_addr = FIH_I2C_ADDR_IMX258_ND1_EEPROM, 		// 6
	.type = ((FIH_BBS_CAMERA_LOCATION_MAIN << 4) | FIH_BBS_CAMERA_MODULE_EEPROM),
	.module_name = "EEPROM (IMX258)",
	},
	{
	.i2c_addr = FIH_I2C_ADDR_IMX258_ND1_ACTUATOR, 		// 7
	.type = ((FIH_BBS_CAMERA_LOCATION_MAIN << 4) | FIH_BBS_CAMERA_MODULE_ACTUATOR),
	.module_name = "Actuator (DW9714_IMX258)",
	},
	{
	.i2c_addr = FIH_I2C_ADDR_S5K4H8_ND1_CAMERA, 		// 8
	.type = ((FIH_BBS_CAMERA_LOCATION_FRONT << 4) | FIH_BBS_CAMERA_MODULE_IC),
	.module_name = "Camera ic (S5K4H8)",
	},
	{
	.i2c_addr = FIH_I2C_ADDR_S5K4H8_ND1_EEPROM, 		// 9
	.type = ((FIH_BBS_CAMERA_LOCATION_FRONT << 4) | FIH_BBS_CAMERA_MODULE_EEPROM),
	.module_name = "EEPROM (S5K4H8)",
	},
	{
	.i2c_addr = FIH_I2C_ADDR_S5K4H8_ND1_ACTUATOR, 		// 10
	.type = ((FIH_BBS_CAMERA_LOCATION_FRONT << 4) | FIH_BBS_CAMERA_MODULE_ACTUATOR),
	.module_name = "Actuator (GT9762_S5K4H8)",
	},
	/*ND1 end*/

	/*MS3 start*/
	{
	.i2c_addr = FIH_I2C_ADDR_IMX258_CAMERA, 		// 11
	.type = ((FIH_BBS_CAMERA_LOCATION_MAIN << 4) | FIH_BBS_CAMERA_MODULE_IC),
	.module_name = "Camera ic (IMX258)",
	},
	{
	.i2c_addr = FIH_I2C_ADDR_IMX258_EEPROM, 		// 12
	.type = ((FIH_BBS_CAMERA_LOCATION_MAIN << 4) | FIH_BBS_CAMERA_MODULE_EEPROM),
	.module_name = "EEPROM (IMX258)",
	},
	{
	.i2c_addr = FIH_I2C_ADDR_IMX258_ACTUATOR, 		// 13
	.type = ((FIH_BBS_CAMERA_LOCATION_MAIN << 4) | FIH_BBS_CAMERA_MODULE_ACTUATOR),
	.module_name = "Actuator (DW9714_IMX258)",
	},
	{
	.i2c_addr = FIH_I2C_ADDR_IMX219_CAMERA, 		// 14
	.type = ((FIH_BBS_CAMERA_LOCATION_FRONT << 4) | FIH_BBS_CAMERA_MODULE_IC),
	.module_name = "Camera ic (IMX219)",
	},
	/*MS3 end*/
};

/**********************************************************************
                         Public Function                              *
**********************************************************************/

void fih_bbs_camera_msg(int module, int error_code)
{
  char param1[32]; // Camera ic, actuator, EEPROM, OIS,¡K
  char param2[64]; // I2C flash error, power up fail, ¡K

	switch (module) {
		case FIH_BBS_CAMERA_MODULE_IC: strcpy(param1, "Camera ic"); break;
		case FIH_BBS_CAMERA_MODULE_EEPROM: strcpy(param1, "EEPROM"); break;
		case FIH_BBS_CAMERA_MODULE_ACTUATOR: strcpy(param1, "Actuator"); break;
		case FIH_BBS_CAMERA_MODULE_OIS: strcpy(param1, "OIS"); break;
		default: strcpy(param1, "Unknown"); break;
  }

	switch (error_code) {
		case FIH_BBS_CAMERA_ERRORCODE_POWER_UP: strcpy(param2, "Power up fail"); break;
		case FIH_BBS_CAMERA_ERRORCODE_POWER_DW: strcpy(param2, "Power down fail"); break;
		case FIH_BBS_CAMERA_ERRORCODE_MCLK_ERR: strcpy(param2, "MCLK error"); break;
		case FIH_BBS_CAMERA_ERRORCODE_I2C_READ: strcpy(param2, "i2c read err"); break;
		case FIH_BBS_CAMERA_ERRORCODE_I2C_WRITE: strcpy(param2, "i2c write err"); break;
		case FIH_BBS_CAMERA_ERRORCODE_I2C_WRITE_SEQ: strcpy(param2, "i2c seq write err"); break;
		case FIH_BBS_CAMERA_ERRORCODE_UNKOWN: strcpy(param2, "unknow error"); break;
		default: strcpy(param2, "Unknown"); break;
	}

  printk("BBox::UPD;88::%s::%s\n", param1, param2);

  return ;
}
EXPORT_SYMBOL(fih_bbs_camera_msg);

void fih_bbs_camera_msg_by_type(int type, int error_code)
{
  char param1[32]; // Camera ic, actuator, EEPROM, OIS,¡K
  char param2[64]; // I2C flash error, power up fail, ¡K
  int isList=0;
  int i=0;

  for(i=0; i<(sizeof(fih_camera_list)/sizeof(struct fih_bbs_camera_module_list)); i++)
  {
    if(type == fih_camera_list[i].type)
    {
      strcpy(param1, fih_camera_list[i].module_name);
      isList = 1;
      break;
    }
  }

  //Do not find in list.
  if(!isList)
  {
    strcpy(param1, "Unknown ");
  }

	switch (error_code) {
		case FIH_BBS_CAMERA_ERRORCODE_POWER_UP: strcpy(param2, "Power up fail"); break;
		case FIH_BBS_CAMERA_ERRORCODE_POWER_DW: strcpy(param2, "Power down fail"); break;
		case FIH_BBS_CAMERA_ERRORCODE_MCLK_ERR: strcpy(param2, "MCLK error"); break;
		case FIH_BBS_CAMERA_ERRORCODE_I2C_READ: strcpy(param2, "i2c read err"); break;
		case FIH_BBS_CAMERA_ERRORCODE_I2C_WRITE: strcpy(param2, "i2c write err"); break;
		case FIH_BBS_CAMERA_ERRORCODE_I2C_WRITE_SEQ: strcpy(param2, "i2c seq write err"); break;
		case FIH_BBS_CAMERA_ERRORCODE_UNKOWN: strcpy(param2, "unknow error"); break;
		default: strcpy(param2, "Unknown"); break;
	}

  printk("BBox::UPD;88::%s::%s\n", param1, param2);

  return ;
}
EXPORT_SYMBOL(fih_bbs_camera_msg_by_type);

void fih_bbs_camera_msg_by_addr(int addr, int error_code)
{
  char param1[32]; // Camera ic, actuator, EEPROM, OIS,¡K
  char param2[64]; // I2C flash error, power up fail, ¡K
  int isList=0;
  int i=0;
  u8 bbs_id=99,bbs_err=99,camera=0,module=0;

  for(i=0; i<(sizeof(fih_camera_list)/sizeof(struct fih_bbs_camera_module_list)); i++)
  {
    if(addr == (fih_camera_list[i].i2c_addr>>1))
    {
      camera = fih_camera_list[i].type >> 4;
      module = fih_camera_list[i].type & 0xf;
      if(camera == FIH_BBS_CAMERA_LOCATION_MAIN)
        bbs_id = FIH_BBSUEC_MAIN_CAM_ID;
      else if(camera == FIH_BBS_CAMERA_LOCATION_FRONT)
        bbs_id = FIH_BBSUEC_FRONT_CAM_ID;
      strcpy(param1, fih_camera_list[i].module_name);
      isList = 1;
      break;
    }
  }

  //Do not find in list.
  if(!isList)
    strcpy(param1, "Unknown module");

	switch (error_code) {
		case FIH_BBS_CAMERA_ERRORCODE_POWER_UP: strcpy(param2, "Power up fail"); break;
		case FIH_BBS_CAMERA_ERRORCODE_POWER_DW: strcpy(param2, "Power down fail"); break;
		case FIH_BBS_CAMERA_ERRORCODE_MCLK_ERR: strcpy(param2, "MCLK error"); break;
		case FIH_BBS_CAMERA_ERRORCODE_I2C_READ: strcpy(param2, "i2c read err"); break;
		case FIH_BBS_CAMERA_ERRORCODE_I2C_WRITE: strcpy(param2, "i2c write err"); break;
		case FIH_BBS_CAMERA_ERRORCODE_I2C_WRITE_SEQ: strcpy(param2, "i2c seq write err"); break;
		case FIH_BBS_CAMERA_ERRORCODE_UNKOWN: strcpy(param2, "unknow error"); break;
		default: strcpy(param2, "Unknown"); break;
	}
	switch (error_code) {
		case FIH_BBS_CAMERA_ERRORCODE_POWER_UP:
		case FIH_BBS_CAMERA_ERRORCODE_POWER_DW:
			if (module == FIH_BBS_CAMERA_MODULE_IC)
				bbs_err = FIH_BBSUEC_CAMERA_ERRORCODE_SENSOR_POWER;
			else if (module == FIH_BBS_CAMERA_MODULE_ACTUATOR)
				bbs_err = FIH_BBSUEC_CAMERA_ERRORCODE_ACTUATOR_POWER;
			break;
		case FIH_BBS_CAMERA_ERRORCODE_MCLK_ERR:
			break;
		case FIH_BBS_CAMERA_ERRORCODE_I2C_READ:
		case FIH_BBS_CAMERA_ERRORCODE_I2C_WRITE:
		case FIH_BBS_CAMERA_ERRORCODE_I2C_WRITE_SEQ:
			if (module == FIH_BBS_CAMERA_MODULE_IC)
				bbs_err = FIH_BBSUEC_CAMERA_ERRORCODE_SENSOR_I2C;
			else if (module == FIH_BBS_CAMERA_MODULE_EEPROM)
				bbs_err = FIH_BBSUEC_CAMERA_ERRORCODE_EEPROM_I2C;
			else if (module == FIH_BBS_CAMERA_MODULE_ACTUATOR)
				bbs_err = FIH_BBSUEC_CAMERA_ERRORCODE_ACTUATOR_I2C;
			break;
		case FIH_BBS_CAMERA_ERRORCODE_UNKOWN:
		default:
			break;
	}

  printk("BBox::UPD;88::%s::%s\n", param1, param2);
  printk("BBox::UEC;%d::%d\n", bbs_id, bbs_err);

  return ;
}
EXPORT_SYMBOL(fih_bbs_camera_msg_by_addr);

void fih_bbs_camera_msg_by_soensor_info(int addr, const char *sensor_name, int position, const char *actuator_name, int error_code)
{
  char param1[32]; // Camera ic, actuator, EEPROM, OIS
  char param2[64]; // I2C flash error, power up fail
  int isList=0;
  int i=0, j = 0, len = 0;
  u8 bbs_id=99, bbs_err=99, camera=0, module=0;
  char *m, *n, res[32];
  char s_name[32];

  //pr_err("%s:%d, Enter, addr(data) = 0x%x\n", __func__, __LINE__,addr<<1);

  /*To upper case*/
  if(sensor_name!=NULL){
      j=0;
      while(*sensor_name!='\0'){
        s_name[j]=toupper(*sensor_name);
        j++;
        sensor_name++;
      }
      //pr_err("%s:%d, sensor_name = %s\n", __func__, __LINE__, s_name);
  }

#if 0
    if(actuatorName!=NULL && ((int)(strlen(actuatorName))>1) ){
      //pr_err("%s:%d strlen(actuatorName) = %d\n", __func__, __LINE__, (int)strlen(actuatorName));
      //pr_err("%s:%d, actuatorName = %s\n", __func__, __LINE__, actuatorName);
      j=0;
      while(*actuatorName!='\0'){
        a_name[j]=toupper(*actuatorName);
        j++;
        actuatorName++;
      }
      //pr_err("%s:%d, a_name = %s\n", __func__, __LINE__, a_name);
      //pr_err("%s:%d, actuatorName = %s\n", __func__, __LINE__, actuatorName);
  }
#endif

  /*ACTUATOR ERROR*/
  if(actuator_name!=NULL && sensor_name==NULL){

    /*To upper case*/
    j=0;
    while(*actuator_name!='\0'){
      aName[j]=toupper(*actuator_name);
      j++;
      actuator_name++;
    }
    aName[j] = '\0';
    //pr_err("%s:%d, actuator_name = %s\n", __func__, __LINE__, aName);

    for(i=0; i<(sizeof(fih_camera_list)/sizeof(struct fih_bbs_camera_module_list)); i++)
    {
         module = fih_camera_list[i].type & 0xf;
         if(module== FIH_BBS_CAMERA_MODULE_ACTUATOR){
             m = strchr(fih_camera_list[i].module_name, '(')+1;
             n = strchr(fih_camera_list[i].module_name, '_');
             len = strlen(m) - strlen(n);

             strncpy(res, m, len);
             res[len] = '\0';
             //pr_err("%s:%d, res = %s\n", __func__, __LINE__, res);

             if(!strncmp(res, aName, strlen(res))){
                 if(position==FIH_BBS_CAMERA_LOCATION_MAIN){
                     bbs_id = FIH_BBSUEC_MAIN_CAM_ID;
                 }else if(position == FIH_BBS_CAMERA_LOCATION_FRONT){
                     bbs_id = FIH_BBSUEC_FRONT_CAM_ID;
                 }

		 strcpy(param1, fih_camera_list[i].module_name);
                 isList = 1;
		 break;
             }
         }
     }
  }

  /*SENSOR ERROR*/
  if(actuator_name==NULL && sensor_name!=NULL){
    for(i=0; i<(sizeof(fih_camera_list)/sizeof(struct fih_bbs_camera_module_list)); i++)
    {
      if(addr == (fih_camera_list[i].i2c_addr>>1))
      {
         camera = fih_camera_list[i].type >> 4;
         module = fih_camera_list[i].type & 0xf;

         m = strchr(fih_camera_list[i].module_name, '(')+1;
         n = strchr(fih_camera_list[i].module_name, ')');
         len = strlen(m) - strlen(n);

         strncpy(res, m, len);
         res[len] = '\0';

         if(!strncmp(res, s_name, strlen(res))){
             if(position==FIH_BBS_CAMERA_LOCATION_MAIN){
                 bbs_id = FIH_BBSUEC_MAIN_CAM_ID;
             }else if(position == FIH_BBS_CAMERA_LOCATION_FRONT){
                 bbs_id = FIH_BBSUEC_FRONT_CAM_ID;
             }

             strcpy(param1, fih_camera_list[i].module_name);
             isList = 1;
             break;
          }
       }
    }
  }

#if 0
  /*I2C READ/WRITE ERROR*/
  if(actuator_name==NULL && sensor_name==NULL){
	//pr_err("%s:%d, addr(data) = 0x%x\n", __func__, __LINE__,addr<<1);
	char *m, *n, act[16];
	int len;

	for(i=0; i<(sizeof(fih_camera_list)/sizeof(struct fih_bbs_camera_module_list)); i++)
	{
            if(addr == (fih_camera_list[i].i2c_addr>>1)){
		pr_err("%s:%d, i = %d\n", __func__, __LINE__, i);
		pr_err("%s:%d, module_name = %s\n", __func__, __LINE__, fih_camera_list[i].module_name);
		pr_err("%s:%d, position = %d\n", __func__, __LINE__, position);


                if(position==FIH_BBS_CAMERA_LOCATION_MAIN+1){	// cci_i2c_master of main camera = 1
                     bbs_id = FIH_BBSUEC_MAIN_CAM_ID;

		     /*special case: only  D1C s5k3p3's sid = 0X20, cci_i2c_master =1*/
		     if((addr<<1)==0x20){
			strcpy(param1, fih_camera_list[0].module_name);
			isList = 1;
		        //break;
		        goto FILL_param2;
		     }else if((addr<<1)==0x18){

                       m = strchr(fih_camera_list[i].module_name, '(')+1;
                       n = strchr(fih_camera_list[i].module_name, '_');
                       len = strlen(m) - strlen(n);

                       strncpy(act, m, len);
                       act[len] = '\0';
		       pr_err("%s:%d, a_name (global parm.) = %s, act = %s\n", __func__, __LINE__, a_name, act);

			if(!strncmp(act, a_name, strlen(act))){
                            strcpy(param1, fih_camera_list[i].module_name);
                            isList = 1;
                            //break;
                            goto FILL_param2;
			}
                      }

		     count++;
		     tmp[count-1]=i;

                 }else if(position >1||position==FIH_BBS_CAMERA_LOCATION_FRONT-1){	// cci_i2c_master of front camera = 0
                 //position == FIH_BBS_CAMERA_LOCATION_FRONT-1
                     bbs_id = FIH_BBSUEC_FRONT_CAM_ID;

                     if((addr<<1)==0x18){

                        m = strchr(fih_camera_list[i].module_name, '(')+1;
                        n= strchr(fih_camera_list[i].module_name, '_');
                        len = strlen(m) - strlen(n);

                        strncpy(act, m, len);
                        act[len] = '\0';

			if(!strncmp(act, a_name, strlen(act))){
                            strcpy(param1, fih_camera_list[i].module_name);
                            isList = 1;
                            //break;
                            goto FILL_param2;
			}
                     }

		     count++;
		     tmp[count-1]=i;
                 }

            }
	}

        if(count>1){

            /*If camera ic and EEPROM has the same slave address,
            it seems as camera ic has some error*/
            if((tmp[count-1]-tmp[count-2]) == 1){
                strcpy(param1, fih_camera_list[tmp[count-2]].module_name);
                isList = 1;
                //goto FILL_param2;
            }else{
                //pr_err("%s:%d, position = 0x%x (%d)\n", __func__, __LINE__, position, position);
                if(position==0x4088){	//S5K4H8_ND1
                    strcpy(param1, fih_camera_list[8].module_name);
                    isList = 1;
                    //goto FILL_param2;
                }else if(position==0x219){
                    strcpy(param1, fih_camera_list[14].module_name);
                    isList = 1;
                    //goto FILL_param2;
                }else{
                    strcpy(param1, fih_camera_list[tmp[count-1]].module_name);
                    isList = 1;
		}
            }
        }else{
            strcpy(param1, fih_camera_list[i].module_name);
            isList = 1;
        }
  }

FILL_param2:
#endif

    //Do not find in list.
    if(!isList)
        strcpy(param1, "Unknown module");

	switch (error_code) {
		case FIH_BBS_CAMERA_ERRORCODE_POWER_UP: strcpy(param2, "Power up fail"); break;
		case FIH_BBS_CAMERA_ERRORCODE_POWER_DW: strcpy(param2, "Power down fail"); break;
		case FIH_BBS_CAMERA_ERRORCODE_MCLK_ERR: strcpy(param2, "MCLK error"); break;
		case FIH_BBS_CAMERA_ERRORCODE_I2C_READ: strcpy(param2, "i2c read err"); break;
		case FIH_BBS_CAMERA_ERRORCODE_I2C_WRITE: strcpy(param2, "i2c write err"); break;
		case FIH_BBS_CAMERA_ERRORCODE_I2C_WRITE_SEQ: strcpy(param2, "i2c seq write err"); break;
		case FIH_BBS_CAMERA_ERRORCODE_UNKOWN: strcpy(param2, "unknow error"); break;
		default: strcpy(param2, "Unknown"); break;
	}
	switch (error_code) {
		case FIH_BBS_CAMERA_ERRORCODE_POWER_UP:
		case FIH_BBS_CAMERA_ERRORCODE_POWER_DW:
			if (module == FIH_BBS_CAMERA_MODULE_IC)
				bbs_err = FIH_BBSUEC_CAMERA_ERRORCODE_SENSOR_POWER;
			else if (module == FIH_BBS_CAMERA_MODULE_ACTUATOR)
				bbs_err = FIH_BBSUEC_CAMERA_ERRORCODE_ACTUATOR_POWER;
			break;
		case FIH_BBS_CAMERA_ERRORCODE_MCLK_ERR:
			break;
		case FIH_BBS_CAMERA_ERRORCODE_I2C_READ:
		case FIH_BBS_CAMERA_ERRORCODE_I2C_WRITE:
		case FIH_BBS_CAMERA_ERRORCODE_I2C_WRITE_SEQ:
			if (module == FIH_BBS_CAMERA_MODULE_IC)
				bbs_err = FIH_BBSUEC_CAMERA_ERRORCODE_SENSOR_I2C;
			else if (module == FIH_BBS_CAMERA_MODULE_EEPROM)
				bbs_err = FIH_BBSUEC_CAMERA_ERRORCODE_EEPROM_I2C;
			else if (module == FIH_BBS_CAMERA_MODULE_ACTUATOR)
				bbs_err = FIH_BBSUEC_CAMERA_ERRORCODE_ACTUATOR_I2C;
			break;
		case FIH_BBS_CAMERA_ERRORCODE_UNKOWN:
		default:
			break;
	}

	//reset count to 0
       count = 0;

	pr_err("%s%d, BBox::UPD;88::%s::%s\n", __func__, __LINE__, param1, param2);
        pr_err("%s%d, BBox::UEC;%d::%d\n", __func__, __LINE__, bbs_id, bbs_err);
}
EXPORT_SYMBOL(fih_bbs_camera_msg_by_soensor_info);

/************************** End Of File *******************************/
