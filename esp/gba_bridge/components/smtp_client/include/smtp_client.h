#include <stdint.h>
#include <stddef.h>
#include "esp_event.h"
#include "mbedtls/platform.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/ssl.h"

#ifndef SMTP_CLIENT_H
#define SMTP_CLIENT_H

/* Constants that are configurable in menuconfig */
#define MAIL_SERVER         CONFIG_SMTP_SERVER
#define MAIL_PORT           CONFIG_SMTP_PORT_NUMBER
#define SENDER_MAIL         CONFIG_SMTP_SENDER_MAIL
#define SENDER_PASSWORD     CONFIG_SMTP_SENDER_PASSWORD
#define RECIPIENT_MAIL      CONFIG_SMTP_RECIPIENT_MAIL

#define SERVER_USES_STARTSSL 1


#define TASK_STACK_SIZE     (8 * 1024)
#define BUF_SIZE            4096


#define VALIDATE_MBEDTLS_RETURN(ret, min_valid_ret, max_valid_ret, goto_label)  \
    do {                                                                        \
        if (ret < min_valid_ret || ret > max_valid_ret) {                       \
            goto goto_label;                                                    \
        }                                                                       \
    } while (0)                                                                 \


/**
 * Root cert for smtp.googlemail.com, taken from server_root_cert.pem
 *
 * The PEM file was extracted from the output of this command:
 * openssl s_client -showcerts -connect smtp.googlemail.com:587 -starttls smtp
 *
 * The CA root cert is the last cert given in the chain of certs.
 *
 * To embed it in the app binary, the PEM file is named
 * in the component.mk COMPONENT_EMBED_TXTFILES variable.
 */

extern const uint8_t server_root_cert_pem_start[] asm("_binary_server_root_cert_pem_start");
extern const uint8_t server_root_cert_pem_end[]   asm("_binary_server_root_cert_pem_end");

extern char *gba_message;
extern char *gba_message_subject;
extern char *gba_message_recipient[3];




void init_wifi();

void smtp_client_task(void *pvParameters);
#endif //SMTP_CLIENT_H