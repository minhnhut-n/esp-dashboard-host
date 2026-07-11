/**
 * ESP Dashboard Host - API Module
 * Handles REST API communication with the ESP device
 */

const API = {
    baseUrl: '',
    timeout: 5000,

    setBaseUrl(url) {
        this.baseUrl = url.replace(/\/+$/, '');
    },

    getUrl(endpoint) {
        return `${this.baseUrl}${endpoint}`;
    },

    async request(method, endpoint, body = null) {
        if (!this.baseUrl) {
            throw new Error('API base URL not set. Configure ESP IP in Connection settings.');
        }

        const options = {
            method,
            headers: {
                'Content-Type': 'application/json',
                'Accept': 'application/json'
            },
            signal: AbortSignal.timeout(this.timeout)
        };

        if (body && (method === 'POST' || method === 'PUT' || method === 'PATCH')) {
            options.body = JSON.stringify(body);
        }

        try {
            const response = await fetch(this.getUrl(endpoint), options);
            if (!response.ok) {
                throw new Error(`HTTP ${response.status}: ${response.statusText}`);
            }
            const contentType = response.headers.get('content-type');
            if (contentType && contentType.includes('application/json')) {
                return await response.json();
            }
            return await response.text();
        } catch (error) {
            if (error.name === 'TimeoutError' || error.name === 'AbortError') {
                throw new Error('Request timed out. Check device connection.');
            }
            throw error;
        }
    },

    // Sensor Data
    async getSensorData() { return this.request('GET', '/api/data'); },
    async getSystemInfo() { return this.request('GET', '/api/system'); },

    // Controls
    async setRelay(relay, state) { return this.request('POST', '/api/relay', { relay, state }); },
    async setPWM(pwm, value) { return this.request('POST', '/api/pwm', { pwm, value }); },

    // Device Management
    async reboot() { return this.request('POST', '/api/reboot'); },
    async factoryReset() { return this.request('POST', '/api/reset'); },

    // WiFi Configuration
    async setWiFiConfig(config) { return this.request('POST', '/api/wifi', config); },
    async getWiFiConfig() { return this.request('GET', '/api/wifi'); },
    async scanWiFi() { return this.request('GET', '/api/wifi/scan'); },

    // Device Configuration
    async setDeviceConfig(config) { return this.request('POST', '/api/config', config); },
    async getDeviceConfig() { return this.request('GET', '/api/config'); },

    // NTP / Time
    async setNTPConfig(config) { return this.request('POST', '/api/ntp', config); },

    // Health Check
    async ping() {
        try {
            await this.request('GET', '/api/ping');
            return true;
        } catch {
            return false;
        }
    }
};