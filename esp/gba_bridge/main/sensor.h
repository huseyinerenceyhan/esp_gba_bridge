#ifndef SENSOR_H
#define SENSOR_H

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "dht.h" // Header from the esp-idf-lib/dht component

// Define the GPIO pin and sensor type
#define SENSOR_TYPE DHT_TYPE_DHT11
#define SENSOR_GPIO 4 // Make sure this is the GPIO pin you are using




void dht_task(void *pvParameters);


#endif //SENSOR_H