#ifndef __APP_TEST_SERVER_H__
#define __APP_TEST_SERVER_H__

#include "app_test_server_http.h"
#include "app_test_server_rtsp.h"
#include "app_test_server_rtmp.h"
#include "event_listener.h"
#include "event_server.h"
#include "message_block.h"
#include "head_parser.h"
#include "url_parser.h"

#define MAX_CONNECT_LEN 1024
class app_test_server;

typedef enum app_test_server_state_s{
	APP_TEST_SERVER_ERROR = -1,//失败
	APP_TEST_SERVER_START = 0,//开始
	APP_TEST_SERVER_END,//结束
	APP_TEST_SERVER_MOD_END,//结束
}app_test_server_state_t;

typedef struct app_test_user_data_s{
	app_test_server *ats;//应用对象
	event_connection_t *ec;//连接结构体
	Socket *s; //客户端套接字
	app_test_server_http *http;
	app_test_server_rtsp *rtsp;
	app_test_server_rtmp *rtmp;
	app_test_server_state_t state;//当前状态
	message_block *recv_data;
}app_test_user_data_t;

typedef struct app_test_server_s{
	app_test_server *ats;
	event_server *es;//server对象
	event_listener *el;//listener对象
}app_test_server_t;


class app_test_server{
public:
	app_test_server();
public:
	int app_test_server_init();
	int app_test_server_begin_thread();
private:
	//设置需要释放的http对象
	static void app_test_server_destroy_mod_rtmp(app_test_server_rtmp *rtmp);
	static void app_test_server_destroy_mod_rtsp(app_test_server_rtsp *rtsp);
	static void app_test_server_destroy_mod_http(app_test_server_http *http);
	int app_test_server_process(app_test_user_data_t *atud);
	//
	void app_test_server_impl_detach(app_test_user_data_t *atud);
	//用户连接请求清除
	void app_test_server_clean(app_test_user_data_t *atud);
	
	static void app_test_server_listen(Socket *listen_s, void * user_data);//监听回调
	static void app_test_server_create_connect_success(event_connection_t * ec,void *data);//创建成功回调
	static void app_test_server_create_connect_error(event_queue_connection_t * eqc, int err);
	static void app_test_server_event_cb(struct event_connection_s *ec, int event_task, void *user_data);

	static Dthr_ret WINAPI app_test_server_thread(LPVOID lParam);
private:
	app_test_server_t test_server;
	CPthread *thread;
};

#endif
