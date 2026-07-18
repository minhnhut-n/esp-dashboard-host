const state = {
  deviceIp: '',
  endpoint: '/api/ping',
  method: 'GET',
  body: ''
};

const statusBadge = document.getElementById('statusBadge');
const responseBox = document.getElementById('responseBox');

function setStatus(text, type = 'secondary') {
  statusBadge.textContent = text;
  statusBadge.className = `status-badge badge badge-${type}`;
}

function showResponse(payload) {
  responseBox.textContent = typeof payload === 'string' ? payload : JSON.stringify(payload, null, 2);
}

async function sendRequest() {
  const deviceIp = document.getElementById('deviceIp').value.trim();
  const endpoint = document.getElementById('endpoint').value.trim();
  const method = document.getElementById('method').value;
  const body = document.getElementById('body').value.trim();

  if (!deviceIp) {
    setStatus('Enter an IP', 'warning');
    showResponse('Please enter the ESP device IP address.');
    return;
  }

  const url = `http://${deviceIp}${endpoint}`;
  const normalizedMethod = method.toUpperCase();
  const options = {
    method: normalizedMethod,
    headers: { 'Content-Type': 'application/json' }
  };

  if (body && !['GET', 'HEAD'].includes(normalizedMethod)) {
    options.body = body;
  } else if (body && ['GET', 'HEAD'].includes(normalizedMethod)) {
    showResponse('GET/HEAD requests cannot carry a body in the browser. The body was ignored. Use POST if you want to send JSON.');
  }

  setStatus('Sending...', 'info');

  try {
    const res = await fetch(url, options);
    const text = await res.text();

    let parsed;
    try {
      parsed = JSON.parse(text);
    } catch (error) {
      parsed = text;
    }

    showResponse(parsed);
    setStatus(res.ok ? 'Success' : `HTTP ${res.status}`, res.ok ? 'success' : 'danger');
  } catch (error) {
    showResponse(`Request failed: ${error.message}`);
    setStatus('Failed', 'danger');
  }
}

function fillPreset(endpoint, method, body) {
  document.getElementById('endpoint').value = endpoint;
  document.getElementById('method').value = method;
  document.getElementById('body').value = body;
}

document.getElementById('sendBtn').addEventListener('click', sendRequest);
document.getElementById('pingBtn').addEventListener('click', () => {
  fillPreset('/api/ping', 'GET', '');
  sendRequest();
});
document.getElementById('dataBtn').addEventListener('click', () => {
  fillPreset('/api/data', 'GET', '');
  sendRequest();
});
document.getElementById('relayOnBtn').addEventListener('click', () => {
  fillPreset('/api/relay', 'POST', '{"relay":1,"state":true}');
  sendRequest();
});
document.getElementById('relayOffBtn').addEventListener('click', () => {
  fillPreset('/api/relay', 'POST', '{"relay":1,"state":false}');
  sendRequest();
});
document.getElementById('rebootBtn').addEventListener('click', () => {
  fillPreset('/api/reboot', 'POST', '');
  sendRequest();
});
