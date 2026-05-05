#include "sensor.h"
#include "my_globals.h"
uint8_t last_temperature = 0;
uint8_t last_humidity = 0;

volatile int reading_changed = 0;

void dht_task(void *pvParameters)
{
    short temperature, humidity;
    uint8_t t, h;
    printf("DHT Task Started!\n");
    while(1)
    {
        // This library uses a simple function to read integer data
        if (dht_read_data(SENSOR_TYPE, SENSOR_GPIO, &humidity, &temperature) == ESP_OK)
        {
            t = (uint8_t) (temperature / 10);
            h = (uint8_t) (humidity / 10);
            if(last_temperature != t || last_humidity != h){
                reading_changed = 1;
            }
            else{
                reading_changed = 0;
            }
            last_temperature = t;
            last_humidity = h;
            if(reading_changed){
                printf("new sensor data");
                uint8_t val = 240;//TELL THE GBA WE WILL BE SENDING SENSOR READINGS
                xQueueSend(spi_send_queue, &val, 0);
                xQueueSend(spi_send_queue, &last_temperature, 0);
                xQueueSend(spi_send_queue, &last_humidity, 0);
            }
            ESP_LOGI("DHT11_SENSOR", "Humidity: %d, Temperature: %d°C", last_humidity, last_temperature);
        }
        else
        {
            ESP_LOGE("DHT11_SENSOR", "Failed to read data from sensor");
        }

        // Wait 5 seconds before the next reading
        vTaskDelay(pdMS_TO_TICKS(5000));

    }
}