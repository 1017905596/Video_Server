#include "app_test_server.h"

static app_test_server_http *http_buf[MAX_CONNECT_LEN];
static app_test_server_rtsp *rtsp_buf[MAX_CONNECT_LEN];
static app_test_server_rtmp *rtmp_buf[MAX_CONNECT_LEN];
static app_test_server_state_t last_state;

app_test_server::app_test_server(){
	test_server.ats = this;
}

void app_test_server::app_test_server_destroy_mod_http(app_test_server_http *http){
	last_state = APP_TEST_SERVER_MOD_END;
	for(int i = 0; i < MAX_CONNECT_LEN; i++){
		if(http_buf[i] == NULL){
			http_buf[i] = http;
			return ;
		}
	}
}
void app_test_server::app_test_server_destroy_mod_rtsp(app_test_server_rtsp *rtsp){
	last_state = APP_TEST_SERVER_MOD_END;
	for(int i = 0; i < MAX_CONNECT_LEN; i++){
		if(rtsp_buf[i] == NULL){
			rtsp_buf[i] = rtsp;
			return ;
		}
	}
}
void app_test_server::app_test_server_destroy_mod_rtmp(app_test_server_rtmp *rtmp){
	last_state = APP_TEST_SERVER_MOD_END;
	for(int i = 0; i < MAX_CONNECT_LEN; i++){
		if(rtmp_buf[i] == NULL){
			rtmp_buf[i] = rtmp;
			return ;
		}
	}
}
Dthr_ret WINAPI app_test_server::app_test_server_thread(LPVOID lParam){
	while(1){
		if(last_state ==  APP_TEST_SERVER_MOD_END){
			for(int i = 0; i < MAX_CONNECT_LEN; i++){
				if(http_buf[i] != NULL){
					user_log_printf("free http:%p\n",http_buf[i]);
					delete http_buf[i];
					http_buf[i] = NULL;
				}else if(rtsp_buf[i] != NULL){
					user_log_printf("free rtsp:%p\n",rtsp_buf[i]);
					delete rtsp_buf[i];
					rtsp_buf[i] = NULL;
				}else if(rtmp_buf[i] != NULL){
					user_log_printf("free rtmp:%p\n",rtmp_buf[i]);
					delete rtmp_buf[i];
					rtmp_buf[i] = NULL;
				}
			}
			last_state = APP_TEST_SERVER_ERROR;
		}
		sleep_millisecond(1000);
	}
	return NULL;
}

/***************************************************************
//释放当前连接节点
//
***************************************************************/
void app_test_server::app_test_server_clean(app_test_user_data_t *atud){
	if(atud != NULL){
		if(atud->ec != NULL){
			if(atud->ec->es != NULL){
				atud->ec->es->event_server_connection_remove_event(atud->ec,EVENT_EMASK);
				atud->ec->es->event_server_destroy_connection(atud->ec);
			}
		}
		if(atud->recv_data != NULL){
			delete atud->recv_data;
			atud->recv_data = NULL;
		}
		free(atud);
		atud = NULL;
	}
}

void app_test_server::app_test_server_impl_detach(app_test_user_data_t *atud){
	atud->ec = NULL;
}

int app_test_server::app_test_server_process(app_test_user_data_t *atud){
	char *version = (char *)atud->recv_data->message_block_get_rd_ptr();
	if(strstr(version,"RTSP") != NULL){
		user_log_printf("start RTSP play\n");
		atud->rtsp = new app_test_server_rtsp(app_test_server_destroy_mod_rtsp,atud->ec,version,atud->recv_data->message_block_get_data_len());
		int ret = atud->rtsp->app_test_server_rtsp_impl_run();
		if(ret == 0){
			atud->ats->app_test_server_impl_detach(atud);
			atud->state = APP_TEST_SERVER_END;
			user_log_printf("goto rtsp server play\n");
		}else{
			atud->state = APP_TEST_SERVER_ERROR;
		}
	}else if(strstr(version,"HTTP") != NULL){
		user_log_printf("start HTTP play\n");
		atud->http = new app_test_server_http(app_test_server_destroy_mod_http,atud->ec,version,atud->recv_data->message_block_get_data_len());
		int ret = atud->http->app_test_server_http_impl_run();
		if(ret == 0){
			atud->ats->app_test_server_impl_detach(atud);
			atud->state = APP_TEST_SERVER_END;
			user_log_printf("goto http server play\n");
		}else{
			atud->state = APP_TEST_SERVER_ERROR;
		}
	}else if(version[0] == 0x03 && atud->recv_data->message_block_get_data_len() == 1537){
		user_log_printf("start rtmp play\n");
		atud->rtmp = new app_test_server_rtmp(app_test_server_destroy_mod_rtmp,atud->ec,version,atud->recv_data->message_block_get_data_len());
		int ret = atud->rtmp->app_test_server_rtmp_impl_run();
		if(ret == 0){
			atud->ats->app_test_server_impl_detach(atud);
			atud->state = APP_TEST_SERVER_END;
			user_log_printf("goto rtmp server play\n");
		}else{
			atud->state = APP_TEST_SERVER_ERROR;
		}
	}else{
		atud->state = APP_TEST_SERVER_ERROR;
	}
	return 0;
}


