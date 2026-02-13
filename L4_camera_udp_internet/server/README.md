### 使用方法
目录下的三个可执行文件，按照后缀区分操作系统，`main.go`是源码，仅供参考
```shell
esp32-cam-stream-convertor-windows.exe --help
  -http int
        HTTP对外推流的端口 (default 9091)
  -udp int
        UDP推流的端口 (default 9090)
```

linux系统启动示例：
```shell
./esp32-cam-stream-convertor -udp 9090 -http 9091
```