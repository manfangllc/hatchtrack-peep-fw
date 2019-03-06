/***** Includes *****/

#include <string.h>
#include "system.h"
#include "memory.h"
#include "esp_spiffs.h"

/***** Local Data *****/

static SemaphoreHandle_t _mutex;

static char * _file_lut[] = {
  /*
   * Max file length is 16 characters, including zero termination character.
   * This leaves 15 characters for each file name. Visual guide below...
   *
  ________MAX_LEN
  */
  NULL, // MEMORY_ITEM_INVALID
  "/p/test", // MEMORY_ITEM_TEST
  "/p/state", // MEMORY_ITEM_STATE
  "/p/data", // MEMORY_ITEM_DATA_LOG
  "/p/ssid", // MEMORY_ITEM_WIFI_SSID
  "/p/pass", // MEMORY_ITEM_WIFI_PASS
  "/p/mcnt", // MEMORY_ITEM_MEASURE_COUNT
  "/p/msec", // MEMORY_ITEM_MEASURE_SEC
};

/***** Global Functions *****/

bool
memory_init(void)
{
  bool r = true;
  esp_err_t err = ESP_OK;
  esp_vfs_spiffs_conf_t conf = {
    .base_path = "/p",
    .partition_label = NULL,
    .max_files = 2,
    .format_if_mount_failed = true
  };

  if (r) {
    // Use settings defined above to initialize and mount SPIFFS filesystem.
    // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
    err = esp_vfs_spiffs_register(&conf);
    if (err == ESP_FAIL) {
      printf("Failed to mount or format filesystem\n");
      r = false;
    }
    else if (err == ESP_ERR_NOT_FOUND) {
      printf("Failed to find SPIFFS partition\n");
      r = false;
    }
    else if (err != ESP_OK) {
      printf("Failed to initialize SPIFFS (%d)\n", err);
      r = false;
    }
  }

  if (r) {
    _mutex = xSemaphoreCreateMutex();
  }

  // TODO: Remove this...
  if (r) {
    size_t total = 0, used = 0;
    err = esp_spiffs_info(NULL, &total, &used);
    if (err != ESP_OK) {
      printf("Failed to get SPIFFS partition information\n");
    }
    else {
      printf("Partition size: total: %d, used: %d\n", total, used);
    }
  }

  return r;
}

int32_t
memory_get_item(enum memory_item item, uint8_t * dst, uint32_t len)
{
  FILE * fp = NULL;
  int32_t s = -1;
  bool r = true;

  if (item <= MEMORY_ITEM_INVALID) {
    printf("item = %d\n", item);
    r = false;
  }

  if (xSemaphoreTake(_mutex, portMAX_DELAY)) {
    if (r) {
      fp = fopen(_file_lut[item], "r");
      if (!fp) {
        printf("failed to open %s\n", _file_lut[item]);
        r = false;
      }
    }

    if (r) {
      s = fread(dst, sizeof(uint8_t), len, fp);
    }

    if (fp) {
      fclose(fp);
    }
    xSemaphoreGive(_mutex);
  }

  return s;
}

int32_t
memory_set_item(enum memory_item item, uint8_t * src, uint32_t len)
{
  FILE * fp = NULL;
  int32_t s = -1;
  bool r = true;

  if (item <= MEMORY_ITEM_INVALID) {
    r = false;
  }

  if (xSemaphoreTake(_mutex, portMAX_DELAY)) {
    if (r) {
      fp = fopen(_file_lut[item], "w");
      if (NULL == fp) {
        printf("failed to open %s\n", _file_lut[item]);
        r = false;
      }
    }

    if (r) {
      s = fwrite(src, sizeof(uint8_t), len, fp);
    }

    if (fp) {
      fclose(fp);
    }
    xSemaphoreGive(_mutex);
  }

  return s;
}

bool
memory_delete_item(enum memory_item item)
{
  bool r = false;

  if (xSemaphoreTake(_mutex, portMAX_DELAY)) {
    if (0 != remove(_file_lut[item])) {
      r = false;
    }
    xSemaphoreGive(_mutex);
  }

  return r;
}
