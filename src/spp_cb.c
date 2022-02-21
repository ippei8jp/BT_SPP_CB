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


static char* esp_spp_event_to_str(esp_spp_cb_event_t event);


void esp_spp_cb(esp_spp_cb_event_t event, esp_spp_cb_param_t *param)                   // SPPイベントのコールバッグ
{
    ESP_LOGV(TAG, "SPP_CB_EVT: %s(%d)", esp_spp_event_to_str(event), event);

    switch (event) {
    case ESP_SPP_INIT_EVT:                                  // 初期化完了
        ESP_LOGV(TAG, "    status : %d", param->init.status);
        if (param->init.status == ESP_SPP_SUCCESS) {
#ifdef  SPP_CLIENT_MODE         // SPP クライアントモード
#else  // SPP_CLIENT_MODE
            // SPPサーバのスタート
            esp_spp_start_srv(ESP_SPP_SEC_AUTHENTICATE, ESP_SPP_ROLE_SLAVE, 0, SPP_SERVER_NAME);
            /*
                第1パラメータ
                    ESP_SPP_SEC_NONE
                    ESP_SPP_SEC_AUTHORIZE
                    ESP_SPP_SEC_AUTHENTICATE 
                第2パラメータ
                    ESP_SPP_ROLE_MASTER         initiatorのとき
                    ESP_SPP_ROLE_SLAVE          Accepterのとき
                第3パラメータ
                    取得するチャネル
                    0: 任意のチャネル
                第4パラメータ
                    サーバ名
            */
#endif  // SPP_CLIENT_MODE
        }
        break;
    case ESP_SPP_UNINIT_EVT:
        ESP_LOGV(TAG, "    status : %d", param->uninit.status);
        break;
    case ESP_SPP_DISCOVERY_COMP_EVT:
        ESP_LOGV(TAG, "    status  ; %d", param->disc_comp.status);
        ESP_LOGV(TAG, "    scn_num : %d", param->disc_comp.scn_num);
        for (int i = 0; i < (int)param->disc_comp.scn_num; i++) {
            ESP_LOGV(TAG, "      [%d]scn          : %d", i, param->disc_comp.scn[i]);
            ESP_LOGV(TAG, "      [%d]service_name : %s", i, param->disc_comp.service_name[i]);
        }
#ifdef  SPP_CLIENT_MODE         // SPP クライアントモード
        if (param->disc_comp.status == ESP_SPP_SUCCESS) {
            if (param->disc_comp.scn_num >= 1) {
                found_scn1            = true;
                host_service_channel1  =  param->disc_comp.scn[0];
            }
            if (param->disc_comp.scn_num >= 2) {        // 2個以上あったら2個目も記憶しておく
                found_scn2            = true;
                host_service_channel2 =  param->disc_comp.scn[1];
            }
        }
#endif  // SPP_CLIENT_MODE
        break;
    case ESP_SPP_OPEN_EVT:
        ESP_LOGV(TAG, "    BD_ADDR : %s", bdaddr_to_str(param->open.rem_bda, NULL));
        ESP_LOGV(TAG, "    status  : %d", param->open.status);
        ESP_LOGV(TAG, "    handle  : %d", param->open.handle);
        ESP_LOGV(TAG, "    fd      : %d", param->open.fd);
#ifdef  SPP_CLIENT_MODE         // SPP クライアントモード
        if (param->open.status == ESP_SPP_SUCCESS) {
            // オープンユーザハンドラ
            spp_open_handler(param->open.handle, param->open.fd, param->open.rem_bda);
        }
#endif  // SPP_CLIENT_MODE
        break;
    case ESP_SPP_CLOSE_EVT:                                 // クローズ時
        ESP_LOGV(TAG, "    status       : %d", param->close.status);
        ESP_LOGV(TAG, "    port_status  : %d", param->close.port_status);
        ESP_LOGV(TAG, "    handle       : %d", param->close.handle);
        ESP_LOGV(TAG, "    async        : %s", param->close.async ? "true" : "false");
        if (param->close.status == ESP_SPP_SUCCESS) {
            // クローズユーザハンドラ
            spp_close_handler(param->close.handle);
        }
        break;
    case ESP_SPP_START_EVT:
        ESP_LOGV(TAG, "    status : %d", param->start.status);
        ESP_LOGV(TAG, "    handle : %d", param->start.handle);
        ESP_LOGV(TAG, "    sec_id : %d", param->start.sec_id);
        ESP_LOGV(TAG, "    scn    : %d", param->start.scn);
        ESP_LOGV(TAG, "    use_co : %s", param->start.use_co ? "true" : "false");
        break;
    case ESP_SPP_CL_INIT_EVT:
        ESP_LOGV(TAG, "    status : %d", param->cl_init.status);
        ESP_LOGV(TAG, "    handle : %d", param->cl_init.handle);
        ESP_LOGV(TAG, "    sec_id : %d", param->cl_init.sec_id);
        ESP_LOGV(TAG, "    use_co : %s", param->cl_init.use_co ? "true" : "false");
        break;
    case ESP_SPP_SRV_OPEN_EVT:                              // オープン時
        ESP_LOGV(TAG, "    BD_ADDR           : %s", bdaddr_to_str(param->srv_open.rem_bda, NULL));
        ESP_LOGV(TAG, "    status            : %d", param->srv_open.status);
        ESP_LOGV(TAG, "    handle            : %d", param->srv_open.handle);
        ESP_LOGV(TAG, "    new_listen_handle : %d", param->srv_open.new_listen_handle);
        ESP_LOGV(TAG, "    fd                : %d", param->srv_open.fd);
#ifdef  SPP_CLIENT_MODE         // SPP クライアントモード
#else   // SPP_CLIENT_MODE
        if (param->srv_open.status == ESP_SPP_SUCCESS) {
            // オープンユーザハンドラ
            spp_open_handler(param->srv_open.handle, param->srv_open.fd, param->srv_open.rem_bda);
        }
#endif  // SPP_CLIENT_MODE
        break;
      case ESP_SPP_DATA_IND_EVT :           // callbackモード時のみ
        ESP_LOGV(TAG, "    status : %d", param->data_ind.status);
        ESP_LOGV(TAG, "    handle : %d", param->data_ind.handle);
        ESP_LOGV(TAG, "    len    : %d", param->data_ind.len);
        esp_log_buffer_hex(TAG,          param->data_ind.data,param->data_ind.len);
        if (param->data_ind.status == ESP_SPP_SUCCESS) {
            // 受信ユーザハンドラ
            spp_read_handler(param->data_ind.handle, param->data_ind.data, param->data_ind.len);
        }
        break;
      case ESP_SPP_CONG_EVT :               // callbackモード時のみ     // cong → congestion → 輻輳/集中
        ESP_LOGV(TAG, "    status : %d", param->cong.status);
        ESP_LOGV(TAG, "    handle : %d", param->cong.handle);
        ESP_LOGV(TAG, "    cong   : %s", param->cong.cong ? "true" : "false");
        if (param->cong.status == ESP_SPP_SUCCESS) {
            // 輻輳ステータス変更ユーザハンドラ
            spp_cong_handler(param->cong.handle, param->cong.cong);
        }
        break;
      case ESP_SPP_WRITE_EVT :              // callbackモード時のみ
        ESP_LOGV(TAG, "    status : %d", param->write.status);
        ESP_LOGV(TAG, "    handle : %d", param->write.handle);
        ESP_LOGV(TAG, "    len    : %d", param->write.len);
        ESP_LOGV(TAG, "    cong   : %s", param->write.cong ? "true" : "false");
        if (param->write.status == ESP_SPP_SUCCESS) {
            // 送信完了ユーザハンドラ
            spp_write_handler(param->write.handle, param->write.cong, param->write.len);
        }
        break;
      case ESP_SPP_SRV_STOP_EVT :
        ESP_LOGV(TAG, "    status : %d", param->srv_stop.status);
        ESP_LOGV(TAG, "    scn    : %d", param->srv_stop.scn);
        break;
    default:
        break;
    }
}


