#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>

// Maximum lengths for configuration values
#define CONFIG_WIFI_SSID_MAX_LEN 32
#define CONFIG_WIFI_PASSWORD_MAX_LEN 64

// Configuration structure
typedef struct {
    char wifi_ssid[CONFIG_WIFI_SSID_MAX_LEN];
    char wifi_password[CONFIG_WIFI_PASSWORD_MAX_LEN];
} config_t;

// Initialize configuration (reads from .env file in SPIFFS)
bool config_init(void);

// Get the global configuration
const config_t* config_get(void);

// Get individual values
const char* config_get_wifi_ssid(void);
const char* config_get_wifi_password(void);

#endif // CONFIG_H
