

struct fih_mem_map {
    char module_name[32];
    int slave_addr;
    struct msm_camera_reg_settings_t mem_settings[MSM_EEPROM_MEMORY_MAP_MAX_SIZE];
    int memory_map_size;
};

typedef struct fih_mem_map fih_mem_map_t;

fih_mem_map_t fih_sensor_id_map[] =
{
    {
        .module_name = "imx258_ms3",
        .slave_addr = 0x34,
        .mem_settings =
        {
          {0x0100, MSM_CAMERA_I2C_WORD_ADDR, 0x01, MSM_CAMERA_I2C_BYTE_DATA, MSM_CAM_WRITE, 10 },
          {0x0A02, MSM_CAMERA_I2C_WORD_ADDR, 0x0F, MSM_CAMERA_I2C_BYTE_DATA, MSM_CAM_WRITE, 0 },
          {0x0A00, MSM_CAMERA_I2C_WORD_ADDR, 0x01, MSM_CAMERA_I2C_BYTE_DATA, MSM_CAM_WRITE, 0 },
          {0x0A24, MSM_CAMERA_I2C_WORD_ADDR, 0x0B, MSM_CAMERA_I2C_BYTE_DATA, MSM_CAM_READ, 0 },
          {0x0100, MSM_CAMERA_I2C_WORD_ADDR, 0x00, MSM_CAMERA_I2C_BYTE_DATA, MSM_CAM_WRITE, 1 },
        },
        .memory_map_size = 5,
    },
    {
        .module_name = "s5k3l8_nd1",
        .slave_addr = 0x20,
        .mem_settings =
        {
          {0x0100, MSM_CAMERA_I2C_WORD_ADDR, 0x01, MSM_CAMERA_I2C_BYTE_DATA, MSM_CAM_WRITE, 10 },
          {0x6028, MSM_CAMERA_I2C_WORD_ADDR, 0x40, MSM_CAMERA_I2C_BYTE_DATA, MSM_CAM_WRITE, 0 },
          {0x0A02, MSM_CAMERA_I2C_WORD_ADDR, 0x00, MSM_CAMERA_I2C_BYTE_DATA, MSM_CAM_WRITE, 0 },
          {0x0A00, MSM_CAMERA_I2C_WORD_ADDR, 0x01, MSM_CAMERA_I2C_BYTE_DATA, MSM_CAM_WRITE, 0 },
          {0x0A24, MSM_CAMERA_I2C_WORD_ADDR, 0x06, MSM_CAMERA_I2C_BYTE_DATA, MSM_CAM_READ, 0 },
          {0x0100, MSM_CAMERA_I2C_WORD_ADDR, 0x00, MSM_CAMERA_I2C_BYTE_DATA, MSM_CAM_WRITE, 1 },
        },
        .memory_map_size = 6,
    },
    {
        .module_name = "s5k4h8_nd1",
        .slave_addr = 0x20,
        .mem_settings =
        {
            { 0x0100, MSM_CAMERA_I2C_WORD_ADDR, 0x01, MSM_CAMERA_I2C_BYTE_DATA, MSM_CAM_WRITE, 1 },
            { 0x0A02, MSM_CAMERA_I2C_WORD_ADDR, 0x03, MSM_CAMERA_I2C_BYTE_DATA, MSM_CAM_WRITE, 0 }, /* Page 3 */
            { 0x0A00, MSM_CAMERA_I2C_WORD_ADDR, 0x01, MSM_CAMERA_I2C_BYTE_DATA, MSM_CAM_WRITE, 1 }, /* enable */
            { 0x0A21, MSM_CAMERA_I2C_WORD_ADDR, 0x07, MSM_CAMERA_I2C_BYTE_DATA, MSM_CAM_READ, 0 }, /* read data */
            { 0x0100, MSM_CAMERA_I2C_WORD_ADDR, 0x00, MSM_CAMERA_I2C_BYTE_DATA, MSM_CAM_WRITE, 1 },
        },
        .memory_map_size = 5,
    },
    {
        .module_name = "s5k4h8",
        .slave_addr = 0x5A,
        .mem_settings =
        {
            { 0x0100, MSM_CAMERA_I2C_WORD_ADDR, 0x01, MSM_CAMERA_I2C_BYTE_DATA, MSM_CAM_WRITE, 1 },
            { 0x0A02, MSM_CAMERA_I2C_WORD_ADDR, 0x03, MSM_CAMERA_I2C_BYTE_DATA, MSM_CAM_WRITE, 0 }, /* Page 3 */
            { 0x0A00, MSM_CAMERA_I2C_WORD_ADDR, 0x01, MSM_CAMERA_I2C_BYTE_DATA, MSM_CAM_WRITE, 1 }, /* enable */
            { 0x0A21, MSM_CAMERA_I2C_WORD_ADDR, 0x07, MSM_CAMERA_I2C_BYTE_DATA, MSM_CAM_READ, 0 }, /* read data */
            { 0x0100, MSM_CAMERA_I2C_WORD_ADDR, 0x00, MSM_CAMERA_I2C_BYTE_DATA, MSM_CAM_WRITE, 1 },
        },
        .memory_map_size = 5,
    },
};


