#include "lcd_native.h"

// LCD module defines
#define LCD_LINEONE 0x00   // start of line 1
#define LCD_LINETWO 0x40   // start of line 2
#define LCD_LINETHREE 0x10 // start of line 3
#define LCD_LINEFOUR 0x50  // start of line 4

#define LCD_BACKLIGHT 0x08
#define LCD_ENABLE 0x04
#define LCD_COMMAND 0x00
#define LCD_WRITE 0x01

#define LCD_SET_DDRAM_ADDR 0x80
#define LCD_READ_BF 0x40

// LCD instructions
#define LCD_CLEAR 0x01             // replace all characters with ASCII 'space'
#define LCD_HOME 0x02              // return cursor to first position on first line
#define LCD_ENTRY_MODE 0x06        // shift cursor from left to right on read/write
#define LCD_DISPLAY_OFF 0x08       // turn display off
#define LCD_DISPLAY_ON 0x0C        // display on, cursor off, don't blink character
#define LCD_FUNCTION_RESET 0x30    // reset the LCD
#define LCD_FUNCTION_SET_4BIT 0x28 // 4-bit data, 2-line display, 5 x 7 font
#define LCD_SET_CURSOR 0x80        // set cursor position

// Pin mappings
// P0 -> RS
// P1 -> RW
// P2 -> E
// P3 -> Backlight
// P4 -> D4
// P5 -> D5
// P6 -> D6
// P7 -> D7

static char tag[] = "LCD Driver";
static uint8_t LCD_addr;
static uint8_t SDA_pin;
static uint8_t SCL_pin;
static uint8_t LCD_cols;
static uint8_t LCD_rows;
static bool isInit = false;

static void LCD_writeNibble(uint8_t nibble, uint8_t mode);
static void LCD_writeByte(uint8_t data, uint8_t mode);
static void LCD_pulseEnable(uint8_t nibble);
i2c_master_dev_handle_t dev_handle;

static esp_err_t I2C_init(void)
{
    i2c_master_bus_config_t i2c_bus_conf = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .sda_io_num = SDA_pin,
        .scl_io_num = SCL_pin,
        .i2c_port = -1,
        .flags.enable_internal_pullup = true,
    };
    i2c_master_bus_handle_t bus_handle;
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_conf, &bus_handle));

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = LCD_addr,
        .scl_speed_hz = 100000,
    };

    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));
    return ESP_OK;
}

void LCD_init(uint8_t addr, uint8_t dataPin, uint8_t clockPin, uint8_t cols, uint8_t rows)
{
    LCD_addr = addr;
    SDA_pin = dataPin;
    SCL_pin = clockPin;
    LCD_cols = cols;
    LCD_rows = rows;
    esp_err_t error = I2C_init();

    if (error != ESP_OK)
    {
        ESP_LOGE(tag, "I2C init failed with error: %d", error);
        return;
    }
    else
    {
        isInit = true;
    }

    vTaskDelay(100 / portTICK_PERIOD_MS); // Initial 40 mSec delay

    // Reset the LCD controller
    LCD_writeNibble(LCD_FUNCTION_RESET, LCD_COMMAND);    // First part of reset sequence
    vTaskDelay(10 / portTICK_PERIOD_MS);                 // 4.1 mS delay (min)
    LCD_writeNibble(LCD_FUNCTION_RESET, LCD_COMMAND);    // second part of reset sequence
    ets_delay_us(200);                                   // 100 uS delay (min)
    LCD_writeNibble(LCD_FUNCTION_RESET, LCD_COMMAND);    // Third time's a charm
    LCD_writeNibble(LCD_FUNCTION_SET_4BIT, LCD_COMMAND); // Activate 4-bit mode
    ets_delay_us(80);                                    // 40 uS delay (min)

    // --- Busy flag now available ---
    // Function Set instruction
    LCD_writeByte(LCD_FUNCTION_SET_4BIT, LCD_COMMAND); // Set mode, lines, and font
    ets_delay_us(80);

    // Clear Display instruction
    LCD_writeByte(LCD_CLEAR, LCD_COMMAND); // clear display RAM
    vTaskDelay(2 / portTICK_PERIOD_MS);    // Clearing memory takes a bit longer

    // Entry Mode Set instruction
    LCD_writeByte(LCD_ENTRY_MODE, LCD_COMMAND); // Set desired shift characteristics
    ets_delay_us(80);

    LCD_writeByte(LCD_DISPLAY_ON, LCD_COMMAND); // Ensure LCD is set to on
}

void LCD_setCursor(uint8_t col, uint8_t row)
{
    if (isInit == false)
    {
        ESP_LOGE(tag, "LCD not initialized");
        return;
    }
    if (row > LCD_rows - 1)
    {
        ESP_LOGE(tag, "Cannot write to row %d. Please select a row in the range (0, %d)", row, LCD_rows - 1);
        row = LCD_rows - 1;
    }
    uint8_t row_offsets[] = {LCD_LINEONE, LCD_LINETWO, LCD_LINETHREE, LCD_LINEFOUR};
    LCD_writeByte(LCD_SET_DDRAM_ADDR | (col + row_offsets[row]), LCD_COMMAND);
}

void LCD_writeChar(char c)
{
    if (isInit == false)
    {
        ESP_LOGE(tag, "LCD not initialized");
        return;
    }
    LCD_writeByte(c, LCD_WRITE); // Write data to DDRAM
}

void LCD_writeStr(char *str)
{
    if (isInit == false)
    {
        ESP_LOGE(tag, "LCD not initialized");
        return;
    }
    while (*str)
    {
        LCD_writeChar(*str++);
    }
}

void LCD_home(void)
{
    if (isInit == false)
    {
        ESP_LOGE(tag, "LCD not initialized");
        return;
    }
    LCD_writeByte(LCD_HOME, LCD_COMMAND);
    vTaskDelay(2 / portTICK_PERIOD_MS); // This command takes a while to complete
}

void LCD_clearScreen(void)
{
    if (isInit == false)
    {
        ESP_LOGE(tag, "LCD not initialized");
        return;
    }
    LCD_writeByte(LCD_CLEAR, LCD_COMMAND);
    vTaskDelay(2 / portTICK_PERIOD_MS); // This command takes a while to complete
}

static void LCD_writeNibble(uint8_t nibble, uint8_t mode)
{
    uint8_t data = (nibble & 0xF0) | mode | LCD_BACKLIGHT;
    i2c_master_transmit(dev_handle, &data, 1, 2000 / portTICK_PERIOD_MS);
    LCD_pulseEnable(data); // Clock data into LCD
}

static void LCD_writeByte(uint8_t data, uint8_t mode)
{
    LCD_writeNibble(data & 0xF0, mode);
    LCD_writeNibble((data << 4) & 0xF0, mode);
}

static void LCD_pulseEnable(uint8_t data)
{
    uint8_t buf;
    buf = data | LCD_ENABLE;
    i2c_master_transmit(dev_handle, &buf, 1, 2000 / portTICK_PERIOD_MS);
    ets_delay_us(1);
    buf = (data & ~LCD_ENABLE);
    i2c_master_transmit(dev_handle, &buf, 1, 2000 / portTICK_PERIOD_MS);
    ets_delay_us(500);
}
