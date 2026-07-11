/**
 * ESP Dashboard Host - WebSocket Module
 * Handles real-time WebSocket communication with the ESP device
 */

const WSManager = {
    ws: null,
    url: '',
    reconnectInterval: 3000,
    maxReconnectAttempts: 20,
    reconnectAttempts: 0,
    isConnected: false,
    heartbeatInterval: null,
    heartbeatMs: 30000,
    onMessage: null,
    onConnect: null,
    onDisconnect: null,
    onError: null,
    msgSent: 0,
    msgRecv: 0,

    connect(url) {
        if (url) this.url = url;
        if (!this.url) {
            console.warn('WebSocket URL not set');
            return;
        }

        if (this.ws) this.ws.close();
        this.updateStatus('connecting');

        try {
            this.ws = new WebSocket(this.url);

            this.ws.onopen = () => {
                this.isConnected = true;
                this.reconnectAttempts = 0;
                this.updateStatus('connected');
                this.startHeartbeat();
                if (this.onConnect) this.onConnect();
            };

            this.ws.onmessage = (event) => {
                this.msgRecv++;
                try {
                    const data = JSON.parse(event.data);
                    if (this.onMessage) this.onMessage(data);
                } catch (e) {
                    console.warn('WS parse error:', e);
                }
            };

            this.ws.onclose = (event) => {
                this.isConnected = false;
                this.updateStatus('disconnected');
                this.stopHeartbeat();
                if (this.onDisconnect) this.onDisconnect(event);
                this.attemptReconnect();
            };

            this.ws.onerror = (error) => {
                if (this.onError) this.onError(error);
            };
        } catch (error) {
            this.updateStatus('disconnected');
            this.attemptReconnect();
        }
    },

    send(data) {
        if (this.ws && this.isConnected && this.ws.readyState === WebSocket.OPEN) {
            this.ws.send(JSON.stringify(data));
            this.msgSent++;
        }
    },

    requestData(type) { this.send({ action: type }); },
    setRelay(relay, state) { this.send({ action: 'setRelay', relay, state }); },
    setPWM(pwm, value) { this.send({ action: 'setPWM', pwm, value }); },

    disconnect() {
        this.reconnectAttempts = this.maxReconnectAttempts;
        this.stopHeartbeat();
        if (this.ws) { this.ws.close(); this.ws = null; }
        this.isConnected = false;
        this.updateStatus('disconnected');
    },

    attemptReconnect() {
        if (this.reconnectAttempts >= this.maxReconnectAttempts) return;
        this.reconnectAttempts++;
        const delay = Math.min(this.reconnectInterval * Math.pow(1.5, this.reconnectAttempts - 1), 30000);
        setTimeout(() => { if (!this.isConnected) this.connect(); }, delay);
    },

    startHeartbeat() {
        this.stopHeartbeat();
        this.heartbeatInterval = setInterval(() => {
            if (this.ws && this.isConnected) this.send({ action: 'ping' });
        }, this.heartbeatMs);
    },

    stopHeartbeat() {
        if (this.heartbeatInterval) { clearInterval(this.heartbeatInterval); this.heartbeatInterval = null; }
    },

    updateStatus(status) {
        const dot = document.querySelector('.connection-indicator i');
        const text = document.getElementById('connText');
        const footerStatus = document.getElementById('footerStatus');
        const connStatusText = document.getElementById('connStatusText');
        const connStatusDetail = document.getElementById('connStatusDetail');
        const connStatusIcon = document.querySelector('#connStatusIcon i');
        const connStatusBadge = document.getElementById('connStatusBadge');

        if (dot) {
            dot.className = 'fas fa-circle';
            dot.style.color = status === 'connected' ? '#5CB85C' : status === 'connecting' ? '#F0AD4E' : '#D9534F';
        }
        if (text) text.textContent = status === 'connected' ? 'Connected' : status === 'connecting' ? 'Connecting...' : 'Disconnected';
        if (footerStatus) footerStatus.textContent = status === 'connected' ? '🟢 Connected' : status === 'connecting' ? '🟡 Connecting...' : '🔴 Disconnected';
        if (connStatusText) connStatusText.textContent = status === 'connected' ? 'Connected' : status === 'connecting' ? 'Connecting...' : 'Disconnected';
        if (connStatusDetail) {
            connStatusDetail.textContent = status === 'connected' ? 'Real-time connection established.' :
                status === 'connecting' ? 'Attempting to connect to device...' : 'Configure your ESP IP address and click Connect.';
        }
        if (connStatusIcon) {
            connStatusIcon.className = 'fas fa-circle fa-4x';
            connStatusIcon.style.color = status === 'connected' ? '#5CB85C' : status === 'connecting' ? '#F0AD4E' : '#D9534F';
        }
        if (connStatusBadge) {
            connStatusBadge.innerHTML = status === 'connected' ? '<span class="badge badge-success">Online</span>' :
                status === 'connecting' ? '<span class="badge badge-warning">Connecting</span>' : '<span class="badge badge-danger">Offline</span>';
        }
    }
};