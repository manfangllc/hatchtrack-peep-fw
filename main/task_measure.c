/***** Includes *****/

#include <time.h>

#include "tasks.h"
#include "aws_mqtt.h"
#include "aws_mqtt_shadow.h"
#include "hal.h"
#include "hatch_config.h"
#include "hatch_measurement.h"
#include "json_parse.h"
#include "memory.h"
#include "memory_measurement_db.h"
#include "state.h"
#include "system.h"
#include "wifi.h"

/***** Defines *****/

// Comment this out to enter deep sleep when not active.
//#define _NO_DEEP_SLEEP 1
#define _BUFFER_LEN (2048)
#define _AWS_SHADOW_GET_TIMEOUT_SEC (30)
#define _UNIX_TIMESTAMP_THRESHOLD (1546300800)
#define _HATCH_CONFIG_DEFAULT_MEASURE_INTERVAL_SEC (5 * 60)
#define _HATCH_CONFIG_DEFAULT_END_UNIX_TIMESTAMP (2147483647)

#if defined(PEEP_TEST_STATE_MEASURE) || (PEEP_TEST_STATE_MEASURE_CONFIG)
  // SSID of the WiFi AP connect to.
  #define _TEST_WIFI_SSID "thesignal"
  // Password of the WiFi AP to connect to.
  #define _TEST_WIFI_PASSWORD "palmerho"
  // Leave this value alone unless you have a good reason to change it.
  #define _TEST_HATCH_CONFIG_UUID "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"
  // Seconds that Peep should deep sleep before taking a new measurement.
  #define _TEST_HATCH_CONFIG_MEASURE_INTERVAL_SEC 900
  // UTC Unix timestamp at which Peep should stop taking measurements.
  #define _TEST_HATCH_CONFIG_END_UNIX_TIMESTAMP 1735084800
#endif

/***** Extern Data *****/

extern const uint8_t _root_ca_start[]   asm("_binary_root_ca_txt_start");
extern const uint8_t _root_ca_end[]   asm("_binary_root_ca_txt_end");

extern const uint8_t _cert_start[]   asm("_binary_cert_txt_start");
extern const uint8_t _cert_end[]   asm("_binary_cert_txt_end");

extern const uint8_t _key_start[]   asm("_binary_key_txt_start");
extern const uint8_t _key_end[]   asm("_binary_key_txt_end");

extern const uint8_t _uuid_start[]   asm("_binary_uuid_txt_start");
extern const uint8_t _uuid_end[]   asm("_binary_uuid_txt_end");

/***** Local Data *****/

static struct hatch_configuration _config;

static EventGroupHandle_t _sync_event_group = NULL;
static const int SYNC_BIT = BIT0;

/***** Local Functions *****/

static bool _format_json(uint8_t * buf, uint32_t buf_len, struct hatch_measurement * meas, char * peep_uuid, char * hatch_uuid)
{
  int32_t bytes = 0;
  bool r = true;

  bytes = snprintf(
    (char *) buf,
    buf_len,
    "{\n"
    "\"unixTime\": %d,\n"
    "\"peepUUID\": \"%s\",\n"
    "\"hatchUUID\": \"%s\",\n"
    "\"temperature\": %f,\n"
    "\"humidity\": %f,\n"
    "\"pressure\": %f,\n"
    "\"gasResistance\": %f\n"
    "}",
    meas->unix_timestamp,
    (const char *) peep_uuid,
    (const char *) hatch_uuid,
    meas->temperature,
    meas->humidity,
    meas->air_pressure,
    meas->gas_resistance);

  if ((bytes < 0) || (bytes >= buf_len)) {
    r = false;
  }

  return r;
}

