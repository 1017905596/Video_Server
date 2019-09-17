#ifndef __REACTOR_H__
#define __REACTOR_H__

#include "reactor_select.h"

class reactor{
public:
	reactor(size_t max_event_num);
	~reactor();
public:
	//创建事件处理器(可读事件、可写事件、定时器事件)
	event_handler_t * reactor_create_event();
	//释放事件处理器
	void reactor_destroy_event(event_handler_t *ev);

	//添加读写事件并且设置到反应器中
	int reactor_set_event(event_handler_t *ev,Socket *s,int mask,event_cb_t event_cb,void *arg);
	//添加定时器事件并且设置到反应器中
	int reactor_set_timer(event_handler_t *ev,Socket *s,unsigned int interval_ms,event_cb_t event_cb,void *arg);

	//添加到反应器
	int reactor_add_event(event_handler_t * ev);
	//从反应器移除事件处理器
	int reactor_remove_event(event_handler_t * ev);

	//反应器无限循环，直至被要求退出
	int reactor_loop_forever();
	//反应器退出
	int reactor_exit_loop();
private:
	//读、写、定时器事件回调加入活跃链表
	static int reactor_active_event_cb(struct event_handler_s * ev, int mask);
	//从活跃链表中读取事件回调给上层
	int reactor_process_active_event();
private:
	//定时器对象
	head_timer * tq;
	//select 对象
	reactor_select *select;
	//活跃事件链表对象
	lxlist *active_list;
	//退出反应器标志
	int is_wait_exit;
};

void reactor_validation_test();
#endif
