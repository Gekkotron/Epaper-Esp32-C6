#include "webserver.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "cJSON.h"
#include "epaper/epaper.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "webserver";
static httpd_handle_t server = NULL;

// HTML page for the root endpoint
static const char* index_html =
    "<!DOCTYPE html>"
    "<html>"
    "<head>"
    "<title>E-Paper Display</title>"
    "<meta name='viewport' content='width=device-width, initial-scale=1'>"
    "<style>"
    "body{font-family:Arial;margin:20px;background:#f0f0f0}"
    ".container{max-width:800px;margin:0 auto;background:white;padding:20px;border-radius:10px;box-shadow:0 2px 10px rgba(0,0,0,0.1)}"
    "h1{color:#333}"
    "input,textarea,button,select{width:100%;padding:10px;margin:5px 0;border:1px solid #ddd;border-radius:5px;box-sizing:border-box;font-size:14px}"
    "button{background:#007bff;color:white;border:none;cursor:pointer;font-size:16px;margin-top:10px}"
    "button:hover{background:#0056b3}"
    "button.delete{background:#dc3545;width:auto;padding:5px 15px}"
    "button.add{background:#28a745}"
    ".status{padding:10px;background:#d4edda;border:1px solid #c3e6cb;border-radius:5px;margin:10px 0}"
    ".text-item{border:1px solid #ddd;padding:15px;margin:10px 0;border-radius:5px;background:#f9f9f9;position:relative}"
    ".inline{display:inline-block;width:48%;margin:1%}"
    ".row{display:flex;gap:10px}"
    "</style>"
    "</head>"
    "<body>"
    "<div class='container'>"
    "<h1>E-Paper Display Control</h1>"
    "<div id='status' class='status'>Ready</div>"
    "<div style='background:#f9f9f9;padding:15px;border-radius:5px;margin:10px 0'>"
    "<label style='font-weight:bold;color:#555'>Screen Orientation:</label>"
    "<select id='globalOrientation' style='width:100%;margin-top:5px'>"
    "<option value='0'>0° (Normal)</option>"
    "<option value='1'>90° (Clockwise)</option>"
    "<option value='2'>180° (Upside Down)</option>"
    "<option value='3'>270° (Counter-Clockwise)</option>"
    "</select>"
    "</div>"
    "<h3>Multiple Texts</h3>"
    "<div id='texts'></div>"
    "<button class='add' onclick='addText()'>+ Add Text</button>"
    "<button onclick='sendAll()'>Update Display</button>"
    "<button onclick='clearScreen()'>Clear Screen</button>"
    "<hr>"
    "<h3>Quick Examples</h3>"
    "<button onclick='example1()'>Example 1: Title + Subtitle</button>"
    "<button onclick='example2()'>Example 2: Status Display</button>"
    "</div>"
    "<script>"
    "let textId=0;"
    "function showStatus(msg,isError){"
    "const s=document.getElementById('status');"
    "s.textContent=msg;"
    "s.style.background=isError?'#f8d7da':'#d4edda';"
    "s.style.borderColor=isError?'#f5c6cb':'#c3e6cb';"
    "}"
    "function addText(text='Hello',x=10,y=10,color=1,scale=2,font=1){"
    "const id=textId++;"
    "const div=document.createElement('div');"
    "div.className='text-item';"
    "div.id='text-'+id;"
    "div.innerHTML=`"
    "<button class='delete' onclick='removeText(${id})' style='float:right'>Delete</button>"
    "<label style='display:block;margin-bottom:5px;font-weight:bold;color:#555'>Text:</label>"
    "<input type='text' id='t${id}' placeholder='Enter text to display' value='${text}'>"
    "<div class='row'>"
    "<input type='number' id='x${id}' placeholder='X' value='${x}' style='width:20%'>"
    "<input type='number' id='y${id}' placeholder='Y' value='${y}' style='width:20%'>"
    "<input type='number' id='s${id}' placeholder='Scale' value='${scale}' min='1' max='5' style='width:20%'>"
    "<select id='c${id}' style='width:20%'>"
    "<option value='1' ${color==1?'selected':''}>Black</option>"
    "<option value='2' ${color==2?'selected':''}>Red</option>"
    "<option value='0' ${color==0?'selected':''}>White</option>"
    "</select>"
    "<select id='f${id}' style='width:20%'>"
    "<option value='0' ${font==0?'selected':''}>Small</option>"
    "<option value='1' ${font==1?'selected':''}>Medium</option>"
    "<option value='2' ${font==2?'selected':''}>Large</option>"
    "</select>"
    "</div>`;"
    "document.getElementById('texts').appendChild(div);"
    "}"
    "function removeText(id){"
    "document.getElementById('text-'+id).remove();"
    "}"
    "function sendAll(){"
    "const items=[];"
    "document.querySelectorAll('.text-item').forEach(el=>{"
    "const id=el.id.split('-')[1];"
    "items.push({"
    "text:document.getElementById('t'+id).value,"
    "x:parseInt(document.getElementById('x'+id).value),"
    "y:parseInt(document.getElementById('y'+id).value),"
    "color:parseInt(document.getElementById('c'+id).value),"
    "scale:parseInt(document.getElementById('s'+id).value),"
    "font:parseInt(document.getElementById('f'+id).value)"
    "});"
    "});"
    "if(items.length===0){showStatus('Add some text first!',true);return;}"
    "const orientation=parseInt(document.getElementById('globalOrientation').value);"
    "showStatus('Updating display...');"
    "fetch('/api/multi',{"
    "method:'POST',"
    "headers:{'Content-Type':'application/json'},"
    "body:JSON.stringify({orientation:orientation,texts:items})"
    "})"
    ".then(r=>r.json())"
    ".then(d=>showStatus(d.message))"
    ".catch(e=>showStatus('Error: '+e,true));"
    "}"
    "function clearScreen(){"
    "showStatus('Clearing...');"
    "fetch('/api/clear',{method:'POST'})"
    ".then(r=>r.json())"
    ".then(d=>showStatus(d.message))"
    ".catch(e=>showStatus('Error: '+e,true));"
    "}"
    "function example1(){"
    "document.getElementById('texts').innerHTML='';"
    "document.getElementById('globalOrientation').value='0';"
    "textId=0;"
    "addText('E-Paper Display',10,10,1,2,1);"
    "addText('Web API Ready!',10,40,2,1,1);"
    "sendAll();"
    "}"
    "function example2(){"
    "document.getElementById('texts').innerHTML='';"
    "document.getElementById('globalOrientation').value='0';"
    "textId=0;"
    "addText('Status: Online',10,10,1,1,1);"
    "addText('Temp: 25C',10,25,1,1,1);"
    "addText('WiFi: OK',10,40,2,1,1);"
    "sendAll();"
    "}"
    "addText();"
    "</script>"
    "</body>"
    "</html>";

