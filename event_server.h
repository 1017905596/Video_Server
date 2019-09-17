#ifndef __EVENT_SERVER_H__
#define __EVENT_SERVER_H__

#include "reactor.h"

#define SERVER_MAX_THREAD_NUM 8
class event_server;

/* 用户请求结构体，主要是listener监听的用户请求传过来的消息 */
typedef struct event_queue_connection_s{
	Socket *s;        /* 用户请求的套接口 */
	int type;            /* 用户请求的类型 */
	void * user_data;        /* 用户请求的数据 */
	lxlist_head_t now_list; 
}event_queue_connection_t;

/* server一次连接所触发的回调函数 */
typedef void (*event_connection_event_cb_t)(struct event_connection_s *ec, int event_task, void *user_data );
/* 一次连接结构体 */
typedef struct event_connection_s{
	event_server *es;    /* server结构体 */
	Socket *s;                /* 连接的套接口 */
	reactor * r;            /* 连接的反应器 */
	size_t thread_index;            /* 连接所在的线程的索引 */
	event_handler_t * ev_read;    /* 连接中可读事件处理器 */
	event_handler_t * ev_write;    /* 连接中可写事件处理器 */
	event_handler_t * ev_timer;/* 连接中定时器事件处理器 */

	event_connection_event_cb_t f_event_cb;    /* 这次连接中所触发的回调函数 */
	void *  user_data;            /* 自定义数据 */
}event_connection_t;

/* 线程结构体 */
typedef struct event_thread_s{    
	size_t index;                    /* 线程索引 */
	event_server *es;             /* server主体结构体 */
	reactor * r;            /* 反应器 */
	event_handler_t *ev_timer; //定时器链表通信
	CPthread *thread;
}event_thread_t;


/* 成功回调函数，用于设置最后的处理函数(event_connection_event_cb_t) */
typedef void (*create_connection_success_t)(event_connection_t * ec,void *data);
/* 失败的回调,与上面的成功回调正好相反 */
typedef void (*create_connection_error_t)(event_queue_connection_t * eqc, int err );


class event_server{
public:
	event_server(create_connection_success_t su_cb,create_connection_error_t er_cb,int max_th,void *data);
	~event_server();

public:
	//将用户连接加入连接链表中
	int event_server_join_connection_list(int socket,string &client_ip);
	//用户连接退出，释放数据
	int event_server_destoy_queue_connection(event_queue_connection_t *eqc);
	//设置用户层数据处理的回调
	int event_server_set_event_cb(event_connection_t *ec,event_connection_event_cb_t event_cb,void *data);
	//添加事件到ec反应堆
	int event_server_connection_add_event(event_connection_t * ec, int event_mask, unsigned int ms);
	//移除事件
	int event_server_connection_remove_event(event_connection_t * ec, int event_mask);
	//释放ec事件处理器
	int event_server_destroy_connection(event_connection_t *ec);
	//start线程池
	int event_server_start_thread();
private:
	//获取ec事件处理器
	int event_server_get_connection(event_connection_t *ec,event_thread_t *et,event_queue_connection_t *eqc);
	//线程函数
	static Dthr_ret WINAPI event_server_thread(LPVOID lParam);
	//线程定时器时间处理连接链表中的用户数据
	static void event_server_thread_timer(Socket *s, int mask, void *arg, struct event_handler_s * ev);
	//事件回调，实际作用返回给上层数据处理
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