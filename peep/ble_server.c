#include <string.h>

#include "freertos/FreeRTOS.h"
#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"
#include "esp_log.h"
#include "sdkconfig.h"

/***** Defines *****/

#define GATTS_SERVICE_UUID_TEST_A   0x00FF
#define GATTS_CHAR_UUID_TEST_A      0xFF01
#define GATTS_DESCR_UUID_TEST_A     0x3333
#define GATTS_NUM_HANDLE_TEST_A     4

#define TEST_DEVICE_NAME            "PEEP"
#define TEST_MANUFACTURER_DATA_LEN  17

#define GATTS_DEMO_CHAR_VAL_LEN_MAX 0x40

#define PREPARE_BUF_SIZE 1024

#define FLAG_ADV_CONFIG      (1 << 0)
#define FLAG_SCAN_RSP_CONFIG (1 << 1)

/***** Local Function Prototpyes *****/

static void
gatts_profile_0_event_handler(
  esp_gatts_cb_event_t event,
  esp_gatt_if_t gatts_if,
  esp_ble_gatts_cb_param_t *param);

/***** Structs *****/

typedef struct {
  uint8_t * buf;
  uint32_t len;
  uint32_t len_max;
} prepare_type_env_t;

struct gatts_profile_inst {
  esp_gatts_cb_t gatts_cb;
  uint16_t gatts_if;
  uint16_t app_id;
  uint16_t conn_id;
  uint16_t service_handle;
  esp_gatt_srvc_id_t service_id;
  uint16_t char_handle;
  esp_bt_uuid_t char_uuid;
  esp_gatt_perm_t perm;
  esp_gatt_char_prop_t property;
  uint16_t descr_handle;
  esp_bt_uuid_t descr_uuid;
};

/***** Local Data *****/

static void(*_callback)(uint8_t * buf, uint32_t len) = NULL;

uint8_t char1_str[] = {0x11, 0x22, 0x33};
esp_attr_value_t gatts_demo_char1_val =
{
  .attr_max_len = GATTS_DEMO_CHAR_VAL_LEN_MAX,
  .attr_len     = sizeof(char1_str),
  .attr_value   = char1_str,
};

static uint8_t adv_config_done = 0;

static uint8_t adv_service_uuid128[32] = {
  /* LSB <--------------------------------------------------------------------------------> MSB */
  //first uuid, 16bit, [12],[13] is the value
  0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xEE, 0x00, 0x00, 0x00,
  //second uuid, 32bit, [12], [13], [14], [15] is the value
  0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00,
};

