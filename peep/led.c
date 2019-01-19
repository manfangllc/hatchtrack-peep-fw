/***** Includes *****/

#include "led.h"
#include "system.h"
#include "driver/ledc.h"
#include "driver/gpio.h"
#include "soc/gpio_struct.h"

/***** Defines *****/

#define _GPIO_LED_RED 5
#define _GPIO_LED_GREEN 18
#define _GPIO_LED_BLUE 19


#define _PWM_RESOLUTION (LEDC_TIMER_16_BIT)
#define _PWM_FREQ_HZ 100

#if _PWM_RESOLUTION==LEDC_TIMER_16_BIT
#define _PWM_DUTY_MAX (0xFFFF)
#endif

#define _PWM_LEDC_TIMER LEDC_TIMER_0

/***** Local Data *****/


/***** Local Functions *****/

static void
_timer_init(void)
{
  ledc_timer_config_t timer_conf;
  esp_err_t err;

  timer_conf.duty_resolution = LEDC_TIMER_16_BIT;
  timer_conf.freq_hz = _PWM_FREQ_HZ;
  timer_conf.speed_mode = LEDC_HIGH_SPEED_MODE;
  timer_conf.timer_num = _PWM_LEDC_TIMER;

  err = ledc_timer_config(&timer_conf);
  RESULT_TEST_ERROR_TRACE(err == ESP_OK, "failed");
}

static void
_led_off(uint32_t pin)
{
  gpio_config_t io_conf;
  esp_err_t err;

  io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
  io_conf.mode = GPIO_MODE_INPUT;
  io_conf.pin_bit_mask = ((uint64_t) 1) << pin;
  io_conf.pull_down_en = 0;
  io_conf.pull_up_en = 1;

  err = gpio_config(&io_conf);
  RESULT_TEST_ERROR_TRACE(err == ESP_OK, "failed");
}

static void
_led_on(uint32_t pin, ledc_channel_t channel)
{
  ledc_channel_config_t ledc_conf;
  esp_err_t err;

  ledc_conf.timer_sel = _PWM_LEDC_TIMER;
  ledc_conf.channel = channel;
  ledc_conf.duty = 60535;
  ledc_conf.gpio_num = pin;
  ledc_conf.intr_type = LEDC_INTR_DISABLE;
  ledc_conf.speed_mode = LEDC_HIGH_SPEED_MODE;

  err = ledc_channel_config(&ledc_conf);
  RESULT_TEST_ERROR_TRACE(err == ESP_OK, "failed");
}

/***** Global Functions *****/

bool
led_init(void)
{
  _timer_init();

  return true;
}

void
led_red(bool is_on)
{
  if (is_on) {
    _led_on(_GPIO_LED_RED, LEDC_CHANNEL_0);
  }
  else {
    _led_off(_GPIO_LED_RED);
  }
}

void
led_green(bool is_on)
{
  if (is_on) {
    _led_on(_GPIO_LED_GREEN, LEDC_CHANNEL_0);
  }
  else {
    _led_off(_GPIO_LED_GREEN);
  }
}

void
led_blue(bool is_on)
{
  if (is_on) {
    _led_on(_GPIO_LED_BLUE, LEDC_CHANNEL_0);
  }
  else {
    _led_off(_GPIO_LED_BLUE);
  }
}
