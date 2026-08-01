#include "dashboard_page.h"
#include "http_json_helper.h"

#include <stdio.h>
#include <string.h>

static const char *dashboard_html =
    "<!DOCTYPE html>"
    "<html lang=\"en\">"
    "<head>"
    "  <meta charset=\"UTF-8\" />"
    "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\" />"
    "  <title>ESP-IDF Dashboard</title>"
    "  <style>"
    "    body{font-family:Arial,sans-serif;background:#0f172a;color:#e2e8f0;margin:0;padding:24px;}"
    "    .card{max-width:720px;margin:0 auto;background:#111827;border:1px solid #334155;border-radius:16px;padding:24px;}"
    "    h1{margin:0 0 8px;}"
    "    .muted{color:#94a3b8;}"
    "    button{margin:6px 6px 0 0;padding:10px 14px;border:none;border-radius:10px;background:#38bdf8;color:white;cursor:pointer;}"
    "    button.secondary{background:#1f2937;}"
    "    button.success{background:#22c55e;}"
    "    button.danger{background:#ef4444;}"
    "    pre{background:#020617;padding:14px;border-radius:12px;white-space:pre-wrap;word-break:break-word;}"
    "  </style>"
    "</head>"
    "<body>"
    "  <div class=\"card\">"
    "    <h1>ESP-IDF Dashboard</h1>"
    "    <p class=\"muted\">Basic internal dashboard for the current firmware REST API.</p>"
    "    <button onclick=\"call('/api/ping')\">Ping</button>"
    "    <button onclick=\"call('/api/data')\" class=\"secondary\">Get Data</button>"
    "    <button onclick=\"call('/api/relay', 'POST', '{\"relay\":1,\"state\":true}')\" class=\"success\">Relay ON</button>"
    "    <button onclick=\"call('/api/relay', 'POST', '{\"relay\":1,\"state\":false}')\" class=\"danger\">Relay OFF</button>"
    "    <button onclick=\"call('/api/reboot', 'POST')\" class=\"danger\">Reboot</button>"
    "    <pre id=\"out\">Click a button to test the ESP REST API.</pre>"
    "    <script>"
    "      function call(path, method='GET', body='') {"
    "        const ip = prompt('ESP IP address', '192.168.1.40');"
    "        if (!ip) return;"
    "        const url = 'http://' + ip + path;"
    "        const options = {method: method, headers: {'Content-Type':'application/json'}};"
    "        if (body) options.body = body;"
    "        fetch(url, options).then(async (res) => {"
    "          const text = await res.text();"
    "          document.getElementById('out').textContent = 'Status: ' + res.status + '\\n\\n' + text;"
    "        }).catch((err) => {"
    "          document.getElementById('out').textContent = 'Request failed: ' + err.message;"
    "        });"
    "      }"
    "    </script>"
    "  </div>"
    "</body>"
    "</html>";

esp_err_t dashboard_get_handler(httpd_req_t *req) {
    ESP_LOGI("HTTP_EVENT", "GET %s", req->uri);
    httpd_resp_set_type(req, GUI_TYPE_HTML);
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_sendstr(req, dashboard_html);
    return ESP_OK;
}

esp_err_t favicon_get_handler(httpd_req_t *req) {
    const char *icon = "\x00";
    httpd_resp_set_type(req, IMAGE_TYPE_ICON);
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    return httpd_resp_send(req, icon, 0);
}