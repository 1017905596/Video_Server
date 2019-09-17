#include "reactor_select.h"

reactor_select::reactor_select(int max_size,reactor_active_event_t cb){
	fd_slot_used_min = 0;
	fd_slot_used_max = 0;
	max_fd_size = max_size;
	event_cb = cb;

	events = (reactor_select_event_t *)malloc(max_fd_size * sizeof(reactor_select_event_t));
	if(events != NULL){
		for(int i =0; i < max_fd_size; i++){
			events[i].fd = -1;
			events[i].ev_read = NULL;
			events[i].ev_write = NULL;
		}
	}
	/* 分配可读、可写和异常文件描述符集合 */
	fd_set_read = (fd_set *)malloc(sizeof(fd_set));
	fd_set_write = (fd_set *)malloc(sizeof(fd_set));
	fd_set_used_read = (fd_set *)malloc(sizeof(fd_set));
	fd_set_used_write = (fd_set *)malloc(sizeof(fd_set));
	fd_set_used_except = (fd_set *)malloc(sizeof(fd_set));

	/* 清零可读、可写和异常文件描述符集合 */
	FD_ZERO(fd_set_read);
	FD_ZERO(fd_set_write);
	FD_ZERO(fd_set_used_read);
	FD_ZERO(fd_set_used_write);
	FD_ZERO(fd_set_used_except);
}

