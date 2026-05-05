idf_component_register(SRCS "smtp_client.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_event mbedtls esp_netif esp_timer

                    PRIV_REQUIRES lwip esp_wifi nvs_flash protocol_examples_common
                    EMBED_TXTFILES "server_root_cert.pem"
                    )