/*
 * SPDX-FileCopyrightText: 2022-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include "smtp_client.h"
 #include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "esp_err.h"
#include "esp_log.h"
#include "usb/usb_host.h"
#include "errno.h"
#include "driver/gpio.h"

#include "usb/hid_host.h"
#include "usb/hid_usage_keyboard.h"
#include "usb/hid_usage_mouse.h"

#include "esp_system.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"


#include "text_editor.h"
#include "mail_screen.h"

#include "my_globals.h"

#include "sensor.h"

#include "esp_timer.h"

#define GBA_HOST    SPI2_HOST

//#define PIN_NUM_MISO 13
#define PIN_NUM_MOSI 11
#define PIN_NUM_CLK  12
#define PIN_NUM_CS   10

/* GPIO Pin number for quit from example logic */
#define APP_QUIT_PIN                GPIO_NUM_0
spi_device_handle_t spi;
spi_bus_config_t buscfg;
spi_device_interface_config_t devcfg;
uint8_t data;

static const char *TAG = "example";

QueueHandle_t app_event_queue = NULL;
QueueHandle_t spi_send_queue = NULL;

QueueHandle_t text_editor_queue = NULL;
text_editor *te;

bool capslock = false;





uint64_t time_key_start;
uint64_t time_key_end;
uint64_t time_spi_start;
uint64_t time_spi_end;


/**
 * @brief APP event group
 *
 * Application logic can be different. There is a one among other ways to distinguish the
 * event by application event group.
 * In this example we have two event groups:
 * APP_EVENT            - General event, which is APP_QUIT_PIN press event (Generally, it is IO0).
 * APP_EVENT_HID_HOST   - HID Host Driver event, such as device connection/disconnection or input report.
 */
typedef enum {
    APP_EVENT = 0,
    APP_EVENT_HID_HOST
} app_event_group_t;

/**
 * @brief APP event queue
 *
 * This event is used for delivering the HID Host event from callback to a task.
 */
typedef struct {
    app_event_group_t event_group;
    /* HID Host - Device related info */
    struct {
        hid_host_device_handle_t handle;
        hid_host_driver_event_t event;
        void *arg;
    } hid_host_device;
} app_event_queue_t;

/**
 * @brief HID Protocol string names
 */
static const char *hid_proto_name_str[] = {
    "NONE",
    "KEYBOARD",
    "MOUSE"
};

/**
 * @brief Key event
 */
typedef struct {
    enum key_state {
        KEY_STATE_PRESSED = 0x00,
        KEY_STATE_RELEASED = 0x01
    }state;
    uint8_t modifier;
    uint8_t key_code;
} key_event_t;

/* Main char symbol for ENTER key */
#define KEYBOARD_ENTER_MAIN_CHAR    '\n'
/* When set to 1 pressing ENTER will be extending with LineFeed during serial debug output */
#define KEYBOARD_ENTER_LF_EXTEND    1
void send_data(spi_device_handle_t spi, const uint8_t *data, int len)
{
    esp_err_t ret;
    spi_transaction_t t;
    if (len == 0) {
        return;    //no need to send anything
    }
    memset(&t, 0, sizeof(t));       //Zero out the transaction
    t.length = len * 8;             //Len is in bytes, transaction length is in bits.
    t.tx_buffer = data;             //Sent Data
    t.user = (void*)1;              //D/C needs to be set to 1
    ret = spi_device_polling_transmit(spi, &t); //Transmit!
    assert(ret == ESP_OK);          //Should have had no issues.

    time_spi_end = esp_timer_get_time();
    //printf("\nTIME SPENT SPI TRANSFER: %lld us \n",time_spi_end-time_spi_start);
    //printf("SENT: %d\n", *data);


}
void spi_task(void *pvParameters) {
    uint8_t byte_to_send;
    while (1) {
        if (xQueueReceive(spi_send_queue, &byte_to_send, portMAX_DELAY)) {
            send_data(spi, &byte_to_send, 1);
        }
        vTaskDelay(4);
    }
}

