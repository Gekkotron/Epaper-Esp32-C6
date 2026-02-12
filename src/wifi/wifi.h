#ifndef WIFI_H
#define WIFI_H

#include <stdbool.h>
#include "esp_err.h"

/**
 * @brief WiFi connection status
 */
typedef enum {
    WIFI_STATUS_DISCONNECTED = 0,
    WIFI_STATUS_CONNECTING,
    WIFI_STATUS_CONNECTED,
    WIFI_STATUS_FAILED
} wifi_status_t;

/**
 * @brief Initialize WiFi in station mode
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t wifi_mgr_init(void);

/**
 * @brief Connect to a WiFi network
 *
 * @param ssid WiFi network SSID
 * @param password WiFi network password
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t wifi_mgr_connect(const char *ssid, const char *password);

/**
 * @brief Disconnect from WiFi network
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t wifi_mgr_disconnect(void);

/**
 * @brief Get current WiFi connection status
 *
 * @return Current WiFi status
 */
wifi_status_t wifi_mgr_get_status(void);

/**
 * @brief Check if WiFi is connected
 *
 * @return true if connected, false otherwise
 */
bool wifi_mgr_is_connected(void);

/**
 * @brief Wait for WiFi connection with timeout
 *
 * @param timeout_ms Timeout in milliseconds
 * @return ESP_OK if connected, ESP_ERR_TIMEOUT if timeout reached
 */
esp_err_t wifi_mgr_wait_for_connection(uint32_t timeout_ms);

/**
 * @brief Get IP address as string
 *
 * @param ip_str Buffer to store IP string (minimum 16 bytes)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t wifi_mgr_get_ip_address(char *ip_str);

/**
 * @brief Deinitialize WiFi
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t wifi_mgr_deinit(void);

#endif // WIFI_H
