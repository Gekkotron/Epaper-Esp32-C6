#ifndef WEBSERVER_H
#define WEBSERVER_H

#include "esp_err.h"

// Initialize and start the web server
esp_err_t webserver_start(void);

// Stop the web server
void webserver_stop(void);

#endif // WEBSERVER_H
