#ifndef __EVENT_LISTENER_H_
#define __EVENT_LISTENER_H_

#include "reactor.h"

class event_listener;
//最大监听地址数量
#define EVENT_LISTEN_NUM 64

/* listener地址结构体 */
typedef struct event_listener_addr_s{
	size_t index;                            /* 索引 */
	Socket *s;                        /* 套接口 */
	event_listener *el;              //监听对象

	event_handler_t *ev_read;    /* 可读事件处理器 */
	int type;                            /* 类型 */
}event_listener_addr_t;

/* 监听回调函数 */
typedef void (*event_listen_cb_t)(Socket *listen_s, void * user_data);

class event_listener{

public:
	event_listener(event_listen_cb_t listen_cb,void *data);
	~event_listener();
public:
	int event_listener_add_listen(int server_port);
	int event_listener_start_thread();
private:
	//监听线程
	static Dthr_ret WINAPI event_listener_thread(LPVOID lParam);
	//有新请求到达时触发的处理函数
	static void event_listener_listen_cb(Socket *s, int mask, void *arg, struct event_handler_s * ev);
	
private:
	reactor *r;
	CPthread *thread;
	event_listen_cb_t event_listen_cb;
	void *userdata;
	event_listener_addr_t listener[EVENT_LISTEN_NUM];
};




#endif