void text_editor_task(void *pvParameters) {
    uint8_t data;
    while (1) {
        if (xQueueReceive(text_editor_queue, &data, portMAX_DELAY)) {
            text_editor_update(data, te);
        }
        //vTaskDelay(1);
    }
}




/**
 * @brief Scancode to ascii table
 */
const uint8_t spi_keycode2ascii [101][4] = {
    {0, 0, 0, 0}, /* HID_KEY_NO_PRESS        */
    {0, 0, 0, 0}, /* HID_KEY_ROLLOVER        */
    {0, 0, 0, 0}, /* HID_KEY_POST_FAIL       */
    {0, 0, 0, 0}, /* HID_KEY_ERROR_UNDEFINED */
    {'a', 'A', 233, 0}, /* HID_KEY_A               */
    {'b', 'B', 0, 0}, /* HID_KEY_B               */
    {'c', 'C', 230, 0}, /* HID_KEY_C               */
    {'d', 'D', 0, 0}, /* HID_KEY_D               */
    {'e', 'E', 0, 0}, /* HID_KEY_E               */
    {'f', 'F', 0, 0}, /* HID_KEY_F               */
    {'g', 'G', 0, 0}, /* HID_KEY_G  10             */
    {'h', 'H', 0, 0}, /* HID_KEY_H               */
    {136, 'I', 0, 0}, /* ı              */
    {'j', 'J', 0, 0}, /* HID_KEY_J               */
    {'k', 'K', 0, 0}, /* HID_KEY_K               */
    {'l', 'L', 0, 0}, /* HID_KEY_L               */
    {'m', 'M', 0, 0}, /* HID_KEY_M               */
    {'n', 'N', 0, 0}, /* HID_KEY_N               */
    {'o', 'O', 0, 0}, /* HID_KEY_O               */
    {'p', 'P', 0, 0}, /* HID_KEY_P               */
    {'q', 'Q', 0, '@'}, /* HID_KEY_Q     20          */
    {'r', 'R', 0, 0}, /* HID_KEY_R               */
    {'s', 'S', 0, 0}, /* HID_KEY_S               */
    {'t', 'T', 0, 0}, /* HID_KEY_T               */
    {'u', 'U', 0, 0}, /* HID_KEY_U               */
    {'v', 'V', 232, 0}, /* HID_KEY_V               */
    {'w', 'W', 0, 0}, /* HID_KEY_W               */
    {'x', 'X', 231, 0}, /* HID_KEY_X               */
    {'y', 'Y', 0, 0}, /* HID_KEY_Y               */
    {'z', 'Z', 0, 0}, /* HID_KEY_Z               */
    {'1', '!', 0, 0}, /* HID_KEY_1         30      */
    {'2', '\'', 0, 0}, /* HID_KEY_2               */
    {'3', '^', 0, 0}, /* HID_KEY_3               */
    {'4', '+', 0, '$'}, /* HID_KEY_4               */
    {'5', '%', 0, 0}, /* HID_KEY_5               */
    {'6', '&', 0, 0}, /* HID_KEY_6               */
    {'7', '/', 0, '{'}, /* HID_KEY_7               */
    {'8', '(', 0, '['}, /* HID_KEY_8               */
    {'9', ')', 0, ']'}, /* HID_KEY_9               */
    {'0', '=', 0, '}'}, /* HID_KEY_0               */
    {KEYBOARD_ENTER_MAIN_CHAR, KEYBOARD_ENTER_MAIN_CHAR, KEYBOARD_ENTER_MAIN_CHAR, 0}, /* HID_KEY_ENTER   40       */
    {220, 220, 220, 0}, /* HID_KEY_ESC             */
    {'\b', '\b', '\b', 0}, /* HID_KEY_DEL             */
    {221, 221, 221, 0}, /* HID_KEY_TAB             */
    {' ', ' ', 0, 0}, /* HID_KEY_SPACE           */
    {'*', '?', 0, '\\'}, /* HID_KEY_MINUS           */
    {'-', '_', 0, 0}, /* HID_KEY_EQUAL           */
    {128, 129, 0, 0}, /* ğ   */
    {138, 139, 0, 0}, /* ü   */
    {0, 0, 0, 0}, /*       */
    {',', ';', 0, 0}, /* HID_KEY_SHARP       50    */  // HOTFIX: for NonUS Keyboards repeat HID_KEY_BACK_SLASH
    {130, 131, 0, 0}, /* ş           */
    {'i', 137, 0, 0}, /* I           */
    {'"', 0, 0, 0}, /* HID_KEY_TILDE           */
    {132, 133, 0, 0}, /* ö           */
    {134, 135, 0, 0}, /* ç         */
    {'.', ':', 0, 0}, /* HID_KEY_SLASH           */
    {0, 0, 0, 0}, /*CAPS LOCK*/
    {255, 255, 255, 255}, /*F1*/ 
    {0, 0, 0, 0}, /**/
    {0, 0, 0, 0}, /*                     60*/
    {0, 0, 0, 0}, /**/
    {0, 0, 0, 0}, /**/
    {0, 0, 0, 0}, /**/
    {0, 0, 0, 0}, /**/
    {0, 0, 0, 0}, /**/
    {0, 0, 0, 0}, /**/
    {0, 0, 0, 0}, /**/
    {0, 0, 0, 0}, /**/
    {0, 0, 0, 0}, /**/
    {251, 251, 251, 0}, /*PRINT SCREEN          70*/
    {0, 0, 0, 0}, /**/
    {0, 0, 0, 0}, /**/
    {250, 250, 250, 0}, /*INSERT*/
    {205, 215, 0, 0}, /*HOME*/
    {0, 0, 0, 0}, /*PAGE UP*/
    {204, 204, 0, 0}, /*DELETE*/
    {206, 216, 0, 0}, /*END*/
    {0, 0, 0, 0}, /*PAGE DOWN*/
    {202, 212, 0, 0}, /*RIGHT ARROW*/
    {200, 210, 0, 0}, /*LEFT ARROW                 80*/
    {203, 213, 0, 0}, /*DOWN ARROW*/
    {201, 211, 0, 0}, /*UP ARROW */
    {253, 253, 253, 253}, /*        numpad NUMLOCK*/
    {'/', '/', 0, 0}, /*        numpad '/'  */
    {'*', '*', 0, 0}, /*        numpad '*'  */
    {'-', '-', 0, 0}, /*        numpad '-'  */
    {'+', '+', 0, 0}, /*        numpad '+'  */
    {'\n', '\n', '\n', 0}, /*        numpad enter*/
    {'1', '1', 0, 0}, /*        numpad 1    */
    {'2', '2', 0, 0}, /*        numpad 2                      90*/
    {'3', '3', 0, 0}, /*        numpad 3    */
    {'4', '4', 0, 0}, /*        numpad 4    */
    {'5', '5', 0, 0}, /*        numpad 5    */
    {'6', '6', 0, 0}, /*        numpad 6    */
    {'7', '7', 0, 0}, /*        numpad 7    */
    {'8', '8', 0, 0}, /*        numpad 8    */
    {'9', '9', 0, 0}, /*        numpad 9    */
    {'0', '0', 0, 0}, /**/
    {0, 0, 0, 0}, /**/
    {'<', '>', 0, '|'} /*                            100*/
};