static bool aws_publish_measurements(uint8_t * buf, uint32_t buf_len, char * peep_uuid, char * hatch_uuid)
{
   bool                     Result;
   uint32_t                 TotalMeasurements = 0;
   unsigned int             Index;
   struct hatch_measurement old;

   /* Determine how many measurements require uploading.                 */
   TotalMeasurements = memory_measurement_db_total();
   LOGI("%d old measurements to upload", TotalMeasurements);
   if(TotalMeasurements > 0)
   {
      Result = memory_measurement_db_read_open();
      if(Result)
      {
         for(Index = 0; ((Index<TotalMeasurements)&&(Result)); Index++)
         {
           Result = memory_measurement_db_read_entry(&old);
           if (Result)
           {
              Result = _format_json(buf, buf_len, &old, peep_uuid, hatch_uuid);
              if (Result)
              {
                 Result = aws_mqtt_publish("hatchtrack/data/put", (char *) buf, false);
              }
           }
         }

         memory_measurement_db_read_close();

         /* If successful, delete the old data.                          */
         if(Result)
         {
           LOGI("deleting old data");
           memory_measurement_db_delete_all();
         }
      }
   }
   else
   {
      Result = true;
   }

   return(Result);
}

static bool _get_wifi_ssid_pasword(char * ssid, char * password)
{
  int len = 0;
  bool r = true;

#ifdef PEEP_TEST_STATE_MEASURE
  (void) len;
  strcpy(ssid, _TEST_WIFI_SSID);
  strcpy(password, _TEST_WIFI_PASSWORD);
#else
  if (r) {
    len = memory_get_item(
      MEMORY_ITEM_WIFI_SSID,
      (uint8_t *) ssid,
      WIFI_SSID_LEN_MAX);
    if (len <= 0) {
      r = false;
    }
    else {
      LOGI("SSID = %s", ssid);
    }
  }

  if (r) {
    len = memory_get_item(
      MEMORY_ITEM_WIFI_PASS,
      (uint8_t *) password,
      WIFI_PASSWORD_LEN_MAX);
    if (len <= 0) {
      r = false;
    }
    else {
      LOGI("PASS = %s", password);
    }
  }
#endif

  return r;
}

static void _shadow_callback(uint8_t * buf, uint16_t buf_len)
{
  bool r = true;

#if defined(PEEP_TEST_STATE_MEASURE) || defined(PEEP_TEST_STATE_MEASURE_CONFIG)
  LOGI("AWS Shadow: %s", buf);
#endif

  r = json_parse_hatch_config_msg((char *) buf, &_config);
  if (r) {
    xEventGroupSetBits(_sync_event_group, SYNC_BIT);
  }
  else {
    LOGE("error decoding shadow JSON document");
    LOGE("received %d bytes", buf_len);
    ESP_LOG_BUFFER_HEXDUMP(__func__, buf, buf_len, ESP_LOG_ERROR);
  }
}

#if defined(PEEP_TEST_STATE_MEASURE)

   /* Test function to read and print stored measurements.               */
static void PrintStoredMeasurements(void)
{
   bool                     Result;
   uint32_t                 TotalMeasurements = 0;
   unsigned int             Index;
   struct hatch_measurement old;


   LOGI("PrintStoredMeasurements() enter");

   /* Determine how many measurements require uploading.                 */
   TotalMeasurements = memory_measurement_db_total();
   if(TotalMeasurements > 0)
   {
      Result = memory_measurement_db_read_open();
      if(Result)
      {
         for(Index = 0; ((Index<TotalMeasurements)&&(Result)); Index++)
         {
           Result = memory_measurement_db_read_entry(&old);
           if (Result)
           {
              LOGI("Stored Measurement %u:",        (Index + 1));
              LOGI("   Timestamp   : %u",         old.unix_timestamp);
              LOGI("   Temperature : %f C, %f F", old.temperature, ((old.temperature * ((float)(1.8))) + ((float)32)));
              LOGI("   humidity    : %f",         old.humidity);
           }
         }

         memory_measurement_db_read_close();
      }
   }
   else
   {
      Result = true;
   }

   LOGI("PrintStoredMeasurements() exit %d", (int)Result);
}

#endif

/***** Global Functions *****/

