#ifndef __EVENT_SERVER_H__
#define __EVENT_SERVER_H__

#include "reactor.h"

#define SERVER_MAX_THREAD_NUM 8
class event_server;

/* �û�����ṹ�壬��Ҫ��listener�������û����󴫹�������Ϣ */
typedef struct event_queue_connection_s{
	Socket *s;        /* �û�������׽ӿ� */
	int type;            /* �û���������� */
	void * user_data;        /* �û���������� */
	lxlist_head_t now_list; 
}event_queue_connection_t;

/* serverһ�������������Ļص����� */
typedef void (*event_connection_event_cb_t)(struct event_connection_s *ec, int event_task, void *user_data );
/* һ�����ӽṹ�� */
typedef struct event_connection_s{
	event_server *es;    /* server�ṹ�� */
	Socket *s;                /* ���ӵ��׽ӿ� */
	reactor * r;            /* ���ӵķ�Ӧ�� */
	size_t thread_index;            /* �������ڵ��̵߳����� */
	event_handler_t * ev_read;    /* �����пɶ��¼������� */
	event_handler_t * ev_write;    /* �����п�д�¼������� */
	event_handler_t * ev_timer;/* �����ж�ʱ���¼������� */

	event_connection_event_cb_t f_event_cb;    /* ����������������Ļص����� */
	void *  user_data;            /* �Զ������� */
}event_connection_t;

/* �߳̽ṹ�� */
typedef struct event_thread_s{    
	size_t index;                    /* �߳����� */
	event_server *es;             /* server����ṹ�� */
	reactor * r;            /* ��Ӧ�� */
	event_handler_t *ev_timer; //��ʱ������ͨ��
	CPthread *thread;
}event_thread_t;


/* �ɹ��ص������������������Ĵ�����(event_connection_event_cb_t) */
typedef void (*create_connection_success_t)(event_connection_t * ec,void *data);
/* ʧ�ܵĻص�,������ĳɹ��ص������෴ */
typedef void (*create_connection_error_t)(event_queue_connection_t * eqc, int err );


class event_server{
public:
	event_server(create_connection_success_t su_cb,create_connection_error_t er_cb,int max_th,void *data);
	~event_server();

public:
	//���û����Ӽ�������������
	int event_server_join_connection_list(int socket,string &client_ip);
	//�û������˳����ͷ�����
	int event_server_destoy_queue_connection(event_queue_connection_t *eqc);
	//�����û������ݴ���Ļص�
	int event_server_set_event_cb(event_connection_t *ec,event_connection_event_cb_t event_cb,void *data);
	//����¼���ec��Ӧ��
	int event_server_connection_add_event(event_connection_t * ec, int event_mask, unsigned int ms);
	//�Ƴ��¼�
	int event_server_connection_remove_event(event_connection_t * ec, int event_mask);
	//�ͷ�ec�¼�������
	int event_server_destroy_connection(event_connection_t *ec);
	//start�̳߳�
	int event_server_start_thread();
private:
	//��ȡec�¼�������
	int event_server_get_connection(event_connection_t *ec,event_thread_t *et,event_queue_connection_t *eqc);
	//�̺߳���
	static Dthr_ret WINAPI event_server_thread(LPVOID lParam);
	//�̶߳�ʱ��ʱ�䴦�����������е��û�����
	static void event_server_thread_timer(Socket *s, int mask, void *arg, struct event_handler_s * ev);
	//�¼��ص���ʵ�����÷��ظ��ϲ����ݴ���
	static void event_server_connection_event_cb(Socket *s, int mask, void *arg, struct event_handler_s * ev);
private:
	create_connection_success_t create_success_cb;
	create_connection_error_t create_error_cb;
	void *user_data;
	lxlist *connection_list;
	event_thread_t thread[SERVER_MAX_THREAD_NUM];
	int now_thread_num;
};


#endif