/**
 * @brief Makes new line depending on report output protocol type
 *
 * @param[in] proto Current protocol to output
 */
static void  print_device_protocol(hid_protocol_t proto)
{
    static hid_protocol_t prev_proto_output = -1;

    if (prev_proto_output != proto) {
        prev_proto_output = proto;
        printf("\r\n");
        if (proto == HID_PROTOCOL_MOUSE) {
            printf("Mouse\r\n");
        } else if (proto == HID_PROTOCOL_KEYBOARD) {
            printf("Keyboard\r\n");
        } else {
            printf("Generic\r\n");
        }
        fflush(stdout);
    }
}

/**
 * @brief HID Keyboard modifier verification for capitalization application (right or left shift)
 *
 * @param[in] modifier
 * @return true  Modifier was pressed (left or right shift)
 * @return false Modifier was not pressed (left or right shift)
 *
 */
static inline int determine_mod(uint8_t modifier)
{
    
    if(modifier == 64 ||modifier == 5){
        printf("ALT");
        return 3;
    }

    if (((modifier & HID_LEFT_SHIFT) == HID_LEFT_SHIFT) ||
            ((modifier & HID_RIGHT_SHIFT) == HID_RIGHT_SHIFT)) {
         printf("SHIFT\n");
                return 1;
    }
    
    if (((modifier & HID_LEFT_CONTROL) == HID_LEFT_CONTROL) ||
            ((modifier & HID_RIGHT_CONTROL) == HID_RIGHT_CONTROL)) {
                printf("CTRL\n");
        return 2;
    }


    return 0;
}

