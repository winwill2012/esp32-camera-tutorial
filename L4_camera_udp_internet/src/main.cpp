#include <Arduino.h>
#include "esp_camera.h"
#include "camera_pins.h"
#include <WiFi.h>
#include <WiFiUdp.h>

// UDP数据包最大的长度（MTU为1500，多余的100留给包头）
#define MAX_UDP_PACKET_SIZE 1400

// 公网服务器推流地址
auto udpTargetIP = "119.29.197.186"; // UDP接收端的IP地址
constexpr uint16_t udpTargetPort = 9090; // UDP接收端的端口号

WiFiUDP udp; // UDP客户端对象
unsigned int localUdpPort = 1234; // 本地UDP端口

void sendFrameOverUDP() {
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("获取图像帧失败");
        return;
    }
    // MJPEG的开头和结尾两个字节是0XFF 0XD8 .... 0XFF 0XD9
    const int totalBytes = fb->len;
    int bytesSent = 0;
    while (bytesSent < totalBytes) {
        const int chunkSize = min(MAX_UDP_PACKET_SIZE, totalBytes - bytesSent);
        udp.beginPacket(udpTargetIP, udpTargetPort);
        udp.write(fb->buf + bytesSent, chunkSize);
        udp.endPacket();
        bytesSent += chunkSize;
        delay(1);
    }
    esp_camera_fb_return(fb);
}

// 初始化WiFi（原有逻辑不变）
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
    Serial.println(WiFi.localIP());
}

void setup() {
    Serial.begin(115200);

    // 摄像头初始化（原有逻辑不变）
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
    config.pixel_format = PIXFORMAT_JPEG;
    config.frame_size = FRAMESIZE_VGA;
    config.jpeg_quality = 5;
    config.fb_count = 2;

    const esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("摄像头初始化失败，错误代码: %0X\n", err);
    }
    Serial.println("初始化摄像头成功");
    initWiFi();
}

void loop() {
    sendFrameOverUDP();
    delay(10);
}
