#include "reactor.h"

reactor::reactor(size_t max_event_num){
	select = new reactor_select(max_event_num,reactor_active_event_cb);
	tq = new head_timer(2*max_event_num);
	active_list = new lxlist();
	is_wait_exit = 0;
}

reactor::~reactor(){
	if(select != NULL){
		delete select;
		select = NULL;
	}
	if(tq != NULL){
		delete tq;
		tq = NULL;
	}
	if(active_list != NULL){
		delete active_list;
		active_list = NULL;
	}
}
/***********************************************************************************
 *   功能：创建事件处理器
 *   @return：
 *           NULL                                失败
 *           ！NULL                              成功
 ************************************************************************************/
event_handler_t *reactor::reactor_create_event(){
	event_handler_t *ev = new event_handler_t;
	if(ev == NULL){
		return NULL;
	}
	memset(ev, 0, sizeof(event_handler_t));
	return ev;
}
/***********************************************************************************
*   功能：销毁事件处理器
*   @param :
*           event_handler_t * ev           事件处理器
*   @return：
*           无
************************************************************************************/
void reactor::reactor_destroy_event(event_handler_t *ev){
	if(ev != NULL){
		free(ev);
		ev = NULL;
	}
}
/***********************************************************************************
*   功能：设置事件处理器并添加到反应器中
*   @param :
*           event_handler_t * ev           事件处理器
*           Socket  *s                     套接口对象
*           int mask                       事件类型(可读事件、可写事件、定时器事件）
*           event_cb_t event_cb            回调函数
*           void * arg                     回调函数参数
*   @return：
*           0                                   成功
*           -1                                  失败，已存在或者事件处理器为NULL
************************************************************************************/
int reactor::reactor_set_event(event_handler_t *ev,Socket *s,int mask,event_cb_t event_cb,void *arg){
	if(BIT_ENABLED(ev->flag,EVENT_FLAG_ALL_LIST)){
		return -1;
	}else{
		ev->r = this;
		ev->s = s;              //套接口
		ev->event_mask = mask; //事件类型
		ev->cb_arg = arg;       //回调参数
		ev->cb_func = event_cb; //回调函数
	}
	return 0;
}
/***********************************************************************************
*   功能：设置定时器事件并添加到反应器中
*   @param :
*           event_handler_t* ev            事件处理器
*           Socket  *s                     套接口对象
*           event_cb_t event_cb            定时器回调函数
*           void * arg                     回调函数的参数
8           unsigned int interval_ms       定时时间
*   @return：
*           0                                   成功
*           -1                                  失败，已存在该事件或则事件处理器为NULL
************************************************************************************/
int reactor::reactor_set_timer(event_handler_t *ev,Socket *s,unsigned int interval_ms,event_cb_t event_cb,void *arg){
	if(BIT_ENABLED(ev->flag,EVENT_FLAG_ALL_LIST)){
		return -1;
	}else{
		ev->r = this;
		ev->interval_ms = interval_ms; //事件间隔
		ev->s = s;                     //套接口
		ev->event_mask = EVENT_TIMER; //事件类型
		ev->cb_arg = arg;              //回调参数
		ev->cb_func = event_cb;        //回调函数
	}
	return 0;
}
/***********************************************************************************
*   功能：添加事件处理器到反应器上
*   @param :
*           event_handler_t * ev           事件处理器
*   @return：
*           0                                   成功
*           -1                                  失败
************************************************************************************/
int reactor::reactor_add_event(event_handler_t * ev){
	//注册可读事件或可写事件
	if(BIT_ENABLED(ev->event_mask, EVENT_READ|EVENT_WRITE)){
		//将事件处理器添加到对应模式的反应器中
		if( select->reactor_imp_add_event(ev) == 0){
			SET_BITS(ev->flag, EVENT_FLAG_IO_LIST);
			return 0;
		}
	}
	
	if(BIT_ENABLED(ev->event_mask, EVENT_TIMER)){
		ev->timerid = tq->timer_add_queue(ev,ev->interval_ms);
		SET_BITS(ev->flag, EVENT_FLAG_TIMER_LIST);
		return 0;
	}
	return -1;
}
/***********************************************************************************
*   功能：将事件处理器从反应器中移除
*   @param :
*           event_handler_t * ev           事件处理器
*   @return：
*           0                                   成功
*           -1                                  失败
************************************************************************************/
int reactor::reactor_remove_event(event_handler_t * ev){
	if(BIT_ENABLED(ev->flag, EVENT_FLAG_TIMER_LIST)){ //在定时器链表中
		CLR_BITS(ev->flag, EVENT_FLAG_TIMER_LIST );   //清除标志 
		if(ev->timerid != -1){
			tq->timer_del_queue(ev->timerid);
		}
	}else if(BIT_ENABLED(ev->flag, EVENT_FLAG_IO_LIST)){//读写事件还未触发
		CLR_BITS(ev->flag, EVENT_FLAG_IO_LIST);       //清除标志
		select->reactor_imp_remove_event(ev);
		ev->event_mask = 0;
	}else if(BIT_ENABLED(ev->flag, EVENT_FLAG_ACTIVE_LIST)){//事件已经触发，但未上报
		CLR_BITS(ev->flag, EVENT_FLAG_ACTIVE_LIST); 
		active_list->lxlist_del(&ev->list_active);
		ev->active_mask = 0;
	}
	return 0;
}
/***********************************************************************************
*   功能：处理活跃链表中的事件（调用回调函数）
*   @return：
*            >=0                                 被处理的活跃事件标号()
************************************************************************************/
int reactor::reactor_process_active_event(){
	event_handler_t * ev = NULL;
	int active_events = 0;
	int ev_active_mask = 0;
	while(!active_list->lxlist_is_empty()){
		//从链表中消息
		ev =  LXLIST_FIRST_ENTRY(active_list->lxlist_get_head(),event_handler_t,list_active);
		//删除指定链表节点
		active_list->lxlist_del(&ev->list_active);
		//清楚事件标志
		CLR_BITS(ev->flag, EVENT_FLAG_ACTIVE_LIST);
		//保存事件状态
		ev_active_mask = ev->active_mask;

		if(BIT_DISABLED(ev->event_mask,EVENT_IO_PERSIST)){
			reactor_remove_event(ev);
		}
		ev->cb_func(ev->s,ev_active_mask,ev->cb_arg,ev);
		active_events ++;
	}
	return active_events;
}
/***********************************************************************************
*   功能：将一个事件添加到活跃链表,如果已经在链表中,则增加事件的活跃类型（定时器、读、写事件的回调）
*   @param :
*           struct event_handler_s * ev    事件处理器
*           int mask                       事件类型
*   @return：
*            0
************************************************************************************/
int reactor::reactor_active_event_cb(struct event_handler_s * ev, int mask){
	if(BIT_ENABLED( ev->flag, EVENT_FLAG_ACTIVE_LIST)){
		ev->active_mask |= mask;
	}else {
		SET_BITS(ev->flag, EVENT_FLAG_ACTIVE_LIST);
		ev->active_mask = mask;
		ev->r->active_list->lxlist_add_tail(&ev->list_active);
	}
	return 0;
}
/***********************************************************************************
*   功能：反应器无限循环
*   @return：
*            0                                   成功退出
*           -1                                  错误
************************************************************************************/
int reactor::reactor_loop_forever(){
	timeval_t tv;
	int ret = 0;

	tv.sec = 0;
	tv.usec = 10*1000;
	while(!is_wait_exit){
		//定时器事件
		if(tq->timer_queue_size() > 0){
			event_handler_t * ev = NULL;
			int ret = tq->timer_get_expire((void **)&ev);
			if(ret == 0){
				while(ret == 0){
					ev->timerid = -1;
					reactor_active_event_cb(ev,EVENT_TIMER);
					ret = tq->timer_get_expire((void **)&ev);
				}
			}
		}
		//Socket 读写事件
		select->reactor_imp_dispath_event(&tv);
		//读取活跃链表回调
		ret = reactor_process_active_event();
		if(ret == 0){
			sleep_millisecond(5);
		}
	}
	return 0;
}
/***********************************************************************************
*   功能：退出反应器循环(该功能为退出reactor时使用，其余时间不要使用否则重新启动不了)
*   @return：
*            0
************************************************************************************/
int reactor::reactor_exit_loop(){
	is_wait_exit = 1;
	return 0;
}



