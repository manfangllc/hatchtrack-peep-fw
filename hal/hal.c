/***** Includes *****/

#include "system.h"
#include "bme680.h"
#include "icm20602.h"
#include "driver/i2c.h"

/***** Defines *****/

#define _I2C_SDA_PIN 32
#define _I2C_SCL_PIN 33
#define _I2C_CLK_FREQ_HZ 100000

#define _I2C_ADDR_BME680 (BME680_I2C_ADDR_PRIMARY)
#define _I2C_ADDR_ICM20602 (0x69)

/***** Local Data *****/

static struct bme680_dev _bme680;
static struct icm20602_dev _icm20602;
static i2c_port_t _i2c = I2C_NUM_0;
static SemaphoreHandle_t _lock = NULL;

/***** Local Functions *****/

int8_t
_i2c_write_reg(uint8_t slave_addr, uint8_t reg, uint8_t * buf, uint16_t len)
{
  i2c_cmd_handle_t cmd;
  esp_err_t r = ESP_OK;
  uint32_t n = 0;

  cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  // clear read bit
  i2c_master_write_byte(cmd, (slave_addr << 1) | 0x00, true);
  i2c_master_write_byte(cmd, reg, true);
  for (n = 0; n < len; n++) {
    i2c_master_write_byte(cmd, *buf++, true);
  }
  i2c_master_stop(cmd);
  r = i2c_master_cmd_begin(_i2c, cmd, 1000 / portTICK_PERIOD_MS);
  i2c_cmd_link_delete(cmd);

  return (ESP_OK == r) ? 0 : -1;
}

