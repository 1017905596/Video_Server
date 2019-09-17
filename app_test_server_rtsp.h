#ifndef _APP_TEST_SERVER_RTSP_H_
#define _APP_TEST_SERVER_RTSP_H_

#include "event_listener.h"
#include "event_server.h"
#include "message_block.h"
#include "head_parser.h"
#include "url_parser.h"
#include "ts_parser.h"


#define SESSION_LEN 10
class app_test_server_rtsp;
typedef struct app_test_rtsp_pcr_s{
	unsigned int send_cur_ms;
	unsigned int send_begin_ms;
	unsigned int content_begin_ms;
	unsigned int content_cur_ms;
}app_test_rtsp_pcr_t;

typedef enum play_mod_rtsp_e{
	rtsp_error = -1,
	rtsp_init,
	rtsp_describe,
	rtsp_setup,
	rtsp_play,
	rtsp_pause,
	rtsp_teardown
}play_mod_rtsp_t;

typedef enum play_mod_data_e{
	data_error = -1,
	data_start,
	data_play,
	data_pause,
	data_seek,
	data_end
}play_mod_data_t;

typedef enum rtsp_state_s{
	APP_TEST_SERVER_RTSP_ERROR = -1,//ʧ��
	APP_TEST_SERVER_RTSP_START = 0,//��ʼ
	APP_TEST_SERVER_RTSP_REQ_HEAD,//����ͷ
	APP_TEST_SERVER_RTSP_PROCESS,//����
}rtsp_state_t;

typedef void (*destroy_rtsp_cb_t)(app_test_server_rtsp *rtsp);

typedef struct app_test_rtsp_data_s{
	ts_parser *ts;
	destroy_rtsp_cb_t destroy_cb;
	app_test_server_rtsp *rtsp;
	event_connection_t *ec;//���ӽṹ��
	head_parser *parser;
	head_parser *rtsp_req;
	url_parser *parser_url;
	play_mod_data_t play_data;
	play_mod_rtsp_t play_mode;
	rtsp_state_t state;//��ǰ״̬
	message_block *recv_data;
	message_block *send_data;

	char rtsp_session[SESSION_LEN + 1];
	//string rtsp_cseq;
	file_info_t file_info;
	//�ļ�id
	HANDLE file_id;
	DWORD file_cur;
	DWORD file_size;
	DWORD file_pos;

	DWORD file_start;
	DWORD file_end;

	int is_rtp;
	int is_tcp;
	unsigned int packet_size;
	int wiat_udp_client;
	uint64 last_data_pts;
	unsigned short rtp_seq;
	uint64 rtp_ssid;
	unsigned int first_pcr;
	unsigned int cur_pcr;

	char file_cache_data[256 + 1024*64/(188*7)*(188*7)];
	unsigned int file_cache_data_len;
	unsigned int file_cache_data_pos;

	char udp_client_ip[64];
	unsigned short udp_client_port;
	Socket *udp_s;
	event_handler_t *udp_ev_read;

	app_test_rtsp_pcr_t file_pcr_info;
}app_test_rtsp_data_t;

class app_test_server_rtsp{
public:
	app_test_server_rtsp(destroy_rtsp_cb_t cb,event_connection_t *ec,char *message,int len);

public:
	int app_test_server_rtsp_impl_run();

private:
	uint64 app_test_server_rtsp_get_first_pts(app_test_rtsp_data_t *atud);
	int app_test_server_rtsp_parse_range(app_test_rtsp_data_t *atud,int*start_time);
	//����Ƿ��ͳ���
	int app_test_server_rtsp_check_send_too_fast(app_test_rtsp_pcr_t *pcr,unsigned int now_ms);
	//���´�ts�ļ�pcr������ʹ��
	int app_test_server_rtsp_updata_file_pcr(app_test_rtsp_pcr_t *pcr,unsigned int cur_pcr,unsigned int now_ms);
	int app_test_server_rtsp_file_send(app_test_rtsp_data_t *atud,char *file_buf,int file_buf_len);
	int app_test_server_rtsp_file_data(app_test_rtsp_data_t *atud);
	//����udp
	int app_test_server_rtsp_process_setup_create_udp(app_test_rtsp_data_t *atud,char *source_ip,unsigned short* source_port);
	//����transport
	int app_test_server_rtsp_process_setup_parse_transport_info(app_test_rtsp_data_t *atud,char  *trans_port);
	//����transport
	int app_test_server_rtsp_process_setup_parse_transport(app_test_rtsp_data_t *atud,char  *trans_port,int trans_port_len);
	//��������
	int app_test_server_rtsp_process_setparameter(app_test_rtsp_data_t *atud);
	int app_test_server_rtsp_process_getparameter(app_test_rtsp_data_t *atud);
	int app_test_server_rtsp_process_teardown(app_test_rtsp_data_t *atud);
	int app_test_server_rtsp_process_pause(app_test_rtsp_data_t *atud);
	int app_test_server_rtsp_process_play(app_test_rtsp_data_t *atud);
	int app_test_server_rtsp_process_setup(app_test_rtsp_data_t *atud);
	int app_test_server_rtsp_process_describe(app_test_rtsp_data_t *atud);
	int app_test_server_rtsp_process_options(app_test_rtsp_data_t *atud);
	//��������
	int app_test_server_rtsp_process(app_test_rtsp_data_t *atud);
	//������Ӷ�������
	void app_test_server_rtsp_clean(app_test_rtsp_data_t *atud);
	//��������
	int app_test_server_rtsp_do_rep_cmd_and_content(app_test_rtsp_data_t *atud,const char*code,const char*code_string,char*buf,int len);
	//��������
	int app_test_server_rtsp_do_rep_cmd(app_test_rtsp_data_t *atud,const char*code,const char*code_string);
	//�������ݽӿ�
	int app_test_server_rtsp_check_send(app_test_rtsp_data_t *atud);
	//���ݻص��ӿڣ�����timer��
	static void app_test_server_rtsp_event_cb(struct event_connection_s *ec, int event_task, void *user_data );
	//http ��Ӧͷ
	int app_test_server_rtsp_init_req(app_test_rtsp_data_t *atud);
	//��ȡ�ļ�·��
	int app_test_server_rtsp_get_content_path(app_test_rtsp_data_t *atud,string &play_path);

	static void app_test_server_rtsp_udp_read(Socket *s, int mask, void *arg, struct event_handler_s * ev);
private:
	app_test_rtsp_data_t app_test_rtsp;
};





#endif
