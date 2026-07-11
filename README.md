# ESP Dashboard Host

A professional web-based dashboard for ESP32/ESP8266 devices with real-time sensor monitoring, device control, and comprehensive WiFi/device configuration. Built with Bootstrap 4 and inspired by the CodiePie admin template design.

## Project Structure

```
esp-dashboard-host/
├── dashboard/                  # Web dashboard (standalone HTML + JS)
│   ├── index.html             # Main dashboard (Bootstrap 4, CodiePie style)
│   ├── css/
│   │   └── style.css          # Full stylesheet (dark/light themes, 3 sidebar styles)
│   └── js/
│       ├── api.js             # REST API client module
│       ├── websocket.js       # WebSocket client with auto-reconnect
│       └── dashboard.js       # Main application logic
├── firmware/                   # For your ESP-IDF code
│   ├── src/                   # Source files directory
│   └── data/                  # SPIFFS/LittleFS data directory
└── README.md
```

## Dashboard Features

### Main Sections

#### Overview
The main dashboard provides a quick overview of all sensor data:
- **Temperature Card** - Displays current temperature in °C/°F with a thermometer icon
- **Humidity Card** - Shows humidity percentage with a water droplet icon
- **Pressure Card** - Displays atmospheric pressure in hPa with a compression icon
- **Analog Input Card** - Shows raw analog value or voltage reading
- **Uptime Card** - Device uptime in hours
- **WiFi Signal Card** - Current WiFi signal strength in dBm

**Quick Actions Panel:**
- Relay 1/2/3 buttons for quick toggle
- Refresh button to manually update data
- Last update timestamp

#### Sensors
Detailed sensor monitoring with visual indicators:
- **Environmental Sensors** - Temperature, humidity, and pressure with progress bars
- **Digital Inputs** - Two digital input indicators (ON/OFF badges)
- **Analog Display** - Large analog value display with progress bar (0-4095 range)

#### Controls
Device control interface:
- **Relay Controls** - Three relay switches with ON/OFF state indicators
- **PWM Controls** - Two PWM sliders (0-255 range) for GPIO 14 and GPIO 12
- **Device Actions** - Reboot and Factory Reset buttons

#### System
Comprehensive system information:
- **Device Info** - Device name, firmware version, chip model, revision, CPU cores/frequency
- **Memory** - Free/total heap and PSRAM in human-readable format (B/KB/MB/GB)
- **Network** - IP address, MAC address, WiFi SSID, RSSI, reconnect count
- **Uptime** - Large uptime display with detailed breakdown

### Configuration Sections

#### WiFi Settings
Full WiFi configuration with three modes:
- **Station Mode** - Connect to existing WiFi network
  - SSID and password configuration
  - DHCP or Static IP assignment
  - Static IP fields: IP address, Gateway, Subnet mask
  - DNS configuration: Primary and secondary DNS servers
- **Access Point Mode** - ESP acts as WiFi hotspot
  - AP SSID and password
  - Channel selection (1, 6, 11)
  - Max client connections (1, 4, 8)
  - AP IP address configuration
- **Station + AP Mode** - Both modes simultaneously
- **WiFi Band** - 2.4 GHz, 5 GHz, or Auto selection
- **Hostname** - Device hostname configuration
- **Network Scanner** - Scan and display available WiFi networks with signal strength and security indicators

#### Connection
ESP device connection settings:
- **ESP IP Address** - Target device IP address
- **WebSocket Port** - Port for WebSocket connection (default: 80)
- **Connection Timeout** - Timeout in milliseconds (1000-30000)
- **Auto-connect** - Automatically connect on page load
- **SSL/WSS** - Use secure WebSocket connection
- **Connection Status** - Visual indicator, latency, message counts, uptime

#### Device Config
Device-specific configuration:
- **Device Name** - Custom device name
- **MQTT Settings** - Broker address, port, topic prefix
- **Time & NTP** - Timezone selection, NTP server, update interval

#### Dashboard Settings
User interface customization:
- **Theme** - Dark, Light, or Auto (system preference)
- **Sidebar Style** - Three options:
  - Style 1: Light theme
  - Style 2: Light with indicator
  - Style 3: Dark purple (default)
- **Refresh Interval** - Auto-refresh interval in milliseconds (100-10000)
- **Temperature Unit** - Celsius or Fahrenheit
- **Notification Toggle** - Enable/disable toast notifications
- **Auto-refresh** - Enable/disable automatic data refresh

### Design Features

- **CodiePie-inspired layout** with top navbar, sidebar, breadcrumbs
- **3 sidebar styles**: Dark (Style 3), Light (Style 1), Light with indicator (Style 2)
- **Dark/Light/Auto theme** with full dark mode CSS
- **Responsive** - works on desktop, tablet, and mobile
- **Page loader** animation on startup
- **Toast notifications** for user feedback (success, error, warning, info)
- **LocalStorage** for settings persistence
- **No login required** - direct connection to ESP device