/***************************************************************
//数据处理回调
//
***************************************************************/
/************************************************************************/
//HTTP
/* :GET / HTTP/1.1
Host: 127.0.0.1:7000
Accept: 
Accept-Language: zh_CN
User-Agent: VLC/3.0.5 LibVLC/3.0.5
Range: bytes=0-                                                         */
//rtsp
/*OPTIONS rtsp://127.0.0.1:7000 RTSP/1.0
CSeq: 1
User-Agent: RealMedia Player Version 6.0.9.1235 (linux-2.0-libc6-i386-gcc2.95)*/
/************************************************************************/
void app_test_server::app_test_server_event_cb(struct event_connection_s *ec, int event_task, void *user_data ){
	int ret = 0;
	app_test_user_data_t *atud = (app_test_user_data_t *)user_data;
	
	if(event_task == EVENT_READ){
		if(ec->s != NULL){
			ret = ec->s->Receive(atud->recv_data->message_block_get_wr_ptr(),atud->recv_data->message_block_get_space());
			
			if(ret > 0){
				atud->recv_data->message_block_wr_pos_add(ret);
				user_log_printf("recv new user request:%.*s\n",
					atud->recv_data->message_block_get_data_len(),atud->recv_data->message_block_get_rd_ptr());
			}else{
				atud->state = APP_TEST_SERVER_ERROR;
			}
		}
	}

	if(event_task == EVENT_TIMER){
		ec->es->event_server_connection_add_event(ec,EVENT_TIMER, 50);
	}
	//主要为了区分协议
	if(atud->state == APP_TEST_SERVER_START){
		if(atud->recv_data->message_block_get_data_len() != 0){
			atud->ats->app_test_server_process(atud);
		}
	}

	if(atud->state == APP_TEST_SERVER_ERROR || atud->state == APP_TEST_SERVER_END){
		atud->ats->app_test_server_clean(atud);
	}
}

/***************************************************************
//连接成功
//
***************************************************************/
void app_test_server::app_test_server_create_connect_success(event_connection_t * ec,void *data){
	if(ec == NULL){
		return ;
	}
	app_test_server_t *test_server = (app_test_server_t *)data;
	app_test_user_data_t *atud = (app_test_user_data_t *)malloc(sizeof(app_test_user_data_t));
	
	memset(atud,0,sizeof(app_test_user_data_t));
	atud->ats = test_server->ats;
	atud->ec = ec;
	atud->state = APP_TEST_SERVER_START;
	atud->recv_data = new message_block(1024*512);

	user_log_printf("connect success\n");

	//设置数据处理回调函数
	atud->ec->es->event_server_set_event_cb(atud->ec,app_test_server_event_cb,atud);
	atud->ec->es->event_server_connection_add_event(atud->ec,EVENT_READ|EVENT_IO_PERSIST, 0);
	atud->ec->es->event_server_connection_add_event(atud->ec,EVENT_TIMER, 100);
}
/***************************************************************
//连接失败
//
***************************************************************/
void app_test_server::app_test_server_create_connect_error(event_queue_connection_t * eqc, int err){
	if(eqc->s != NULL){
		delete eqc->s;
		eqc->s = NULL;
	}
}
/***************************************************************
//接收监听响应（accpet处理）
//
***************************************************************/
void app_test_server::app_test_server_listen(Socket *listen_s, void * user_data){
	app_test_server_t *ats = (app_test_server_t *)user_data;
	int socket = listen_s->Accept();
	user_log_printf("listen accept new connection\n");
	ats->es->event_server_join_connection_list(socket,listen_s->client_ip);
}

/***************************************************************
//初始化监听以及server
//
***************************************************************/
int app_test_server::app_test_server_init(){
	test_server.es = new event_server(app_test_server_create_connect_success,app_test_server_create_connect_error,1,&test_server);
	if(test_server.es == NULL){
		return -1;
	}
	test_server.el = new event_listener(app_test_server_listen,&test_server);
	if(test_server.el == NULL){
		return -1;
	}
	test_server.el->event_listener_add_listen(7000);
	test_server.el->event_listener_add_listen(1935);
	user_log_printf("start listening \n");

	thread = new CPthread();
	thread->Create(app_test_server_thread,&test_server);
	return 0;
}
/***************************************************************
//开启线程
//
***************************************************************/
int app_test_server::app_test_server_begin_thread(){
	if(test_server.el != NULL){
		test_server.el->event_listener_start_thread();
	}
	if(test_server.el != NULL){
		test_server.es->event_server_start_thread();
	}
	return 0;
}