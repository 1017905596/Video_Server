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
	APP_TEST_SERVER_HTTP_ERROR = -1,//ʧ��
	APP_TEST_SERVER_HTTP_START = 0,//��ʼ
	APP_TEST_SERVER_HTTP_REQ_HEAD,//����ͷ
	APP_TEST_SERVER_HTTP_PROCESS,//����
	APP_TEST_SERVER_HTTP_SEND_FILE,//�����ļ�
	APP_TEST_SERVER_HTTP_SEND_CLOSE,//������ϣ��ر�
	APP_TEST_SERVER_HTTP_END,//����
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
	event_connection_t *ec;//���ӽṹ��
	head_parser *parser;
	head_parser *http_req;
	url_parser *parser_url;
	http_state_t state;//��ǰ״̬
	message_block *recv_data;
	message_block *send_data;
	head_parser_range_t range;

	//�ļ�id
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
	//���캯��������http server����
	app_test_server_http(destroy_cb_t cb,event_connection_t *ec,char *message,int len);
public:
	//�ṩ����server���õĽӿ�
	int app_test_server_http_impl_run();
private:
	//��ȡ�ļ�·��
	int app_test_server_http_get_content_path(app_test_http_data_t *atud,string &play_path);
	//��ȡ�Զ���url filestart or fileend
	int app_test_server_http_set_file_pos_by_file(app_test_http_data_t *atud);
	//��ȡ�ͻ���range����
	int app_test_server_http_set_file_pos_by_range(app_test_http_data_t *atud);
	//����Ƿ��ͳ���
	int app_test_server_http_check_send_too_fast(app_test_http_pcr_t *pcr,unsigned int now_ms);
	//���´�ts�ļ�pcr������ʹ��
	int app_test_server_http_updata_file_pcr(app_test_http_pcr_t *pcr,unsigned int cur_pcr,unsigned int now_ms);
	//http��Ӧͷ��ʼ��
	int app_test_server_http_init_req(app_test_http_data_t *atud);
	//����ts�ļ�
	int app_test_server_http_play_ts(app_test_http_data_t *atud);
	//����hls�ļ�
	int app_test_server_http_play_m3u8(app_test_http_data_t *atud);
	//����ļ�����ֱ�ӷ���
	int app_test_server_http_play_m3u8_hls_file(app_test_http_data_t *atud);
	//���ts�ļ����ڣ���������index�ļ�
	int app_test_server_http_play_m3u8_ts_file(app_test_http_data_t *atud,string &path);
	//����hls�����ļ�
	int app_test_server_http_play_m3u8_build_hls_file(app_test_http_data_t *atud,string &ts_path,string &m3u8_path);
	//����2��http��ַ
	int app_test_server_http_display_mode(app_test_http_data_t *atud);
	//����1��http��ַ
	int app_test_server_http_process(app_test_http_data_t *atud);
	//������Ӷ�������
	void app_test_server_http_clean(app_test_http_data_t *atud);
	//�����ļ�����
	int app_test_server_http_send_file(app_test_http_data_t *atud);
	//�������ݽӿ�
	int app_test_server_http_check_send(app_test_http_data_t *atud);
	//���ݻص��ӿڣ�����timer��
	static void app_test_server_http_event_cb(struct event_connection_s *ec, int event_task, void *user_data );
private:
	app_test_http_data_t app_test_http;
};
#endif