//used to make turkish characters behave like normal letters with capslock
static inline int determine_key_type(int key_code) {
    if (key_code < 29) {
        return 0;
    }
    switch (key_code) {
        case 47: case 48:
        case 51: case 52:
        case 54: case 55:
            return 0;
        default:
            return 1;
    }
}

/**
 * @brief HID Keyboard get char symbol from key code
 *
 * @param[in] modifier  Keyboard modifier data
 * @param[in] key_code  Keyboard key code
 * @param[in] key_char  Pointer to key char data
 *
 * @return true  Key scancode converted successfully
 * @return false Key scancode unknown
 */
static inline bool keyboard_get_char(uint8_t modifier,
                                         uint8_t key_code,
                                         unsigned char *key_char)
{
    if(57 == key_code){
        capslock = !capslock;
        printf("CAPSLOCK");
    }
    uint8_t mod;
    uint8_t is_key_non_letter = determine_key_type(key_code);
    mod = (determine_mod(modifier)) ;

    //printf("\n\nmodifier code:     %d\n",modifier);
    //printf("mod after determine:     %d\n\n",mod);
    printf("Cash Rules Everything Around Me:%d \n",key_code);
    //if ((key_code >= HID_KEY_A) && (key_code <=  HID_KEY_NUM_LOCK))
    if ((key_code >= HID_KEY_A && key_code <=  100)) {
        if(capslock){
            if(1 == mod){//SHIFT is pressed with capslock on
                if(is_key_non_letter){//NUMBERS and PUNCTUTAION STUFF etc.
                    mod = 1;
                }
                else{
                    mod = 0;
                }

            }
            else if(0 == mod){
                if(is_key_non_letter){//NUMBERS and PUNCTUTAION STUFF etc.
                    mod = 0;
                }
                else{
                    mod = 1;
                }
            }
        }

        *key_char = spi_keycode2ascii[key_code][mod];

    } else {
        // All other key pressed
        return false;
    }

    return true;
}

/**
 * @brief HID Keyboard print char symbol
 *
 * @param[in] key_char  Keyboard char to stdout
 */
static inline void keyboard_print_char(unsigned int key_char)
{
    if (!!key_char) {
        printf("%c-%d-%x\n",key_char, key_char, key_char);
#if (KEYBOARD_ENTER_LF_EXTEND)
        if (KEYBOARD_ENTER_MAIN_CHAR == key_char) {
            printf("%c-%d-%x\n",key_char, key_char, key_char);
        }
#endif // KEYBOARD_ENTER_LF_EXTEND
        fflush(stdout);
    }
}

