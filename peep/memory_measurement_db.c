/***** Local Data *****/

#include "memory_measurement_db.h"
#include "hatch_measurement.h"
#include "system.h"

/***** Defines *****/

#define _FILE "/p/db"

/***** Local Data *****/

static SemaphoreHandle_t _mutex = NULL;
static FILE * _fp = NULL;

/***** Global Functions *****/

bool
memory_measurement_db_init(void)
{
  _mutex = xSemaphoreCreateMutex();

  return (NULL != _mutex) ? true : false;
}

bool
memory_measurement_db_add(struct hatch_measurement * p_meas)
{
  uint8_t * src = (uint8_t *) p_meas;
  uint32_t len = sizeof(struct hatch_measurement);
  int32_t s = 0;
  bool r = true;

  if (_fp) {
    r = false;
  }

  if ((r) && xSemaphoreTake(_mutex, portMAX_DELAY)) {
    if (r) {
      _fp = fopen(_FILE, "a");
      if (NULL == _fp) {
        LOGE("failed to open %s", _FILE);
        r = false;
      }
    }

    if (r) {
      s = fwrite(src, sizeof(uint8_t), len, _fp);
    }

    if (s != len) {
      LOGE("wrote %d of %d bytes", s, len);
      r = false;
    }

    if (_fp) {
      fclose(_fp);
      _fp = NULL;
    }

    xSemaphoreGive(_mutex);
  }

  return r;
}

uint32_t
memory_measurement_db_total(void)
{
  uint32_t total = 0;
  bool r = true;

  if (_fp) {
    r = false;
  }

  if ((r) && xSemaphoreTake(_mutex, portMAX_DELAY)) {
    if (r) {
      _fp = fopen(_FILE, "r");
      if (NULL == _fp) {
        LOGE("failed to open %s", _FILE);
        r = false;
      }
    }

    if (r) {
      fseek(_fp, 0L, SEEK_END);
      total = ftell(_fp);
      total = total / sizeof(struct hatch_measurement);
      fclose(_fp);
      _fp = NULL;
    }

    xSemaphoreGive(_mutex);
  }

  return total;
}

bool
memory_measurement_db_delete_all(void)
{
  bool r = true;

  if (_fp) {
    r = false;
  }

  if ((r) && xSemaphoreTake(_mutex, portMAX_DELAY)) {
    if (0 != remove(_FILE)) {
      r = false;
    }

    xSemaphoreGive(_mutex);
  }

  return r;
}

bool
memory_measurement_db_read_open(void)
{
  bool r = true;

  if (_fp) {
    r = false;
  }

  if ((r) && xSemaphoreTake(_mutex, portMAX_DELAY)) {
    if (r) {
      _fp = fopen(_FILE, "r");
      if (NULL == _fp) {
        LOGE("failed to open %s", _FILE);
        r = false;
      }
    }

    xSemaphoreGive(_mutex);
  }

  return r;
}

bool
memory_measurement_db_read_entry(struct hatch_measurement * p_meas)
{
  uint8_t * dst = (uint8_t *) p_meas;
  uint32_t len = sizeof(struct hatch_measurement);
  int32_t s = -1;
  bool r = true;

  if (NULL == _fp) {
    r = false;
  }

  if ((r) && xSemaphoreTake(_mutex, portMAX_DELAY)) {
    if (r) {
      s = fread(dst, sizeof(uint8_t), len, _fp);
    }

    xSemaphoreGive(_mutex);
  }

  return s;
}

bool
memory_measurement_db_read_close(void)
{
  bool r = true;

  if (NULL == _fp) {
    r = false;
  }

  if ((r) && xSemaphoreTake(_mutex, portMAX_DELAY)) {
    fclose(_fp);
    _fp = NULL;

    xSemaphoreGive(_mutex);
  }

  return r;
}
