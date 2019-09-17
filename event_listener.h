#ifndef __EVENT_LISTENER_H_
#define __EVENT_LISTENER_H_

#include "reactor.h"

class event_listener;
//��������ַ����
#define EVENT_LISTEN_NUM 64

/* listener��ַ�ṹ�� */
typedef struct event_listener_addr_s{
	size_t index;                            /* ���� */
	Socket *s;                        /* �׽ӿ� */
	event_listener *el;              //��������

	event_handler_t *ev_read;    /* �ɶ��¼������� */
	int type;                            /* ���� */
}event_listener_addr_t;

/* �����ص����� */
typedef void (*event_listen_cb_t)(Socket *listen_s, void * user_data);

class event_listener{

public:
	event_listener(event_listen_cb_t listen_cb,void *data);
	~event_listener();
public:
	int event_listener_add_listen(int server_port);
	int event_listener_start_thread();
private:
	//�����߳�
	static Dthr_ret WINAPI event_listener_thread(LPVOID lParam);
	//�������󵽴�ʱ�����Ĵ�����
	static void event_listener_listen_cb(Socket *s, int mask, void *arg, struct event_handler_s * ev);
	
private:
	reactor *r;
	CPthread *thread;
	event_listen_cb_t event_listen_cb;
	void *userdata;
	event_listener_addr_t listener[EVENT_LISTEN_NUM];
};




#endif
