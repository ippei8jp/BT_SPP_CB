/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "nvs.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
#include "esp_bt_device.h"
#include "esp_spp_api.h"

#include "spp_test.h"
#include "spp_user_hdr.h"
#include "bt_utils.h"
#include "uart_console.h"

#define TAG                 __func__


static bool get_name_from_eir(uint8_t *eir, char *bdname, uint8_t *bdname_len);
static void pin_req_input(esp_bt_gap_cb_param_t *param, esp_bt_pin_code_t pin_code);
#if     CONFIG_BT_SSP_ENABLED
static int cfn_req_input(esp_bt_gap_cb_param_t *param);
static uint32_t key_req_input(esp_bt_gap_cb_param_t *param);
#endif  // CONFIG_BT_SSP_ENABLED
static char* esp_gap_event_to_str(esp_bt_gap_cb_event_t event);
static char* esp_gap_prop_to_str(esp_bt_gap_dev_prop_type_t prop);

void esp_bt_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{
    ESP_LOGV(TAG, "GAP_CB_EVT: %s(%d)", esp_gap_event_to_str(event), event);
    switch (event) {
      case ESP_BT_GAP_AUTH_CMPL_EVT:                             // Authentication complete event  認証完了
        ESP_LOGV(TAG, "    BD_ADDR        : %s", bdaddr_to_str(param->auth_cmpl.bda, NULL));
        ESP_LOGV(TAG, "    authentication : %s", (param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS) ? "success" : "fail");
        ESP_LOGV(TAG, "    status         : %d", param->auth_cmpl.stat);
        ESP_LOGV(TAG, "    device         : %s", param->auth_cmpl.device_name);
        break;

      case ESP_BT_GAP_PIN_REQ_EVT:           // Legacy Pairing Pin code request 
        ESP_LOGV(TAG, "    BD_ADDR      : %s", bdaddr_to_str(param->pin_req.bda, NULL));
        ESP_LOGV(TAG, "    min_16_digit : %s", param->pin_req.min_16_digit ? "true" : "false");
        {
            // PINコードの入力取得
            esp_bt_pin_code_t pin_code;
            pin_req_input(param, pin_code);
            esp_bt_gap_pin_reply(param->pin_req.bda, true, param->pin_req.min_16_digit ? 16 : 4, pin_code);
        }
        break;

#if CONFIG_BT_SSP_ENABLED
      case ESP_BT_GAP_CFM_REQ_EVT:            // Security Simple Pairing User Confirmation request. 
        ESP_LOGV(TAG, "    BD_ADDR : %s", bdaddr_to_str(param->cfm_req.bda, NULL));
        {
            // 数値の確認
            int key_in;
            printf("**** the passkey Notify number : %d\n", param->cfm_req.num_val);
            key_in = cfn_req_input(param);
            esp_bt_gap_ssp_confirm_reply(param->cfm_req.bda, (key_in == 'y') ? true : false);
        }
        break;

      case ESP_BT_GAP_KEY_NOTIF_EVT:          // Security Simple Pairing Passkey Notification 
        ESP_LOGV(TAG, "    BD_ADDR : %s", bdaddr_to_str(param->key_notif.bda, NULL));
        {
            printf("**** passkey : %d\n", param->key_notif.passkey);
        }
        break;

      case ESP_BT_GAP_KEY_REQ_EVT:            // Security Simple Pairing Passkey request 本当はpasskey入力ルーチンが必要
        ESP_LOGV(TAG, "    BD_ADDR : %s", bdaddr_to_str(param->key_req.bda, NULL));
        {
            uint32_t    passkey;
            passkey = key_req_input(param);
            esp_bt_gap_ssp_passkey_reply(param->key_req.bda, true, passkey);
        }
        break;
#endif  // CONFIG_BT_SSP_ENABLED

      case ESP_BT_GAP_MODE_CHG_EVT:
        ESP_LOGV(TAG, "    BD_ADDR : %s", bdaddr_to_str(param->mode_chg.bda, NULL));
        ESP_LOGV(TAG, "    mode    : %d", param->mode_chg.mode);
        break;

      case ESP_BT_GAP_CONFIG_EIR_DATA_EVT :              // Config EIR(Extended Inquiry Response) data event
        ESP_LOGV(TAG, "    stat         : %d", param->config_eir_data.stat);
        ESP_LOGV(TAG, "    eir_type_num : %d", param->config_eir_data.eir_type_num);
        //     以下で表示されるのが設定された EIR Data Type
        //     値の意味については、 %HOMEPATH%\.platformio\packages\framework-espidf\components\bt\host\bluedroid\api\include\api\esp_gap_bt_api.h を参照
        //         ESP_BT_EIR_TYPE_*** のdefine文
        esp_log_buffer_hex(TAG,                param->config_eir_data.eir_type, param->config_eir_data.eir_type_num);
        break;

      case ESP_BT_GAP_REMOVE_BOND_DEV_COMPLETE_EVT :     // ペアリング解除したとき
        ESP_LOGV(TAG, "    BD_ADDR : %s", bdaddr_to_str(param->remove_bond_dev_cmpl.bda, NULL));
        ESP_LOGV(TAG, "    status  : %d", param->remove_bond_dev_cmpl.status);
        break;

      case ESP_BT_GAP_DISC_RES_EVT :                     //       Device discovery result event
        ESP_LOGV(TAG, "    BD_ADDR  : %s", bdaddr_to_str(param->disc_res.bda, NULL));
        ESP_LOGV(TAG, "    num_prop : %d", param->disc_res.num_prop);
        for (int i = 0; i < param->disc_res.num_prop; i++) {
            ESP_LOGV(TAG, "      %d :  %s      %d", i, esp_gap_prop_to_str(param->disc_res.prop[i].type), param->disc_res.prop[i].len);
            switch(param->disc_res.prop[i].type) {
              case ESP_BT_GAP_DEV_PROP_BDNAME:      //  Bluetooth device name, value type is int8_t []
                // これが出力されてる機器ってあるのかな? Windowsでは出力されていないみたい
                ESP_LOGV(TAG, "        %d    '%s'", param->disc_res.prop[i].len, (char*)param->disc_res.prop[i].val);
                // esp_log_buffer_char(TAG, param->disc_res.prop[i].val, param->disc_res.prop[i].len);
                break;
              case ESP_BT_GAP_DEV_PROP_COD:         //  Class of Device, value type is uint32_t
                ESP_LOGV(TAG, "        %d    0x%06x", param->disc_res.prop[i].len, *(uint32_t*)param->disc_res.prop[i].val);
                break;
              case ESP_BT_GAP_DEV_PROP_RSSI:        //  Received Signal strength Indication, value type is int8_t, ranging from -128 to 127 
                ESP_LOGV(TAG, "        %d    %d",     param->disc_res.prop[i].len, *(int8_t*)param->disc_res.prop[i].val);
                break;
              case ESP_BT_GAP_DEV_PROP_EIR:         //  Extended Inquiry Response, value type is uint8_t [] 
                // データをダンプしてみる
                //     データ構造については、
                //         https://www.bluetooth.org/docman/handlers/downloaddoc.ashx?doc_id=478726
                //         の Vol-3 Part-C Section-8 を参照。データサイズは240バイト固定(マクロ参照できないので数値で書いておく)
                //     EIR Data Typeについては、 %HOMEPATH%\.platformio\packages\framework-espidf\components\bt\host\bluedroid\api\include\api\esp_gap_bt_api.h を参照
                //         ESP_BT_EIR_TYPE_*** のdefine文
                ESP_LOG_BUFFER_HEXDUMP(TAG, param->disc_res.prop[i].val, 240/* HCI_EXT_INQ_RESPONSE_LEN */, ESP_LOG_VERBOSE);
                ;   // ↑の行をコメントアウトするとエラーになるので空行を入れておく
                char bdname[ESP_BT_GAP_MAX_BDNAME_LEN + 1];
                uint8_t bdname_len;
                bool eir_ret = get_name_from_eir(param->disc_res.prop[i].val, bdname, &bdname_len);
                if (eir_ret) {
                    ESP_LOGV(TAG, "        %d    '%s'", bdname_len, bdname);
                } else {
                    ESP_LOGV(TAG, "        **FAILED**");
                }
#ifdef  SPP_CLIENT_MODE         // SPP クライアントモード
                if (strlen(remote_device_name) == bdname_len && strncmp(bdname, remote_device_name, bdname_len) == 0) {
                    memcpy(host_bd_address, param->disc_res.bda, ESP_BD_ADDR_LEN);
                    found_bd_addr   = true;
#if 0   // 元のサンプルプログラムの処理は削除
                    esp_spp_start_discovery(host_bd_address);
                    esp_bt_gap_cancel_discovery();
#endif
                }
#endif  // SPP_CLIENT_MODE
                break;
              default :
                break;
            }
        }
        break;

      case ESP_BT_GAP_DISC_STATE_CHANGED_EVT :           //       Discovery state changed event
        ESP_LOGV(TAG, "    state : %d", param->disc_st_chg.state);
        break;

      case ESP_BT_GAP_RMT_SRVCS_EVT :                    //       Get remote services event
        ESP_LOGV(TAG, "    BD_ADDR   : %s", bdaddr_to_str(param->rmt_srvcs.bda, NULL));
        ESP_LOGV(TAG, "    stat      : %d", param->rmt_srvcs.stat);
        ESP_LOGV(TAG, "    num_uuids : %d", param->rmt_srvcs.num_uuids);
        // param->rmt_srvcs.uuid_listのデータ構成がどうなってるかわからん...
        break;

      case ESP_BT_GAP_RMT_SRVC_REC_EVT :                 //       Get remote service record event
        ESP_LOGV(TAG, "    BD_ADDR : %s", bdaddr_to_str(param->rmt_srvc_rec.bda, NULL));
        ESP_LOGV(TAG, "    stat    : %d", param->rmt_srvc_rec.stat);
        break;

      case ESP_BT_GAP_READ_RSSI_DELTA_EVT :              //       Read rssi event
        ESP_LOGV(TAG, "    BD_ADDR    : %s", bdaddr_to_str(param->read_rssi_delta.bda, NULL));
        ESP_LOGV(TAG, "    stat       : %d", param->read_rssi_delta.stat);
        ESP_LOGV(TAG, "    rssi_delta : %d", param->read_rssi_delta.rssi_delta);
        break;

      case ESP_BT_GAP_SET_AFH_CHANNELS_EVT :             //       Set AFH channels event
        ESP_LOGV(TAG, "    stat       : %d", param->set_afh_channels.stat);
        break;

      case ESP_BT_GAP_READ_REMOTE_NAME_EVT :             //      Read Remote Name event
        ESP_LOGV(TAG, "    stat       : %d", param->read_rmt_name.stat);
        ESP_LOGV(TAG, "    rmt_name   : %s", param->read_rmt_name.rmt_name);
        break;

      case ESP_BT_GAP_QOS_CMPL_EVT :                     //        QOS complete event
        ESP_LOGV(TAG, "    BD_ADDR    : %s", bdaddr_to_str(param->qos_cmpl.bda, NULL));
        ESP_LOGV(TAG, "    stat       : %d", param->qos_cmpl.stat);
        ESP_LOGV(TAG, "    t_poll     : %d", param->qos_cmpl.t_poll);
        break;

      default: 
        ESP_LOGV(TAG, "    **** not handled ****");
        break;
    }
    return;
}