// GET / - Root page
static esp_err_t index_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, index_html, strlen(index_html));
    return ESP_OK;
}

// POST /api/text - Display text
static esp_err_t api_text_handler(httpd_req_t *req) {
    char content[512];
    int ret = httpd_req_recv(req, content, sizeof(content) - 1);

    if (ret <= 0) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    content[ret] = '\0';

    // Parse JSON
    cJSON *json = cJSON_Parse(content);
    if (json == NULL) {
        const char *resp = "{\"error\":\"Invalid JSON\"}";
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, resp, strlen(resp));
        cJSON_Delete(json);
        return ESP_FAIL;
    }

    // Extract parameters
    cJSON *text_item = cJSON_GetObjectItem(json, "text");
    cJSON *x_item = cJSON_GetObjectItem(json, "x");
    cJSON *y_item = cJSON_GetObjectItem(json, "y");
    cJSON *color_item = cJSON_GetObjectItem(json, "color");
    cJSON *scale_item = cJSON_GetObjectItem(json, "scale");
    cJSON *clear_item = cJSON_GetObjectItem(json, "clear");

    // Default values
    const char *text = text_item && cJSON_IsString(text_item) ? text_item->valuestring : "Hello";
    uint16_t x = x_item && cJSON_IsNumber(x_item) ? x_item->valueint : 10;
    uint16_t y = y_item && cJSON_IsNumber(y_item) ? y_item->valueint : 10;
    uint8_t color = color_item && cJSON_IsNumber(color_item) ? color_item->valueint : COLOR_BLACK;
    uint8_t scale = scale_item && cJSON_IsNumber(scale_item) ? scale_item->valueint : 1;
    bool clear = clear_item && cJSON_IsBool(clear_item) ? cJSON_IsTrue(clear_item) : true;

    ESP_LOGI(TAG, "Displaying text: '%s' at (%d,%d) color=%d scale=%d", text, x, y, color, scale);

    // Clear display if requested
    if (clear) {
        epaper_display_clear();
    }

    // Draw text
    epaper_draw_text(x, y, text, color, scale);

    // Update display
    epaper_display_update();

    // Send response
    const char *resp = "{\"success\":true,\"message\":\"Text displayed\"}";
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp, strlen(resp));

    cJSON_Delete(json);
    return ESP_OK;
}

