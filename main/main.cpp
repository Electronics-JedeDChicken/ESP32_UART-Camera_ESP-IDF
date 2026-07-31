/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

// See Notes.txt...
// https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/index.html
// https://youtu.be/eot6COwCPF0
// To dos- board (selected custom board...), port, psram, 
    // Increase baud rate (for more throughput), improve img qual, on flash when needed..., throw away technique
    // UART protocol- simple vs standard vs w/ markers (magic#?)...
    // Btn as interrupt not polling, Btn debounce, Tasks&Schedule
        // Ignore presses while busy (processing please wait...)
    // Multiple imgs w/ timestamps&idx's- let python handle
        // For idx- python scans directory to know latest idx > send to esp32 > esp32 uses that
    // Reliability- check size, ACKs, retransmissions, timeout, #retransmissions
        // CRC, packetize img
    // Refactor receiver.py, 
    // else vs no else...

// FreeRTOS (https://youtu.be/WQGAs9MwXno)- xTaskCreate()... scheduling, vTaskDelay()
// UART- 
    // CH340- popular&low-cost USB-UART bridge chip... for MCUs to communicate w/ computers via USB
    // ESP32 UART0 > CH340 > USB > Laptop OR ESP32TX > Adapter RX then Adapter TX > ESP32 RX then GND <> GND
// OV2640 Camera- 
// Logging- 2 min args to be passed- tag & msg
    // Log lvls- LOGV (verbose, highest) > LOGD (debug), LOGI (info, default?), LOGW (warn), LOGE (err)
        // Verbose- wordy?
        // Adjust in Menuconfig- Component config > Log output

// Tips
// Close serial monitor when not using (to upload...)
// Just upload (not upload and monitor) then manually monitor in serial monitor
// Full clean (clean build envi/folder & dependencies..., when added dependencies?) vs Erase Flash ()

// Roadmap
// Installations- platformio & esp-idf
// Other Prereqs- uploading, serial monitor, main.cpp, check GPIO... (blink led, read btn state, detect btn press/release, polling&interrupt)
// FreeRTOS- tasks, vTaskDelay()
// UART- send txt then receive on laptop (py)
// Camera- init ov2640, verify psram, capture frame, print img size
// UART Img Protocol- header, size, jpeg data (packet&framing)
// Send JPEG- 
// Finalize- classes, debounce, multiple photos, CRC/checksum, timestamp, 

// Headers- C/C++ > Other Posix > IDF Headers > Component > Public > Private Headers
// #include ”<>” (custom? / project / esp-idf component headers) vs <<>> (standard C/C++ library)
    // include/ folder…- platformio automatically adds this to compiler’s search path
    // main.cpp has #include “camera.hpp” that calls camera_init() func (implemented in camera.cpp)- linker connects the func call to implementation…
#include <iostream>  // std::cout <<
// #include <stdio.h>  // C, std::printf()
// # include <print>  // C++, modern, std::print()
#include <string>
// #include <string.h>  // For C
// #include <cstring>  // For C++, legacy & low-level
#include <cstdint>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"  // These 2 has vTaskDelay(), 
#include "esp_log.h"  // For serial monitor / logging
// #include "esp_err.h"
#include "driver/gpio.h"  // Has the config stuff, must add in CMake (v6.0.2 update)
#include "driver/uart.h"

// #include "esp_psram.h"
#include "camera.hpp"

// #include <inttypes.h>
// #include "sdkconfig.h"
// #include "esp_chip_info.h"
// #include "esp_flash.h"
// #include "esp_system.h"

// Good practice to put pins here?
#define BTN_PIN GPIO_NUM_13  // #define- 
#define FLASH_PIN GPIO_NUM_4
#define UART_CHANNEL UART_NUM_0  // UART0 pins are already connected internally to CH340

// static const char* TAG = "MAIN";  // W/ const qualifier (have scope, only accessible w/in that scope)
    // Can also do #define TAG "MAIN" (macros)- macros have limitation (no scope thus can be accessed everywhere, waste of mem)
    // Identifies msg src?, 1 per module/file (to know w/c module logs) but can be many per whole project 
        // (standard, but can be many per module/file too…)
        // static- visible only inside module/file?
        // ESP-IDF logging expects C str?
static const char *CAMERA_TAG = "CAMERA_MAIN";

volatile TickType_t tick_prev = 0;
constexpr TickType_t DEBOUNCE_TICKS = pdMS_TO_TICKS(50);  // 50ms

