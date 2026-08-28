/**
 * file name: http_handler.c
 * brief: HTTP handler layer - endpoints, JSON helpers, dashboard page.
 *
 * Responsibilities:
 *   - parse the request (method, uri, body) and build the JSON response
 *   - delegate every "action" to http_service (persist / control / exit)
 *   - server lifecycle + endpoint registration are owned by http_manager
 *
 * author: minhnhut.n
 */

#include "http_handler.h"
#include "http_service.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "esp_system.h"
#include "cJSON.h"

static const char* TAG = "HTTP_HANDLER";

/* ------------------------- JSON helpers ---------------------------------- */

/* CORS pre-flight headers, reused by every OPTIONS endpoint. */
static esp_err_t set_cors_headers(httpd_req_t* req) {
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
    return ESP_OK;
}

/* Main json logic: serialize the cJSON root and send it as a string. */
esp_err_t json_response_https(httpd_req_t* req, cJSON* root) {
    char* res = cJSON_Print(root);
    if (res == NULL) {
        ESP_LOGE(TAG, "cJSON_Print failed");
        return ESP_ERR_NO_MEM;
    }
    httpd_resp_set_type(req, DATA_TYPE_JSON);
    httpd_resp_sendstr(req, res);
    free(res);
    return ESP_OK;
}

esp_err_t json_options_handler(httpd_req_t* req) {
    set_cors_headers(req);
    return httpd_resp_sendstr(req, "");
}

/* --------------------------- REST endpoints ------------------------------ */

// End point, GET: /api/ping
esp_err_t json_get_ping(httpd_req_t* req) {
    ESP_LOGI(TAG, "GET %s", req->uri);
    cJSON* root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "status", "ok");
    cJSON_AddNumberToObject(root, "timestamp", (double)(esp_timer_get_time() / 1000));

    esp_err_t ret = json_response_https(req, root);
    cJSON_Delete(root);   // memleak guard
    return ret;
}

// GET /api/data
esp_err_t json_get_data(httpd_req_t* req) {
    ESP_LOGI(TAG, "GET %s", req->uri);
    cJSON* root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "sensor", 25);
    cJSON_AddStringToObject(root, "state", "ok");
    esp_err_t ret = json_response_https(req, root);
    cJSON_Delete(root);
    return ret;
}

// End point, POST: /api/relay
esp_err_t json_post_relay(httpd_req_t* req) {
    char buf[128];
    int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (ret <= 0) {
        return ESP_FAIL;
    }
    buf[ret] = '\0';

    cJSON* root = cJSON_Parse(buf);
    if (root == NULL) {
        ESP_LOGE(TAG, "Failed to parse JSON");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }

    /* optional payload: {"relay": 1, "state": true} */
    int32_t relay = 0;
    bool state = false;
    cJSON* relay_item = cJSON_GetObjectItem(root, "relay");
    cJSON* state_item = cJSON_GetObjectItem(root, "state");
    if (cJSON_IsNumber(relay_item)) {
        relay = relay_item->valueint;
    }
    if (cJSON_IsBool(state_item)) {
        state = cJSON_IsTrue(state_item);
    }

    http_service_control_device(relay, state);

    cJSON_Delete(root);
    cJSON* resp = cJSON_CreateObject();
    cJSON_AddStringToObject(resp, "status", "ok");
    esp_err_t sta = json_response_https(req, resp);
    cJSON_Delete(resp);
    return sta;
}

// POST /api/wifi_cred_post
esp_err_t json_post_wifi_cred(httpd_req_t* req) {
    char buf[128];
    int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (ret <= 0) {
        return ESP_FAIL;
    }
    buf[ret] = '\0';

    cJSON* root = cJSON_Parse(buf);
    if (root == NULL) {
        ESP_LOGE(TAG, "Failed to parse JSON");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }

    const char* ssid = NULL;
    const char* password = NULL;
    cJSON* ssid_item = cJSON_GetObjectItem(root, "ssid");
    cJSON* pass_item = cJSON_GetObjectItem(root, "password");
    if (cJSON_IsString(ssid_item) && cJSON_IsString(pass_item)) {
        ssid     = ssid_item->valuestring;
        password = pass_item->valuestring;
    }

    if (ssid == NULL || password == NULL) {
        ESP_LOGE(TAG, "Missing WiFi credentials");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing WiFi credentials");
        cJSON_Delete(root);
        return ESP_FAIL;
    }

    esp_err_t srv = http_service_save_wifi_creds(ssid, password);
    if (srv != ESP_OK) {
        ESP_LOGE(TAG, "save wifi creds failed: %s", esp_err_to_name(srv));
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Failed to save WiFi credentials");
        return srv;
    }

    cJSON_Delete(root);
    cJSON* resp = cJSON_CreateObject();
    cJSON_AddStringToObject(resp, "status", "ok");
    esp_err_t sta = json_response_https(req, resp);
    cJSON_Delete(resp);
    return sta;
}

