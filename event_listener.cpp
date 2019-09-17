#include "event_listener.h"

event_listener::event_listener(event_listen_cb_t listen_cb,void *data){
	event_listen_cb = listen_cb;
	userdata = data;
	for(int i = 0; i < EVENT_LISTEN_NUM; i++){
		memset(&listener[i],0,sizeof(event_listener_addr_t));
		listener[i].el = this;
		listener[i].index = i;
	}
	r = new reactor(1024);
	thread = new CPthread();
}

event_listener::~event_listener(){
	for(int i = 0; i < EVENT_LISTEN_NUM; i++){
		if(r != NULL){
			if(listener[i].ev_read != NULL){
				r->reactor_destroy_event(listener[i].ev_read);
				listener[i].ev_read = NULL;
			}
			delete r;
			r = NULL;
		}

		if(listener[i].s != NULL){
			delete listener[i].s;
			listener[i].s = NULL;
		}

		if(thread != NULL){
			delete thread;
			thread = NULL;
		}
	}
}

/***************************************************************
//监听触发
//
***************************************************************/
void event_listener::event_listener_listen_cb(Socket *s, int mask, void *arg, struct event_handler_s * ev){
	event_listener_addr_t *listener_addr = (event_listener_addr_t *)arg;
	event_listener *el = listener_addr->el;
	//设置回调到用户层，执行accept
	el->event_listen_cb(listener_addr->s,el->userdata);
}

/***************************************************************
//设置端口进行连接
//
***************************************************************/
int event_listener::event_listener_add_listen(int server_port){
	int index = 0;
	for(index = 0; index < EVENT_LISTEN_NUM; index++){
		if(listener[index].s == NULL){
			break;
		}
	}

	if(index >= EVENT_LISTEN_NUM){
		return -1;
	}

	listener[index].s = new Socket();
	if(listener[index].s->Create(AF_INET,SOCK_STREAM) == -1){
		user_log_printf("socket create error\n");
		goto EXIT_FUNC;
	}
	//设置为非柱塞模式
	listener[index].s->set_socket_nonblock(1);
	//绑定端口
	if(listener[index].s->Bind(server_port) != 0){
		user_log_printf("socket Bind error\n");
		goto EXIT_FUNC;
	}
	//设置监听
	if(listener[index].s->Listen(512) != 0){
		user_log_printf("socket listen error\n");
		goto EXIT_FUNC;
	}
	//创建读事件
	listener[index].ev_read = r->reactor_create_event();
	if(listener[index].ev_read == NULL){
		user_log_printf("create ev_read error\n");
		goto EXIT_FUNC;
	}
	//添加到反应器中
	if(r->reactor_set_event(listener[index].ev_read,listener[index].s,EVENT_READ|EVENT_IO_PERSIST,event_listener_listen_cb,&listener[index]) != 0){
		user_log_printf("set add event error\n");
		goto EXIT_FUNC;
	}
	//添加反应器
	if(r->reactor_add_event(listener[index].ev_read) != 0){
		goto EXIT_FUNC;
	}
	return 0;
EXIT_FUNC:
	if(listener[index].s != NULL){
		delete listener[index].s;
		listener[index].s = NULL;
	}
	return -1;
}
/***************************************************************
//开启线程
//
***************************************************************/
int event_listener::event_listener_start_thread(){
	thread->Create(event_listener_thread,this);

	return 0;
}

/***************************************************************
//线程函数
//
***************************************************************/
Dthr_ret WINAPI event_listener::event_listener_thread(LPVOID lParam){
	event_listener *el = (event_listener *)lParam;
	el->r->reactor_loop_forever();
	return NULL;
}