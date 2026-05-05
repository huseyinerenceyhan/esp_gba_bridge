#ifndef MY_GLOBALS_H
#define MY_GLOBALS_H



extern volatile int sending_mail;

extern uint8_t last_temperature;
extern uint8_t last_humidity;

extern volatile int reading_changed;

extern QueueHandle_t spi_send_queue;

#endif // MY_GLOBALS_H