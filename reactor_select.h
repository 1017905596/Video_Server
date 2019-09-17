#ifndef __REACTOR_SELECT_H__
#define __REACTOR_SELECT_H__

#include <stdio.h>
#include "lxlist.h"
#include "timer.h"
#include "Pthread.h"
#include "Socket.h"
#include "userlib_type.h"



#define EVENT_READ             1    /* 读事件 */
#define EVENT_WRITE            2    /* 写事件 */
#define EVENT_TIMER            4    /* 定时器事件 */
#define EVENT_IO_PERSIST       8    /* 持续性IO，如套接口持续读或写 */
#define EVENT_EMASK    (EVENT_READ|EVENT_WRITE|EVENT_TIMER)
#define EVENT_FLAG_IO_LIST      0x00000100    /* 事件标志：事件在IO链表上 */
#define EVENT_FLAG_ACTIVE_LIST  0x00000200    /* 事件标志：事件在活跃链表上 */
#define EVENT_FLAG_TIMER_LIST   0x00000400    /* 事件标志：事件在定时器链表上 */
#define EVENT_FLAG_ALL_LIST (EVENT_FLAG_IO_LIST|EVENT_FLAG_ACTIVE_LIST|EVENT_FLAG_TIMER_LIST)

struct event_handler_s;
class reactor;

/* 事件回调函数 */
typedef void (*event_cb_t)(Socket *s, int mask, void *arg, struct event_handler_s * ev);
/* 激活事件回调函数（主要是将事件挂载到反应器的活跃链表） */
typedef int (*reactor_active_event_t)(struct event_handler_s * ev, int mask );


/* 事件处理器结构体 */
typedef struct event_handler_s{
	volatile unsigned int flag;        /* 事件所属链表标志 */
	int interval_ms;                  //间隔时长

	Socket *s;                          /* 套接口对象 */
	reactor *r;                       //保存当前反应器对象
	int timerid;                     //定时器id
	int event_mask;                 /* 活跃事件类型：读、写、定时器？？ */
	int active_mask;                 /* 活跃事件类型：读、写、定时器？？ */
	lxlist_head_t list_active;      /* 装载活跃事件链表 */

	//关于回调
	event_cb_t cb_func;        /* 事件回调函数 */
	void * cb_arg;                /* 回调函数参数 */
}event_handler_t;

/* 事件结构 */
typedef struct reactor_select_event_s{
	int fd;                                    /* 套接口 */
	event_handler_t *ev_read;                /* 可读事件处理器 */
	event_handler_t *ev_write;                /* 可写事件处理器 */
}reactor_select_event_t;


class reactor_select{
public:
	reactor_select(int max_size,reactor_active_event_t cb);
	~reactor_select();
public:
	int reactor_imp_dispath_event(const timeval_t *tv);//检查是否有时间就绪，若有则触发对应的处理函数
	int reactor_imp_remove_event(event_handler_t *ev);//移除事件，即不再关心其读写
	int reactor_imp_add_event(event_handler_t *ev);//注册监听事件（可读可写事件）
private:
	int reactor_select_find_empty(int fd);//找一空位置，注册套接口（已注册则复用）
	int reactor_select_find_fd(int fd);//找到指定套接口所在位置
	int reactor_select_rebuild_fd_set();//重建被文件描述符集合
	int reactor_select_fd_set_copy(fd_set *t ,const fd_set * s);//克隆文件描述符集合
private:
	reactor_active_event_t event_cb;   //回调
	int max_fd_size;                   //最大缓存个数（最大文件描述符数量）
	reactor_select_event_t *events;    //缓存时间
	fd_set * fd_set_read;              //可读事件文件描述符集合
	fd_set * fd_set_write;             //可写事件文件描述符集合
	fd_set * fd_set_used_read;         //当前已注册的可读事件文件描述符集合
	fd_set * fd_set_used_write;        //当前已注册的可写事件文件描述符集合 
	fd_set * fd_set_used_except;       //当前已注册的异常事件文件描述符集合
	int fd_slot_used_min;              //已注册的最小文件描述符
	int fd_slot_used_max;              //已注册的最大文件描述符 
	int fd_width;                      //文件描述符范围（已注册文件描述符个数
};

#endif