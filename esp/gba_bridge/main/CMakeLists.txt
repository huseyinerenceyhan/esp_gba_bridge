idf_component_register(SRCS "sensor.c" "mail_screen.c" "text_editor.c" "visual.c" "gapbuf.c" "main.c"
                       INCLUDE_DIRS "."
                       PRIV_REQUIRES usb esp_driver_gpio esp_driver_spi smtp_client esp_timer
                       )
