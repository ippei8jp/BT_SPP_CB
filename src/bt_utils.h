/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

// extern宣言
extern void show_paired_devices(void);
extern void remove_all_paired_devices(void);
extern char* bdaddr_to_str(uint8_t* bd_addr, char* buff);
extern bool str_to_bdaddr(char* buff, esp_bd_addr_t bd_addr);
