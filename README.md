# Video_Server
支持http/ts、http/hls、rtsp/ts,rtmp协议的推流服务


流串格式：
rtsp://192.168.174.128:7000/play/11.ts\r\n
http://192.168.174.128:7000/play/cctv5.ts\r\n
http://192.168.174.128:7000/play/cctv5.m3u8\r\n

rtmp 占时只支持vod播放，live待开发。。。
play:rtmp://192.168.174.128/vod/11
publish:rtmp://192.168.174.128/vod/22