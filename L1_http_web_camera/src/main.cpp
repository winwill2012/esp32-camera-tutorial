#include <Arduino.h>
#include "esp_camera.h"
#include "camera_pins.h"
#include <WebServer.h>
WebServer server(80);

void handleRoot() {
    String html = R"HTML(
    <!DOCTYPE html>
    <html>
      <head>
        <title>ESP32 OV5640 摄像头流</title>
        <meta charset="utf-8">
        <style>
          body { text-align: center; margin-top: 50px; }
          img { max-width: 90%; border: 2px solid #333; }
          h1 { color: #333; font-family: Arial; }
        </style>
      </head>
      <body>
        <h1>ESP32 OV5640 摄像头实时流</h1>
        <!-- 引用MJPEG流接口 -->
        <img src="/stream" />
      </body>
    </html>
  )HTML";
    server.send(200, "text/html", html);
}

void handleStream() {
    // 设置响应头，告诉浏览器这是MJPEG流
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.send(200, "multipart/x-mixed-replace; boundary=frame");

    while (true) {
        // 获取摄像头帧数据
        camera_fb_t *fb = esp_camera_fb_get();
        if (!fb) {
            Serial.println("获取图像帧失败");
            delay(1000);
            continue;
        }

        // 拼接MJPEG帧边界（固定格式）
        String head = "--frame\r\nContent-Type: image/jpeg\r\nContent-Length: " + String(fb->len) + "\r\n\r\n";

        // 发送帧头和图像数据
        server.sendContent(head);
        server.sendContent(reinterpret_cast<const char *>(fb->buf), fb->len);
        server.sendContent("\r\n");

        // 释放帧缓冲区
        esp_camera_fb_return(fb);

        // 检查客户端是否断开连接，避免死循环
        if (!server.client().connected()) {
            break;
        }
        // // 控制帧率（可根据需要调整）
        delay(20);
    }
}

// 初始化WiFi连接
void initWiFi() {
    WiFiClass::mode(WIFI_STA);
    WiFi.begin("Xiaomi_E15A", "19910226");

    Serial.print("连接WiFi中...");
    while (!WiFi.isConnected()) {
        delay(500);
        Serial.print(".");
    }

    Serial.println();
    Serial.print("WiFi连接成功！IP地址: ");
    Serial.println(WiFi.localIP()); // 打印ESP32的局域网IP
}

void setup() {
    Serial.begin(115200);

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
    config.frame_size = FRAMESIZE_SVGA; // 800x600
    config.jpeg_quality = 8; // 0-63，数值越小质量越高
    config.fb_count = 2;

    // 摄像头初始化
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("摄像头初始化失败，错误代码: 0x%x", err);
    }
    Serial.println("初始化摄像头成功");

    initWiFi();

    // 注册路由：根路径返回网页，/stream返回图像流
    server.on("/", handleRoot);
    server.on("/stream", handleStream);

    // 启动Web服务器
    server.begin();
}

void loop() {
    server.handleClient();
}