// ================================================================================================
// Bluetooth SPP event→文字列変換(デバッグ用)
// ================================================================================================
static char* esp_spp_event_to_str(esp_spp_cb_event_t event)
{
    switch(event) {
      case ESP_SPP_INIT_EVT :           return "ESP_SPP_INIT_EVT";             // ✔      When SPP is inited, the event comes
      case ESP_SPP_UNINIT_EVT :         return "ESP_SPP_UNINIT_EVT";           //         When SPP is uninited, the event comes
      case ESP_SPP_DISCOVERY_COMP_EVT : return "ESP_SPP_DISCOVERY_COMP_EVT";   // ✔      When SDP discovery complete, the event comes
      case ESP_SPP_OPEN_EVT :           return "ESP_SPP_OPEN_EVT";             // ✔      When SPP Client connection open, the event comes
      case ESP_SPP_CLOSE_EVT :          return "ESP_SPP_CLOSE_EVT";            // ✔      When SPP connection closed, the event comes
      case ESP_SPP_START_EVT :          return "ESP_SPP_START_EVT";            // ✔      When SPP server started, the event comes
      case ESP_SPP_CL_INIT_EVT :        return "ESP_SPP_CL_INIT_EVT";          // ✔      When SPP client initiated a connection, the event comes
      case ESP_SPP_DATA_IND_EVT :       return "ESP_SPP_DATA_IND_EVT";         //         When SPP connection received data, the event comes, only for ESP_SPP_MODE_CB
      case ESP_SPP_CONG_EVT :           return "ESP_SPP_CONG_EVT";             //         When SPP connection congestion status changed, the event comes, only for ESP_SPP_MODE_CB
      case ESP_SPP_WRITE_EVT :          return "ESP_SPP_WRITE_EVT";            //         When SPP write operation completes, the event comes, only for ESP_SPP_MODE_CB
      case ESP_SPP_SRV_OPEN_EVT :       return "ESP_SPP_SRV_OPEN_EVT";         // ✔      When SPP Server connection open, the event comes
      case ESP_SPP_SRV_STOP_EVT :       return "ESP_SPP_SRV_STOP_EVT";         //         When SPP server stopped, the event comes
      default:  break;
    }
    return "UNKNOWN EVENT";
}