// POST /api/multi - Display multiple texts
static esp_err_t api_multi_handler(httpd_req_t *req) {
    // Allocate buffer on heap to save stack space
    char *content = malloc(2048);
    if (content == NULL) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    int ret = httpd_req_recv(req, content, 2047);

    if (ret <= 0) {
        free(content);
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    content[ret] = '\0';

    // Parse JSON
    cJSON *json = cJSON_Parse(content);
    free(content);  // Free buffer after parsing

    if (json == NULL) {
        const char *resp = "{\"error\":\"Invalid JSON\"}";
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, resp, strlen(resp));
        return ESP_FAIL;
    }

    // Get optional global orientation (applied to all texts)
    cJSON *orientation_item = cJSON_GetObjectItem(json, "orientation");
    if (orientation_item && cJSON_IsNumber(orientation_item)) {
        uint8_t orientation = orientation_item->valueint;
        ESP_LOGI(TAG, "Setting global orientation to %d°", orientation * 90);
        epaper_set_orientation(orientation);
    }

    // Get texts array
    cJSON *texts = cJSON_GetObjectItem(json, "texts");
    if (!cJSON_IsArray(texts)) {
        const char *resp = "{\"error\":\"texts must be an array\"}";
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, resp, strlen(resp));
        cJSON_Delete(json);
        return ESP_FAIL;
    }

    int count = cJSON_GetArraySize(texts);
    ESP_LOGI(TAG, "Drawing %d text items", count);

    // Clear display first
    epaper_display_clear();

    // Draw each text item
    for (int i = 0; i < count; i++) {
        cJSON *item = cJSON_GetArrayItem(texts, i);
        if (!cJSON_IsObject(item)) continue;

        cJSON *text_item = cJSON_GetObjectItem(item, "text");
        cJSON *x_item = cJSON_GetObjectItem(item, "x");
        cJSON *y_item = cJSON_GetObjectItem(item, "y");
        cJSON *color_item = cJSON_GetObjectItem(item, "color");
        cJSON *scale_item = cJSON_GetObjectItem(item, "scale");
        cJSON *font_item = cJSON_GetObjectItem(item, "font");

        const char *text = text_item && cJSON_IsString(text_item) ? text_item->valuestring : "";
        uint16_t x = x_item && cJSON_IsNumber(x_item) ? x_item->valueint : 0;
        uint16_t y = y_item && cJSON_IsNumber(y_item) ? y_item->valueint : 0;
        uint8_t color = color_item && cJSON_IsNumber(color_item) ? color_item->valueint : COLOR_BLACK;
        uint8_t scale = scale_item && cJSON_IsNumber(scale_item) ? scale_item->valueint : 1;
        uint8_t font = font_item && cJSON_IsNumber(font_item) ? font_item->valueint : 1; // Default to medium font

        ESP_LOGI(TAG, "  [%d] '%s' at (%d,%d) color=%d scale=%d font=%d", i, text, x, y, color, scale, font);

        // Use appropriate font (uses global orientation)
        if (font == 0) {
            epaper_draw_text(x, y, text, color, scale);  // Small 5x8 font
        } else if (font == 1) {
            epaper_draw_text_6x12(x, y, text, color, scale);  // Medium 6x12 font
        } else {
            epaper_draw_text_8x16(x, y, text, color, scale);  // Large 8x16 font
        }
    }

    // Update display once with all texts
    epaper_display_update();

    // Send response
    char resp[128];
    snprintf(resp, sizeof(resp), "{\"success\":true,\"message\":\"%d texts displayed\"}", count);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp, strlen(resp));

    cJSON_Delete(json);
    return ESP_OK;
}

// POST /api/clear - Clear display
static esp_err_t api_clear_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "Clearing display");

    epaper_display_clear();
    epaper_display_update();

    const char *resp = "{\"success\":true,\"message\":\"Display cleared\"}";
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp, strlen(resp));

    return ESP_OK;
}

