/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/


#define OPEN_HDR_NUM    8           // パラメータテーブル数

// パラメータテーブル
struct _open_hdr_params {
    bool                    use;
    esp_bd_addr_t           bda;
    uint32_t                bd_handle;
};

extern struct _open_hdr_params   open_hdr_params[];

extern void spp_open_handler(uint32_t handle, int fd, esp_bd_addr_t bda);
extern void spp_close_handler(uint32_t bd_handle);
extern void spp_read_handler(uint32_t bd_handle, uint8_t* data, int len);
extern void spp_write_handler(uint32_t bd_handle, bool cong, int len);
extern void spp_cong_handler(uint32_t bd_handle, bool cong);

extern size_t spp_write(uint8_t* buff, size_t  len);
extern size_t spp_read(uint8_t* buff, size_t max_len);
extern void spp_user_init(void);

extern void spp_close_all_handle(void);

