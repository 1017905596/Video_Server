#ifndef _APP_TEST_SERVER_HTTP_H_
#define _APP_TEST_SERVER_HTTP_H_

#include "event_listener.h"
#include "event_server.h"
#include "message_block.h"
#include "head_parser.h"
#include "url_parser.h"
#include "ts_parser.h"

class app_test_server_http;

#define M3U8_SPLICE_TIME_MS        (5*1000)

typedef enum http_state_s{
	APP_TEST_SERVER_HTTP_ERROR = -1,//失败
	APP_TEST_SERVER_HTTP_START = 0,//开始
	APP_TEST_SERVER_HTTP_REQ_HEAD,//解析头
	APP_TEST_SERVER_HTTP_PROCESS,//处理
	APP_TEST_SERVER_HTTP_SEND_FILE,//发送文件
	APP_TEST_SERVER_HTTP_SEND_CLOSE,//发送完毕，关闭
	APP_TEST_SERVER_HTTP_END,//结束
}http_state_t;

typedef void (*destroy_cb_t)(app_test_server_http *http);


typedef struct nn_ms_pm_helper_s{
	unsigned int send_cur_ms;
	unsigned int send_begin_ms;
	unsigned int content_begin_ms;
	unsigned int content_cur_ms;
}app_test_http_pcr_t;

typedef struct app_test_http_data_s{
	ts_parser *ts;
	destroy_cb_t destroy_cb;
	app_test_server_http *http;
	event_connection_t *ec;//连接结构体
	head_parser *parser;
	head_parser *http_req;
	url_parser *parser_url;
	http_state_t state;//当前状态
	message_block *recv_data;
	message_block *send_data;
	head_parser_range_t range;

	//文件id
	HANDLE file_id;
	DWORD file_cur;
	DWORD file_size;
	DWORD file_pos;

	DWORD file_start;
	DWORD file_end;

	app_test_http_pcr_t file_pcr_info;
	unsigned int first_pcr;
	unsigned int cur_pcr;
	unsigned int byte_rate;
	unsigned int send_data_byte;
	int is_m3u8_file;
}app_test_http_data_t;

class app_test_server_http{
public:
	//构造函数，创建http server对象
	app_test_server_http(destroy_cb_t cb,event_connection_t *ec,char *message,int len);
public:
	//提供给主server调用的接口
	int app_test_server_http_impl_run();
private:
	//获取文件路径
	int app_test_server_http_get_content_path(app_test_http_data_t *atud,string &play_path);
	//获取自定义url filestart or fileend
	int app_test_server_http_set_file_pos_by_file(app_test_http_data_t *atud);
	//获取客户端range请求
	int app_test_server_http_set_file_pos_by_range(app_test_http_data_t *atud);
	//检测是否发送超速
	int app_test_server_http_check_send_too_fast(app_test_http_pcr_t *pcr,unsigned int now_ms);
	//更新大ts文件pcr，限速使用
	int app_test_server_http_updata_file_pcr(app_test_http_pcr_t *pcr,unsigned int cur_pcr,unsigned int now_ms);
	//http响应头初始化
	int app_test_server_http_init_req(app_test_http_data_t *atud);
	//播放ts文件
	int app_test_server_http_play_ts(app_test_http_data_t *atud);
	//播放hls文件
	int app_test_server_http_play_m3u8(app_test_http_data_t *atud);
	//如果文件存在直接发送
	int app_test_server_http_play_m3u8_hls_file(app_test_http_data_t *atud);
	//如果ts文件存在，尝试生成index文件
	int app_test_server_http_play_m3u8_ts_file(app_test_http_data_t *atud,string &path);
	//生成hls索引文件
	int app_test_server_http_play_m3u8_build_hls_file(app_test_http_data_t *atud,string &ts_path,string &m3u8_path);
	//处理2级http地址
	int app_test_server_http_display_mode(app_test_http_data_t *atud);
	//处理1级http地址
	int app_test_server_http_process(app_test_http_data_t *atud);
	//清除连接对象数据
	void app_test_server_http_clean(app_test_http_data_t *atud);
	//发送文件数据
	int app_test_server_http_send_file(app_test_http_data_t *atud);
	//发送数据接口
	int app_test_server_http_check_send(app_test_http_data_t *atud);
	//数据回调接口（处理timer）
	static void app_test_server_http_event_cb(struct event_connection_s *ec, int event_task, void *user_data );
private:
	app_test_http_data_t app_test_http;
};
#endif
