function call(path, method = 'GET', body = '') {
  // Uses the ESP32's actual serving IP dynamically (e.g., http://192.168.4.1)
  const url = window.location.origin + path;
  
  const options = {
    method: method,
    headers: {
      'Content-Type': 'application/json'
    }
  };
  
  if (body) {
    options.body = body;
  }
  
  fetch(url, options)
    .then(async (res) => {
      const text = await res.text();
      document.getElementById('out').textContent = 'Status: ' + res.status + '\n\n' + text;
    })
    .catch((err) => {
      document.getElementById('out').textContent = 'Request failed: ' + err.message;
    });
}