//---------------------------test--------------------------------
static void event_test_cb(Socket *s, int mask, void *arg, struct event_handler_s * ev){
	if(BIT_ENABLED(mask, EVENT_READ)){
		char buf[1024] = {0};
		unsigned short client_port = 0;
		char client_ip[64] = {0};

		int ret = s->RecvFrom(buf,sizeof(buf),&client_port,client_ip);
		if(ret > 0){
			printf("client ip:%s,port:%d\n",client_ip,client_port);
			printf("buf:%s\n",buf);
			s->SendTo(buf,sizeof(buf),client_ip,client_port);
		}
	}

	if(BIT_ENABLED(mask, EVENT_TIMER)){
		printf( "time_ms:%llu, event timer \n", get_millisecond64());
		if(ev->r->reactor_add_event(ev) != 0)
		{
			printf( "time_ms:%llu, yy_f_reactor_add_event event timer \n", get_millisecond64() );
		}
	}
}

void reactor_validation_test(){
	reactor *r = new reactor(64);
	event_handler_t * ev = r->reactor_create_event();
	event_handler_t * ev_timer = r->reactor_create_event();
	Socket *s_r = new Socket();
	int s = s_r->Create(AF_INET,SOCK_DGRAM);
	//int s = s_r->Create("10.10.10.10:7000",SOCK_DGRAM);
	if(s == -1){
		printf("socket create error\n");
		return;
	}
	if(s_r->Bind(7000) != 0){
		printf("Bind error\n");
		return ;
	}

	//加入监听事件
	if(r->reactor_set_event(ev,s_r,EVENT_READ|EVENT_IO_PERSIST,event_test_cb,ev) != 0){
		printf("reactor_set_add_event 1 error\n");
		return ;
	}
	if(r->reactor_add_event(ev) != 0){
		return ;
	}
	//定时1s
	if(r->reactor_set_timer(ev_timer,s_r,100,event_test_cb,ev_timer) != 0){
		printf("reactor_set_add_event 2 error\n");
		return ;
	}
	if(r->reactor_add_event(ev_timer) != 0){
		return ;
	}

	while(1){
		r->reactor_loop_forever();
	}
}