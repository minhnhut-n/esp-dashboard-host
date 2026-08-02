# ESP Dashboard Host

A professional web-based dashboard for ESP32 devices with real-time sensor monitoring, device control, and comprehensive WiFi/device configuration. Built with vanilla JavaScript and served directly from the ESP32 device using ESP-IDF HTTP server.

## Project Structure

```
esp-dashboard-host/
├── firmware/                      # ESP-IDF firmware
│   ├── src/
│   │   └── main.c                 # Main application entry point
│   ├── lib/
│   │   ├── dashboard_page.c/h     # HTTP server and dashboard page handler
│   │   ├── dashboard.html         # Embedded dashboard HTML (served from SPIFFS)
│   │   ├── dashboard.js           # Dashboard JavaScript (embedded in firmware)
│   │   ├── wifi_api.c/h           # WiFi management (STA/AP modes)
│   │   ├── http_json_helper.c/h   # HTTP JSON request/response helpers
│   │   └── event_bus.c/h          # Event handling system
│   ├── data/
│   │   └── web.html               # Simple test page (served from SPIFFS)
│   ├── include/                   # Header files
│   ├── test/                      # Unit tests
│   ├── platformio.ini             # PlatformIO configuration
│   └── CMakeLists.txt             # ESP-IDF build configuration
├── .gitignore
├── LICENSE
└── README.md
```

## Features

### Dashboard Interface

The web dashboard is served directly from the ESP32 device and provides:

#### Main Dashboard
- **Sensor Monitoring** - Real-time display of temperature, humidity, pressure, analog inputs
- **Device Controls** - Relay switches (3x), PWM controls (2x), reboot and factory reset
- **System Information** - Device info, memory usage, network status, uptime
- **Quick Actions** - Fast relay toggles, manual refresh, connection status

#### Configuration Sections
- **WiFi Settings** - Station/AP/Both modes, static IP, DNS configuration, network scanner
- **Connection Settings** - ESP IP, WebSocket port, timeout, auto-connect, SSL support
- **Device Config** - Device name, MQTT settings, NTP/time configuration
- **Dashboard Settings** - Theme (dark/light/auto), sidebar style, refresh interval, temperature unit

#### Design Features
- **CodiePie-inspired layout** with top navbar, sidebar, and breadcrumbs
- **3 sidebar styles**: Dark purple (default), Light, Light with indicator
- **Dark/Light/Auto theme** support
- **Responsive design** - works on desktop, tablet, and mobile
- **Toast notifications** for user feedback
- **LocalStorage** for settings persistence
- **Real-time updates** via WebSocket with auto-reconnect

### Firmware Features

- **ESP-IDF based** - Native ESP-IDF framework with event-driven architecture
- **HTTP Server** - REST API with JSON endpoints
- **WebSocket Support** - Real-time bidirectional communication
- **WiFi Management** - Station, AP, and Station+AP modes with automatic fallback
- **SPIFFS/LittleFS** - File system for serving web assets
- **Event Bus System** - Decoupled event handling for WiFi, HTTP, and application events

## Getting Started

### Prerequisites

- **ESP-IDF** v4.4 or v5.0+ (recommended)
- **PlatformIO** (optional, alternative to ESP-IDF)
- **Python** 3.7+
- **CMake** 3.16+
- **Git**

### Installation

#### Option 1: ESP-IDF (Recommended)

1. **Clone the repository:**
   ```bash
   git clone https://github.com/minhnhut-n/esp-dashboard-host.git
   cd esp-dashboard-host/firmware
   ```

2. **Set up ESP-IDF environment:**
   ```bash
   # Install ESP-IDF (if not already installed)
   git clone -b release/v5.0 --recursive https://github.com/espressif/esp-idf.git ~/esp/esp-idf
   cd ~/esp/esp-idf
   ./install.sh esp32
   . ./export.sh
   ```