int8_t
_i2c_read_reg(uint8_t slave_addr, uint8_t reg, uint8_t * buf, uint16_t len)
{
  i2c_cmd_handle_t cmd;
  esp_err_t r = ESP_OK;

  cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  // clear read bit
  i2c_master_write_byte(cmd, (slave_addr << 1) | 0x00, true);
  i2c_master_write_byte(cmd, reg, true);
  i2c_master_stop(cmd);
  r = i2c_master_cmd_begin(_i2c, cmd, 1000 / portTICK_PERIOD_MS);
  i2c_cmd_link_delete(cmd);

  if (ESP_OK == r) {
    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    // set read bit
    i2c_master_write_byte(cmd, (slave_addr << 1) | 0x01, true);
    i2c_master_read(cmd, buf, len, false);
    r = i2c_master_cmd_begin(_i2c, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
  }

  return (ESP_OK == r) ? 0 : -1;
}

static bool
_i2c_master_init(void)
{
  i2c_config_t conf;
  esp_err_t r = ESP_OK;

  conf.mode = I2C_MODE_MASTER;
  conf.sda_io_num = _I2C_SDA_PIN;
  conf.scl_io_num = _I2C_SCL_PIN;
  conf.sda_pullup_en = GPIO_PULLUP_DISABLE;
  conf.scl_pullup_en = GPIO_PULLUP_DISABLE;
  conf.master.clk_speed = _I2C_CLK_FREQ_HZ;

  if (ESP_OK == r) {
    r = i2c_param_config(_i2c, &conf);
  }

  if (ESP_OK == r) {
    r = i2c_driver_install(_i2c, conf.mode, 0, 0, 0);
  }

  return (ESP_OK == r) ? true : false;
}

static bool
_i2c_master_free(void)
{
  esp_err_t r = ESP_OK;

  r = i2c_driver_delete(_i2c);

  return (ESP_OK == r) ? true : false;
}

static void
_mutex_lock(uint8_t id)
{
  (void) id;
  xSemaphoreTake(_lock, portMAX_DELAY);
}

static void
_mutex_unlock(uint8_t id)
{
  (void) id;
  xSemaphoreGive(_lock);
}

static void
_sleep(uint32_t delay_ms)
{
  TickType_t rtos_ticks = delay_ms / portTICK_PERIOD_MS;
  if (0 == rtos_ticks) {
    rtos_ticks = 1;
  }
  vTaskDelay(rtos_ticks);
}

static bool
_bme680_init(void)
{
  int status = 0;
  bool r = true;

  if (r) {
    _bme680.dev_id = _I2C_ADDR_BME680;
    _bme680.intf = BME680_I2C_INTF;
    _bme680.read = &_i2c_read_reg;
    _bme680.write = &_i2c_write_reg;
    _bme680.delay_ms = &_sleep;

    status = bme680_init(&_bme680);
    if (0 != status) {
      r = false;
    }
  }

  if (r) {
    // These settings stay constant through our application.
    _bme680.tph_sett.os_temp = BME680_OS_8X;
    _bme680.tph_sett.os_hum = BME680_OS_2X;
    _bme680.tph_sett.os_pres = BME680_OS_4X;
    _bme680.tph_sett.filter = BME680_FILTER_SIZE_3;
    _bme680.gas_sett.heatr_temp = 320; // 320*C
    _bme680.gas_sett.heatr_dur = 150; // 150 ms
    _bme680.gas_sett.run_gas = BME680_ENABLE_GAS_MEAS;
    _bme680.power_mode = BME680_FORCED_MODE;
  }

  return r;
}

static bool
_icm20602_init(void)
{
  int status = 0;
  bool r = true;

  _icm20602 = (struct icm20602_dev) ICM20602_DEFAULT_INIT();

  _icm20602.id = _I2C_ADDR_ICM20602;
  _icm20602.hal_wr = _i2c_write_reg;
  _icm20602.hal_rd = _i2c_read_reg;
  _icm20602.hal_sleep = _sleep;
  _icm20602.mutex_lock = _mutex_lock;
  _icm20602.mutex_unlock = _mutex_unlock;

  if (r) {
    _lock = xSemaphoreCreateMutex();
  }

  status = icm20602_init(&_icm20602);
  if (0 != status) {
    r = false;
  }
  return r;
}

/***** Global Functions *****/

bool
hal_init(void)
{

  bool r = true;

  if (r) {
    r = _i2c_master_init();
    RESULT_TEST(r, "failed to initialize i2c interface!\n");
  }

  if (r) {
    r = _bme680_init();
  }

  if (r) {
    r = _icm20602_init();
  }

  return r;
}

bool
hal_read_temperature_humdity(float * p_temperature, float * p_humidity)
{
  struct bme680_field_data data;
  uint16_t measure_delay = 0;
  int status = 0;
  bool r = true;

  const uint16_t sensor_settings =
    BME680_OST_SEL |
    BME680_OSH_SEL |
    BME680_OSP_SEL |
    BME680_FILTER_SEL |
    BME680_GAS_SENSOR_SEL;

  float temperature = 0;
  float humidity = 0;
  float pressure = 0;
  float gas_resistance = 0;

  if (r) {
    // don't do anything till we request a reading
    _bme680.power_mode = BME680_FORCED_MODE;
    status = bme680_set_sensor_settings(sensor_settings, &_bme680);
    if (0 > status) {
      r = false;
    }
  }

  if (r) {
    status = bme680_set_sensor_mode(&_bme680);
    if (0 > status) {
      r = false;
    }
  }

  if (r) {
    bme680_get_profile_dur(&measure_delay, &_bme680);
    LOGI("measure delay = %d\n", measure_delay);
    _sleep(measure_delay * 2);
  }

  if (r) {
    status = bme680_get_sensor_data(&data, &_bme680);
    if (0 != status) {
      LOGE("failed to read data (%d)!\n", status);
      r = false;
    }
  }

  if (r) {
    temperature = data.temperature / 100.0;
    humidity = data.humidity / 1000.0;
    pressure = data.pressure;
    gas_resistance = data.gas_resistance;

    LOGI(
      "t=%f\nh=%f\np=%f\ng=%f\n",
      temperature,
      humidity,
      pressure,
      gas_resistance);
    
    *p_temperature = temperature;
    *p_humidity = humidity;
  }

  return r;
}

bool
hal_read_accel(float * p_gx, float * p_gy, float * p_gz)
{
  int8_t status = 0;

  status = icm20602_read_accel(&_icm20602, p_gx, p_gy, p_gz);

  return (0 == status) ? true : false;
}

/***** Unit Tests *****/

#ifdef PEEP_UNIT_TEST_BUILD
TEST_CASE("HAL BME680", "[hal.c]")
{
  bool r = true;

  printf("* Initializing I2C interface.\n");
  r = _i2c_master_init();
  TEST_ASSERT(r);

  printf("* Initializing BME680.\n");
  r = _bme680_init();
  TEST_ASSERT(r);

  printf("* Performing measurement... ");
  float t, h;
  r = hal_read_temperature_humdity(&t, &h);
  TEST_ASSERT(r);
  printf("T=%f, H=%f\n", t, h);

  printf("* Free I2C interface.\n");
  r = _i2c_master_free();
  TEST_ASSERT(r);
}

TEST_CASE("HAL ICM20602", "[hal.c]")
{
  bool r = true;

  printf("* Initializing I2C interface.\n");
  r = _i2c_master_init();
  TEST_ASSERT(r);

  printf("* Initializing ICM20602.\n");
  r = _icm20602_init();
  TEST_ASSERT(r);

  printf("* Performing measurement... \n");
  float x, y, z;
  r = hal_read_accel(&x, &y, &z);
  TEST_ASSERT(r);
  printf("X=%f, Y=%f, Z=%f\n", x, y, z);

  printf("* Free I2C interface.\n");
  r = _i2c_master_free();
  TEST_ASSERT(r);
}
#endif