/**
 * @brief Key Event. Key event with the key code, state and modifier.
 *
 * @param[in] key_event Pointer to Key Event structure
 *
 */
static void key_event_callback(key_event_t *key_event)
{
    unsigned char key_char;

     print_device_protocol
    (HID_PROTOCOL_KEYBOARD);
    //printf("modifier: %d\n",key_event->modifier);
    if (KEY_STATE_PRESSED == key_event->state) {
        if (keyboard_get_char(key_event->modifier,
                                  key_event->key_code, &key_char)) {

            data = key_char;
            if(0 == data){
                return;
            }
            time_spi_start = esp_timer_get_time();
            xQueueSend(spi_send_queue, &key_char, 0);
            time_key_end = esp_timer_get_time();
            //printf("\nTIME SPENT IN KEY EVENT: %lld\n",time_key_end-time_key_start);
            //printf("\nSent value: %d\n",data);
            xQueueSend(text_editor_queue, &key_char, 0);
            keyboard_print_char(key_char);
            

        }
    }
}

/**
 * @brief Key buffer scan code search.
 *
 * @param[in] src       Pointer to source buffer where to search
 * @param[in] key       Key scancode to search
 * @param[in] length    Size of the source buffer
 */
static inline bool key_found(const uint8_t *const src,
                             uint8_t key,
                             unsigned int length)
{
    for (unsigned int i = 0; i < length; i++) {
        if (src[i] == key) {
            return true;
        }
    }
    return false;
}

/**
 * @brief USB HID Host Keyboard Interface report callback handler
 *
 * @param[in] data    Pointer to input report data buffer
 * @param[in] length  Length of input report data buffer
 */
static void hid_host_keyboard_report_callback(const uint8_t *const data, const int length)
{
    time_key_start = esp_timer_get_time();
    hid_keyboard_input_report_boot_t *kb_report = (hid_keyboard_input_report_boot_t *)data;

    if (length < sizeof(hid_keyboard_input_report_boot_t)) {
        return;
    }

    static uint8_t prev_keys[HID_KEYBOARD_KEY_MAX] = { 0 };
    key_event_t key_event;

    for (int i = 0; i < HID_KEYBOARD_KEY_MAX; i++) {

        // key has been released verification
        if (prev_keys[i] > HID_KEY_ERROR_UNDEFINED &&
                !key_found(kb_report->key, prev_keys[i], HID_KEYBOARD_KEY_MAX)) {
            key_event.key_code = prev_keys[i];
            key_event.modifier = 0;
            key_event.state = KEY_STATE_RELEASED;
            key_event_callback(&key_event);
        }

        // key has been pressed verification
        if (kb_report->key[i] > HID_KEY_ERROR_UNDEFINED &&
                !key_found(prev_keys, kb_report->key[i], HID_KEYBOARD_KEY_MAX)) {
            key_event.key_code = kb_report->key[i];
            key_event.modifier = kb_report->modifier.val;
            key_event.state = KEY_STATE_PRESSED;
            key_event_callback(&key_event);
        }
    }

    memcpy(prev_keys, &kb_report->key, HID_KEYBOARD_KEY_MAX);
}

/**
 * @brief USB HID Host Mouse Interface report callback handler
 *
 * @param[in] data    Pointer to input report data buffer
 * @param[in] length  Length of input report data buffer
 */
static void hid_host_mouse_report_callback(const uint8_t *const data, const int length)
{
    hid_mouse_input_report_boot_t *mouse_report = (hid_mouse_input_report_boot_t *)data;

    if (length < sizeof(hid_mouse_input_report_boot_t)) {
        return;
    }

    static int x_pos = 0;
    static int y_pos = 0;

    // Calculate absolute position from displacement
    x_pos += mouse_report->x_displacement;
    y_pos += mouse_report->y_displacement;

     print_device_protocol
    (HID_PROTOCOL_MOUSE);

    printf("X: %06d\tY: %06d\t|%c|%c|\r",
           x_pos, y_pos,
           (mouse_report->buttons.button1 ? 'o' : ' '),
           (mouse_report->buttons.button2 ? 'o' : ' '));
    fflush(stdout);
}