// GET /api/wifi_cred
esp_err_t json_get_wifi_cred(httpd_req_t* req) {
    ESP_LOGI(TAG, "GET %s", req->uri);

    char arg_ssid[NVS_SSID_SIZE] = {0};
    char arg_pass[NVS_PASS_SIZE] = {0};
    esp_err_t load_err = http_service_load_wifi_creds(arg_ssid, NVS_SSID_SIZE,
                                                      arg_pass, NVS_PASS_SIZE);

    cJSON* root = cJSON_CreateObject();
    if (root == NULL) {
        ESP_LOGE(TAG, "failed to create response object");
        return ESP_ERR_NO_MEM;
    }

    if (load_err != ESP_OK) {
        ESP_LOGW(TAG, "load wifi creds failed: %s", esp_err_to_name(load_err));
        cJSON_AddStringToObject(root, "status", "error");
        cJSON_AddStringToObject(root, "ssid", "");
        cJSON_AddStringToObject(root, "pass", "");
    } else {
        cJSON_AddStringToObject(root, "status", "ok");
        cJSON_AddStringToObject(root, "ssid", arg_ssid);
        cJSON_AddStringToObject(root, "pass", arg_pass);
    }

    esp_err_t ret = json_response_https(req, root);
    cJSON_Delete(root);
    return ret;
}

// POST /api/reboot
esp_err_t json_post_reboot(httpd_req_t* req) {
    cJSON* root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "status", "rebooting");
    esp_err_t ret = json_response_https(req, root);
    cJSON_Delete(root);
    esp_restart();
    return ret;
}

// POST /api/exit
esp_err_t json_post_exit(httpd_req_t* req) {
    cJSON* root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "status", "exiting");
    esp_err_t ret = json_response_https(req, root);

    // post the exit request on the event bus (consumed by the app layer)
    http_service_request_exit();

    cJSON_Delete(root);
    return ret;
}

/* ------------------------- Dashboard page -------------------------------- */