// ================================================================================================
// EIR(Extended Inquiry Response)データからデバイス名を取得
// ================================================================================================
static bool get_name_from_eir(uint8_t *eir, char *bdname, uint8_t *bdname_len)
{
    uint8_t *rmt_bdname = NULL;
    uint8_t rmt_bdname_len = 0;

    if (!eir) {
        return false;
    }

    rmt_bdname = esp_bt_gap_resolve_eir_data(eir, ESP_BT_EIR_TYPE_CMPL_LOCAL_NAME, &rmt_bdname_len);
    if (!rmt_bdname) {
        rmt_bdname = esp_bt_gap_resolve_eir_data(eir, ESP_BT_EIR_TYPE_SHORT_LOCAL_NAME, &rmt_bdname_len);
    }

    if (rmt_bdname) {
        if (rmt_bdname_len > ESP_BT_GAP_MAX_BDNAME_LEN) {
            rmt_bdname_len = ESP_BT_GAP_MAX_BDNAME_LEN;
        }

        if (bdname) {
            memcpy(bdname, rmt_bdname, rmt_bdname_len);
            bdname[rmt_bdname_len] = '\0';
        }
        if (bdname_len) {
            *bdname_len = rmt_bdname_len;
        }
        return true;
    }

    return false;
}



static void pin_req_input(esp_bt_gap_cb_param_t *param, esp_bt_pin_code_t pin_code)
{
#if 1
        if (param->pin_req.min_16_digit) {
            // とりあえず固定値を返しておく
            printf("**** input pincode(16 digits) :  0000 0000 0000 0000\n");
            memcpy(pin_code, "0000000000000000", 16);      // NULL terminateは不要
        }
        else {
            // とりあえず固定値を返しておく
            printf("**** input pincode(4 digits) : 1234\n");
            memcpy(pin_code, "1234", 4);                   // NULL terminateは不要
        }
#else
        // 本来はこんな感じ
        char pin_code_buff[17];
        int pin_code_len;
        if (param->pin_req.min_16_digit) {
            do {
                printf("**** input pincode(16 digits) : ");
                fflush(stdout);
                pin_code_len = uart_gets(pin_code_buff, sizeof(pin_code_buff));
            } while (passkey_len != 16);
            memcpy(pin_code, pin_code_buff, 16);      // NULL terminateは不要
        }
        else {
            do {
                printf("**** input pincode(4 digits) : ");
                fflush(stdout);
                pin_code_len = uart_gets(pin_code_buff, sizeof(pin_code_buff));
            } while (passkey_len != 4);
            memcpy(pin_code, pin_code_buff, 4);      // NULL terminateは不要
        }
#endif
    return;
}


