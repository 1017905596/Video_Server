#include "event_server.h"

static CCritSec static_lock;

event_server::event_server(create_connection_success_t su_cb,create_connection_error_t er_cb,int max_th,void *data){
	//赋值
	create_success_cb = su_cb;
	create_error_cb = er_cb;
	user_data = data;
	//创建通信链表
	connection_list = new lxlist();
	//初始化线程
	if(max_th >= SERVER_MAX_THREAD_NUM || max_th <= 0){
		max_th = 1;
	}
	now_thread_num = max_th;
	for(int i = 0; i < max_th; i++){
		thread[i].index = i;
		thread[i].es = this;
		thread[i].r = new reactor(1024);
		thread[i].thread = new CPthread();

		thread[i].ev_timer = thread[i].r->reactor_create_event();
		thread[i].r->reactor_set_timer(thread[i].ev_timer,NULL,50,event_server_thread_timer,&thread[i]);
		thread[i].r->reactor_add_event(thread[i].ev_timer);
	}
}


event_server::~event_server(){
	for(int i = 0; i < now_thread_num; i++){
		if(thread[i].ev_timer != NULL){
			thread[i].r->reactor_remove_event(thread[i].ev_timer);
			thread[i].r->reactor_destroy_event(thread[i].ev_timer);
			thread[i].ev_timer = NULL;
		}
		delete thread[i].thread;
		thread[i].thread = NULL;
		delete thread[i].r;
		thread[i].r = NULL;
	}
	if(connection_list != NULL){
		delete connection_list;
		connection_list = NULL;
	}
}