// Declarations
// Inits
void gpioInit();
void uartInit();

// Tasks
// void loggerTask(void *Parameters);
// void counterTask(void *Parameters);
TaskHandle_t camera_task_handle = nullptr;  // 
void cameraTask(void *Parameters);

// Funcs
bool isBtnPressed(int state_prev);
void capture();
void uartSendSize(size_t size);
// void uartSendSize(uint32_t size);
void uartSendImg(const uint8_t * img, size_t size);

// ISR
static void IRAM_ATTR btnISRHandler(void *arg) {  // Call this upon btn press
    BaseType_t higherPriorityTaskWoken = pdFALSE;
    
    // Debounce
    TickType_t tick_current = xTaskGetTickCountFromISR();
    if ((tick_current - tick_prev) < DEBOUNCE_TICKS) {
        return;
    }
    tick_prev = tick_current;

    vTaskNotifyGiveFromISR(camera_task_handle, &higherPriorityTaskWoken);

    if (higherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}

extern "C" void app_main() {  // Force C linkage so framework / C-based bootloader can link entry func / Name Mangling?
    // void func() (modern C++) vs void func(void) (C-style, also valid in C++)

    // Removed Print example

    // Inits
    gpioInit();
    uartInit();

    // Interrupt Inits
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BTN_PIN, btnISRHandler, nullptr);

    // Tasks
    /*
    xTaskCreate(  // Dynamic
        loggerTask,     // Task func
        "LoggerTask",   // Task name, for debugging (seen when all running tasks are previewed...)
        2048,           // Stack size, 1000 can be enough?
        NULL,           // Params, no params for now...
        1,              // Prio, lower the lower prio (important when 2 tasks compete for resources)
        NULL            // Task handle, allows to w/ task from w/in other tasks
    );

    xTaskCreate(
        counterTask, 
        "CounterTask", 
        2048, 
        NULL, 
        1, 
        NULL
    );
    */
    xTaskCreate(
        cameraTask, 
        "CameraTask", 
        4096, 
        NULL, 
        2, 
        &camera_task_handle
    );  // After initializing this then you can use FreeRTOS task APIs to handle...

    // Testing GPIO & serial monitor, UART
    /*
    int state_prev = 1;
    while (1) {  // Like Main
        int state_current = gpio_get_level(BTN_PIN);
        if (state_prev == 1 && state_current == 0) {
            // ESP_LOGI(TAG, "Button Pressed!");  // For testing GPIO & serial monitor
            const char *msg = "CAPTURE\n";
            uart_write_bytes(UART_CHANNEL, msg, strlen(msg));
        }
        state_prev = state_current;
        vTaskDelay(pdMS_TO_TICKS(20));
    }
    */

    // int state_prev = gpio_get_level(BTN_PIN);  // Track prev state outside loop init, pullup
    // Testing esp32-camera
    // #if ESP_CAMERA_SUPPORTED
    if (ESP_OK != cameraInit()) {
        return;
    }

    // while (1) {
        // if (isBtnPressed(state_prev)) {
        //     capture();
        // }
        // state_prev = gpio_get_level(BTN_PIN);
        // ESP_LOGI(TAG, "Running...");
        // vTaskDelay(pdMS_TO_TICKS(20));  // Simple debounce, polling interval?, convert (20) ms to ticks, 
        // or vTaskDelay(20 / portTICK_PERIOD_MS);
    // }
    // #else
    //     ESP_LOGE("CAMERA_MAIN", "Camera support is not available for this chip");
    //     return;
    // #endif
}

// Definitions
// Inits
void gpioInit() {
    // GPIO
    // esp_err_t
    // gpio_reset_pin(BTN_PIN);
    gpio_set_direction(BTN_PIN, GPIO_MODE_INPUT);  // Can click on syntax/method then F12 to look deeper...
    gpio_set_pull_mode(BTN_PIN, GPIO_PULLUP_ONLY);
    gpio_set_intr_type(BTN_PIN, GPIO_INTR_NEGEDGE);

    // Flash
    gpio_set_direction(FLASH_PIN, GPIO_MODE_OUTPUT);
}