#if CONFIG_BT_SSP_ENABLED

static int cfn_req_input(esp_bt_gap_cb_param_t *param)
{
    int key_in;

#if 1
    // とりあえずyが入力されたとみなす
    printf("**** Accept? (y/n) : y\n");
    key_in = 'y';
#else
    // 本来はこんな感じ
    printf("**** Accept? (y/n) : ");
    fflush(stdout);
    key_in = uart_getchar();
    printf("%c\n", key_in);
#endif
    return key_in;
}

static uint32_t key_req_input(esp_bt_gap_cb_param_t *param)
{
    uint32_t    passkey;
#if 1
    // とりあえず固定値を返しておく
    printf("**** input paskey : 123456\n");
    passkey = 123456;
#else
    // 本来はこんな感じ
    char        passkey_buff[16];
    int         passkey_len;
    do {
        printf("**** input paskey : ");
        fflush(stdout);
        passkey_len = uart_gets(passkey_buff, sizeof(passkey_buff));
    } while (passkey_len == 0);
    passkey = (uint32_t)strtol(passkey_buff, NULL, 10);
#endif
    return passkey;
}
#endif  // CONFIG_BT_SSP_ENABLED

// ================================================================================================
// Bluetooth GAP event→文字列変換(デバッグ用)
// ================================================================================================
static char* esp_gap_event_to_str(esp_bt_gap_cb_event_t event)
{
    switch(event) {
      case ESP_BT_GAP_DISC_RES_EVT :                   return "ESP_BT_GAP_DISC_RES_EVT";                    //       Device discovery result event
      case ESP_BT_GAP_DISC_STATE_CHANGED_EVT :         return "ESP_BT_GAP_DISC_STATE_CHANGED_EVT";          //       Discovery state changed event
      case ESP_BT_GAP_RMT_SRVCS_EVT :                  return "ESP_BT_GAP_RMT_SRVCS_EVT";                   //       Get remote services event
      case ESP_BT_GAP_RMT_SRVC_REC_EVT :               return "ESP_BT_GAP_RMT_SRVC_REC_EV";                 //       Get remote service record event
      case ESP_BT_GAP_AUTH_CMPL_EVT :                  return "ESP_BT_GAP_AUTH_CMPL_EVT";                   // ✔    Authentication complete event
      case ESP_BT_GAP_PIN_REQ_EVT :                    return "ESP_BT_GAP_PIN_REQ_EVT";                     // ✔    Legacy Pairing Pin code request
      case ESP_BT_GAP_CFM_REQ_EVT :                    return "ESP_BT_GAP_CFM_REQ_EVT";                     // ✔    Security Simple Pairing User Confirmation request.
      case ESP_BT_GAP_KEY_NOTIF_EVT :                  return "ESP_BT_GAP_KEY_NOTIF_EVT";                   // ✔    Security Simple Pairing Passkey Notification
      case ESP_BT_GAP_KEY_REQ_EVT :                    return "ESP_BT_GAP_KEY_REQ_EVT";                     // ✔    Security Simple Pairing Passkey request
      case ESP_BT_GAP_READ_RSSI_DELTA_EVT :            return "ESP_BT_GAP_READ_RSSI_DELTA_EVT";             //       Read rssi event
      case ESP_BT_GAP_CONFIG_EIR_DATA_EVT :            return "ESP_BT_GAP_CONFIG_EIR_DATA_EVT";             // ▲    Config EIR data event
      case ESP_BT_GAP_SET_AFH_CHANNELS_EVT :           return "ESP_BT_GAP_SET_AFH_CHANNELS_EVT";            //       Set AFH channels event
      case ESP_BT_GAP_READ_REMOTE_NAME_EVT :           return "ESP_BT_GAP_READ_REMOTE_NAME_EVT";            //      Read Remote Name event
      case ESP_BT_GAP_MODE_CHG_EVT :                   return "ESP_BT_GAP_MODE_CHG_EVT";                    // ✔    パワマネ関連のイベント
      case ESP_BT_GAP_REMOVE_BOND_DEV_COMPLETE_EVT :   return "ESP_BT_GAP_REMOVE_BOND_DEV_COMPLETE_EVT";    // ▲    ペアリング解除したとき
      case ESP_BT_GAP_QOS_CMPL_EVT :                   return "ESP_BT_GAP_QOS_CMPL_EVT";                    //        QOS complete event
      default:  break;
    }
    return "UNKNOWN EVENT";
}


static char* esp_gap_prop_to_str(esp_bt_gap_dev_prop_type_t prop)
{
    switch(prop) {
      case ESP_BT_GAP_DEV_PROP_BDNAME :     return "ESP_BT_GAP_DEV_PROP_BDNAME";    //  Bluetooth device name, value type is int8_t []
      case ESP_BT_GAP_DEV_PROP_COD    :     return "ESP_BT_GAP_DEV_PROP_COD   ";    //  Class of Device, value type is uint32_t
      case ESP_BT_GAP_DEV_PROP_RSSI   :     return "ESP_BT_GAP_DEV_PROP_RSSI  ";    //  Received Signal strength Indication, value type is int8_t, ranging from -128 to 127 
      case ESP_BT_GAP_DEV_PROP_EIR    :     return "ESP_BT_GAP_DEV_PROP_EIR   ";    //  Extended Inquiry Response, value type is uint8_t [] 
      default : break;
    }
    return "UNKNOWN PROP";
}
