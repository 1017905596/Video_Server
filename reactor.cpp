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
 *   ���ܣ������¼�������
 *   @return��
 *           NULL                                ʧ��
 *           ��NULL                              �ɹ�
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
*   ���ܣ������¼�������
*   @param :
*           event_handler_t * ev           �¼�������
*   @return��
*           ��
************************************************************************************/
void reactor::reactor_destroy_event(event_handler_t *ev){
	if(ev != NULL){
		free(ev);
		ev = NULL;
	}
}
/***********************************************************************************
*   ���ܣ������¼�����������ӵ���Ӧ����
*   @param :
*           event_handler_t * ev           �¼�������
*           Socket  *s                     �׽ӿڶ���
*           int mask                       �¼�����(�ɶ��¼�����д�¼�����ʱ���¼���
*           event_cb_t event_cb            �ص�����
*           void * arg                     �ص���������
*   @return��
*           0                                   �ɹ�
*           -1                                  ʧ�ܣ��Ѵ��ڻ����¼�������ΪNULL
************************************************************************************/
int reactor::reactor_set_event(event_handler_t *ev,Socket *s,int mask,event_cb_t event_cb,void *arg){
	if(BIT_ENABLED(ev->flag,EVENT_FLAG_ALL_LIST)){
		return -1;
	}else{
		ev->r = this;
		ev->s = s;              //�׽ӿ�
		ev->event_mask = mask; //�¼�����
		ev->cb_arg = arg;       //�ص�����
		ev->cb_func = event_cb; //�ص�����
	}
	return 0;
}
/***********************************************************************************
*   ���ܣ����ö�ʱ���¼�����ӵ���Ӧ����
*   @param :
*           event_handler_t* ev            �¼�������
*           Socket  *s                     �׽ӿڶ���
*           event_cb_t event_cb            ��ʱ���ص�����
*           void * arg                     �ص������Ĳ���
8           unsigned int interval_ms       ��ʱʱ��
*   @return��
*           0                                   �ɹ�
*           -1                                  ʧ�ܣ��Ѵ��ڸ��¼������¼�������ΪNULL
************************************************************************************/
int reactor::reactor_set_timer(event_handler_t *ev,Socket *s,unsigned int interval_ms,event_cb_t event_cb,void *arg){
	if(BIT_ENABLED(ev->flag,EVENT_FLAG_ALL_LIST)){
		return -1;
	}else{
		ev->r = this;
		ev->interval_ms = interval_ms; //�¼����
		ev->s = s;                     //�׽ӿ�
		ev->event_mask = EVENT_TIMER; //�¼�����
		ev->cb_arg = arg;              //�ص�����
		ev->cb_func = event_cb;        //�ص�����
	}
	return 0;
}
/***********************************************************************************
*   ���ܣ�����¼�����������Ӧ����
*   @param :
*           event_handler_t * ev           �¼�������
*   @return��
*           0                                   �ɹ�
*           -1                                  ʧ��
************************************************************************************/
int reactor::reactor_add_event(event_handler_t * ev){
	//ע��ɶ��¼����д�¼�
	if(BIT_ENABLED(ev->event_mask, EVENT_READ|EVENT_WRITE)){
		//���¼���������ӵ���Ӧģʽ�ķ�Ӧ����
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
*   ���ܣ����¼��������ӷ�Ӧ�����Ƴ�
*   @param :
*           event_handler_t * ev           �¼�������
*   @return��
*           0                                   �ɹ�
*           -1                                  ʧ��
************************************************************************************/
int reactor::reactor_remove_event(event_handler_t * ev){
	if(BIT_ENABLED(ev->flag, EVENT_FLAG_TIMER_LIST)){ //�ڶ�ʱ��������
		CLR_BITS(ev->flag, EVENT_FLAG_TIMER_LIST );   //�����־ 
		if(ev->timerid != -1){
			tq->timer_del_queue(ev->timerid);
		}
	}else if(BIT_ENABLED(ev->flag, EVENT_FLAG_IO_LIST)){//��д�¼���δ����
		CLR_BITS(ev->flag, EVENT_FLAG_IO_LIST);       //�����־
		select->reactor_imp_remove_event(ev);
		ev->event_mask = 0;
	}else if(BIT_ENABLED(ev->flag, EVENT_FLAG_ACTIVE_LIST)){//�¼��Ѿ���������δ�ϱ�
		CLR_BITS(ev->flag, EVENT_FLAG_ACTIVE_LIST); 
		active_list->lxlist_del(&ev->list_active);
		ev->active_mask = 0;
	}
	return 0;
}
/***********************************************************************************
*   ���ܣ������Ծ�����е��¼������ûص�������
*   @return��
*            >=0                                 ������Ļ�Ծ�¼����()
************************************************************************************/
int reactor::reactor_process_active_event(){
	event_handler_t * ev = NULL;
	int active_events = 0;
	int ev_active_mask = 0;
	while(!active_list->lxlist_is_empty()){
		//����������Ϣ
		ev =  LXLIST_FIRST_ENTRY(active_list->lxlist_get_head(),event_handler_t,list_active);
		//ɾ��ָ������ڵ�
		active_list->lxlist_del(&ev->list_active);
		//����¼���־
		CLR_BITS(ev->flag, EVENT_FLAG_ACTIVE_LIST);
		//�����¼�״̬
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
*   ���ܣ���һ���¼���ӵ���Ծ����,����Ѿ���������,�������¼��Ļ�Ծ���ͣ���ʱ��������д�¼��Ļص���
*   @param :
*           struct event_handler_s * ev    �¼�������
*           int mask                       �¼�����
*   @return��
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
*   ���ܣ���Ӧ������ѭ��
*   @return��
*            0                                   �ɹ��˳�
*           -1                                  ����
************************************************************************************/
int reactor::reactor_loop_forever(){
	timeval_t tv;
	int ret = 0;

	tv.sec = 0;
	tv.usec = 10*1000;
	while(!is_wait_exit){
		//��ʱ���¼�
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
		//Socket ��д�¼�
		select->reactor_imp_dispath_event(&tv);
		//��ȡ��Ծ����ص�
		ret = reactor_process_active_event();
		if(ret == 0){
			sleep_millisecond(5);
		}
	}
	return 0;
}
/***********************************************************************************
*   ���ܣ��˳���Ӧ��ѭ��(�ù���Ϊ�˳�reactorʱʹ�ã�����ʱ�䲻Ҫʹ�÷���������������)
*   @return��
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

	//��������¼�
	if(r->reactor_set_event(ev,s_r,EVENT_READ|EVENT_IO_PERSIST,event_test_cb,ev) != 0){
		printf("reactor_set_add_event 1 error\n");
		return ;
	}
	if(r->reactor_add_event(ev) != 0){
		return ;
	}
	//��ʱ1s
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