reactor_select::~reactor_select(){

	if(events != NULL){
		free(events);
		events = NULL;
	}
	if(fd_set_read != NULL){
		free(fd_set_read);
		fd_set_read = NULL;
	}
	if(fd_set_write != NULL){
		free(fd_set_write);
		fd_set_write = NULL;
	}
	if(fd_set_used_read != NULL){
		free(fd_set_used_read);
		fd_set_used_read = NULL;
	}
	if(fd_set_used_write != NULL){
		free(fd_set_used_write);
		fd_set_used_write = NULL;
	}
	if(fd_set_used_except != NULL){
		free(fd_set_used_except);
		fd_set_used_except = NULL;
	}
	user_log_printf("~reactor_select\n");
}
/***********************************************************************************
*   功能：克隆文件描述符集合
*   @param :
*           fd_set *t                   目的描述符集合
*           fd_set *s                   源描述符集合
*   @return：
*           0
************************************************************************************/
int reactor_select::reactor_select_fd_set_copy(fd_set *t ,const fd_set * s){
	memcpy(t, s, sizeof(fd_set));
	return 0;
}
/***********************************************************************************
*   功能：找一空位置，注册套接口（已注册则复用）
*   @param :
*            int fd        ---    将被监听的套接口
*   @return：
*           -1              ---            失败
*            >=0            ---          成功，空闲位置
************************************************************************************/
int reactor_select::reactor_select_find_empty(int fd){
	if( fd < max_fd_size ){
		return fd;
	}
	return -1;
}
/***********************************************************************************
*   功能：重建被文件描述符集合
*   @param :
*            无
*   @return：
*           -1            ---            失败
*            0            ---            成功
************************************************************************************/
int reactor_select::reactor_select_rebuild_fd_set(){
	FD_ZERO(fd_set_read);        /* 清零读可读可写文件描述符集合 */
	FD_ZERO(fd_set_write);
	fd_width = -1;
	for(int i = fd_slot_used_min; i <= fd_slot_used_max; i++){
		if(events[i].fd != -1){
			if(events[i].ev_read != NULL){
				FD_SET(events[i].fd,fd_set_read);
			}
			if(events[i].ev_write != NULL){
				FD_SET(events[i].fd,fd_set_write);
			}
			if(events[i].ev_read != NULL || events[i].ev_write != NULL){
				if(fd_width < events[i].fd){
					fd_width = events[i].fd;
				}
			}
		}
	}
	return 0;
}
/***********************************************************************************
*   功能：找到指定套接口所在位置
*   @param :
*            int fd                    ---                套接口
*   @return：
*           -1            ---            失败
*            0            ---            成功
************************************************************************************/
int reactor_select::reactor_select_find_fd(int fd){
	if( fd < max_fd_size ){
		return fd;
	}
	return -1;
}
/***********************************************************************************
*   功能：注册监听事件（可读可写事件，等）
*   @param :
*            event_handler_t* ev        ---            事件处理器
*   @return：
*           -1            ---            失败
*            0            ---            成功
************************************************************************************/
int reactor_select::reactor_imp_add_event(event_handler_t *ev){
	if(ev == NULL || ev->s == NULL || ev->s->m_nSocket == -1){
		return -1;
	}

	int slot = reactor_select_find_empty(ev->s->m_nSocket);
	if(slot < 0){
		return -1;
	}

	events[slot].fd = ev->s->m_nSocket;
	if(BIT_ENABLED(ev->event_mask,EVENT_READ)){
		events[slot].ev_read = ev;
	}
	if(BIT_ENABLED(ev->event_mask,EVENT_WRITE)){
		events[slot].ev_write = ev;
	}
	if(events[slot].ev_write == NULL && events[slot].ev_read == NULL){
		events[slot].fd = -1;
	}
	//已使用最小位置
	if(fd_slot_used_min > slot){       
		fd_slot_used_min = slot;
	}
	//已使用最大位置
	if(fd_slot_used_max <= slot){
		fd_slot_used_max = slot+1;
	}
	return reactor_select_rebuild_fd_set();
}
/***********************************************************************************
*   功能：移除事件，即不再关心其读写
*   @param :
*            event_handler_t* ev        ---            事件处理器
*   @return：
*           -1            ---            失败
*            0            ---            成功
************************************************************************************/
int reactor_select::reactor_imp_remove_event(event_handler_t *ev){
	int slot = reactor_select_find_fd(ev->s->m_nSocket);
	if(slot < 0){
		return -1;
	}
	if(BIT_ENABLED(ev->event_mask,EVENT_READ)){
		events[slot].ev_read = NULL;
	}
	if(BIT_ENABLED(ev->event_mask,EVENT_WRITE)){
		events[slot].ev_write = NULL;
	}
	if(events[slot].ev_write == NULL && events[slot].ev_read == NULL){
		events[slot].fd = -1;
	}
	return reactor_select_rebuild_fd_set();
}
/***********************************************************************************
*   功能：检查是否有时间就绪，若有则触发对应的处理函数
*   @param :
*            const timeval_t* tv    ---            超时时间
*   @return：
*           -1            ---            失败
*            0            ---            成功
************************************************************************************/
int reactor_select::reactor_imp_dispath_event(const timeval_t *tv){
	struct timeval tv_temp;
	if( tv == NULL )
	{
		tv_temp.tv_sec = 0;        /* 秒数 */
		tv_temp.tv_usec = 0;    /* 微秒 */
	}
	else
	{
		tv_temp.tv_sec = tv->sec;
		tv_temp.tv_usec = tv->usec;
	}

	reactor_select_fd_set_copy(fd_set_used_read,fd_set_read);
	reactor_select_fd_set_copy(fd_set_used_write,fd_set_write);
	reactor_select_fd_set_copy(fd_set_used_except,fd_set_write);

	int ret = select(fd_width + 1,fd_set_used_read,fd_set_used_write,fd_set_used_except,&tv_temp);
	if(ret > 0){
		for(int i = fd_slot_used_min; i < fd_slot_used_max; i++){
			if(events[i].fd != -1){
				//可读事件
				if(events[i].ev_read != NULL && FD_ISSET(events[i].fd,fd_set_used_read)){
					event_cb(events[i].ev_read,EVENT_READ);
				}
				//可写事件
				if(events[i].ev_write != NULL && FD_ISSET(events[i].fd,fd_set_used_write)){
					event_cb(events[i].ev_write,EVENT_WRITE);
				}
				//异常事件
				if(events[i].ev_write != NULL && FD_ISSET(events[i].fd,fd_set_used_except)){
					event_cb(events[i].ev_write,EVENT_WRITE);
				}
			}
		}
	}else{
		return ret;
	}
	return 0;
}
