/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

// HOST or CLIENT モード
// HOSTモード時は以下をコメントアウトする
// #define SPP_CLIENT_MODE 1

// デバイス名等
#define BT_DEVICE_NAME      "ESP32"
#define SPP_SERVER_NAME     "SPP_SERVER"

// 接続先デバイス名(CLIENT モード時のみ使用)
#define REMOTE_DEVICE_NAME  "NCC-1701F"  // デバイス名


// extern宣言
#ifdef  SPP_CLIENT_MODE         // SPP クライアントモード
extern const char remote_device_name[];  // デバイス名
extern esp_bd_addr_t    host_bd_address;
extern uint8_t          host_service_channel1;
extern uint8_t          host_service_channel2;
extern bool             found_bd_addr;
extern bool             found_scn1;
extern bool             found_scn2;
#endif  // SPP_CLIENT_MODE