// POST /api/rect - Draw rectangle
static esp_err_t api_rect_handler(httpd_req_t *req) {
    char content[256];
    int ret = httpd_req_recv(req, content, sizeof(content) - 1);

    if (ret <= 0) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    content[ret] = '\0';

    cJSON *json = cJSON_Parse(content);
    if (json == NULL) {
        const char *resp = "{\"error\":\"Invalid JSON\"}";
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, resp, strlen(resp));
        return ESP_FAIL;
    }

    // Extract parameters
    cJSON *x_item = cJSON_GetObjectItem(json, "x");
    cJSON *y_item = cJSON_GetObjectItem(json, "y");
    cJSON *w_item = cJSON_GetObjectItem(json, "w");
    cJSON *h_item = cJSON_GetObjectItem(json, "h");
    cJSON *color_item = cJSON_GetObjectItem(json, "color");
    cJSON *clear_item = cJSON_GetObjectItem(json, "clear");

    uint16_t x = x_item && cJSON_IsNumber(x_item) ? x_item->valueint : 0;
    uint16_t y = y_item && cJSON_IsNumber(y_item) ? y_item->valueint : 0;
    uint16_t w = w_item && cJSON_IsNumber(w_item) ? w_item->valueint : 50;
    uint16_t h = h_item && cJSON_IsNumber(h_item) ? h_item->valueint : 50;
    uint8_t color = color_item && cJSON_IsNumber(color_item) ? color_item->valueint : COLOR_BLACK;
    bool clear = clear_item && cJSON_IsBool(clear_item) ? cJSON_IsTrue(clear_item) : false;

    ESP_LOGI(TAG, "Drawing rect: (%d,%d) %dx%d color=%d", x, y, w, h, color);

    if (clear) {
        epaper_display_clear();
    }

    epaper_rect(x, y, w, h, color);
    epaper_display_update();

    const char *resp = "{\"success\":true,\"message\":\"Rectangle drawn\"}";
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp, strlen(resp));

    cJSON_Delete(json);
    return ESP_OK;
}

// POST /api/orientation - Set global screen orientation
static esp_err_t api_orientation_handler(httpd_req_t *req) {
    char content[128];
    int ret = httpd_req_recv(req, content, sizeof(content) - 1);

    if (ret <= 0) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    content[ret] = '\0';

    cJSON *json = cJSON_Parse(content);
    if (json == NULL) {
        const char *resp = "{\"error\":\"Invalid JSON\"}";
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, resp, strlen(resp));
        return ESP_FAIL;
    }

    // Extract orientation parameter
    cJSON *orientation_item = cJSON_GetObjectItem(json, "orientation");
    if (!orientation_item || !cJSON_IsNumber(orientation_item)) {
        const char *resp = "{\"error\":\"Missing or invalid orientation parameter\"}";
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, resp, strlen(resp));
        cJSON_Delete(json);
        return ESP_FAIL;
    }

    uint8_t orientation = orientation_item->valueint;
    if (orientation > ORIENTATION_270) {
        const char *resp = "{\"error\":\"Invalid orientation value (must be 0-3)\"}";
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, resp, strlen(resp));
        cJSON_Delete(json);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Setting global orientation to %d° (%d)", orientation * 90, orientation);
    epaper_set_orientation(orientation);

    char resp[128];
    snprintf(resp, sizeof(resp), "{\"success\":true,\"orientation\":%d,\"degrees\":%d}", orientation, orientation * 90);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp, strlen(resp));

    cJSON_Delete(json);
    return ESP_OK;
}

// Start web server
esp_err_t webserver_start(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;
    config.server_port = 80;

    ESP_LOGI(TAG, "Starting web server on port %d", config.server_port);

    if (httpd_start(&server, &config) == ESP_OK) {
        // Register URI handlers
        httpd_uri_t index_uri = {
            .uri = "/",
            .method = HTTP_GET,
            .handler = index_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &index_uri);

        httpd_uri_t api_text_uri = {
            .uri = "/api/text",
            .method = HTTP_POST,
            .handler = api_text_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &api_text_uri);

        httpd_uri_t api_multi_uri = {
            .uri = "/api/multi",
            .method = HTTP_POST,
            .handler = api_multi_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &api_multi_uri);

        httpd_uri_t api_clear_uri = {
            .uri = "/api/clear",
            .method = HTTP_POST,
            .handler = api_clear_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &api_clear_uri);

        httpd_uri_t api_rect_uri = {
            .uri = "/api/rect",
            .method = HTTP_POST,
            .handler = api_rect_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &api_rect_uri);

        httpd_uri_t api_orientation_uri = {
            .uri = "/api/orientation",
            .method = HTTP_POST,
            .handler = api_orientation_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &api_orientation_uri);

        ESP_LOGI(TAG, "Web server started successfully");
        return ESP_OK;
    }

    ESP_LOGE(TAG, "Failed to start web server");
    return ESP_FAIL;
}

// Stop web server
void webserver_stop(void) {
    if (server) {
        httpd_stop(server);
        server = NULL;
        ESP_LOGI(TAG, "Web server stopped");
    }
}
