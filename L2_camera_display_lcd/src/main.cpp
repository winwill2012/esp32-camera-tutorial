#include <Arduino.h>
#include "esp_camera.h"
#include "camera_pins.h"
#include <TFT_eSPI.h>
#include <TJpg_Decoder.h>

TFT_eSPI tft = TFT_eSPI();

// JPEG解码完成后，执行的回调函数
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap) {
    // 将图像信息显示到屏幕中心点,内部会有裁剪的功能
    // 这里案例中：
    // 摄像头输出的分辨率是 400*296,而屏幕分辨率是：480*320
    // 所以可以增加偏移量，让图像刚好显示在屏幕正中央
    tft.pushImage(x + 40, y + 12, w, h, bitmap);
    return true;
}

void setup() {
    Serial.begin(115200);
    // 解码后的RGB565需要用两个字节来表示一个像素的颜色值，这里交换为true表示使用小端序来表示
    // 对于ESP32/STM32都需要设置为true
    TJpgDec.setSwapBytes(true);
    // 设置解码完成后的回调函数
    TJpgDec.setCallback(tft_output);
    tft.init();
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
    config.pixel_format = PIXFORMAT_JPEG; // 压缩过的jpeg图片
    config.frame_size = FRAMESIZE_CIF; // 400X296（建议<=屏幕分辨率）
    config.jpeg_quality = 20; // 0-63，数值越小质量越高
    config.fb_count = 2;
    config.fb_location = CAMERA_FB_IN_PSRAM; // 缓存放到PSRAM中
    config.grab_mode = CAMERA_GRAB_LATEST; // 只保留最新的一帧图像，老图像如果业务处理不过来，就直接丢弃了

    // 摄像头初始化
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("摄像头初始化失败，错误代码: 0x%x", err);
    }
    Serial.println("初始化摄像头成功");
    tft.fillScreen(TFT_BLACK);
}

void loop() {
    // 从摄像头获取一帧图像
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("取帧失败");
        return;
    }
    // JPEG解码，每解码完成一个MCU，会调用上面设置的回调函数将图像显示在屏幕
    TJpgDec.drawJpg(0, 0, fb->buf, fb->len);
    // 释放图像缓冲区
    esp_camera_fb_return(fb);
}
