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
	APP_TEST_SERVER_ERROR = -1,//ʧ��
	APP_TEST_SERVER_START = 0,//��ʼ
	APP_TEST_SERVER_END,//����
	APP_TEST_SERVER_MOD_END,//����
}app_test_server_state_t;

typedef struct app_test_user_data_s{
	app_test_server *ats;//Ӧ�ö���
	event_connection_t *ec;//���ӽṹ��
	Socket *s; //�ͻ����׽���
	app_test_server_http *http;
	app_test_server_rtsp *rtsp;
	app_test_server_rtmp *rtmp;
	app_test_server_state_t state;//��ǰ״̬
	message_block *recv_data;
}app_test_user_data_t;

typedef struct app_test_server_s{
	app_test_server *ats;
	event_server *es;//server����
	event_listener *el;//listener����
}app_test_server_t;


class app_test_server{
public:
	app_test_server();
public:
	int app_test_server_init();
	int app_test_server_begin_thread();
private:
	//������Ҫ�ͷŵ�http����
	static void app_test_server_destroy_mod_rtmp(app_test_server_rtmp *rtmp);
	static void app_test_server_destroy_mod_rtsp(app_test_server_rtsp *rtsp);
	static void app_test_server_destroy_mod_http(app_test_server_http *http);
	int app_test_server_process(app_test_user_data_t *atud);
	//
	void app_test_server_impl_detach(app_test_user_data_t *atud);
	//�û������������
	void app_test_server_clean(app_test_user_data_t *atud);
	
	static void app_test_server_listen(Socket *listen_s, void * user_data);//�����ص�
	static void app_test_server_create_connect_success(event_connection_t * ec,void *data);//�����ɹ��ص�
	static void app_test_server_create_connect_error(event_queue_connection_t * eqc, int err);
	static void app_test_server_event_cb(struct event_connection_s *ec, int event_task, void *user_data);

	static Dthr_ret WINAPI app_test_server_thread(LPVOID lParam);
private:
	app_test_server_t test_server;
	CPthread *thread;
};

#endif