/**
 * @brief USB HID Host Generic Interface report callback handler
 *
 * 'generic' means anything else than mouse or keyboard
 *
 * @param[in] data    Pointer to input report data buffer
 * @param[in] length  Length of input report data buffer
 */
static void hid_host_generic_report_callback(const uint8_t *const data, const int length)
{
     print_device_protocol
    (HID_PROTOCOL_NONE);
    for (int i = 0; i < length; i++) {
        printf("%02X", data[i]);
    }
    putchar('\r');
}

/**
 * @brief USB HID Host interface callback
 *
 * @param[in] hid_device_handle  HID Device handle
 * @param[in] event              HID Host interface event
 * @param[in] arg                Pointer to arguments, does not used
 */
void hid_host_interface_callback(hid_host_device_handle_t hid_device_handle,
                                 const hid_host_interface_event_t event,
                                 void *arg)
{
    uint8_t data[64] = { 0 };
    size_t data_length = 0;
    hid_host_dev_params_t dev_params;
    ESP_ERROR_CHECK(hid_host_device_get_params(hid_device_handle, &dev_params));

    switch (event) {
    case HID_HOST_INTERFACE_EVENT_INPUT_REPORT:
        ESP_ERROR_CHECK(hid_host_device_get_raw_input_report_data(hid_device_handle,
                                                                  data,
                                                                  64,
                                                                  &data_length));

        if (HID_SUBCLASS_BOOT_INTERFACE == dev_params.sub_class) {
            if (HID_PROTOCOL_KEYBOARD == dev_params.proto) {
                hid_host_keyboard_report_callback(data, data_length);
            } else if (HID_PROTOCOL_MOUSE == dev_params.proto) {
                hid_host_mouse_report_callback(data, data_length);
            }
        } else {
            hid_host_generic_report_callback(data, data_length);
        }

        break;
    case HID_HOST_INTERFACE_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "HID Device, protocol '%s' DISCONNECTED",
                 hid_proto_name_str[dev_params.proto]);
        ESP_ERROR_CHECK(hid_host_device_close(hid_device_handle));
        break;
    case HID_HOST_INTERFACE_EVENT_TRANSFER_ERROR:
        ESP_LOGI(TAG, "HID Device, protocol '%s' TRANSFER_ERROR",
                 hid_proto_name_str[dev_params.proto]);
        break;
    default:
        ESP_LOGE(TAG, "HID Device, protocol '%s' Unhandled event",
                 hid_proto_name_str[dev_params.proto]);
        break;
    }
}

/**
 * @brief USB HID Host Device event
 *
 * @param[in] hid_device_handle  HID Device handle
 * @param[in] event              HID Host Device event
 * @param[in] arg                Pointer to arguments, does not used
 */
void hid_host_device_event(hid_host_device_handle_t hid_device_handle,
                           const hid_host_driver_event_t event,
                           void *arg)
{
    hid_host_dev_params_t dev_params;
    ESP_ERROR_CHECK(hid_host_device_get_params(hid_device_handle, &dev_params));

    switch (event) {
    case HID_HOST_DRIVER_EVENT_CONNECTED:
        ESP_LOGI(TAG, "HID Device, protocol '%s' CONNECTED",
                 hid_proto_name_str[dev_params.proto]);

        const hid_host_device_config_t dev_config = {
            .callback = hid_host_interface_callback,
            .callback_arg = NULL
        };

        ESP_ERROR_CHECK(hid_host_device_open(hid_device_handle, &dev_config));
        if (HID_SUBCLASS_BOOT_INTERFACE == dev_params.sub_class) {
            ESP_ERROR_CHECK(hid_class_request_set_protocol(hid_device_handle, HID_REPORT_PROTOCOL_BOOT));
            if (HID_PROTOCOL_KEYBOARD == dev_params.proto) {
                ESP_ERROR_CHECK(hid_class_request_set_idle(hid_device_handle, 0, 0));
            }
        }
        ESP_ERROR_CHECK(hid_host_device_start(hid_device_handle));
        break;
    default:
        break;
    }
}

