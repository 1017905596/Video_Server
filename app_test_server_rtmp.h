#ifndef _APP_TEST_SERVER_RTMP_H_
#define _APP_TEST_SERVER_RTMP_H_

#include "event_listener.h"
#include "event_server.h"
#include "message_block.h"
#include "rtmp_amf0.h"
#include <string>

class app_test_server_rtmp;
#define SEND_DATA_MAX 1024*1024
#define TAG_SIZE_LEN  4


typedef enum rtmp_state_s{
	APP_TEST_SERVER_RTMP_ERROR = -1,//失败
	APP_TEST_SERVER_RTMP_WAIT_C0C1 = 0,//开始
	APP_TEST_SERVER_RTMP_WAIT_C2,
	APP_TEST_SERVER_RTMP_WAIT_CONNECT,
	APP_TEST_SERVER_RTMP_WAIT_CREATESTREAM,
	APP_TEST_SERVER_RTMP_WAIT_PLAY,
	APP_TEST_SERVER_RTMP_FILE_START,//处理
	APP_TEST_SERVER_RTMP_FILE_SEND,
	APP_TEST_SERVER_RTMP_FILE_END,
	APP_TEST_SERVER_RTMP_FILE_RECV_START,
	APP_TEST_SERVER_RTMP_FILE_RECV,
}rtmp_state_t;

typedef struct rtmp_chunk_info_s{
	unsigned int message_size;
	unsigned int last_message_size;
	unsigned int message_type;
	unsigned int time_stamp_delta;
	unsigned int time_stamp;
	message_block *chunk_data;
}rtmp_chunk_info_t;

typedef struct flv_metadata_info_s{
	double audiosamplerate;
	char fps[32];
}flv_metadata_info_t;


typedef void (*destroy_rtmp_cb_t)(app_test_server_rtmp *rtmp);

typedef struct app_test_rtmp_data_s{
	destroy_rtmp_cb_t destroy_cb;
	app_test_server_rtmp *rtmp;
	event_connection_t *ec;//连接结构体
	rtmp_state_t state;//当前状态
	message_block *recv_data;
	message_block *send_data;

	//文件相关
	HANDLE file_id;
	DWORD file_len;
	DWORD file_cur;
	uint64 send_file_time;
	uint64 video_time;
	double audio_time;
	int video_time_add;
	double audio_time_add;
	flv_metadata_info_t flv_metadata;
	char flv_info[512];
	int flv_info_len;
	
	unsigned int client_chunk_size;
	unsigned int server_chunk_size;
	rtmp_chunk_info_t chunk_info[64];
}app_test_rtmp_data_t;

class app_test_server_rtmp{
public:
	app_test_server_rtmp(destroy_rtmp_cb_t cb,event_connection_t *ec,char *message,int len);

public:
	int app_test_server_rtmp_impl_run();

private:
	//文件相关
	int app_test_server_rtmp_onmetadata_result(app_test_rtmp_data_t * atud);
	int app_test_server_rtmp_parser_data_info(app_test_rtmp_data_t *atud);
	int app_test_server_rtmp_process_data(app_test_rtmp_data_t *atud);
	int app_test_server_rtmp_write_flv_framedata(app_test_rtmp_data_t *atud);
	int app_test_server_rtmp_write_flv(app_test_rtmp_data_t *atud,rtmp_chunk_info_t *info);
	//设置需要使用的参数
	int app_test_server_rtmp_set_bandwidth(char *pag_buf,int pag_len);
	int app_test_server_rtmp_set_win_ack(char *pag_buf,int pag_len);
	int app_test_server_rtmp_set_chunk_size(char *pag_buf,int pag_len);
	int app_test_server_rtmp_connect_result(char *pag_buf,int pag_len);
	int app_test_server_rtmp_create_stream_result(char *pag_buf,int pag_len);
	int app_test_server_rtmp_play_result(char *pag_buf,int pag_len);
	int app_test_server_rtmp_play_stream_begin(char *pag_buf,int pag_len);
	int app_test_server_rtmp_play_onstatus_stream_start(char *pag_buf,int pag_len);
	int app_test_server_rtmp_play_rtmpsampleaccess(char *pag_buf,int pag_len);
	int app_test_server_rtmp_play_onstatus_data_start(char *pag_buf,int pag_len);
	int app_test_server_rtmp_publish_result(char *pag_buf,int pag_len);
	
	//解析
	int app_test_server_rtmp_parser_message(app_test_rtmp_data_t *atud);
	int app_test_server_rtmp_parser_process(app_test_rtmp_data_t *atud,rtmp_chunk_info_t *info);
	int app_test_server_rtmp_parser_process_cmd_14(app_test_rtmp_data_t *atud,rtmp_chunk_info_t *info);
	int app_test_server_rtmp_parser_connect(app_test_rtmp_data_t *atud,rtmp_chunk_info_t *info);
	int app_test_server_rtmp_parser_createstream(app_test_rtmp_data_t *atud,rtmp_chunk_info_t *info);
	int app_test_server_rtmp_parser_play(app_test_rtmp_data_t *atud,rtmp_chunk_info_t *info);
	int app_test_server_rtmp_parser_publish(app_test_rtmp_data_t *atud,rtmp_chunk_info_t *info);
	int app_test_server_rtmp_parser_publish_dataframe(app_test_rtmp_data_t *atud,rtmp_chunk_info_t *info);
	//响应
	int app_test_server_rtmp_onmetadata(app_test_rtmp_data_t *atud);
	int app_test_server_rtmp_publish(app_test_rtmp_data_t *atud);
	int app_test_server_rtmp_play(app_test_rtmp_data_t *atud);
	int app_test_server_rtmp_create_stream(app_test_rtmp_data_t *atud);
	int app_test_server_rtmp_connect(app_test_rtmp_data_t *atud);
	int app_test_server_rtmp_s0s1s2(app_test_rtmp_data_t *atud);
	//清除连接对象数据
	void app_test_server_rtmp_clean(app_test_rtmp_data_t *atud);
	//发送数据接口
	int app_test_server_rtmp_check_send(app_test_rtmp_data_t *atud);
	//数据回调接口（处理timer）
	static void app_test_server_rtmp_event_cb(struct event_connection_s *ec, int event_task, void *user_data );
private:
	app_test_rtmp_data_t app_test_rtmp;
	rtmp_amf0 amf0_process;
	string rtmp_app;
	string rtmp_stream;

	double now_trans_id;
};
#endif
