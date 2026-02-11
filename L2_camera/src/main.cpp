#include <Arduino.h>
#include "esp_camera.h"
#include "camera_pins.h"
#include <SPI.h>
#include <TFT_eSPI.h> // Hardware-specific library
#include <TJpg_Decoder.h>

TFT_eSPI tft = TFT_eSPI(); // Invoke custom library

bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap) {
    tft.pushImage(x, y, w, h, bitmap);
    return true;
}

void setup() {
    Serial.begin(115200);
    TJpgDec.setSwapBytes(true);
    TJpgDec.setCallback(tft_output);
    tft.init();
    // if (tft.initDMA()) {
    //     Serial.println("启用DMA成功");
    // } else {
    //     Serial.println("启用DMA失败");
    // }
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK); // 清屏为黑色
    tft.setTextColor(TFT_WHITE); // 设置文字颜色
    tft.setCursor(10, 10);
    tft.println("Camera starting..."); // 显示初始化提示
    // 初始化摄像头，引脚详情参考根目录下的引脚图
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.ledc_timer = LEDC_TIMER_0;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.pixel_format = PIXFORMAT_JPEG; // 压缩过的jpeg图片
    config.frame_size = FRAMESIZE_HVGA; // 800x600
    config.jpeg_quality = 20; // 0-63，数值越小质量越高
    config.fb_count = 2;
    config.fb_location = CAMERA_FB_IN_PSRAM; // 缓存放到psram中
    config.grab_mode = CAMERA_GRAB_LATEST;

    // 摄像头初始化
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("摄像头初始化失败，错误代码: 0x%x", err);
    }
    Serial.println("初始化摄像头成功");
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(10, 10);
    tft.println("Camera ok");
}

void loop() {
    uint32_t t0 = micros(); // 1. 总耗时起点

    camera_fb_t *fb = esp_camera_fb_get();
    uint32_t t1 = micros(); // 2. 取帧耗时
    Serial.printf("取帧: %d us\n", t1 - t0);

    TJpgDec.drawJpg(0, 0, fb->buf, fb->len);
    uint32_t t2 = micros(); // 3. 解码+绘制总耗时
    Serial.printf("解码+绘制: %d us\n", t2 - t1);

    esp_camera_fb_return(fb);
    uint32_t t3 = micros(); // 4. 总循环耗时
    Serial.printf("总耗时: %d us\n\n", t3 - t0);
}