/**
 * @brief Start USB Host install and handle common USB host library events while app pin not low
 *
 * @param[in] arg  Not used
 */
static void usb_lib_task(void *arg)
{
    const usb_host_config_t host_config = {
        .skip_phy_setup = false,
        .intr_flags = ESP_INTR_FLAG_LEVEL1,
    };

    ESP_ERROR_CHECK(usb_host_install(&host_config));
    xTaskNotifyGive(arg);

    while (true) {
        uint32_t event_flags;
        usb_host_lib_handle_events(portMAX_DELAY, &event_flags);
        // In this example, there is only one client registered
        // So, once we deregister the client, this call must succeed with ESP_OK
        if (event_flags & USB_HOST_LIB_EVENT_FLAGS_NO_CLIENTS) {
            ESP_ERROR_CHECK(usb_host_device_free_all());
            break;
        }
    }

    ESP_LOGI(TAG, "USB shutdown");
    // Clean up USB Host
    vTaskDelay(10); // Short delay to allow clients clean-up
    ESP_ERROR_CHECK(usb_host_uninstall());
    vTaskDelete(NULL);
}

/**
 * @brief BOOT button pressed callback
 *
 * Signal application to exit the HID Host task
 *
 * @param[in] arg Unused
 */
static void gpio_isr_cb(void *arg)
{
    BaseType_t xTaskWoken = pdFALSE;
    const app_event_queue_t evt_queue = {
        .event_group = APP_EVENT,
    };

    if (app_event_queue) {
        xQueueSendFromISR(app_event_queue, &evt_queue, &xTaskWoken);
    }

    if (xTaskWoken == pdTRUE) {
        portYIELD_FROM_ISR();
    }
}

/**
 * @brief HID Host Device callback
 *
 * Puts new HID Device event to the queue
 *
 * @param[in] hid_device_handle HID Device handle
 * @param[in] event             HID Device event
 * @param[in] arg               Not used
 */
void hid_host_device_callback(hid_host_device_handle_t hid_device_handle,
                              const hid_host_driver_event_t event,
                              void *arg)
{
    const app_event_queue_t evt_queue = {
        .event_group = APP_EVENT_HID_HOST,
        // HID Host Device related info
        .hid_host_device.handle = hid_device_handle,
        .hid_host_device.event = event,
        .hid_host_device.arg = arg
    };

    if (app_event_queue) {
        xQueueSend(app_event_queue, &evt_queue, 0);
    }
}





