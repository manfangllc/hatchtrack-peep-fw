/***** Includes *****/

#include "json_parse.h"
#include "jsmn.h"
#include "system.h"

/***** Global Functions *****/

bool
json_parse_wifi_credentials_msg(char * js, char * ssid, uint32_t ssid_max_len,
  char * pass, uint32_t pass_max_len)
{
  static char key[128];
  static char value[128];
  jsmntok_t t[16];
  jsmn_parser p;
  jsmntok_t json_value;
  jsmntok_t json_key;
  int string_length;
  int key_length;
  int idx;
  int n;
  int r;

  jsmn_init(&p);

  r = jsmn_parse(&p, js, strlen(js), t, sizeof(t)/(sizeof(t[0])));

   /* Assume the top-level element is an object */
  if ((r < 1) || (t[0].type != JSMN_OBJECT)) {
    printf("Object expected\n");
    return false;
  }

  for (n = 1; n < r; n++) {
    json_value = t[n+1];
    json_key = t[n];
    string_length = json_value.end - json_value.start;
    key_length = json_key.end - json_key.start;

    for (idx = 0; idx < string_length; idx++){
      value[idx] = js[json_value.start + idx];
    }

    for (idx = 0; idx < key_length; idx++){
      key[idx] = js[json_key.start + idx];
    }

    value[string_length] = '\0';
    key[key_length] = '\0';
    LOGI("decoded: %s = %s\n", key, value);

    if (0 == strcasecmp(key, "wifiSSID")) {
      strncpy(ssid, value, ssid_max_len);
    }
    else if (0 == strcasecmp(key, "wifiPassword")) {
      strncpy(pass, value, pass_max_len);
    }

    n++;
  }

  return true;
}

bool
json_parse_hatch_config_msg(char * js, struct hatch_configuration * config)
{
  static char key[128];
  static char value[128];
  jsmntok_t t[16];
  jsmn_parser p;
  jsmntok_t json_value;
  jsmntok_t json_key;
  int string_length;
  int key_length;
  int idx;
  int n;
  int r;

  jsmn_init(&p);

  r = jsmn_parse(&p, js, strlen(js), t, sizeof(t)/(sizeof(t[0])));

   /* Assume the top-level element is an object */
  if ((r < 1) || (t[0].type != JSMN_OBJECT)) {
    printf("Object expected\n");
    return false;
  }

  for (n = 1; n < r; n++) {
    json_value = t[n+1];
    json_key = t[n];
    string_length = json_value.end - json_value.start;
    key_length = json_key.end - json_key.start;

    for (idx = 0; idx < string_length; idx++){
      value[idx] = js[json_value.start + idx];
    }

    for (idx = 0; idx < key_length; idx++){
      key[idx] = js[json_key.start + idx];
    }

    value[string_length] = '\0';
    key[key_length] = '\0';
    LOGI("decoded: %s = %s\n", key, value);

    if (0 == strcmp(key, "hatchUUID")) {
      strncpy(config->uuid, value, UUID_BUF_LEN);
    }
    else if (0 == strcmp(key, "endUnixTimestamp")) {
      config->end_unix_timestamp = strtol(value, NULL, 0);
    }
    else if (0 == strcmp(key, "measureIntervalSec")) {
      config->measure_interval_sec = strtol(value, NULL, 0);
    }

    n++;
  }

  return true;
}
