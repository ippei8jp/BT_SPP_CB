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
#include "freertos/stream_buffer.h"
#include "esp_log.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
#include "esp_bt_device.h"
#include "esp_spp_api.h"

#include "esp_vfs.h"
#include "sys/unistd.h"

#include "spp_test.h"
#include "spp_init.h"
#include "spp_user_hdr.h"
#include "bt_utils.h"
#include "uart_console.h"

// LOG表示用TAG(関数名にしておく)
#define TAG                 __func__

#define SPP_RECV_BUFF_SIZE      2048            // 受信バッファサイズ



static volatile uint32_t                _spp_send_hdl           = 0;
static volatile StreamBufferHandle_t    _spp_send_buff_hdl      = NULL;             // 送信用Stream bufferのハンドル
static volatile bool                    _spp_is_sending         = false;            // 送信中フラグ

static volatile StreamBufferHandle_t    _spp_recv_buff_hdl      = NULL;             // 受信用Stream bufferのハンドル


// パラメータテーブル
struct _open_hdr_params     open_hdr_params[OPEN_HDR_NUM] = {0};


// ================================================================================================
// SPP open event handler(オープンイベント時の処理)
// ================================================================================================
void spp_open_handler(uint32_t bd_handle, int fd, esp_bd_addr_t bda)
{
    int             idx;
    
    for (idx = 0; idx < OPEN_HDR_NUM; idx++) {
        if (!open_hdr_params[idx].use) {
            // 未使用のパラメータテーブルが見つかった
            break;
        }
    }
    if (idx >= OPEN_HDR_NUM) {
        ESP_LOGE(TAG, "Tasks reached the upper limit");
        return;
    }
    ESP_LOGV(TAG, "Parameter table index : %d", idx);

    open_hdr_params[idx].use            = true;
    memcpy(open_hdr_params[idx].bda, bda, sizeof(esp_bd_addr_t)) ;
    open_hdr_params[idx].bd_handle      = bd_handle;
    ESP_LOGI(TAG, "BD handle opend");

    if (_spp_send_hdl == 0) {
        _spp_send_hdl =  bd_handle;
        ESP_LOGV(TAG, "_spp_send_hdl is set to %d", bd_handle);
    }


    return;
}

// ================================================================================================
// SPP close event handler(クローズイベント時の処理)
// ================================================================================================
void spp_close_handler(uint32_t bd_handle)
{
    int             idx;
    
    for (idx = 0; idx < OPEN_HDR_NUM; idx++) {
        if (open_hdr_params[idx].use && open_hdr_params[idx].bd_handle == bd_handle) {
            // 対象のパラメータテーブルが見つかった
            break;
        }
    }
    if (idx >= OPEN_HDR_NUM) {
        ESP_LOGE(TAG, "Tasks reached the upper limit");
        return;
    }
    ESP_LOGV(TAG, "Parameter table index : %d", idx);
    
    ESP_LOGI(TAG, "echo back task tarminate");
    open_hdr_params[idx].use = false;

    if (_spp_send_hdl == bd_handle) {
        _spp_send_hdl =  0;
        ESP_LOGV(TAG, "_spp_send_hdl is cleared");
    }
    return;
}

// ================================================================================================
// SPP read event handler(リードイベント時の処理)
// ================================================================================================
void spp_read_handler(uint32_t bd_handle, uint8_t* data, int len)
{
    if (!_spp_recv_buff_hdl){
        // 受信バッファが用意できていない
        ESP_LOGE(TAG,"not opend");
        return;
    }
    
    // **** Bluetoothの接続ハンドルに関わらず同じバッファに格納する ****
    
    size_t recv_size = xStreamBufferSend(_spp_recv_buff_hdl, data, len, 0);    // 待ちに入らない
    if (recv_size < len) {
        // バッファオーバフロー
        ESP_LOGW(TAG,"Received %d bytes out of %d bytes", recv_size , len);
    }
    return;
}

// ================================================================================================
// SPP write event handler(ライト完了イベント時の処理)
// ================================================================================================
void spp_write_handler(uint32_t bd_handle, bool cong, int len)
{
    if (!cong) {
        // 送信の続きがあれば実行
    }
    return;
}
// ================================================================================================
// SPP congestion status change event handler(輻輳ステータス変更イベント時の処理)
// ================================================================================================
void spp_cong_handler(uint32_t bd_handle, bool cong)
{
    if (!cong) {
        // 送信の続きがあれば再開
    }
    return;
}

// ================================================================================================
// 送受信バッファの初期化
// ================================================================================================
void spp_user_init(void)
{
    // 送信バッファは使用しない
    
    // 受信バッファがなければ作成する
    if (!_spp_recv_buff_hdl) {
        _spp_recv_buff_hdl = xStreamBufferCreate(SPP_RECV_BUFF_SIZE, sizeof(uint8_t));  //initialize the stream buffer
        if (_spp_recv_buff_hdl == NULL){
            // 生成失敗
            ESP_LOGE(TAG,"stream buffer(RECV) creation error\n");
        }
        else {
            ESP_LOGE(TAG,"stream buffer(RECV) is created");
        }
    }
    return;
}

// ================================================================================================
// 送信バッファへデータ書き込み
// ================================================================================================
size_t spp_write(uint8_t* buff, size_t  len)
{
    if (!_spp_send_hdl){
        // オープンされていない
        ESP_LOGE(TAG,"not opend");
        return 0;
    }
    // プロトコルスタックでデータを渡す
    esp_err_t err = esp_spp_write(_spp_send_hdl, len, buff);
    if (err == ESP_OK) {
        ESP_LOGV(TAG,"SENDING");
    } else {
        ESP_LOGE(TAG,"esp_spp_write() error : (%d)", err);
    }
    return len;
}

// ================================================================================================
// 受信バッファからデータ読み出し
// ================================================================================================
size_t spp_read(uint8_t* buff, size_t max_len)
{
    if (!_spp_recv_buff_hdl){
        // 受信バッファが用意できていない
        ESP_LOGE(TAG,"SPP receive buffer is not initialized");
        return 0;
    }
    size_t recv_size = xStreamBufferReceive(_spp_recv_buff_hdl, buff, max_len, 0);    // 待ちに入らない
    return recv_size;
}

// ================================================================================================
// Close all BD handle
// ================================================================================================
void spp_close_all_handle(void)
{
    int             idx;
#if 0
    // 内部処理のログレベルを一時的に変更
    extern void bta_sys_set_trace_level(uint8_t level);
    bta_sys_set_trace_level(5);     /* BT_TRACE_LEVEL_DEBUG */
#endif

    for (idx = 0; idx < OPEN_HDR_NUM; idx++) {
        if (open_hdr_params[idx].use ) {
            ESP_LOGI(TAG, "close handle %d", open_hdr_params[idx].bd_handle);
            // 切断処理
            esp_spp_disconnect(open_hdr_params[idx].bd_handle);
            open_hdr_params[idx].use = false;
        }
    }
#if 0
    // 一時的に変更した内部処理のログレベルを戻す
    bta_sys_set_trace_level(2);     /* BT_TRACE_LEVEL_WARNING */
#endif
    return;
}
