#include "esp_log.h"
// #include "driver/i2c.h"
#include "driver/i2c_master.h"

static const char *TAG_SHT31 = "SHT31";

static i2c_port_t i2c_port = I2C_NUM_0;
static i2c_master_dev_handle_t dev_handle;

#define SDA_IO_NUM GPIO_NUM_21
#define SCL_IO_NUM GPIO_NUM_22

#define I2C_MASTER_TX_BUF_DISABLE 0
#define I2C_MASTER_RX_BUF_DISABLE 0
#define WRITE_BIT I2C_MASTER_WRITE
#define READ_BIT I2C_MASTER_READ
#define ACK_CHECK_EN 0x1
#define ACK_CHECK_DIS 0x0
#define ACK_VAL I2C_MASTER_ACK
#define NACK_VAL I2C_MASTER_NACK

static esp_err_t sht31_init(void)
{
    // i2c_config_t conf;
    // conf.mode = I2C_MODE_MASTER;
    // conf.sda_io_num = SDA_IO_NUM;
    // conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    // conf.scl_io_num = SCL_IO_NUM;
    // conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    // conf.master.clk_speed = 1000000;
    // i2c_param_config(i2c_port, &conf);
    // return i2c_driver_install(i2c_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);

    i2c_master_bus_config_t i2c_bus_conf = {
        .i2c_port = i2c_port,
        .sda_io_num = SDA_IO_NUM,
        .scl_io_num = SCL_IO_NUM,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .flags{
            .enable_internal_pullup = true,
        },
    };
    i2c_master_bus_handle_t bus_handle;
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_conf, &bus_handle));

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = 0x44,
        .scl_speed_hz = 100000,
    };

    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));
    return ESP_OK;
}

static uint8_t sht31_crc(uint8_t *data)
{

    uint8_t crc = 0xff;
    int i, j;
    for (i = 0; i < 2; i++)
    {
        crc ^= data[i];
        for (j = 0; j < 8; j++)
        {
            if (crc & 0x80)
            {
                crc <<= 1;
                crc ^= 0x131;
            }
            else
                crc <<= 1;
        }
    }
    return crc;
}

static esp_err_t sht31_read_temp_humi(float *temp, float *humi)
{
    // See http://wiki.seeedstudio.com/Grove-TempAndHumi_Sensor-SHT31/
    // Write 0x00 to address 0x24
    unsigned char data_wr[] = {0x24, 0x00};
    i2c_master_transmit(dev_handle, data_wr, sizeof(data_wr), 500 / portTICK_PERIOD_MS);

    // Delay 20 ms
    vTaskDelay(2);

    // Read 6 bytes
    size_t size = 6;
    uint8_t *data_rd = (uint8_t *)malloc(size);
    if (size > 1)
    {
        i2c_master_receive(dev_handle, data_rd, size - 1, 500 / portTICK_PERIOD_MS);
    }
    i2c_master_receive(dev_handle, data_rd + size - 1, 1, 500 / portTICK_PERIOD_MS);

    // check error
    if (data_rd[2] != sht31_crc(data_rd) ||
        data_rd[5] != sht31_crc(data_rd + 3))
        return ESP_ERR_INVALID_CRC;

    *temp = -45 + (175 * (float)(data_rd[0] * 256 + data_rd[1]) / 65535.0);
    *humi = 100 * (float)(data_rd[3] * 256 + data_rd[4]) / 65535.0;

    free(data_rd);

    return ESP_OK;
}