void task_measure(void * arg)
{
  char                     *ssid;
  char                     *pass;
  char                     *peep_uuid             = (char *) _uuid_start;
  char                     *root_ca               = (char *) _root_ca_start;
  char                     *cert                  = (char *) _cert_start;
  char                     *key                   = (char *) _key_start;
  bool                      publish_measurements;
  bool                      invalid_time;
  bool                      measurement_stored;
  bool                      Result;
  time_t                    now;
  struct tm                 timeinfo;
  uint8_t                  *buffer;
  EventBits_t               bits                  = 0;
  uint32_t                  total                 = 0;
  uint32_t                  MeasurementInterval   = 60;
  struct hatch_measurement  meas;

  LOGI("start");

#if defined(PEEP_TEST_STATE_MEASURE)
  LOGI("PEEP_TEST_STATE_MEASURE");
#elif defined(PEEP_TEST_STATE_MEASURE_CONFIG)
  LOGI("PEEP_TEST_STATE_MEASURE_CONFIG");
#endif

  /* Allocate the memory needd for this task.*/
  _sync_event_group = xEventGroupCreate();
  buffer            = malloc(_BUFFER_LEN);
  ssid              = malloc(WIFI_SSID_LEN_MAX);
  pass              = malloc(WIFI_PASSWORD_LEN_MAX);

  if ((buffer != NULL) && (ssid != NULL) && (pass != NULL))
  {
     LOGI("initializing hardware");
     Result = hal_init();
     if(!Result)
     {
        /* not cleanest but it will work for now*/
        goto ERROR;
     }

     /* Attempt to get the stored hatch config.*/
     memset((uint8_t *)&_config, 0, sizeof(struct hatch_configuration));

     memory_get_item(MEMORY_ITEM_HATCH_CONFIG, (uint8_t *) &_config, sizeof(struct hatch_configuration));

     if (IS_HATCH_CONFIG_VALID(_config))
     {
        LOGI("loaded previous hatch configuration");

        LOGI("Magic Word                    : 0x%X", _config.magic_word);
        LOGI("Measurements Interval (s)     : %u",   _config.measure_interval_sec);
        LOGI("Consecutive Low Readings      : %u",   _config.consecutive_low_readings);
        LOGI("Consecutive High Readings     : %u",   _config.consecutive_high_readings);
        LOGI("Measurements since Publishing : %u",   _config.measurements_since_last_published);
        LOGI("Measurements before Publishing: %u",   _config.measurements_before_publishing);
     }
     else
     {
       LOGE("failed to load previous hatch configuration");

       /* Load a default value for the config and store to nvm.*/
       HATCH_CONFIG_INIT(_config);

       memory_set_item(MEMORY_ITEM_HATCH_CONFIG, (uint8_t *) &_config, sizeof(struct hatch_configuration));
     }

     LOGI("performing measurement : Time : %ld", time(NULL));

     /* Take the measurement and print some useful info.*/
     memset(&meas, 0, sizeof(meas));

     Result = hal_read_temperature_humdity_pressure_resistance(&(meas.temperature), &(meas.humidity), &(meas.air_pressure), NULL);
     if(Result)
     {
        LOGI("Temperature (C, F): %f, %f", meas.temperature, ((meas.temperature * ((float)(1.8))) + ((float)32)));
        LOGI("humidity          : %f",     meas.humidity);
     }
     else
     {
        /* not cleanest but it will work for now (no cleanup to do, reboot*/
        /* post deep sleep).                                              */
        goto ERROR;
     }

     /* measurement taken, do book taking about when we should publish.   */
     publish_measurements                       = false;
     _config.measurements_since_last_published += 1;

     /* Flag measurement is not stored yet (and time is not yet know to  */
     /* be invalid).                                                     */
     measurement_stored                         = false;
     invalid_time                               = false;

     /* Check to see if the time is valid (pulled from SNTP example.     */
     time(&now);
     localtime_r(&now, &timeinfo);

     // Is time set? If not, tm_year will be (1970 - 1900).
     if (timeinfo.tm_year < (2016 - 1900))
     {
         LOGI("Time is not set yet. Connecting to WiFi and getting time over NTP.");

         invalid_time = true;
     }
     else
     {
        /* Time is valid.   so store measurement timestamp.              */
        meas.unix_timestamp = time(NULL);
        LOGI("current Unix time %u", meas.unix_timestamp);

        /* Check to see if we have reach the end of measurement period.  */
        if(meas.unix_timestamp >= _config.end_unix_timestamp)
        {
           /* reached the end of measurement period.  publish what we    */
           /* have                                                       */
           publish_measurements = true;
        }
     }

     /* check to see if it is time to publish these measurements.         */
     if(_config.measurements_since_last_published >= _config.measurements_before_publishing)
     {
        /* time to publish the measurements.                              */
        publish_measurements = true;

        LOGI("Measurments %u, Threshold %u, Publishing", _config.measurements_since_last_published, _config.measurements_before_publishing);
     }

     /* Check to see if we are in the golden range.                       */
     if(meas.temperature > _config.high_temperature)
     {
        /* Above the golden range, increment counts.                      */
        _config.consecutive_high_readings      += 1;
        _config.consecutive_low_readings        = 0;

        LOGI("High Temp %f C, Threshold %f C, Consecutive %u, Threshold %u", meas.temperature, _config.high_temperature, _config.consecutive_high_readings, _config.consecutive_high_readings_error);

        /* Check to see if this high reading puts us above the threshold */
        /* before we will publish our results.                           */
        if(_config.consecutive_high_readings >= _config.consecutive_high_readings_error)
        {
           publish_measurements = true;
        }
     }
     else if(meas.temperature < _config.low_temperature)
     {
        /* below the golden range, increment counts.                     */
        _config.consecutive_low_readings       += 1;
        _config.consecutive_high_readings       = 0;

        LOGI("Low Temp %f C, Threshold %f C, Consecutive %u, Threshold %u", meas.temperature, _config.low_temperature, _config.consecutive_low_readings, _config.consecutive_low_readings_error);

        /* Check to see if this low reading puts us above the threshold */
        /* before we will publish our results.                          */
        if(_config.consecutive_low_readings >= _config.consecutive_low_readings_error)
        {
           publish_measurements = true;
        }
     }
     else
     {
        LOGI("Golden Range Temp %f C, (%f C - %f C)", meas.temperature, _config.low_temperature, _config.high_temperature);

        _config.consecutive_low_readings        = 0;
        _config.consecutive_high_readings       = 0;
     }

     /* Check to see if we should publish our measurements (or if we     */
     /* should connect to wifi to set time).                             */
     if((publish_measurements) || (invalid_time))
     {
        /* Get the wifi SSID and password.                               */
        Result = _get_wifi_ssid_pasword(ssid, pass);
        if(Result)
        {
           LOGI("WiFi connect to SSID %s", ssid);

           /* Attempt to connect to wifi.                                */
           Result = wifi_connect(ssid, pass, 15);
           if(Result)
           {
              LOGI("WiFi connected to SSID %s", ssid);

              /* If we are here due to invalid time we have not stored a */
              /* timestamp for this measurement.   Since we have         */
              /* connected to wifi, and our wifi code will enable SNTP   */
              /* when doing so, go ahead and update the measurement      */
              /* timestamp here.                                         */
              if(invalid_time)
              {
                 /* store measurement timestamp.                         */
                 meas.unix_timestamp = time(NULL);
                 LOGI("current Unix time %u", meas.unix_timestamp);

                 invalid_time = false;
              }

              /* Attempt to connect shadow to get updated config data.   */
              LOGI("AWS MQTT shadow connect");
              Result = aws_mqtt_shadow_init(root_ca, cert, key, peep_uuid, 60);
              if(Result)
              {
                 LOGI("AWS MQTT shadow get timeout %d seconds", _AWS_SHADOW_GET_TIMEOUT_SEC);
                 Result = aws_mqtt_shadow_get(_shadow_callback, _AWS_SHADOW_GET_TIMEOUT_SEC);
                 if(Result)
                 {
                    /* Wait for shadow to be fetched.                    */
                    bits = 0;
                    while ((bits & SYNC_BIT) == 0)
                    {
                       /* call AWS polling function.                     */
                       aws_mqtt_shadow_poll(2500);

                       /* delay for 100 ms, not sure if this is needed   */
                       /* due to above function also delaying but leave  */
                       /* in for now.                                    */
                       vTaskDelay(100 / portTICK_PERIOD_MS);

                       bits = xEventGroupWaitBits(_sync_event_group, SYNC_BIT, false, true, 0);
                    }

                    LOGI("AWS MQTT shadow disconnect");
                    aws_mqtt_shadow_disconnect();
                 }
                 else
                 {
                    LOGE("Failed to get shadow.");
                 }
              }
              else
              {
                 LOGE("Failed to init mqtt shadow.");
              }

              /* Regardless of status above, now attempt to publish      */
              /* measurements.                                           */
              LOGI("AWS MQTT connect");
              Result = aws_mqtt_init(root_ca, cert, key, peep_uuid, 5);
              if(Result)
              {
                 LOGI("aws_mqtt_init() success.");

                 /* Go ahead and store the new measurement in flash.     */
                 LOGI("storing measurement results in internal flash");
                 memory_measurement_db_add(&meas);
                 total = memory_measurement_db_total();
                 LOGI("%d measurements stored", total);

                 /* Flag measurement is stored and does not need to be   */
                 /* stored later.                                        */
                 measurement_stored = true;

                 LOGI("publishing measurement results over MQTT");
                 Result = aws_publish_measurements(buffer, _BUFFER_LEN, (char *) _uuid_start, _config.uuid);
                 if(Result)
                 {
                    LOGI("published measurement results over MQTT");

                    /* Reset counts.                                     */
                    _config.consecutive_high_readings         = 0;
                    _config.consecutive_low_readings          = 0;
                    _config.measurements_since_last_published = 0;
                 }
                 else
                 {
                    LOGE("FAILED publishing measurement results over MQTT");
                 }
              }
              else
              {
                 LOGE("aws_mqtt_init() failed.");
              }

              /* Disconnect from WiFi.                                   */
              LOGI("WiFi disconnect");
              wifi_disconnect();
           }
           else
           {
              LOGE("Failed to connect to wifi SSID = %s.", ssid);
           }
        }
        else
        {
           LOGE("Failed to get saved SSID/Password for the wifi.");
        }
     }

     /* Store the measurement if we have not already done so.            */
     if(!measurement_stored)
     {
        /* if the time is still invalid, we must not have updated the    */
        /* time so go ahead and return current internal timer.           */
        if(invalid_time)
        {
           /* store measurement timestamp.                               */
           meas.unix_timestamp = time(NULL);
           LOGI("current Unix time %u", meas.unix_timestamp);
        }

        /* Go ahead and store the new measurement in flash.              */
        LOGI("storing measurement results in internal flash");
        memory_measurement_db_add(&meas);
        total = memory_measurement_db_total();
        LOGI("%d measurements stored", total);
     }

     /* Update flash with the config.                                    */
     memory_set_item(MEMORY_ITEM_HATCH_CONFIG, (uint8_t *) &_config, sizeof(struct hatch_configuration));

     /* Check to see if we have reach the end of measurement period.     */
     if(meas.unix_timestamp >= _config.end_unix_timestamp)
     {
        /* Change to the measure config state next.                      */
        peep_set_state(PEEP_STATE_MEASURE_CONFIG);

        /* set the configured measurement interval to 1 sec and this will*/
        /* force us to almost immediately enter the measure config state */
        /* to see if there is an updated measurement configuration.      */
        MeasurementInterval = 1;
     }
     else
     {
        /* Use default measurement interval.                             */
        MeasurementInterval = _config.measure_interval_sec;
     }

     //Note, may need a delay here.

     // feed watchdog
     vTaskDelay(100 / portTICK_PERIOD_MS);

#if defined(PEEP_TEST_STATE_MEASURE)

     /* Test function to read and print stored measurements.             */
     PrintStoredMeasurements();

#endif

ERROR:

     /* enter deep sleep.                                                */
     hal_deep_sleep_timer_and_push_button(MeasurementInterval);
  }
  else
  {
     /* shouldn't happen as we are rebooting each time this is run, but if it
        happens just deep sleep for a bit longer. */
     LOGE_TRAP("failed to allocate memory");

     hal_deep_sleep_timer_and_push_button(5);
  }
}