// The length of adv data must be less than 31 bytes
static esp_ble_adv_data_t adv_data = {
  .set_scan_rsp = false,
  .include_name = true,
  .include_txpower = true,
  .min_interval = 0x20,
  .max_interval = 0x40,
  .appearance = 0x00,
  .manufacturer_len = 0,
  .p_manufacturer_data =  NULL,
  .service_data_len = 0,
  .p_service_data = NULL,
  .service_uuid_len = 32,
  .p_service_uuid = adv_service_uuid128,
  .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

// scan response data
static esp_ble_adv_data_t scan_rsp_data = {
  .set_scan_rsp = true,
  .include_name = true,
  .include_txpower = true,
  .min_interval = 0x20,
  .max_interval = 0x40,
  .appearance = 0x00,
  .manufacturer_len = 0,
  .p_manufacturer_data =  NULL,
  .service_data_len = 0,
  .p_service_data = NULL,
  .service_uuid_len = 32,
  .p_service_uuid = adv_service_uuid128,
  .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

static esp_ble_adv_params_t adv_params = {
  .adv_int_min        = 0x20,
  .adv_int_max        = 0x40,
  .adv_type           = ADV_TYPE_IND,
  .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
  //.peer_addr            =
  //.peer_addr_type       =
  .channel_map        = ADV_CHNL_ALL,
  .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

/* One gatt-based profile one app_id and one gatts_if, this array will store
   the gatts_if returned by ESP_GATTS_REG_EVT */
static struct gatts_profile_inst _profile = {
  .app_id = 0,
  .gatts_cb = gatts_profile_0_event_handler,
  .gatts_if = ESP_GATT_IF_NONE,
  /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
};

static prepare_type_env_t a_prepare_write_env;

/***** Local Functions *****/

static void
example_write_event_env(esp_gatt_if_t gatts_if,
  prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param)
{
  static esp_gatt_rsp_t gatt_rsp;
  esp_gatt_status_t status = ESP_GATT_OK;
  esp_err_t response_err;

  if (param->write.need_rsp) {
    if (param->write.is_prep) {
      // error checking
      if (param->write.offset > a_prepare_write_env.len_max) {
        status = ESP_GATT_INVALID_OFFSET;
      }
      else if ((param->write.offset + param->write.len) > a_prepare_write_env.len_max) {
        status = ESP_GATT_INVALID_ATTR_LEN;
      }

      ESP_LOGI(__func__, "writing %d bytes", param->write.len);
      gatt_rsp.attr_value.len = param->write.len;
      gatt_rsp.attr_value.handle = param->write.handle;
      gatt_rsp.attr_value.offset = param->write.offset;
      gatt_rsp.attr_value.auth_req = ESP_GATT_AUTH_REQ_NONE;

      memcpy(
        gatt_rsp.attr_value.value,
        param->write.value,
        param->write.len);

      response_err = esp_ble_gatts_send_response(
        gatts_if,
        param->write.conn_id,
        param->write.trans_id,
        status,
        &gatt_rsp);

      if (response_err != ESP_OK) {
         ESP_LOGE(__func__, "Send response error\n");
      }

      if (status == ESP_GATT_OK) {
        memcpy(
          prepare_write_env->buf + param->write.offset,
          param->write.value,
          param->write.len);

        prepare_write_env->len += param->write.len;
      }
    }
    else {
      esp_ble_gatts_send_response(
        gatts_if,
        param->write.conn_id,
        param->write.trans_id,
        status,
        NULL);
    }
  }
}

static void
gap_event_handler(
  esp_gap_ble_cb_event_t event,
  esp_ble_gap_cb_param_t *param)
{
  switch (event) {
  case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
    adv_config_done &= (~FLAG_ADV_CONFIG);
    if (adv_config_done == 0) {
      esp_ble_gap_start_advertising(&adv_params);
    }
    break;

  case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:
    adv_config_done &= (~FLAG_SCAN_RSP_CONFIG);
    if (adv_config_done == 0) {
      esp_ble_gap_start_advertising(&adv_params);
    }
    break;

  case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
    if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
      ESP_LOGE(__func__, "Advertising start failed\n");
    }
    break;

  case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
    if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) {
      ESP_LOGE(__func__, "Advertising stop failed\n");
    }
    else {
      ESP_LOGI(__func__, "Stop adv successfully\n");
    }
    break;

  case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
     ESP_LOGI(
      __func__,
      "update connetion params status = %d, min_int = %d, "
      "max_int = %d,conn_int = %d,latency = %d, timeout = %d",
      param->update_conn_params.status,
      param->update_conn_params.min_int,
      param->update_conn_params.max_int,
      param->update_conn_params.conn_int,
      param->update_conn_params.latency,
      param->update_conn_params.timeout);
    break;

  default:
    break;
  }
}

static void
gatts_profile_0_event_handler(esp_gatts_cb_event_t event,
  esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
  esp_gatt_rsp_t rsp;
  esp_err_t err;

  switch (event) {
  case ESP_GATTS_REG_EVT:
    ESP_LOGI(
      __func__,
      "REGISTER_APP_EVT, status %d, app_id %d\n",
      param->reg.status,
      param->reg.app_id);

    _profile.service_id.is_primary = true;
    _profile.service_id.id.inst_id = 0x00;
    _profile.service_id.id.uuid.len = ESP_UUID_LEN_16;
    _profile.service_id.id.uuid.uuid.uuid16 = GATTS_SERVICE_UUID_TEST_A;

    err = esp_ble_gap_set_device_name(TEST_DEVICE_NAME);
    if (ESP_OK != err) {
      ESP_LOGE(__func__, "%s:%d (%d)", __FILE__, __LINE__, err);
    }

    //config adv data
    err = esp_ble_gap_config_adv_data(&adv_data);
    if (ESP_OK != err) {
      ESP_LOGE(__func__, "%s:%d (%d)", __FILE__, __LINE__, err);
    }

    adv_config_done |= FLAG_ADV_CONFIG;
    //config scan response data
    err = esp_ble_gap_config_adv_data(&scan_rsp_data);
    if (err) {
      ESP_LOGE(__func__, "%s:%d (%d)", __FILE__, __LINE__, err);
    }
    adv_config_done |= FLAG_SCAN_RSP_CONFIG;

    esp_ble_gatts_create_service(
      gatts_if,
      &_profile.service_id,
      GATTS_NUM_HANDLE_TEST_A);
    break;

  case ESP_GATTS_READ_EVT:
    ESP_LOGI(
      __func__,
      "GATT_READ_EVT, conn_id %d, trans_id %d, handle %d\n",
      param->read.conn_id,
      param->read.trans_id,
      param->read.handle);

    // TODO: actual read
    memset(&rsp, 0, sizeof(esp_gatt_rsp_t));
    rsp.attr_value.handle = param->read.handle;
    rsp.attr_value.len = 4;
    rsp.attr_value.value[0] = 0xDE;
    rsp.attr_value.value[1] = 0xAD;
    rsp.attr_value.value[2] = 0xBE;
    rsp.attr_value.value[3] = 0xEF;
    esp_ble_gatts_send_response(
      gatts_if,
      param->read.conn_id,
      param->read.trans_id,
      ESP_GATT_OK,
      &rsp);
    break;

  case ESP_GATTS_WRITE_EVT:
    ESP_LOGI(
      __func__,
      "GATT_WRITE_EVT, conn_id %d, trans_id %d, handle %d",
      param->write.conn_id,
      param->write.trans_id,
      param->write.handle);

    if ((!param->write.is_prep) && (_callback)) {
      _callback(param->write.value, param->write.len);
    }
    example_write_event_env(gatts_if, &a_prepare_write_env, param);
    break;

  case ESP_GATTS_EXEC_WRITE_EVT:
    ESP_LOGI(__func__,"ESP_GATTS_EXEC_WRITE_EVT");
    esp_ble_gatts_send_response(
      gatts_if,
      param->write.conn_id,
      param->write.trans_id,
      ESP_GATT_OK,
      NULL);

    if (param->exec_write.exec_write_flag == ESP_GATT_PREP_WRITE_EXEC) {
      // All data has been transfered. Execute on data.
      if (_callback) {
        _callback(a_prepare_write_env.buf, a_prepare_write_env.len);
      }
    }
    else {
      ESP_LOGI(__func__,"ESP_GATT_PREP_WRITE_CANCEL");
    }

    // reset prepare buffer
    a_prepare_write_env.len = 0;

    break;

  case ESP_GATTS_MTU_EVT:
    ESP_LOGI(__func__, "ESP_GATTS_MTU_EVT, MTU %d", param->mtu.mtu);
    break;

  case ESP_GATTS_UNREG_EVT:
    break;

  case ESP_GATTS_CREATE_EVT:
    ESP_LOGI(
      __func__,
      "CREATE_SERVICE_EVT, status %d,  service_handle %d\n",
      param->create.status,
      param->create.service_handle);

    _profile.service_handle = param->create.service_handle;
    _profile.char_uuid.len = ESP_UUID_LEN_16;
    _profile.char_uuid.uuid.uuid16 = GATTS_CHAR_UUID_TEST_A;

    esp_ble_gatts_start_service(_profile.service_handle);
    err = esp_ble_gatts_add_char(_profile.service_handle,
                   &_profile.char_uuid,
                   ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                   ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE,
                   &gatts_demo_char1_val,
                   NULL);
    if (err) {
      ESP_LOGE(__func__, "add char failed, error code =%x", err);
    }
    break;

  case ESP_GATTS_ADD_INCL_SRVC_EVT:
    break;

  case ESP_GATTS_ADD_CHAR_EVT: {
    uint16_t length = 0;
    const uint8_t *prf_char;

    ESP_LOGI(
      __func__,
      "ADD_CHAR_EVT, status %d, attr_handle %d, service_handle %d\n",
      param->add_char.status,
      param->add_char.attr_handle,
      param->add_char.service_handle);

    _profile.char_handle = param->add_char.attr_handle;
    _profile.descr_uuid.len = ESP_UUID_LEN_16;
    _profile.descr_uuid.uuid.uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;

    err = esp_ble_gatts_get_attr_value(
      param->add_char.attr_handle,
      &length,
      &prf_char);
    if (err != ESP_OK) {
      ESP_LOGE(__func__, "%s:%d (%d)", __FILE__, __LINE__, err);
    }

    ESP_LOGI(__func__, "the gatts demo char length = %x\n", length);
    for(int i = 0; i < length; i++){
      ESP_LOGI(__func__, "prf_char[%x] =%x\n",i,prf_char[i]);
    }
    err = esp_ble_gatts_add_char_descr(
      _profile.service_handle,
       &_profile.descr_uuid,
      ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      NULL,
      NULL);
    if (err) {
      ESP_LOGE(__func__, "add char descr failed, error code =%x", err);
    }
    break;
  }

  case ESP_GATTS_ADD_CHAR_DESCR_EVT:
    _profile.descr_handle = param->add_char_descr.attr_handle;
    ESP_LOGI(
      __func__,
      "ADD_DESCR_EVT, status %d, attr_handle %d, "
      "service_handle %d\n",
      param->add_char_descr.status,
      param->add_char_descr.attr_handle,
      param->add_char_descr.service_handle);
    break;

  case ESP_GATTS_DELETE_EVT:
    break;

  case ESP_GATTS_START_EVT:
    ESP_LOGI(
      __func__,
      "SERVICE_START_EVT, status %d, service_handle %d\n",
      param->start.status,
      param->start.service_handle);
    break;

  case ESP_GATTS_STOP_EVT:
    break;

  case ESP_GATTS_CONNECT_EVT: {
    esp_ble_conn_update_params_t conn_params = {0};
    memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
    /* For the IOS system, please reference the apple official documents
    about the ble connection parameters restrictions. */
    conn_params.latency = 0;
    conn_params.max_int = 0x20;    // max_int = 0x20*1.25ms = 40ms
    conn_params.min_int = 0x10;    // min_int = 0x10*1.25ms = 20ms
    conn_params.timeout = 400;    // timeout = 400*10ms = 4000ms

    ESP_LOGI(
    __func__,
    "ESP_GATTS_CONNECT_EVT, conn_id %d, "
    "remote %02x:%02x:%02x:%02x:%02x:%02x:",
    param->connect.conn_id,
    param->connect.remote_bda[0],
    param->connect.remote_bda[1],
    param->connect.remote_bda[2],
    param->connect.remote_bda[3],
    param->connect.remote_bda[4],
    param->connect.remote_bda[5]);

    _profile.conn_id = param->connect.conn_id;
    //start sent the update connection parameters to the peer device.
    esp_ble_gap_update_conn_params(&conn_params);
    break;
  }

  case ESP_GATTS_DISCONNECT_EVT:
    ESP_LOGI(__func__, "ESP_GATTS_DISCONNECT_EVT");
    esp_ble_gap_start_advertising(&adv_params);
    break;

  case ESP_GATTS_CONF_EVT:
    ESP_LOGI(__func__,
         "ESP_GATTS_CONF_EVT, status %d",
         param->conf.status);
    
    if (param->conf.status != ESP_GATT_OK) {
      esp_log_buffer_hex(__func__, param->conf.value, param->conf.len);
    }
    break;

  case ESP_GATTS_OPEN_EVT:
  case ESP_GATTS_CANCEL_OPEN_EVT:
  case ESP_GATTS_CLOSE_EVT:
  case ESP_GATTS_LISTEN_EVT:
  case ESP_GATTS_CONGEST_EVT:
  default:
    break;
  }
}

static void
gatts_event_handler(esp_gatts_cb_event_t event,
  esp_gatt_if_t gatts_if,
  esp_ble_gatts_cb_param_t *param)
{
  /* If event is register event, store the gatts_if for each profile */
  if (event == ESP_GATTS_REG_EVT) {
    if ((param->reg.status == ESP_GATT_OK) && (param->reg.app_id == _profile.app_id)) {
      _profile.gatts_if = gatts_if;
    }
    else {
      ESP_LOGI(__func__, "Reg app failed, app_id %04x, status %d\n",
          param->reg.app_id,
          param->reg.status);
      return;
    }
  }

  /* If the gatts_if equal to profile A, call profile A cb handler.
  NOTE: ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every
  profile cb function. */
  if ((gatts_if == _profile.gatts_if) || (gatts_if == ESP_GATT_IF_NONE)) {
    // make sure callback is registered
    if (_profile.gatts_cb) {
      _profile.gatts_cb(event, gatts_if, param);
    }
  }
}

/***** Global Functions *****/

void
ble_register_write_callback(void(*callback)(uint8_t * buf, uint32_t len))
{
  _callback = callback;
}

bool
ble_enable(uint8_t * buf, uint32_t len)
{
  esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
  static bool run_once = true;
  esp_err_t err = ESP_OK;

  if (run_once) {
    // From comment in esp_bt.h API, this function should only be called once.
    run_once = false;
    err = esp_bt_controller_init(&bt_cfg);
    if (err) {
      printf("%s:%d\n", __func__, __LINE__);
      return false;
    }
  }

  err = esp_bt_controller_enable(ESP_BT_MODE_BTDM);
  if (err) {
    printf("%s:%d\n", __func__, __LINE__);
    return false;
  }

  err = esp_bluedroid_init();
  if (err) {
    printf("%s:%d\n", __func__, __LINE__);
    return false;
  }

  err = esp_bluedroid_enable();
  if (err) {
    printf("%s:%d\n", __func__, __LINE__);
    return false;
  }

  err = esp_ble_gatts_register_callback(gatts_event_handler);
  if (err) {
    printf("%s:%d\n", __func__, __LINE__);
    return false;
  }

  err = esp_ble_gap_register_callback(gap_event_handler);
  if (err) {
    printf("%s:%d\n", __func__, __LINE__);
    return false;
  }

  err = esp_ble_gatts_app_register(_profile.app_id);
  if (err) {
    printf("%s:%d\n", __func__, __LINE__);
    return false;
  }

  err = esp_ble_gatt_set_local_mtu(500);
  if (err) {
    printf("%s:%d\n", __func__, __LINE__);
    return false;
  }

  a_prepare_write_env.buf = buf;
  a_prepare_write_env.len_max = len;
  a_prepare_write_env.len = 0;

  return true;
}

bool
ble_disable(void)
{
  esp_err_t err = ESP_OK;

  err = esp_bluedroid_disable();
  if (ESP_OK != err) {
    printf("%s:%d\n", __func__, __LINE__);
    // keep rolling on...
  }

  _callback = NULL;
  a_prepare_write_env.buf = NULL;
  a_prepare_write_env.len_max = 0;
  a_prepare_write_env.len = 0;

  err = esp_bt_controller_disable();
  if (ESP_OK != err) {
    printf("%s:%d\n", __func__, __LINE__);
    return false;
  }

  return true;
}
