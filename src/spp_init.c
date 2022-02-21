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
#include "gap_cb.h"
#include "spp_cb.h"
#include "spp_user_hdr.h"
#include "bt_utils.h"
#include "uart_console.h"

#define TAG                 __func__


// ================================================================================================
// Bluetooth(SPP)初期化
// ================================================================================================
esp_err_t spp_init(esp_spp_mode_t mode)
{
    esp_err_t err;

    // Bluetooth Low energyモードのメモリ解放
    err = esp_bt_controller_mem_release(ESP_BT_MODE_BLE);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "controller memory release failed: %s", esp_err_to_name(err));
        return err;
    }

    // コントローラ初期化
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    err = esp_bt_controller_init(&bt_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "initialize controller failed: %s", esp_err_to_name(err));
        return err;
    }

    // コントローラの有効化
    err = esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "enable controller failed: %s", esp_err_to_name(err));
        return err;
    }

    // プロトコルスタックの初期化
    err = esp_bluedroid_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "initialize bluedroid failed: %s", esp_err_to_name(err));
        return err;
    }

    // プロトコルスタックの有効化
    err = esp_bluedroid_enable();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "enable bluedroid failed: %s", esp_err_to_name(err));
        return err;
    }

    // GAP コールバックの登録
    err = esp_bt_gap_register_callback(esp_bt_gap_cb);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "gap register failed: %s\n", esp_err_to_name(err));
        return err;
    }

    // SPPコールバックの登録
    err = esp_spp_register_callback(esp_spp_cb);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "spp register failed: %s", esp_err_to_name(err));
        return err;
    }

    // SPP初期化
    err = esp_spp_init(mode);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "spp init failed: %s", esp_err_to_name(err));
        return err;
    }

#if CONFIG_BT_SSP_ENABLED       // セキュア有効時の設定
    // ペアリング時の動作＝YSE/NO選択
    esp_bt_io_cap_t iocap = ESP_BT_IO_CAP_IO;
    /*
        設定可能値
            ESP_BT_IO_CAP_OUT       (DisplayOnly)
            ESP_BT_IO_CAP_IO        (DisplayYesNo)
            ESP_BT_IO_CAP_IN        (KeyboardOnly)
            ESP_BT_IO_CAP_NONE      (NoInputNoOutput)
    */
    esp_bt_gap_set_security_param(ESP_BT_SP_IOCAP_MODE, &iocap, sizeof(uint8_t));
#endif  // CONFIG_BT_SSP_ENABLED

    // レガシーペアリング(Bluetooth 2.0+EDR以前?)の設定
    // ESP_BT_PIN_TYPE_VARIABLE指定時は第2第3パラメータは未参照
    // ESP_BT_PIN_TYPE_FIXED指定時は第2パラメータにPINコードの桁数、第3パラメータにPINコードを指定する
    esp_bt_gap_set_pin(ESP_BT_PIN_TYPE_VARIABLE, 0, 0);

    // デバイス名の設定
    err = esp_bt_dev_set_device_name(BT_DEVICE_NAME);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "set device name failed: %s", esp_err_to_name(err));
        return err;
    }
    // スキャンモードの設定
    err = esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "set scan mode failed: %s", esp_err_to_name(err));
        return err;
    }

    return ESP_OK;
}
