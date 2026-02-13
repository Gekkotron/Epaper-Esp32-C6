#include "config.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "esp_log.h"
#include "esp_spiffs.h"

static const char *TAG = "config";
static config_t g_config = {0};
static bool g_initialized = false;

// Trim whitespace from both ends of a string
static void trim_whitespace(char *str) {
    if (!str) return;

    // Trim leading whitespace
    char *start = str;
    while (*start == ' ' || *start == '\t' || *start == '\r' || *start == '\n') {
        start++;
    }

    // Trim trailing whitespace
    char *end = start + strlen(start) - 1;
    while (end > start && (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n')) {
        end--;
    }
    *(end + 1) = '\0';

    // Move trimmed string to beginning if needed
    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }
}

// Parse a single line from .env file
static void parse_env_line(const char *line) {
    if (!line || line[0] == '#' || line[0] == '\0') {
        return; // Skip comments and empty lines
    }

    char line_copy[256];
    strncpy(line_copy, line, sizeof(line_copy) - 1);
    line_copy[sizeof(line_copy) - 1] = '\0';

    char *equals = strchr(line_copy, '=');
    if (!equals) {
        return; // Not a valid key=value pair
    }

    *equals = '\0';
    char *key = line_copy;
    char *value = equals + 1;

    trim_whitespace(key);
    trim_whitespace(value);

    // Remove quotes from value if present
    if (value[0] == '"' || value[0] == '\'') {
        size_t len = strlen(value);
        if (len > 1 && value[len-1] == value[0]) {
            value[len-1] = '\0';
            value++;
        }
    }

    // Store configuration values
    if (strcmp(key, "WIFI_SSID") == 0) {
        strncpy(g_config.wifi_ssid, value, CONFIG_WIFI_SSID_MAX_LEN - 1);
        g_config.wifi_ssid[CONFIG_WIFI_SSID_MAX_LEN - 1] = '\0';
        ESP_LOGI(TAG, "Loaded WIFI_SSID: %s", g_config.wifi_ssid);
    } else if (strcmp(key, "WIFI_PASSWORD") == 0) {
        strncpy(g_config.wifi_password, value, CONFIG_WIFI_PASSWORD_MAX_LEN - 1);
        g_config.wifi_password[CONFIG_WIFI_PASSWORD_MAX_LEN - 1] = '\0';
        ESP_LOGI(TAG, "Loaded WIFI_PASSWORD: ********");
    }
}

// Initialize SPIFFS and read .env file
bool config_init(void) {
    if (g_initialized) {
        ESP_LOGW(TAG, "Config already initialized");
        return true;
    }

    ESP_LOGI(TAG, "Initializing configuration...");

    // Configure SPIFFS
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true  // Auto-format if mount fails
    };

    // Mount SPIFFS
    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format SPIFFS");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition - did you flash partitions.csv?");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        ESP_LOGW(TAG, "SPIFFS not available - you can:");
        ESP_LOGW(TAG, "  1. Run: pio run --target uploadfs");
        ESP_LOGW(TAG, "  2. Or use hardcoded WiFi credentials in main.c");
        g_initialized = true;
        return false;
    }

    // Open .env file
    FILE *f = fopen("/spiffs/.env", "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open /spiffs/.env file");
        ESP_LOGW(TAG, "Using default configuration");
        esp_vfs_spiffs_unregister(NULL);
        g_initialized = true;
        return false;
    }

    // Read and parse file line by line
    char line[256];
    while (fgets(line, sizeof(line), f) != NULL) {
        parse_env_line(line);
    }

    fclose(f);
    esp_vfs_spiffs_unregister(NULL);

    // Validate configuration
    if (strlen(g_config.wifi_ssid) == 0) {
        ESP_LOGW(TAG, "WIFI_SSID not found in .env file");
    }
    if (strlen(g_config.wifi_password) == 0) {
        ESP_LOGW(TAG, "WIFI_PASSWORD not found in .env file");
    }

    g_initialized = true;
    ESP_LOGI(TAG, "Configuration loaded successfully");
    return true;
}

const config_t* config_get(void) {
    if (!g_initialized) {
        ESP_LOGW(TAG, "Config not initialized, call config_init() first");
    }
    return &g_config;
}

const char* config_get_wifi_ssid(void) {
    return g_config.wifi_ssid;
}

const char* config_get_wifi_password(void) {
    return g_config.wifi_password;
}