void uartInit() {
    // Functional Overview- 1-3 are config, 4 is operation, 5-6 are optional
        // Install Drivers- 
    uart_driver_install(UART_CHANNEL, 1024, 0, 0, NULL, 0);
        // Set Comm Params- baud rate, data bits, stop bits; parity, flow ctrl
    uart_set_baudrate(UART_CHANNEL, 921600);
    uart_set_word_length(UART_CHANNEL, UART_DATA_8_BITS);
    uart_set_stop_bits(UART_CHANNEL, UART_STOP_BITS_1);
        // Set Comm Pins- we use CH340...
        // Run UART Comm- sending/receiving data
        // Interrupts- 
        // Deleting a Driver- freeing allocated resources if a UART comm is no longer required
}

// Tasks
/*
void loggerTask(void *Parameters) {  // Optional params later...
    // for(;;);  //
    while (1) {
        ESP_LOGI("LOGGER", "Running...");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void counterTask(void *Parameters) {
    int count = 0;
    while (1) {
        ESP_LOGI("COUNTER", "Count = %d", count++);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
*/

void cameraTask(void *Parameters) {
    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);  // Sleep til ISR wakes us, portMAX_DELAY to wait indefinitely
        capture();
    }
}

// Funcs
bool isBtnPressed(int state_prev) {  // Pass by val since just check val (didn't modify)
    int state_current = gpio_get_level(BTN_PIN);
    // return (state_prev == 1 && state_current == 0) ? true : false;  // Falling-edge (high-low) detection, Ternary operation
    if (state_prev == 1 && state_current == 0) {
        // ESP_LOGI("BTN_TAG", "Btn Pressed");
        return true;
    }
    else {
        return false;
    }
}

void capture() {
    // ESP_LOGI(CAMERA_TAG, "Attempting to take picture...");
    gpio_set_level(FLASH_PIN, 1);  // Turn on Flash
    vTaskDelay(pdMS_TO_TICKS(200));
    camera_fb_t *pic = esp_camera_fb_get();  // Captures 1 frame from camera, returns pointer to frame buffer
    // esp_camera_fb_return(pic);  // Throw away 1st frame technique
    // pic = esp_camera_fb_get();
    // camera_fb_t (F12, struct containing captured img), has buf/img data, len/size, width, height, format
    gpio_set_level(FLASH_PIN, 0);

    if (pic == nullptr) {
        ESP_LOGE(CAMERA_TAG, "Capture Failed");
        return;  // Returns no value so ok in void... stop immediately...
    }

    for (int i=0; i<3; i++) {  // 3 retries
        // Avoid logs here as they may corrupt JPEG stream
        uartSendSize(pic->len);
        // uartSendSize(static_cast<uint32_t>(pic->len));
        uartSendImg(pic->buf, pic->len);

        uint8_t response;
        int ack = uart_read_bytes(UART_CHANNEL, &response, 1, pdMS_TO_TICKS(2000));  // Returns -1 if err or >=0 for #bytes read

        if (ack == 1) {  // If read 1byte
            if (response == 'A') {
                break;
            }
            else if (response == 'N') {
                continue;
            }
        }
    }

    // Use pic->buf to access the image
    // ESP_LOGI(CAMERA_TAG, "Picture taken w/ size: %zu bytes", pic->len);  // ->- 
    esp_camera_fb_return(pic);  // returns frame buffer to driver (always call when done, otherwise we'll run out of frame buffers)
    // vTaskDelay(5000 / portTICK_RATE_MS);  // Timing in main...
}

/*
void uartSendSize(size_t size) {
    // char buf_cam[32] = "CAPTURE\n";  // We'll put size here... into str
    // snprintf(buf_cam, sizeof(buf_cam), "Size: %zu bytes\n", size);  // 
        // sizeof() (returns allocated mem size) vs strlen (returns str length til \0)
    // ESP_LOGI(CAMERA_TAG, "Sending: %zu", strlen(buf_cam));
    // uart_write_bytes(UART_CHANNEL, buf_cam, strlen(buf_cam));
    
    std::string msg = "Size: " + std::to_string(size) + " bytes\n";
    // ESP_LOGI(CAMERA_TAG, "SENDING: %s", msg.c_str());
    uart_write_bytes(UART_CHANNEL, msg.c_str(), msg.length());  // .c_str() (to const char*), .length() or .size() (size_t), .empty (bool)
}
*/

void uartSendSize(size_t size) {
    uint32_t size_32 = static_cast<uint32_t>(size);
    // 
    uart_write_bytes(UART_CHANNEL, reinterpret_cast<const char *>(&size_32), sizeof(size_32));
}

void uartSendImg(const uint8_t *img, size_t size) {  // const so we won't modify it...
    uart_write_bytes(UART_CHANNEL, reinterpret_cast<const char *>(img), size);
    // 
}