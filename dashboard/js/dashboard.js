/**
 * ESP Dashboard Host - Main Dashboard Application
 */

const Dashboard = {
    config: {
        refreshInterval: 1000,
        theme: 'dark',
        sidebarStyle: 'style-3',
        tempUnit: 'celsius',
        showNotifications: true,
        autoRefresh: true,
        autoConnect: true,
        baseUrl: localStorage.getItem('espDashboard_host') || '',
        wsPort: 80,
        useSSL: false,
        connTimeout: 5000
    },

    state: {
        sensorData: null,
        systemInfo: null,
        isUpdating: false,
        updateTimer: null,
        relayStates: { 1: false, 2: false, 3: false },
        connStartTime: null
    },

    init() {
        this.loadConfig();
        if (this.config.baseUrl) {
            const ip = this.config.baseUrl.replace(/^https?:\/\//, '');
            document.getElementById('espIP').value = ip;
        }
        this.setupNavigation();
        this.setupEventListeners();
        this.setupWebSocket();
        this.applyTheme(this.config.theme);
        this.applySidebarStyle(this.config.sidebarStyle);
        this.loadSavedWifiInfo();
        if (this.config.autoConnect && this.config.baseUrl) {
            setTimeout(() => this.handleConnect(), 500);
        }
        setTimeout(() => {
            document.getElementById('pageLoader').classList.add('hidden');
        }, 800);
        if (window.location.hash) {
            const section = window.location.hash.replace('#', '');
            const navItem = document.querySelector(`.nav-link[data-section="${section}"]`);
            if (navItem) navItem.click();
        }
    },

    loadConfig() {
        try {
            const saved = localStorage.getItem('espDashboardConfig');
            if (saved) Object.assign(this.config, JSON.parse(saved));
        } catch (e) { console.warn('Config load error:', e); }
    },

    saveConfig() {
        try { localStorage.setItem('espDashboardConfig', JSON.stringify(this.config)); }
        catch (e) { console.warn('Config save error:', e); }
    },

    loadSavedWifiInfo() {
        try {
            const wifiInfo = localStorage.getItem('espWifiInfo');
            if (wifiInfo) {
                const info = JSON.parse(wifiInfo);
                document.getElementById('currentSSID').textContent = info.ssid || 'Not connected';
                document.getElementById('currentRSSI').textContent = (info.rssi || '--') + ' dBm';
                document.getElementById('currentIP').textContent = info.ip || '--';
                document.getElementById('currentMAC').textContent = info.mac || '--';
            }
        } catch (e) {}
    },

    setupNavigation() {
        document.querySelectorAll('.sidebar-menu a.nav-link[data-section]').forEach(item => {
            item.addEventListener('click', (e) => {
                e.preventDefault();
                this.switchSection(item.dataset.section);
            });
        });
        document.querySelectorAll('.sidebar-menu li.dropdown > a.has-dropdown').forEach(item => {
            item.addEventListener('click', (e) => {
                e.preventDefault();
                const li = item.closest('li.dropdown');
                li.classList.toggle('active');
                const submenu = li.querySelector('ul.dropdown-menu');
                if (submenu) {
                    submenu.style.display = submenu.style.display === 'block' ? 'none' : 'block';
                }
            });
        });
        document.getElementById('sidebarToggle').addEventListener('click', (e) => {
            e.preventDefault();
            document.body.classList.toggle('sidebar-mini');
        });
    },

    switchSection(section) {
        document.querySelectorAll('.sidebar-menu li').forEach(li => li.classList.remove('active'));
        const parent = document.querySelector(`.nav-link[data-section="${section}"]`)?.closest('li');
        if (parent) parent.classList.add('active');
        document.querySelectorAll('.section').forEach(s => s.classList.remove('active'));
        const target = document.getElementById(`section-${section}`);
        if (target) target.classList.add('active');
        window.location.hash = section;
    },

    setupWebSocket() {
        WSManager.onMessage = (data) => {
            if (data.temperature !== undefined) this.updateDashboardData(data);
            else if (data.freeHeap !== undefined) this.updateSystemInfo(data);
            this.updateConnStats();
        };
        WSManager.onConnect = () => {
            this.state.connStartTime = Date.now();
            this.showToast('Connected to device', 'success');
            if (this.config.autoRefresh) this.startAutoUpdate();
            this.updateConnStats();
        };
        WSManager.onDisconnect = () => {
            this.showToast('Device disconnected', 'warning');
            this.stopAutoUpdate();
            this.state.connStartTime = null;
            this.updateConnStats();
        };
        WSManager.onError = () => {
            this.showToast('Connection error. Check device IP.', 'error');
        };
    },

    handleConnect() {
        const ip = document.getElementById('espIP').value.trim();
        if (!ip) { this.showToast('Please enter the ESP IP address', 'warning'); return; }
        if (!/^(\d{1,3}\.){3}\d{1,3}$/.test(ip)) { this.showToast('Invalid IP address format', 'error'); return; }
        this.config.baseUrl = `http://${ip}`;
        this.config.baseIP = ip;
        this.saveConfig();
        localStorage.setItem('espDashboard_host', this.config.baseUrl);
        API.setBaseUrl(this.config.baseUrl);
        const port = document.getElementById('wsPort').value || 80;
        const useSSL = document.getElementById('useSSL').checked;
        const protocol = useSSL ? 'wss' : 'ws';
        const wsUrl = `${protocol}://${ip}:${port}/ws`;
        WSManager.connect(wsUrl);
        this.showToast(`Connecting to ${ip}...`, 'info');
    },

    handleDisconnect() {
        WSManager.disconnect();
        this.stopAutoUpdate();
        this.showToast('Disconnected', 'info');
    },

    startAutoUpdate() {
        this.stopAutoUpdate();
        this.refreshData();
        this.state.updateTimer = setInterval(() => this.refreshData(), this.config.refreshInterval);
    },

    stopAutoUpdate() {
        if (this.state.updateTimer) { clearInterval(this.state.updateTimer); this.state.updateTimer = null; }
    },

    async refreshData() {
        if (this.state.isUpdating) return;
        this.state.isUpdating = true;
        try {
            if (WSManager.isConnected) {
                WSManager.requestData('getData');
            } else {
                const data = await API.getSensorData();
                this.updateDashboardData(data);
            }
            document.getElementById('lastUpdate').textContent = `Last update: ${new Date().toLocaleTimeString()}`;
        } catch (error) {
            console.warn('Refresh failed:', error.message);
        } finally {
            this.state.isUpdating = false;
        }
    },

    updateDashboardData(data) {
        if (!data) return;
        this.state.sensorData = data;
        const temp = data.temperature;
        const hum = data.humidity;
        const press = data.pressure;
        if (temp !== undefined) {
            const displayTemp = this.config.tempUnit === 'fahrenheit' ? (temp * 9/5 + 32).toFixed(1) : temp.toFixed(1);
            const unit = this.config.tempUnit === 'fahrenheit' ? '°F' : '°C';
            document.getElementById('temperature').textContent = displayTemp;
            document.getElementById('sensor-temp').textContent = `${displayTemp} ${unit}`;
            document.getElementById('tempBar').style.width = `${Math.min((temp / 50) * 100, 100)}%`;
        }
        if (hum !== undefined) {
            document.getElementById('humidity').textContent = hum.toFixed(1);
            document.getElementById('sensor-humidity').textContent = `${hum.toFixed(1)} %`;
            document.getElementById('humidityBar').style.width = `${Math.min(hum, 100)}%`;
        }
        if (press !== undefined) {
            document.getElementById('pressure').textContent = press.toFixed(1);
            document.getElementById('sensor-pressure').textContent = `${press.toFixed(1)} hPa`;
            const pct = ((press - 950) / 150) * 100;
            document.getElementById('pressureBar').style.width = `${Math.min(Math.max(pct, 0), 100)}%`;
        }
        if (data.analogValue !== undefined) {
            document.getElementById('analogValue').textContent = data.analogValue;
            document.getElementById('analogUnit').textContent = 'raw';
            document.getElementById('analogDisplay').textContent = data.analogValue;
            document.getElementById('analogFill').style.width = `${(data.analogValue / 4095) * 100}%`;
        }
        if (data.analogVoltage !== undefined) {
            document.getElementById('analogValue').textContent = data.analogVoltage.toFixed(2);
            document.getElementById('analogUnit').textContent = 'V';
        }
        if (data.uptime !== undefined) {
            document.getElementById('uptime').textContent = Math.floor(data.uptime / 3600);
        }
        if (data.wifiRSSI !== undefined) {
            document.getElementById('wifiRSSI').textContent = data.wifiRSSI;
        }
        this.updateDigitalIndicator('digital1', data.digitalInput1);
        this.updateDigitalIndicator('digital2', data.digitalInput2);
        if (data.relay1 !== undefined) this.updateRelayUI(1, data.relay1);
        if (data.relay2 !== undefined) this.updateRelayUI(2, data.relay2);
        if (data.relay3 !== undefined) this.updateRelayUI(3, data.relay3);
        if (data.pwm1 !== undefined) {
            document.getElementById('pwm1').value = data.pwm1;
            document.getElementById('pwmValue1').textContent = data.pwm1;
        }
        if (data.pwm2 !== undefined) {
            document.getElementById('pwm2').value = data.pwm2;
            document.getElementById('pwmValue2').textContent = data.pwm2;
        }
    },

    updateSystemInfo(info) {
        if (!info) return;
        this.state.systemInfo = info;
        const set = (id, val) => { const el = document.getElementById(id); if (el) el.textContent = val ?? '--'; };
        set('sysDeviceName', info.deviceName);
        set('sysFirmware', info.firmwareVersion);
        set('sysChip', info.chipModel);
        set('sysChipRev', info.chipRevision);
        set('sysCores', info.cpuCores);
        set('sysCpuFreq', info.cpuFreq ? `${info.cpuFreq} MHz` : '--');
        set('sysFreeHeap', info.freeHeap !== undefined ? this.formatBytes(info.freeHeap) : '--');
        set('sysTotalHeap', info.totalHeap !== undefined ? this.formatBytes(info.totalHeap) : '--');
        set('sysFreePsram', info.freePsram !== undefined ? this.formatBytes(info.freePsram) : '--');
        set('sysTotalPsram', info.totalPsram !== undefined ? this.formatBytes(info.totalPsram) : '--');
        set('sysIP', info.ipAddress);
        set('sysMAC', info.macAddress);
        set('sysSSID', info.wifiSSID);
        set('sysRSSI', info.rssi !== undefined ? `${info.rssi} dBm` : '--');
        set('sysReconnects', info.reconnects);
        if (info.uptime !== undefined) {
            const { display, detail } = this.formatUptime(info.uptime);
            set('sysUptime', display);
            set('sysUptimeDetail', detail);
        }
        const nameEl = document.getElementById('navDeviceName');
        if (nameEl) nameEl.textContent = info.deviceName || 'ESP-Dashboard';
        const verEl = document.getElementById('sidebarVersion');
        if (verEl) verEl.textContent = info.firmwareVersion ? `v${info.firmwareVersion}` : 'v1.0.0';
        if (info.wifiSSID || info.ipAddress) {
            localStorage.setItem('espWifiInfo', JSON.stringify({
                ssid: info.wifiSSID, rssi: info.rssi, ip: info.ipAddress, mac: info.macAddress
            }));
            document.getElementById('currentSSID').textContent = info.wifiSSID || 'Not connected';
            document.getElementById('currentRSSI').textContent = (info.rssi ?? '--') + ' dBm';
            document.getElementById('currentIP').textContent = info.ipAddress || '--';
            document.getElementById('currentMAC').textContent = info.macAddress || '--';
        }
    },

    updateConnStats() {
        document.getElementById('connMsgSent').textContent = WSManager.msgSent;
        document.getElementById('connMsgRecv').textContent = WSManager.msgRecv;
        if (this.state.connStartTime) {
            const elapsed = Math.floor((Date.now() - this.state.connStartTime) / 1000);
            document.getElementById('connUptime').textContent = this.formatUptime(elapsed).display;
        }
    },

    handleRelayToggle(relay, state) {
        this.state.relayStates[relay] = state;
        if (WSManager.isConnected) {
            WSManager.setRelay(relay, state);
        } else {
            API.setRelay(relay, state).catch(() => this.updateRelayUI(relay, !state));
        }
        this.showToast(`Relay ${relay} turned ${state ? 'ON' : 'OFF'}`, state ? 'success' : 'warning');
    },

    handlePWMChange(pwm, value) {
        if (WSManager.isConnected) WSManager.setPWM(pwm, value);
        else API.setPWM(pwm, value).catch(() => {});
    },

    setupEventListeners() {
        for (let i = 1; i <= 3; i++) {
            document.getElementById(`relayQuick${i}`).addEventListener('click', () => {
                const newState = !this.state.relayStates[i];
                this.handleRelayToggle(i, newState);
            });
        }
        document.querySelectorAll('.relay-toggle').forEach(toggle => {
            toggle.addEventListener('change', (e) => {
                const relay = parseInt(e.target.dataset.relay);
                this.handleRelayToggle(relay, e.target.checked);
            });
        });
        document.querySelectorAll('.pwm-slider').forEach(slider => {
            slider.addEventListener('input', (e) => {
                const pwm = parseInt(e.target.id.replace('pwm', ''));
                document.getElementById(`pwmValue${pwm}`).textContent = e.target.value;
            });
            slider.addEventListener('change', (e) => {
                const pwm = parseInt(e.target.id.replace('pwm', ''));
                this.handlePWMChange(pwm, parseInt(e.target.value));
            });
        });
        document.getElementById('rebootBtn').addEventListener('click', () => {
            if (confirm('Reboot the device?')) this.handleReboot();
        });
        document.getElementById('rebootQuickBtn').addEventListener('click', () => {
            if (confirm('Reboot the device?')) this.handleReboot();
        });
        document.getElementById('resetBtn').addEventListener('click', () => {
            if (confirm('Factory reset? This will wipe all settings!')) {
                if (confirm('Are you sure? This cannot be undone.')) this.handleFactoryReset();
            }
        });
        document.getElementById('refreshBtn').addEventListener('click', () => {
            this.refreshData();
            this.showToast('Data refreshed', 'info');
        });
        document.getElementById('connectionForm').addEventListener('submit', (e) => {
            e.preventDefault();
            this.handleConnect();
        });
        document.getElementById('connectBtn').addEventListener('click', () => this.handleConnect());
        document.getElementById('disconnectBtn').addEventListener('click', () => this.handleDisconnect());
        document.getElementById('wifiForm').addEventListener('submit', (e) => {
            e.preventDefault();
            this.handleWiFiSave();
        });
        document.getElementById('scanWifiBtn').addEventListener('click', () => this.handleWifiScan());
        document.getElementById('toggleWifiPwd').addEventListener('click', () => {
            const pwd = document.getElementById('wifiPassword');
            pwd.type = pwd.type === 'password' ? 'text' : 'password';
        });
        document.getElementById('wifiIPMode').addEventListener('change', (e) => {
            document.querySelectorAll('.static-ip-field').forEach(el => {
                el.style.display = e.target.value === 'static' ? 'block' : 'none';
            });
        });
        document.getElementById('wifiMode').addEventListener('change', (e) => {
            const mode = e.target.value;
            document.getElementById('stationConfig').style.display = (mode === 'station' || mode === 'station+ap') ? 'block' : 'none';
            document.getElementById('apConfig').style.display = (mode === 'ap' || mode === 'station+ap') ? 'block' : 'none';
            document.getElementById('wifiModeBadge').textContent = mode === 'station' ? 'Station Mode' : mode === 'ap' ? 'AP Mode' : 'Station + AP Mode';
        });
        document.getElementById('deviceConfigForm').addEventListener('submit', (e) => {
            e.preventDefault();
            this.handleDeviceConfigSave();
        });
        document.getElementById('ntpForm').addEventListener('submit', (e) => {
            e.preventDefault();
            this.handleNTPSave();
        });
        document.getElementById('dashboardAppearanceForm').addEventListener('submit', (e) => {
            e.preventDefault();
            this.handleAppearanceSave();
        });
        document.getElementById('dashboardDataForm').addEventListener('submit', (e) => {
            e.preventDefault();
            this.handleDataSettingsSave();
        });
        document.getElementById('themeSelect').addEventListener('change', (e) => {
            this.applyTheme(e.target.value);
        });
        document.getElementById('sidebarStyle').addEventListener('change', (e) => {
            this.applySidebarStyle(e.target.value);
        });
    },

    async handleReboot() {
        const btn = document.getElementById('rebootBtn');
        btn.disabled = true; btn.innerHTML = '<i class="fas fa-spinner fa-spin"></i> Rebooting...';
        try {
            await API.reboot();
            this.showToast('Reboot command sent', 'warning');
            setTimeout(() => { btn.disabled = false; btn.innerHTML = '<i class="fas fa-redo"></i> Reboot Device'; }, 10000);
        } catch (err) {
            this.showToast('Reboot command sent', 'info');
            btn.disabled = false; btn.innerHTML = '<i class="fas fa-redo"></i> Reboot Device';
        }
    },

    async handleFactoryReset() {
        try {
            await API.factoryReset();
            this.showToast('Factory reset initiated', 'warning');
        } catch (err) {
            this.showToast(`Reset failed: ${err.message}`, 'error');
        }
    },

    async handleWiFiSave() {
        const config = {
            mode: document.getElementById('wifiMode').value,
            band: document.getElementById('wifiBand').value,
            ssid: document.getElementById('wifiSSID').value,
            password: document.getElementById('wifiPassword').value,
            ipMode: document.getElementById('wifiIPMode').value,
            staticIP: document.getElementById('wifiStaticIP').value,
            gateway: document.getElementById('wifiGateway').value,
            subnet: document.getElementById('wifiSubnet').value,
            apSSID: document.getElementById('apSSID').value,
            apPassword: document.getElementById('apPassword').value,
            apChannel: parseInt(document.getElementById('apChannel').value),
            apMaxConn: parseInt(document.getElementById('apMaxConn').value),
            apIP: document.getElementById('apIP').value,
            dnsPrimary: document.getElementById('dnsPrimary').value,
            dnsSecondary: document.getElementById('dnsSecondary').value,
            hostname: document.getElementById('wifiHostname').value
        };
        try {
            await API.setWiFiConfig(config);
            this.showToast('WiFi configuration saved. Device will reconnect.', 'success');
        } catch (err) {
            this.showToast(`Failed: ${err.message}`, 'error');
        }
    },

    async handleWifiScan() {
        const list = document.getElementById('wifiNetworkList');
        list.innerHTML = '<div class="list-group-item text-center"><i class="fas fa-spinner fa-spin"></i> Scanning...</div>';
        try {
            const networks = await API.scanWiFi();
            list.innerHTML = '';
            if (networks && networks.length > 0) {
                networks.forEach(net => {
                    const item = document.createElement('a');
                    item.className = 'list-group-item list-group-item-action d-flex justify-content-between align-items-center';
                    const locked = net.secure ? '<i class="fas fa-lock text-warning ml-2"></i>' : '<i class="fas fa-unlock text-success ml-2"></i>';
                    item.innerHTML = `<span><i class="fas fa-wifi mr-2"></i>${net.ssid} ${locked}</span><span class="badge badge-primary badge-pill">${net.rssi} dBm</span>`;
                    item.addEventListener('click', () => {
                        document.getElementById('wifiSSID').value = net.ssid;
                        document.getElementById('wifiPassword').focus();
                    });
                    list.appendChild(item);
                });
            } else {
                list.innerHTML = '<div class="list-group-item text-muted text-center">No networks found</div>';
            }
        } catch (err) {
            list.innerHTML = `<div class="list-group-item text-danger text-center">Scan failed: ${err.message}</div>`;
        }
    },

    async handleDeviceConfigSave() {
        const config = {
            deviceName: document.getElementById('deviceName').value,
            mqttBroker: document.getElementById('mqttBroker').value,
            mqttPort: parseInt(document.getElementById('mqttPort').value),
            mqttTopic: document.getElementById('mqttTopic').value
        };
        try {
            await API.setDeviceConfig(config);
            this.showToast('Device configuration saved', 'success');
        } catch (err) {
            this.showToast(`Failed: ${err.message}`, 'error');
        }
    },

    async handleNTPSave() {
        const config = {
            timezone: document.getElementById('timezone').value,
            ntpServer: document.getElementById('ntpServer').value,
            ntpInterval: parseInt(document.getElementById('ntpInterval').value)
        };
        try {
            await API.setNTPConfig(config);
            this.showToast('Time settings saved', 'success');
        } catch (err) {
            this.showToast(`Failed: ${err.message}`, 'error');
        }
    },

    handleAppearanceSave() {
        const theme = document.getElementById('themeSelect').value;
        const sidebarStyle = document.getElementById('sidebarStyle').value;
        this.config.theme = theme;
        this.config.sidebarStyle = sidebarStyle;
        this.saveConfig();
        this.applyTheme(theme);
        this.applySidebarStyle(sidebarStyle);
        this.showToast('Appearance settings applied', 'success');
    },

    handleDataSettingsSave() {
        const interval = parseInt(document.getElementById('refreshInterval').value);
        if (interval >= 100 && interval <= 10000) this.config.refreshInterval = interval;
        this.config.tempUnit = document.getElementById('tempUnit').value;
        this.config.showNotifications = document.getElementById('showNotifications').checked;
        this.config.autoRefresh = document.getElementById('autoRefresh').checked;
        this.saveConfig();
        if (WSManager.isConnected && this.config.autoRefresh) this.startAutoUpdate();
        else if (!this.config.autoRefresh) this.stopAutoUpdate();
        this.showToast('Data settings applied', 'success');
    },

    applyTheme(theme) {
        if (theme === 'auto') {
            const prefersDark = window.matchMedia('(prefers-color-scheme: dark)').matches;
            document.body.classList.toggle('dark-mode', prefersDark);
            document.body.classList.toggle('layout-4', true);
        } else {
            document.body.classList.toggle('dark-mode', theme === 'dark');
            document.body.classList.toggle('layout-4', true);
        }
    },

    applySidebarStyle(style) {
        document.querySelectorAll('.sidebar-style-1, .sidebar-style-2, .sidebar-style-3').forEach(el => {
            el.classList.remove('sidebar-style-1', 'sidebar-style-2', 'sidebar-style-3');
        });
        const sidebar = document.querySelector('.main-sidebar');
        if (sidebar) sidebar.classList.add(style);
    },

    updateDigitalIndicator(id, state) {
        const el = document.getElementById(id);
        if (el) {
            el.textContent = state ? 'ON' : 'OFF';
            el.className = `badge badge-${state ? 'success' : 'danger'}`;
        }
    },

    updateRelayUI(relay, state) {
        this.state.relayStates[relay] = state;
        const toggle = document.querySelector(`.relay-toggle[data-relay="${relay}"]`);
        const stateLabel = document.getElementById(`relayState${relay}`);
        const quickBtn = document.getElementById(`relayQuick${relay}`);
        if (toggle) toggle.checked = state;
        if (stateLabel) {
            stateLabel.textContent = state ? 'ON' : 'OFF';
            stateLabel.className = `badge badge-${state ? 'success' : 'secondary'}`;
        }
        if (quickBtn) {
            quickBtn.className = `btn btn-block btn-${state ? 'danger' : 'success'}`;
            quickBtn.innerHTML = `<i class="fas fa-power-off"></i> Relay ${relay} ${state ? 'ON' : 'OFF'}`;
        }
    },

    formatBytes(bytes) {
        if (bytes === undefined || bytes === null) return '--';
        if (bytes === 0) return '0 B';
        const k = 1024;
        const sizes = ['B', 'KB', 'MB', 'GB'];
        const i = Math.floor(Math.log(bytes) / Math.log(k));
        return `${parseFloat((bytes / Math.pow(k, i)).toFixed(1))} ${sizes[i]}`;
    },

    formatUptime(seconds) {
        if (seconds === undefined || seconds === null) return { display: '--', detail: '--' };
        const days = Math.floor(seconds / 86400);
        const hours = Math.floor((seconds % 86400) / 3600);
        const minutes = Math.floor((seconds % 3600) / 60);
        const secs = seconds % 60;
        let display;
        if (days > 0) display = `${days}d ${hours}h`;
        else if (hours > 0) display = `${hours}h ${minutes}m`;
        else if (minutes > 0) display = `${minutes}m ${secs}s`;
        else display = `${secs}s`;
        return { display, detail: `${days}d ${hours}h ${minutes}m ${secs}s` };
    },

    showToast(message, type = 'info') {
        if (!this.config.showNotifications) return;
        const container = document.getElementById('toastContainer');
        const toast = document.createElement('div');
        toast.className = `toast ${type}`;
        const icons = { success: '✅', error: '❌', warning: '⚠️', info: 'ℹ️' };
        toast.innerHTML = `<span>${icons[type] || ''}</span><span>${message}</span>`;
        container.appendChild(toast);
        setTimeout(() => { toast.classList.add('fade-out'); setTimeout(() => toast.remove(), 300); }, 4000);
    }
};

document.addEventListener('DOMContentLoaded', () => Dashboard.init());