3. **Configure the project:**
   ```bash
   cd firmware
   idf.py menuconfig
   ```
   
   Configure:
   - Serial flasher config → Flash frequency (80MHz)
   - Partition table → Custom partition table (if needed)
   - Component config → SPIFFS/LittleFS configuration

4. **Build and flash:**
   ```bash
   idf.py build
   idf.py -p (PORT) flash
   ```

5. **Monitor serial output:**
   ```bash
   idf.py -p (PORT) monitor
   ```

#### Option 2: PlatformIO

1. **Clone the repository:**
   ```bash
   git clone https://github.com/minhnhut-n/esp-dashboard-host.git
   cd esp-dashboard-host/firmware
   ```

2. **Build and upload:**
   ```bash
   pio run -t upload
   ```

3. **Monitor serial output:**
   ```bash
   pio device monitor
   ```

### First Use

1. **Power on your ESP32** - It will start in AP mode by default
2. **Connect to the ESP32 AP** - SSID and password shown in serial monitor
3. **Access the dashboard** - Open browser and navigate to `http://192.168.4.1`
4. **Configure WiFi** - Go to WiFi Settings and configure your network
5. **Connect to your network** - After saving, ESP32 will connect to your WiFi
6. **Access via local IP** - Use the IP address shown in serial monitor

## API Reference

### REST Endpoints

| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/api/ping` | Health check |
| GET | `/api/data` | Get sensor data (JSON) |
| GET | `/api/system` | Get system information (JSON) |
| POST | `/api/relay` | Control relay `{"relay": 1, "state": true}` |
| POST | `/api/pwm` | Set PWM `{"pwm": 1, "value": 128}` |
| POST | `/api/reboot` | Reboot device |
| POST | `/api/wifi_cred` | Save WiFi credentials `{"ssid": "...", "password": "..."}` |
| POST | `/api/exit` | Exit AP mode |
| GET | `/api/wifi` | Get WiFi configuration |
| POST | `/api/wifi` | Set WiFi configuration |
| GET | `/api/wifi/scan` | Scan nearby WiFi networks |
| GET | `/api/config` | Get device configuration |
| POST | `/api/config` | Set device configuration |
| POST | `/api/ntp` | Set NTP/time configuration |

### WebSocket

- **Endpoint**: `ws://<device-ip>/ws`
- **Protocol**: JSON messages
- **Auto-reconnect** with exponential backoff
- **Heartbeat** every 30 seconds

### WebSocket Message Format

**Incoming (Device → Dashboard):**
```json
{
  "temperature": 25.5,
  "humidity": 65.2,
  "pressure": 1013.25,
  "analogValue": 2048,
  "analogVoltage": 1.65,
  "digitalInput1": true,
  "digitalInput2": false,
  "relay1": true,
  "relay2": false,
  "relay3": true,
  "pwm1": 128,
  "pwm2": 64,
  "uptime": 3600,
  "wifiRSSI": -65,
  "freeHeap": 123456,
  "ipAddress": "192.168.1.100",
  "macAddress": "AA:BB:CC:DD:EE:FF",
  "wifiSSID": "MyNetwork",
  "rssi": -65,
  "reconnects": 0,
  "deviceName": "ESP-Dashboard",
  "firmwareVersion": "1.0.0",
  "chipModel": "ESP32",
  "chipRevision": 3,
  "cpuCores": 2,
  "cpuFreq": 240
}
```

**Outgoing (Dashboard → Device):**
```json
{ "action": "getData" }
{ "action": "setRelay", "relay": 1, "state": true }
{ "action": "setPWM", "pwm": 1, "value": 128 }
{ "action": "ping" }
```

## Firmware Architecture

### Core Components

#### WiFi API (`wifi_api.c/h`)
- Manages WiFi modes: Station, Access Point, Station+AP
- Handles WiFi credentials storage and retrieval
- Automatic mode switching and reconnection
- Network scanning functionality