/***************************************************************
//定时处理连接链表中的用户数据，并生成连接对象、回调用户层
//
***************************************************************/
void event_server::event_server_thread_timer(Socket *s, int mask, void *arg, struct event_handler_s * ev){
	event_thread_t *et = (event_thread_t *)arg;
	event_server *es = et->es;
	event_queue_connection_t *eqc = NULL;
	//重新加入定时器
	et->r->reactor_add_event(ev);
	//加上锁
	mutex_lock lock(&static_lock);
	
	//线程函数中读取链表中的连接信息
	while(es->connection_list->lxlist_get_user_num()){
		eqc = LXLIST_FIRST_ENTRY(es->connection_list->lxlist_get_head(),event_queue_connection_t,now_list);
		if(eqc != NULL){
			event_connection_t *ec = new event_connection_t;
			if(ec == NULL){
				break;
			}
			int ret = es->event_server_get_connection(ec,et,eqc);
			if(ret < 0){
				es->create_error_cb(eqc,1);
			}else{
				es->create_success_cb(ec,es->user_data);
			}
		}
		es->connection_list->lxlist_del(&eqc->now_list);
		
		if(eqc != NULL){
			free(eqc);
			eqc = NULL;
		}
	}
}
/***************************************************************
//将accept接收到的用户数据加入连接链表
//
***************************************************************/
int event_server::event_server_join_connection_list(int socket,string &client_ip){
	event_queue_connection_t *eqc = new event_queue_connection_t;
	if(eqc == NULL){
		return -1;
	}
	eqc->s = new Socket();
	eqc->s->client_ip = client_ip;
	eqc->s->m_nSocket = socket;

	if(connection_list != NULL){
		connection_list->lxlist_add_tail(&eqc->now_list);
		return 0;
	}else{
		delete eqc->s;
		eqc->s = NULL;

		free(eqc);
		eqc = NULL;
		return -1;
	}
	
}
/***************************************************************
//用户退出连接，释放连接数据
//
***************************************************************/
int event_server::event_server_destoy_queue_connection(event_queue_connection_t *eqc){
	if(eqc != NULL){
		if(eqc->s != NULL){
			delete eqc->s;
			eqc->s = NULL;
		}
		free(eqc);
		eqc = NULL;
		return 0;
	}
	return -1;
}
/***************************************************************
//生成连接对象，注册事件、回调
//
***************************************************************/
int event_server::event_server_get_connection(event_connection_t *ec,event_thread_t *et,event_queue_connection_t *eqc){
	ec->es = et->es;
	ec->thread_index = et->index;
	ec->r = et->r;
	ec->ev_read = ec->r->reactor_create_event();
	ec->ev_write = ec->r->reactor_create_event();
	ec->ev_timer = ec->r->reactor_create_event();
	ec->s = eqc->s;
	return 0;
}
/***************************************************************
//释放连接对象
//
***************************************************************/
int event_server::event_server_destroy_connection(event_connection_t *ec){
	if(ec != NULL){
		if(ec->ev_read != NULL){
			ec->r->reactor_remove_event(ec->ev_read);
			ec->r->reactor_destroy_event(ec->ev_read);
			ec->ev_read = NULL;
		}
		if(ec->ev_write != NULL){
			ec->r->reactor_remove_event(ec->ev_write);
			ec->r->reactor_destroy_event(ec->ev_write);
			ec->ev_write = NULL;
		}
		if(ec->ev_timer != NULL){
			ec->r->reactor_remove_event(ec->ev_timer);
			ec->r->reactor_destroy_event(ec->ev_timer);
			ec->ev_timer = NULL;
		}

		if(ec->s != NULL){
			delete ec->s;
			ec->s = NULL;
		}

		free(ec);
		ec = NULL;
	}
	return 0;
}
/***************************************************************
//连接对象，注册用户层数据处理回调
//
***************************************************************/
int event_server::event_server_set_event_cb(event_connection_t *ec,event_connection_event_cb_t event_cb,void *data){
	if(ec != NULL){
		ec->f_event_cb = event_cb;
		ec->user_data = data;
		return 0;
	}
	return -1;
}
/***************************************************************
//连接对象，添加事件
//
***************************************************************/
int event_server::event_server_connection_add_event(event_connection_t * ec, int event_mask, unsigned int ms){

	if(ec == NULL || ec->r == NULL || ec->ev_read == NULL || ec->ev_write == NULL || ec->ev_timer ==NULL){
		return -1;
	}
	int io_persist = event_mask&EVENT_IO_PERSIST;
	event_mask &= EVENT_EMASK;


	if(event_mask == EVENT_READ){
		ec->r->reactor_remove_event(ec->ev_read);
		ec->r->reactor_set_event(ec->ev_read,ec->s,io_persist|EVENT_READ,ec->es->event_server_connection_event_cb,ec);
		if(ec->r->reactor_add_event(ec->ev_read) < 0){
			return -1;
		}
		return 0;
	}
	if(event_mask == EVENT_WRITE){
		ec->r->reactor_remove_event(ec->ev_write);
		ec->r->reactor_set_event(ec->ev_write,ec->s,io_persist|EVENT_WRITE,ec->es->event_server_connection_event_cb,ec);
		if(ec->r->reactor_add_event(ec->ev_write) < 0){
			return -1;
		}
		return 0;
	}
	if(event_mask == EVENT_TIMER){
		ec->r->reactor_remove_event(ec->ev_timer);
		ec->r->reactor_set_event(ec->ev_timer,ec->s,ms,ec->es->event_server_connection_event_cb,ec);
		if(ec->r->reactor_add_event(ec->ev_timer) < 0){
			return -1;
		}
		return 0;
	}

	return -1;
}
/***************************************************************
//连接对象，移除事件
//
***************************************************************/
int event_server::event_server_connection_remove_event(event_connection_t * ec, int event_mask){
	if(ec != NULL){
		if(BIT_ENABLED( event_mask,EVENT_READ)){
			ec->r->reactor_remove_event(ec->ev_read);
		}
		if(BIT_ENABLED( event_mask,EVENT_WRITE)){
			ec->r->reactor_remove_event(ec->ev_write);
		}
		if(BIT_ENABLED( event_mask,EVENT_TIMER)){
			ec->r->reactor_remove_event(ec->ev_timer);
		}
		return 0;
	}
	return -1;
}

/***************************************************************
//连接对象，读写、定时器事件触发（回调用户层做数据处理）
//
***************************************************************/
void event_server::event_server_connection_event_cb(Socket *s, int mask, void *arg, struct event_handler_s * ev){
	event_connection_t *ec = (event_connection_t *)arg;

	if(ec != NULL && ec->f_event_cb != NULL && ev != NULL){
		if(ec->ev_read == ev){
			ec->f_event_cb(ec,EVENT_READ,ec->user_data);
		}else if(ec->ev_write == ev){
			ec->f_event_cb(ec,EVENT_WRITE,ec->user_data);
		}else if(ec->ev_timer == ev){
			ec->f_event_cb(ec,EVENT_TIMER,ec->user_data);
		}
	}
}

/***************************************************************
//开启线程
//
***************************************************************/
int event_server::event_server_start_thread(){

	for(int i = 0 ; i < now_thread_num;i++){
		thread[i].thread->Create(event_server_thread,&thread[i]);
	}
	return -1;
}

/***************************************************************
//线程函数
//
***************************************************************/
Dthr_ret WINAPI event_server::event_server_thread(LPVOID lParam){
	event_thread_t *et = (event_thread_t *)lParam;
	et->r->reactor_loop_forever();
	return NULL;
}