# ESP Dashboard Host

Web dashboard hosted on an ESP32 (ESP-IDF) — an HTTP server exposing a REST API that controls WiFi mode and credentials, with an async event-driven architecture between HTTP, WiFi, and NVS storage.

## Features

- HTTP server (REST + embedded HTML dashboard)
- WiFi **AP / STA** modes, switchable at runtime
- WiFi credentials stored in **NVS flash** (loaded once at boot, persisted on change)
- Async **event bus** (`esp_event`) + FreeRTOS queue — no blocking in handlers
- cJSON for request/response handling

## Project Structure

```bash
esp-dashboard-host/
├── LICENSE
├── README.md
└── firmware/
    ├── CMakeLists.txt                 # IDF project file
    ├── platformio.ini                 # PlatformIO build config
    ├── sdkconfig.defaults
    ├── sdkconfig.esp32doit-devkit-v1  # board-specific config
    ├── components/
    │   └── json/cJSON/                # vendored cJSON lib
    ├── data/
    │   └── web.html                   # optional static test page
    ├── include/
    ├── test/
    ├── lib/
    │   ├── event_bus/
    │   │   ├── event_bus.c            # esp_event wrapper + registry
    │   │   └── event_bus.h
    │   ├── helper/
    │   │   ├── comp_decomp_data.c     # (placeholder)
    │   │   └── comp_decomp_data.h     # pack/unpack "k:v|k:v" strings
    │   ├── http/
    │   │   ├── http_handler.c/.h      # REST endpoints + JSON helpers + dashboard HTML
    │   │   ├── http_manager.c/.h      # HTTP server lifecycle (start/stop on WiFi events)
    │   │   └── http_service.c/.h      # async bridge: HTTP -> event bus / wifi queue
    │   ├── storage_manager/
    │   │   ├── storage_manager.c/.h   # NVS access, mutex-serialized
    │   └── wifi/
    │       ├── wifi_handler.c/.h      # WiFi FSM: start/stop/connect, driver control
    │       ├── wifi_manager.c/.h      # config/creds/mode state store (STA creds)
    │       └── wifi_service.c/.h      # async queue + dispatcher task
    └── src/
        ├── CMakeLists.txt
        ├── app_controller.c           # (placeholder)
        └── main.c                     # entry: boot AP, load STA creds from flash
```

## Build

```bash
cd firmware
idf.py build flash monitor     # or: pio run -t upload
```

Default AP: SSID `ESP_ALEX` / password `nhut12345` (see `wifi_handler.c`).

## REST API

| Method | URI             | Payload                               | Action                     |
|--------|-----------------|---------------------------------------|----------------------------|
| GET    | `/api/ping`     | –                                     | Health check               |
| GET    | `/api/data`     | –                                     | Sample sensor data         |
| POST   | `/api/wifi_cred`| `{"ssid","password"}`                 | Save STA creds (NVS)       |
| GET    | `/api/wifi_cred`| –                                     | Load STA creds             |
| POST   | `/api/wifi_mode`| `{"mode":"AP\|STA","ssid","pass"}`   | Switch mode (+persist creds) |
| POST   | `/api/relay`    | `{"relay","state"}`                   | Control device (stub)      |
| POST   | `/api/exit`     | –                                     | Request shutdown           |
| GET    | `/`             | –                                     | Dashboard page             |

## Architecture

```
HTTP       http_handler -> http_service -> event_bus / wifi_srv_post_event
WiFi       wifi_service (queue) -> wifi_handler (FSM) -> esp_wifi driver
Storage    storage_manager (NVS) <- wifi_handler flash tasks
```

- Startup: boots **AP** (fixed credentials), then `CREDENTIAL_LOAD_EVENT` restores **STA** creds from flash into the manager (STA-only slot).
- Runtime mode switch carries its own creds; new STA creds are persisted to flash for the next boot. Flash is never reloaded at runtime.

## License

Custom license as attachment in repository.