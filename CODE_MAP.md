    # ESP-Dashboard-Host - Bản đồ cấu trúc code
    (Tạo bởi dash - ASCII tree)

    ## Sơ đồ tổng quan
    .
    |-- LICENSE
    |-- README.md
    |-- CODE_MAP.md (file này)
    `-- firmware/
        |-- .gitignore
        |-- build_log.txt
        |-- CMakeLists.txt
        |-- platformio.ini
        |-- sdkconfig.esp32doit-devkit-v1
        |-- sdkconfig.esp32doit-devkit-v1.bak
        |
        |-- data/
        |   `-- web.html                      --> Giao diện web (dashboard)
        |
        |-- include/
        |   `-- README
        |
        |-- test/
        |   `-- README
        |
        |-- src/
        |   |-- CMakeLists.txt                --> Khai báo sources + include dirs
        |   |-- main.c                        --> Điểm khởi động app_main
        |   `-- app_controller.c              --> (file rỗng - placeholder)
        |
        `-- lib/
            |-- wifi/
            |   |-- wifi_manager.h/.c         --> Lưu trữ cấu hình, credentials, mode (RAM)
            |   |-- wifi_service.h/.c         --> Async service: unified queue + dispatcher task
            |   |-- wifi_handler.h/.c         --> FSM wifi: trạng thái + trigger driver
            |   `-- wifi_manager.h
            |
            |-- storage_manager/
            |   |-- storage_manager.h/.c      --> NVS flash wrapper (mutex serialized)
            |   `--
            |
            `-- http/
                |-- http_handler.c            --> (placeholder - chưa có nội dung)
                |-- http_manager.c            --> (placeholder - chưa có nội dung)
                |-- http_manager.h            --> (file rỗng)
                |-- http_service.c            --> (placeholder - chưa có nội dung)
                `-- http_service.h            --> (file rỗng)

    ## Luồng khởi động (boot flow)
    app_main (src/main.c)
    |-- 1. nvs_flash_init()                       --> Khởi tạo NVS cho esp_wifi
    |-- 2. storage_manager_init()                 --> Tạo namespace "WIFI_STORAGE" + mutex
    |-- 3. wifi_manager_create()                  --> Tạo đối tượng manager (RAM state)
    |-- 4. wifi_srv_init(mgr) + wifi_srv_start()  --> Tạo unified queue + task dispatcher
    |-- 5. status_monitor_task (FreeRTOS)         --> In FSM transition mỗi 2s
    |-- 6. wifi_manager_set_credentials(AP creds) + wifi_srv_switch_mode(WIFI_MODE_AP)
    |-- 7. (sau 1s) wifi_manager_set_credentials(STA creds) + wifi_srv_switch_mode(WIFI_MODE_STA)
    `-- 8. (comment sẵn) CREDENTIAL_LOAD/STORE_EVENT --> Tính năng flash creds (chưa bật)

    ## Kiến trúc Wifi Service (async)
    - caller task --(wifi_srv_post_event)--> [unified queue] --> wifi_service_task
        `-- wifi_handler_process_event(event, data)
            `-- wifi_handler_start_driver (AP) / connect (STA)
            `-- FSM: POWER_OFF -> IDLE -> RUNNING -> CONNECTING -> CONNECTED

    ## Danh sách sự kiện (wifi_srv_event_t)
    |-- WIFI_SRV_AP_EVENT_START
    |-- WIFI_SRV_STA_EVENT_START
    |-- WIFI_SRV_EVENT_STOP
    |-- WIFI_SRV_EVENT_CONNECT
    |-- WIFI_SRV_EVENT_DISCONNECT
    |-- WIFI_SRV_EVENT_STA_STARTED
    |-- WIFI_SRV_EVENT_STA_CONNECTED
    |-- WIFI_SRV_EVENT_STA_GOT_IP
    |-- WIFI_SRV_EVENT_STA_DISCONNECTED
    |-- WIFI_SRV_EVENT_CONNECT_TIMEOUT
    |-- CREDENTIAL_STORE_EVENT    (save wifi creds -> NVS)
    `-- CREDENTIAL_LOAD_EVENT     (load wifi creds <- NVS)

    ## Trạng thái FSM (wifi_fsm_state_t)
    |-- WIFI_FSM_STATE_POWER_OFF
    |-- WIFI_FSM_STATE_IDLE
    |-- WIFI_FSM_STATE_RUNNING
    |-- WIFI_FSM_STATE_CONNECTING
    `-- WIFI_FSM_STATE_CONNECTED

    ## API storage_manager (NVS)
    |-- storage_manager_init / deinit
    |-- storage_manager_read / write / delete
    |-- storage_manager_save_wifi_creds
    |-- storage_manager_load_wifi_creds
    `-- storage_manager_clear_wifi_creds

    ## Dependencies (src/CMakeLists.txt)
    idf_component_register PRIV_REQUIRES: nvs_flash, esp_netif, esp_wifi, esp_event
    Sources: main.c, app_controller.c, wifi_manager.c, wifi_service.c,
            wifi_handler.c, http_handler.c, http_service.c, http_manager.c,
            storage_manager.c