#### HTTP JSON Helper (`http_json_helper.c/h`)
- HTTP server initialization and configuration
- REST endpoint registration
- JSON request/response parsing
- CORS header management

#### Event Bus (`event_bus.c/h`)
- Centralized event handling system
- WiFi events (connect, disconnect, scan)
- HTTP server events (start, stop)
- Application-level events

#### Dashboard Page (`dashboard_page.c/h`)
- Serves the main dashboard HTML/JS
- Handles favicon requests
- Embedded web interface for testing

### Data Flow

```
User Browser
    ↓
HTTP Server (ESP-IDF)
    ↓
REST API Handlers / WebSocket Server
    ↓
Event Bus
    ↓
WiFi API / Application Logic
    ↓
Hardware (GPIO, Sensors, etc.)
```

## Configuration

### ESP-IDF Configuration

Use `idf.py menuconfig` to configure:

- **Serial flasher config**: Set flash frequency and speed
- **Partition table**: Choose default or custom partition scheme
- **SPIFFS/LittleFS**: Configure file system parameters
- **WiFi settings**: Country code, power save mode
- **Component config**: HTTP server buffer sizes, WebSocket settings

### PlatformIO Configuration

Edit `platformio.ini` to customize:

```ini
[env:esp32doit-devkit-v1]
platform = espressif32
board = esp32doit-devkit-v1
framework = espidf
monitor_speed = 115200
```

Supported boards:
- ESP32 DevKit V1 (default)
- ESP32-S2, ESP32-S3, ESP32-C3 (with configuration changes)
- Any ESP32 variant supported by ESP-IDF

## Development

### Modifying the Dashboard

The dashboard web interface is embedded in the firmware. To modify:

1. **Edit dashboard files:**
   - `firmware/lib/dashboard.html` - Main HTML structure
   - `firmware/lib/dashboard.js` - JavaScript logic (if separated)
   - Or modify the embedded string in `firmware/lib/dashboard_page.c`

2. **Rebuild and flash:**
   ```bash
   idf.py build flash
   ```

### Adding New API Endpoints

1. Register handler in `http_json_helper.c`
2. Implement handler function
3. Add URI to `httpd_uri_t` structure
4. Rebuild firmware

### Extending Sensor Support

1. Add sensor reading code in `main.c` or separate module
2. Include sensor data in WebSocket messages
3. Update dashboard HTML/JS to display new sensor
4. Add configuration options if needed

## Troubleshooting

### ESP32 not showing up in serial port
- Install CP210x or CH340 drivers (depending on your board)
- Check USB cable (must be data-capable)
- Try different USB port

### Cannot connect to AP mode
- Ensure you're connecting to the correct SSID
- Check password (shown in serial monitor)
- Try disabling firewall temporarily

### Dashboard not loading
- Check if SPIFFS partition is properly formatted
- Verify HTTP server started (check serial logs)
- Try accessing `http://192.168.4.1` directly

### WebSocket connection fails
- Ensure device and browser are on same network
- Check firewall settings
- Verify WebSocket endpoint is `/ws`

## Tech Stack

### Firmware
- **Framework**: ESP-IDF v4.4/v5.0+
- **Language**: C
- **Build System**: CMake/PlatformIO
- **HTTP Server**: ESP-IDF HTTP Server Component
- **WebSocket**: ESP-IDF WebSocket Server Component
- **File System**: SPIFFS/LittleFS

### Dashboard
- **Frontend**: HTML5, CSS3, Vanilla JavaScript
- **Icons**: Font Awesome 5 (CDN)
- **Communication**: WebSocket (primary) + REST API (fallback)
- **Storage**: LocalStorage for settings persistence

## Browser Compatibility

- Chrome 60+
- Firefox 55+
- Safari 12+
- Edge 79+

## License

Custom license as attachment in repository

## Contributing

Contributions are welcome! Please feel free to submit issues and pull requests.

## Author

minhnhut-n