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
#include <ctype.h>
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

#include "bt_utils.h"

#define TAG                 __func__

// ================================================================================================
// ペアリング済みデバイスの表示
// ================================================================================================
void show_paired_devices(void)
{
    esp_bd_addr_t   dev_list[100];
    int             paired_count = esp_bt_gap_get_bond_device_num(); //ペアリングしたデバイス数を取得
    esp_err_t       result =  esp_bt_gap_get_bond_device_list(&paired_count, dev_list); //ペアリングしたデバイスのアドレス一覧を取得
    if (result == ESP_OK) {
        printf("    BT paired count : %d\n", paired_count);
        for (int i = 0 ; i < paired_count; i++) {
            printf("    [Device %d] : %s\n", i, bdaddr_to_str(dev_list[i], NULL));
        }
    }
    else {
        printf("    esp_bt_gap_get_bond_device_list failed\n");
        return;
    }
}
// ================================================================================================
// ペアリング済み全デバイスのペアリング解除
// ================================================================================================
void remove_all_paired_devices(void)
{
    esp_bd_addr_t   dev_list[100];
    int             paired_count = esp_bt_gap_get_bond_device_num(); //ペアリングしたデバイス数を取得
    esp_err_t       result =  esp_bt_gap_get_bond_device_list(&paired_count, dev_list); //ペアリングしたデバイスのアドレス一覧を取得
    if (result == ESP_OK) {
        printf("    BT paired count : %d\n", paired_count);
        for (int i = 0 ; i < paired_count; i++) {
            printf("    clear [Device %d] : %s\n", i, bdaddr_to_str(dev_list[i], NULL));
            esp_bt_gap_remove_bond_device(dev_list[i]);
        }
    }
    else {
        printf("    esp_bt_gap_get_bond_device_list failed\n");
        return;
    }
}

// ================================================================================================
// BDアドレスを文字列に変換
// ================================================================================================
char* bdaddr_to_str(uint8_t* bd_addr, char* buff)
{
    static char    str_buff[20];        // バッファ指定されていなかった時用の共通バッファ
    if (buff == NULL) {
        buff = str_buff;
    }
    snprintf(buff, 20, "%02x:%02x:%02x:%02x:%02x:%02x",
            bd_addr[0], bd_addr[1], bd_addr[2], bd_addr[3], bd_addr[4], bd_addr[5]);

    return buff;
} 

// ================================================================================================
// 文字列をBDアドレスに変換(デバッグ用なのでガチガチな固定フォーマットのみ対応)
// ================================================================================================
bool str_to_bdaddr(char* buff, esp_bd_addr_t bd_addr)
{
    // 入力文字列は "**:**:**:**:**:**" 固定なので文字列長は17のみ対応
    if (strlen(buff) != 17) {
        // 変換失敗
        return false;
    }
    // ローテクなチェック方法... 動作確認用だからヨシとしよう...(^^ゞ
    // 入力文字列は "**:**:**:**:**:**" 固定なので':'の位置も固定
    // それぞれの文字が16進数かをチェック
    if (isxdigit(buff[0]) && isxdigit(buff[3]) && isxdigit(buff[6]) && isxdigit(buff[ 9]) && isxdigit(buff[12]) && isxdigit(buff[15]) \
     && isxdigit(buff[1]) && isxdigit(buff[4]) && isxdigit(buff[7]) && isxdigit(buff[10]) && isxdigit(buff[13]) && isxdigit(buff[16]) \
     &&          buff[2] == ':' &&    buff[5] == ':' &&    buff[8] == ':' &&    buff[11] == ':' &&    buff[14] == ':') {
        // ':'を'\0'に置き換えて6個の文字列にバラす
        buff[2] = '\0'; buff[5] = '\0'; buff[8] = '\0'; buff[11] = '\0'; buff[14] = '\0'; buff[17] = '\0';
        // 数値に変換して格納
        bd_addr[0] = (uint8_t)strtol(&buff[ 0], NULL, 16);
        bd_addr[1] = (uint8_t)strtol(&buff[ 3], NULL, 16);
        bd_addr[2] = (uint8_t)strtol(&buff[ 6], NULL, 16);
        bd_addr[3] = (uint8_t)strtol(&buff[ 9], NULL, 16);
        bd_addr[4] = (uint8_t)strtol(&buff[12], NULL, 16);
        bd_addr[5] = (uint8_t)strtol(&buff[15], NULL, 16);
        // 変換成功
        return true;
    }
    // 変換失敗
    return false;
} 