void app_main(void)
{

    esp_err_t ret;
    buscfg = (spi_bus_config_t) {
    .miso_io_num = PIN_NUM_CS,
    .mosi_io_num = PIN_NUM_MOSI,
    .sclk_io_num = PIN_NUM_CLK,
    .quadwp_io_num = -1,
    .quadhd_io_num = -1,
    .max_transfer_sz = 1,
    };
    devcfg = (spi_device_interface_config_t) {
    .clock_speed_hz = 256 * 1000,
    .mode = 3,                              
    .spics_io_num = PIN_NUM_CS,             //CS pin
    .queue_size = 1,                    
    // .pre_cb = lcd_spi_pre_transfer_callback, //Specify pre-transfer callback to handle D/C line
    };
    
    //Initialize the SPI bus
    ret = spi_bus_initialize(GBA_HOST, &buscfg, SPI_DMA_CH_AUTO);
    ESP_ERROR_CHECK(ret);
    ret = spi_bus_add_device(GBA_HOST, &devcfg, &spi);
    ESP_ERROR_CHECK(ret);
    data = 0;



    spi_send_queue = xQueueCreate(32, sizeof(uint8_t));
    xTaskCreatePinnedToCore(spi_task, "spi_task", 4096, NULL, 3, NULL, 1);


    te = create_text_editor(1800);
    text_editor_queue = xQueueCreate(4, sizeof(uint8_t));
    xTaskCreatePinnedToCore(text_editor_task, "text_editor_task", 8192, NULL, 1, NULL, 1);

    esp_timer_early_init();

    int64_t prev = esp_timer_get_time();
    init_wifi();
    int64_t now = esp_timer_get_time();
    printf("\nTIME SPENT INITIALIZING WIFI: %lld us \n",now-prev);
    printf("\nprev: %lld us \n",prev);
    printf("\nnow: %lld us \n",now);


    BaseType_t task_created;
    app_event_queue_t evt_queue;
    ESP_LOGI(TAG, "HID Host example");

    // Init BOOT button: Pressing the button simulates app request to exit
    // It will disconnect the USB device and uninstall the HID driver and USB Host Lib
    const gpio_config_t input_pin = {
        .pin_bit_mask = BIT64(APP_QUIT_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .intr_type = GPIO_INTR_NEGEDGE,
    };
    ESP_ERROR_CHECK(gpio_config(&input_pin));
    ESP_ERROR_CHECK(gpio_install_isr_service(ESP_INTR_FLAG_LEVEL1));
    ESP_ERROR_CHECK(gpio_isr_handler_add(APP_QUIT_PIN, gpio_isr_cb, NULL));

    /*
    * Create usb_lib_task to:
    * - initialize USB Host library
    * - Handle USB Host events while APP pin in in HIGH state
    */
    task_created = xTaskCreatePinnedToCore(usb_lib_task,
                                           "usb_events",
                                           4096,
                                           xTaskGetCurrentTaskHandle(),
                                           2, NULL, 0);
    assert(task_created == pdTRUE);

    // Wait for notification from usb_lib_task to proceed
    ulTaskNotifyTake(false, 1000);

    /*
    * HID host driver configuration
    * - create background task for handling low level event inside the HID driver
    * - provide the device callback to get new HID Device connection event
    */
    const hid_host_driver_config_t hid_host_driver_config = {
        .create_background_task = true,
        .task_priority = 5,
        .stack_size = 4096,
        .core_id = 0,
        .callback = hid_host_device_callback,
        .callback_arg = NULL
    };

    ESP_ERROR_CHECK(hid_host_install(&hid_host_driver_config));

    // Create queue
    app_event_queue = xQueueCreate(10, sizeof(app_event_queue_t));

    ESP_LOGI(TAG, "Waiting for HID Device to be connected");


    xTaskCreate(dht_task, "dht_task", 4096, NULL, 4, NULL);

    while (1) {
        // Wait queue
        if (xQueueReceive(app_event_queue, &evt_queue, portMAX_DELAY)) {
            if (APP_EVENT == evt_queue.event_group) {
                // User pressed button
                usb_host_lib_info_t lib_info;
                ESP_ERROR_CHECK(usb_host_lib_info(&lib_info));
                if (lib_info.num_devices == 0) {
                    // End while cycle
                    break;
                } else {
                    ESP_LOGW(TAG, "To shutdown example, remove all USB devices and press button again.");
                    // Keep polling
                }
            }

            if (APP_EVENT_HID_HOST ==  evt_queue.event_group) {
                hid_host_device_event(evt_queue.hid_host_device.handle,
                                      evt_queue.hid_host_device.event,
                                      evt_queue.hid_host_device.arg);
            } 
        }
    }

    ESP_LOGI(TAG, "HID Driver uninstall");
    ESP_ERROR_CHECK(hid_host_uninstall());
    gpio_isr_handler_remove(APP_QUIT_PIN);
    xQueueReset(app_event_queue);
    vQueueDelete(app_event_queue);
}