## Getting Started

### 1. Open the Dashboard

Simply open `dashboard/index.html` in any modern web browser. No build tools or server required.

### 2. Connect to ESP Device

1. Go to **Connection** in the sidebar
2. Enter your ESP device's IP address
3. Click **Connect**

The dashboard will connect via WebSocket for real-time updates, with REST API fallback.

### 3. Configure WiFi (sends SSID/password to ESP)

1. Go to **WiFi Settings**
2. Select WiFi mode (Station, AP, or Both)
3. Enter SSID and password
4. Configure static IP if needed
5. Click **Save & Apply WiFi**

The configuration will be sent to your ESP device via the REST API.

## API Reference

### REST Endpoints

| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/api/data` | Sensor data (JSON) |
| GET | `/api/system` | System information (JSON) |
| POST | `/api/relay` | Set relay `{relay, state}` |
| POST | `/api/pwm` | Set PWM `{pwm, value}` |
| POST | `/api/reboot` | Reboot device |
| POST | `/api/reset` | Factory reset |
| POST | `/api/wifi` | Set WiFi config (full config object) |
| GET | `/api/wifi` | Get current WiFi config |
| GET | `/api/wifi/scan` | Scan nearby WiFi networks |
| POST | `/api/config` | Set device config (name, MQTT) |
| GET | `/api/config` | Get device config |
| POST | `/api/ntp` | Set NTP/time config |
| GET | `/api/ping` | Health check |

### WebSocket

- **Endpoint**: `ws://<device-ip>/ws`
- **Protocol**: JSON messages
- **Auto-reconnect** with exponential backoff
- **Heartbeat** every 30 seconds

### WebSocket Message Protocol

**Incoming Messages:**
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
  "chipRevision": "3",
  "cpuCores": 2,
  "cpuFreq": 240
}
```

**Outgoing Messages:**
```json
{ "action": "getData" }
{ "action": "setRelay", "relay": 1, "state": true }
{ "action": "setPWM", "pwm": 1, "value": 128 }
{ "action": "ping" }
```

## JavaScript Modules

### api.js
REST API client module that handles all HTTP communication with the ESP device:
- `setBaseUrl(url)` - Set the API base URL
- `getSensorData()` - Fetch sensor data
- `getSystemInfo()` - Fetch system information
- `setRelay(relay, state)` - Control relay
- `setPWM(pwm, value)` - Set PWM value
- `reboot()` - Reboot device
- `factoryReset()` - Factory reset device
- `setWiFiConfig(config)` - Configure WiFi
- `scanWiFi()` - Scan for networks
- `setDeviceConfig(config)` - Set device configuration
- `setNTPConfig(config)` - Set NTP configuration
- `ping()` - Health check

### websocket.js
WebSocket client with automatic reconnection:
- `connect(url)` - Connect to WebSocket
- `disconnect()` - Disconnect from WebSocket
- `send(data)` - Send JSON message
- `requestData(type)` - Request data from device
- `setRelay(relay, state)` - Set relay via WebSocket
- `setPWM(pwm, value)` - Set PWM via WebSocket
- `onMessage` - Callback for incoming messages
- `onConnect` - Callback for connection established
- `onDisconnect` - Callback for disconnection
- `onError` - Callback for errors

### dashboard.js
Main application logic:
- `init()` - Initialize dashboard
- `switchSection(section)` - Switch between sections
- `refreshData()` - Refresh sensor data
- `handleConnect()` - Connect to ESP device
- `handleDisconnect()` - Disconnect from ESP device
- `handleRelayToggle(relay, state)` - Toggle relay
- `handlePWMChange(pwm, value)` - Change PWM value
- `handleReboot()` - Reboot device
- `handleFactoryReset()` - Factory reset device
- `handleWiFiSave()` - Save WiFi configuration
- `handleWifiScan()` - Scan WiFi networks
- `handleDeviceConfigSave()` - Save device configuration
- `handleNTPSave()` - Save NTP configuration
- `handleAppearanceSave()` - Save appearance settings
- `handleDataSettingsSave()` - Save data settings
- `applyTheme(theme)` - Apply theme (dark/light/auto)
- `applySidebarStyle(style)` - Apply sidebar style
- `showToast(message, type)` - Show notification toast

## Tech Stack

- **Frontend**: HTML5, CSS3, Vanilla JavaScript
- **CSS Framework**: Bootstrap 4.6.2 (CDN)
- **Icons**: Font Awesome 5 (CDN)
- **jQuery**: 3.6.0 (for Bootstrap components)
- **Communication**: WebSocket (primary) + REST API (fallback)
- **Storage**: LocalStorage for settings persistence

## Browser Compatibility

- Chrome 60+
- Firefox 55+
- Safari 12+
- Edge 79+

## License

MIT