// Did- idf.py add-dependency "espressif/esp32-camera" (automatically created idf_component.yml)
    // idf.py reconfigure
    // ctrl+,- code runner executor map... changed from "python -u" to "py $fileName"

// Add these to CMake...
#include "camera.hpp"  // Why not include/camera.hpp
#include "esp_log.h"

// #include "esp_psram.h"
// ESP_LOGI("PSRAM", "Initialized: %s", esp_psram_is_initialized() ? "YES" : "NO");

// #define BOARD_ESP32CAM_AITHINKER

// #include <esp_system.h>
// #include <nvs_flash.h>
// #include <sys/param.h>

// support IDF 5.x
// #ifndef portTICK_RATE_MS
// #define portTICK_RATE_MS portTICK_PERIOD_MS
// #endif

// ESP32Cam (AiThinker) PIN Map
// #ifdef BOARD_ESP32CAM_AITHINKER
#define CAM_PIN_PWDN 32
#define CAM_PIN_RESET -1 //software reset will be performed
#define CAM_PIN_XCLK 0
#define CAM_PIN_SIOD 26
#define CAM_PIN_SIOC 27

#define CAM_PIN_D7 35
#define CAM_PIN_D6 34
#define CAM_PIN_D5 39
#define CAM_PIN_D4 36
#define CAM_PIN_D3 21
#define CAM_PIN_D2 19
#define CAM_PIN_D1 18
#define CAM_PIN_D0 5
#define CAM_PIN_VSYNC 25
#define CAM_PIN_HREF 23
#define CAM_PIN_PCLK 22
// #endif

static const char *TAG = "CAMERA";  // 

#if ESP_CAMERA_SUPPORTED
static camera_config_t camera_config = {  // Fixed ..., this is a variable not a func
    .pin_pwdn = CAM_PIN_PWDN,
    .pin_reset = CAM_PIN_RESET,
    .pin_xclk = CAM_PIN_XCLK,
    .pin_sccb_sda = CAM_PIN_SIOD,
    .pin_sccb_scl = CAM_PIN_SIOC,

    .pin_d7 = CAM_PIN_D7,
    .pin_d6 = CAM_PIN_D6,
    .pin_d5 = CAM_PIN_D5,
    .pin_d4 = CAM_PIN_D4,
    .pin_d3 = CAM_PIN_D3,
    .pin_d2 = CAM_PIN_D2,
    .pin_d1 = CAM_PIN_D1,
    .pin_d0 = CAM_PIN_D0,
    .pin_vsync = CAM_PIN_VSYNC,
    .pin_href = CAM_PIN_HREF,
    .pin_pclk = CAM_PIN_PCLK,

    // XCLK 20MHz or 10MHz for OV2640 double FPS (Experimental)
    .xclk_freq_hz = 20000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_JPEG,  // YUV422,GRAYSCALE,RGB565,JPEG
    .frame_size = FRAMESIZE_UXGA,  // QQVGA-UXGA, For ESP32, do not use sizes above QVGA when not JPEG. The performance of the ESP32-S series has improved a lot, but JPEG mode always gives better frame rates.
    // Improve this later... quality vs filesize (for transfer)...

    .jpeg_quality = 8,  // 12(default?), 0-63, for OV series camera sensors, lower number means higher quality
    .fb_count = 2,  // When jpeg mode is used, if fb_count more than one, the driver will work in continuous mode.
    .fb_location = CAMERA_FB_IN_PSRAM,
    // .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
    .grab_mode = CAMERA_GRAB_LATEST,
    .sccb_i2c_port = 0,
};

esp_err_t cameraInit() {  // Removed static to be called in .hpp
    //initialize the camera
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Camera Init Failed: %s", esp_err_to_name(err));
        return err;
    }

    sensor_t *sen = esp_camera_sensor_get();
    sen->set_whitebal(sen, 1);  // Auto white bal
    sen->set_awb_gain(sen, 1);  // Auto white bal gain
    sen->set_exposure_ctrl(sen, 1);  // Auto exposure
    sen->set_gain_ctrl(sen, 1);  // Auto gain

    return ESP_OK;
}
#endif