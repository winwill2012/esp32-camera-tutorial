package main

import (
	"bufio"
	"bytes"
	"flag"
	"fmt"
	"net"
	"net/http"
	"time"

	log "github.com/sirupsen/logrus"
)

var (
	udpBuffer  = bytes.NewBuffer(nil) // 接收 UDP 数据的缓冲区
	latestJpeg []byte                 // 最新拼接好的 JPEG 帧
)

// 从缓冲区中提取完整的 JPEG 帧（核心逻辑）
func extractJpegFrame() {
	data := udpBuffer.Bytes()
	// 没有找到帧开始，清空缓冲区（避免无效数据堆积）
	if bytes.Compare(data[:2], []byte{0xFF, 0xD8}) != 0 {
		udpBuffer.Reset()
		return
	}
	if bytes.Compare(data[len(data)-2:], []byte{0xFF, 0xD9}) == 0 {
		latestJpeg = make([]byte, len(data))
		copy(latestJpeg, data) // 深拷贝，避免后续操作覆盖
		udpBuffer.Reset()
		log.Infof("[%s] 提取到完整JPEG帧，大小：%d 字节", time.Now().Format("2006-01-02 15:04:05"), len(latestJpeg))
	} else {
		copy(latestJpeg, []byte{})
	}
}

// 处理 UDP 数据接收
func handleUdpReceiver(udpAddr string) error {
	addr, err := net.ResolveUDPAddr("udp", udpAddr)
	if err != nil {
		log.Errorf("解析UDP地址失败：%v", err)
		return err
	}
	conn, err := net.ListenUDP("udp", addr)
	if err != nil {
		log.Errorf("监听UDP端口失败：%v", err)
		return err
	}
	log.Infof("UDP服务已启动，监听地址：%s", udpAddr)
	// 缓冲区大小适配 ESP32 的分包（1400 字节）
	buf := make([]byte, 1500)
	for {
		n, _, err := conn.ReadFromUDP(buf)
		if err != nil {
			log.Errorf("读取UDP数据失败：%v", err)
			continue
		}
		if n == 0 {
			continue
		}
		udpBuffer.Write(buf[:n])
		extractJpegFrame()
	}
}

func handleMjpegStream(w http.ResponseWriter, r *http.Request) {
	w.Header().Set("Content-Type", "multipart/x-mixed-replace; boundary=frame")

	// 创建缓冲写入器，提升写入效率
	writer := bufio.NewWriter(w)
	for {
		if len(latestJpeg) > 0 {
			// 按 MJPEG 格式发送帧
			writer.WriteString("--frame\r\n")
			writer.WriteString(fmt.Sprintf("Content-Length: %d\r\n", +len(latestJpeg)))
			writer.WriteString("Content-Type: image/jpeg\r\n\r\n")
			writer.Write(latestJpeg)
			writer.WriteString("\r\n")
			writer.Flush()
		}
		time.Sleep(50 * time.Millisecond)
	}
}

var (
	udpPort  int
	httpPort int
)

func main() {
	flag.IntVar(&udpPort, "udp", 9090, "UDP推流的端口")
	flag.IntVar(&httpPort, "http", 9091, "HTTP对外推流的端口")
	flag.Parse()
	go func() {
		if err := handleUdpReceiver(fmt.Sprintf(":%d", udpPort)); err != nil {
			fmt.Printf("UDP服务异常：%v", err)
		}
	}()
	http.HandleFunc("/stream", handleMjpegStream)
	fmt.Printf("HTTP MJPEG 服务已启动，公网访问地址：http://你的服务器IP:%d/stream", httpPort)
	if err := http.ListenAndServe(fmt.Sprintf(":%d", httpPort), nil); err != nil {
		fmt.Printf("HTTP服务启动失败：%v", err)
	}
}
