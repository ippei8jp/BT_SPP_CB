
# 概要

VFS版[https://github.com/ippei8jp/BT_SPP_VFS](https://github.com/ippei8jp/BT_SPP_VFS)をベースにCallback版も作ってみた。  

# 参考

サンプルプログラム ： %HOMEPATH%\.platformio\packages\framework-espidf\examples\bluetooth\bluedroid\classic_bt  
または、  
github : [https://github.com/espressif/esp-idf/tree/master/examples/bluetooth/bluedroid/classic_bt](https://github.com/espressif/esp-idf/tree/master/examples/bluetooth/bluedroid/classic_bt/bt_spp_acceptor)  
の以下のディレクトリ  
bt_spp_acceptor  
bt_spp_initiator  


# プログラムソース

このプログラムソースはSPPサーバモードとSPPクライアントモード両方に対応しています。  
サーバモードで使用するには ``spp_test.h`` の  ``#define SPP_CLIENT_MODE 1`` を有効に、クライアントモードで使用するにはコメントアウトしてください。  

# 確認に使用したツールバージョン

- framework-espidf 3.40301.0 (4.3.1)
- tool-cmake 3.16.4
- tool-esptoolpy 1.30100.210531 (3.1.0)
- tool-idf 1.0.1
- tool-mconf 1.4060000.20190628 (406.0.0)
- tool-ninja 1.9.0
- toolchain-esp32ulp 1.22851.191205 (2.28.51)
- toolchain-riscv32-esp 8.4.0+2021r1
- toolchain-xtensa-esp32 8.4.0+2021r1

# 手順

## プロジェクト新規作成

リポジトリでは実行済み

- JTAG ICE使うときは以下をplatform.iniに追加しておく(リポジトリでは追加済みなので使用しない場合は削除してください)

```
monitor_speed = 115200
debug_tool = minimodule
```

## menuconfigの修正

リポジトリでは変更済み。念のため手順を掲載

- PLATFORMIO サイドバーでPROJECT TASKS→esp32dev→Platform→Run Menuconfig をクリック
    - (Top) → Component config → Bluetooth を選択
        - Bluetoothを選択して有効化
        - Bluetooth controller(ESP32 Dual Mode Bluetooth) →
            - Bluetooth controller mode (BR/EDR/BLE/DUALMODE) で BR/EDR Only を選択
        - Bluedroid options →
            - classic Bluetoothを選択   → その下のSPPを選択
            - Bluetooth Low Energyを選択解除
    - ついでに、(Top) → Component config → Log output → Default log verbosity でLog Levelも変更しておく
    - さらについでに、(Top) → Component config → Log output → Use ANSI terminal colors in log output   を選択解除する
    - (Top)まで戻ってESCでSaveして終了

## プログラムの実行

普通にコンパイルしてFlash書き込みorデバッグすれば良い  
実行してメインループに入るとキー入力で操作できる  

### サーバモード時

サーバモード時は特に操作することはなく、クライアント(PCやAndroid)からターミナルソフトで接続して何か文字を送信すればその文字をエコーバックしてくれる。  
物理的なUARTポートとは異なり、1つのサーバポートに対して複数のクライアントを接続することができる。  
ただし、複数のクライアントを接続した場合のエコーバック先は最初に接続したクライアントのみ。  
(2番目以降は受信のみ)  
この辺りはcallbackルーチンの実装次第。  

### クライアントモード時

クライアントモード時はちょっとめんどくさい。

- Windows側でターミナルソフトでSPPサーバに割り当てたCOMポートを開いておく。
- ``d`` を入力して名前検出を実行。ある程度実行したら ``D`` を入力して停止(放置しておいても30秒くらいで自動で停止する)
    - このとき、検出するリモートデバイス(SPPサーバが動作しているPC)の名前は ``spp_test.h`` の ``#define REMOTE_DEVICE_NAME  "～" `` で定義したものが使用されるので、適宜変更。
- これとは異なるデバイスに接続したい場合は、 ``a`` を入力して手動でBDアドレスを入力してもよい。  
  BDアドレスはフォーマットがXX:XX:XX:XX:XX:XX固定なので、注意。
- ``e`` を入力してサービス検出を行う
    - こんな感じで検出結果が表示される(以下は2個COMポートが開かれている場合)。
    失敗した場合はCOMポートが開かれているか再確認。
    
    ```
    V (627361) esp_spp_cb:     status  ; 0
    V (627361) esp_spp_cb:     scn_num : 2
    V (627361) esp_spp_cb:       [0]scn          : 3
    V (627361) esp_spp_cb:       [0]service_name : COM3
    V (627371) esp_spp_cb:       [1]scn          : 4
    V (627371) esp_spp_cb:       [1]service_name : COM5
    ```
    
- f を入力して1個目のサービスチャネルに接続(上記の例だとCOM3) 
    - 2個目のサービスチャネルに接続するには g を入力する(上記の例だとCOM5)
    (3個目以降に対応したければプログラム改造してちょ)
- 対応するPCのターミナルソフトから何か文字を送信すればその文字をエコーバックしてくれる。
- Windows側のターミナルソフトで接続断すれば接続は切れる。再度COMポートを開いて f  を入力すれば再度接続できる。
- ESP32側から接続断したい場合は Z を入力する。 このとき、世知属されているすべてのチャネルが切断される。(個別に切断したければプログラム改造してちょ)
ESP32側から接続断した場合はサーバ側のCOMポートは開きっぱなしでかまわない

# Windows側の設定

## WindowsのSPPサーバにCOMポートを割り当てる

ESP32をクライアントモードで使うにはWindows側に着信ポートを開いておく必要がある。

- コントロールパネルを開いて、検索ボックスで「bluetooth」と入力し、「bluetooth設定の変更」をクリック
または「設定」アプリから「デバイス」→「Bluetoothとその他のデバイス」→「その他のBluetoothオプション」をクリック
    - オプションタブで
        - 「～検出を許可する」にチェックを入れる
    - COMポートタブを選択
        - 「追加」をクリック
            - 「着信」を選択して「OK」
        - 必要なだけ繰り返す
    - 「OK」をクリック
- デバイスマネージャからCOMポート番号を変更するとうまく動かないみたい。TeraTermだけ?
    
    これだけではESP32側からサービスを見つけられない。あらかじめターミナルソフトで対象COMポートを開いておく必要がある。  
    
    ## Windows110でのペアリング
    
    - コントロールパネルを開いて、検索ボックスで「bluetooth」と入力
    - 「bluetoothデバイスの追加」をクリック
    - 見つかったら選択して「次へ」
    - パスコードが表示されたら双方で確認して「はい」(ぼんやりしてるとタイムアウトするので注意)  
      ※ ESP32側は自動でy入力される。  
    - インストールされるのを待つ
    
    ## Windows10でのペアリング解除
    
    ふつうは解除しなくてもよいと思うけど、動作がおかしくなったりプログラム変更したときなどはいったんペアリング解除して再度ペアリングしたほうがいいかも。
    
- コントロールパネルを開いて、検索ボックスで「bluetooth」と入力し、「デバイスとプリンタの表示」をクリック
    - 「未指定」にある対象のデバイス名(ESP32 または ``spp_test.h`` で設定した名前)を右クリック→「デバイスの削除」を選択
        - 「このデバイスを削除しますか?」と聞かれるので、「はい」

## SPPサービスの有効化

一旦クライアントモードでペアリングして、その後サーバモードに変更すると、Windows側でSPPサービスが有効になっていないので、これを有効にする
(サーバモードでペアリングすると自動で有効になっているみたい。一応確認しておきましょう)。

- コントロールパネルを開いて、検索ボックスで「bluetooth」と入力し、「デバイスとプリンタの表示」をクリック
    - 「未指定」にある対象のデバイス名(ESP32 または ``spp_test.h`` で設定した名前)を右クリック→「プロパティ」を選択
        - 「サービス」タブを開くと「シリアルポート(SPP)’SPP_SERVER’(または ``spp_test.h`` で設定したサービス名)」が表示されるのでクリックしてチェックを入れる
        - 「OK」をクリップ

## 割り当てられたCOMポートの確認

この操作だけだとどのCOMポートに割り当てられたか分からないので確認する。

- コントロールパネルを開いて、検索ボックスで「bluetooth」と入力し、「bluetooth設定の変更」をクリック
または「設定」アプリから「デバイス」→「Bluetoothとその他のデバイス」→「その他のBluetoothオプション」をクリック
    - COMポートタブを選択
        - 方向「発信」で名前にデバイス名とサービス名(デフォルトだとESP32 'SPP_SERVER’)が表示されているCOMポートをターミナルソフトで使用する


# タスク一覧の表示  
Menuconfigで以下の設定を行ってコンパイルするとタスク一覧を表示できる。  
リポジトリは変更済み。不要なら戻してください。  

- PLATFORMIO サイドバーでPROJECT TASKS→esp32dev→Platform→Run Menuconfig をクリック  
    - (Top) → Component config → FreeRTOS を選択  
        - Enable FreeRTOS trace facility を選択して有効化  
        - Enable FreeRTOS stats formatting functions を有効化  
        - Enable display of xCoreID in vTaskList を有効化  
    - (TOP)まで戻ってESCでSaveして終了  

メインループで``t``キーを入力するとその時点でのタスク一覧が表示される。  
表示結果はこんな感じ  
左から、タスク名、タスク状態、プライオリティ、スタック空きサイズ、タスク番号、Core ID(-1はCore指定せず?)
  ※ プライオリティは数値が大きい方がプライオリティが高い

タスク状態の文字は以下を意味する

 ``X`` ：実行中、 ``R`` ：実行可能、``B`` ：ブロック状態 、 ``D``：削除、``S`` ：サスペンド状態

```
main            X       1       1832    5       0
IDLE            R       0       1880    7       1
IDLE            R       0       1856    6       0
...
...
```