static const char* dashboard_html =
    "<!DOCTYPE html>"
    "<html lang=\"en\">"
    "<head>"
    "  <meta charset=\"UTF-8\" />"
    "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\" />"
    "  <title>ESP-IDF Dashboard</title>"
    "  <style>"
    "    body{font-family:Arial,sans-serif;background:#0f172a;color:#e2e8f0;margin:0;padding:24px;}"
    "    .card{max-width:760px;margin:0 auto;background:#111827;border:1px solid #334155;border-radius:16px;padding:24px;}"
    "    h1{margin:0 0 8px;}"
    "    .muted{color:#94a3b8;}"
    "    .row{display:flex;flex-direction:column;gap:6px;margin:10px 0;}"
    "    .password-wrap{display:flex;gap:8px;align-items:center;}"
    "    label{font-size:0.95rem;color:#cbd5e1;}"
    "    input{padding:10px 12px;border:1px solid #475569;border-radius:10px;background:#020617;color:#f8fafc;flex:1;}"
    "    button{margin:6px 6px 0 0;padding:10px 14px;border:none;border-radius:10px;background:#38bdf8;color:white;cursor:pointer;}"
    "    button.small{padding:8px 10px;margin:0;background:#334155;}"
    "    button.secondary{background:#1f2937;}"
    "    button.success{background:#22c55e;}"
    "    button.danger{background:#ef4444;}"
    "    .actions{margin-top:10px;}"
    "    pre{background:#020617;padding:14px;border-radius:12px;white-space:pre-wrap;word-break:break-word;margin-top:16px;}"
    "  </style>"
    "</head>"
    "<body>"
    "  <div class=\"card\">"
    "    <h1>ESP-IDF Dashboard</h1>"
    "    <p class=\"muted\">Basic internal dashboard for the current firmware REST API.</p>"
    "    <div class=\"row\">"
    "      <label for=\"ssid\">SSID</label>"
    "      <input id=\"ssid\" type=\"text\" placeholder=\"MyWiFi\" />"
    "    </div>"
    "    <div class=\"row\">"
    "      <label for=\"password\">Password</label>"
    "      <div class=\"password-wrap\">"
    "        <input id=\"password\" type=\"password\" placeholder=\"Password\" />"
    "        <button type=\"button\" class=\"small\" onclick=\"togglePassword()\">Show</button>"
    "      </div>"
    "    </div>"
    "    <div class=\"actions\">"
    "      <button onclick=\"saveCreds()\" class=\"success\">Save WiFi</button>"
    "      <button type=\"button\" onclick=\"loadCreds()\" class=\"secondary\">Load WiFi</button>"
    "      <button onclick=\"sendExit()\" class=\"danger\">Send Exit</button>"
    "    </div>"
    "    <div class=\"actions\">"
    "      <button onclick=\"call('/api/ping')\">Ping</button>"
    "      <button onclick=\"call('/api/data')\" class=\"secondary\">Get Data</button>"
    "      <button onclick=\"call('/api/relay', 'POST', '{\"relay\":1,\"state\":true}')\" class=\"success\">Relay ON</button>"
    "      <button onclick=\"call('/api/relay', 'POST', '{\"relay\":1,\"state\":false}')\" class=\"danger\">Relay OFF</button>"
    "      <button onclick=\"call('/api/reboot', 'POST')\" class=\"danger\">Reboot</button>"
    "    </div>"
    "    <pre id=\"out\">Click a button to test the ESP REST API.</pre>"
    "    <script>"
    "      function call(path, method='GET', body='') {"
    "        const baseUrl = window.location.origin;"
    "        const url = new URL(path, baseUrl).toString();"
    "        const options = {method: method, headers: {'Content-Type':'application/json'}};"
    "        if (body) options.body = body;"
    "        fetch(url, options).then(async (res) => {"
    "          const text = await res.text();"
    "          document.getElementById('out').textContent = 'Status: ' + res.status + '\\n\\n' + text;"
    "        }).catch((err) => {"
    "          document.getElementById('out').textContent = 'Request failed: ' + err.message;"
    "        });"
    "      }"
    "      function saveCreds() {"
    "        const ssid = document.getElementById('ssid').value.trim();"
    "        const password = document.getElementById('password').value;"
    "        if (!ssid || !password) {"
    "          document.getElementById('out').textContent = 'Please enter both SSID and password.';"
    "          return;"
    "        }"
    "        call('/api/wifi_cred', 'POST', JSON.stringify({ssid: ssid, password: password}));"
    "      }"
    "      function loadCreds() {"
    "        const baseUrl = window.location.origin;"
    "        const url = new URL('/api/wifi_cred', baseUrl).toString();"
    "        const out = document.getElementById('out');"
    "        fetch(url)"
    "          .then(async (res) => {"
    "            const text = await res.text();"
    "            out.textContent = 'Status: ' + res.status + '\\n\\n' + text;"
    "            if (!res.ok) return;"
    "            try {"
    "              const data = JSON.parse(text);"
    "              document.getElementById('ssid').value = data.ssid || '';"
    "              document.getElementById('password').value = data.pass || '';"
    "            } catch (e) {"
    "              out.textContent = 'Failed to parse response: ' + e.message;"
    "            }"
    "          })"
    "          .catch((err) => {"
    "            out.textContent = 'Request failed: ' + err.message;"
    "          });"
    "      }"
    "      function togglePassword() {"
    "        const input = document.getElementById('password');"
    "        const button = event.currentTarget;"
    "        if (input.type === 'password') {"
    "          input.type = 'text';"
    "          button.textContent = 'Hide';"
    "        } else {"
    "          input.type = 'password';"
    "          button.textContent = 'Show';"
    "        }"
    "      }"
    "      function sendExit() {"
    "        call('/api/exit', 'POST');"
    "      }"
    "    </script>"
    "  </div>"
    "</body>"
    "</html>";

esp_err_t dashboard_get_handler(httpd_req_t* req) {
    ESP_LOGI(TAG, "GET %s", req->uri);
    httpd_resp_set_type(req, GUI_TYPE_HTML);
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_sendstr(req, dashboard_html);
    return ESP_OK;
}

esp_err_t favicon_get_handler(httpd_req_t* req) {
    const char* icon = "\x00";
    httpd_resp_set_type(req, IMAGE_TYPE_ICON);
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    return httpd_resp_send(req, icon, 0);
}