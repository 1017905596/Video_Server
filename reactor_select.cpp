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
	/* ����ɶ�����д���쳣�ļ����������� */
	fd_set_read = (fd_set *)malloc(sizeof(fd_set));
	fd_set_write = (fd_set *)malloc(sizeof(fd_set));
	fd_set_used_read = (fd_set *)malloc(sizeof(fd_set));
	fd_set_used_write = (fd_set *)malloc(sizeof(fd_set));
	fd_set_used_except = (fd_set *)malloc(sizeof(fd_set));

	/* ����ɶ�����д���쳣�ļ����������� */
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
*   ���ܣ���¡�ļ�����������
*   @param :
*           fd_set *t                   Ŀ������������
*           fd_set *s                   Դ����������
*   @return��
*           0
************************************************************************************/
int reactor_select::reactor_select_fd_set_copy(fd_set *t ,const fd_set * s){
	memcpy(t, s, sizeof(fd_set));
	return 0;
}
/***********************************************************************************
*   ���ܣ���һ��λ�ã�ע���׽ӿڣ���ע�����ã�
*   @param :
*            int fd        ---    �����������׽ӿ�
*   @return��
*           -1              ---            ʧ��
*            >=0            ---          �ɹ�������λ��
************************************************************************************/
int reactor_select::reactor_select_find_empty(int fd){
	if( fd < max_fd_size ){
		return fd;
	}
	return -1;
}
/***********************************************************************************
*   ���ܣ��ؽ����ļ�����������
*   @param :
*            ��
*   @return��
*           -1            ---            ʧ��
*            0            ---            �ɹ�
************************************************************************************/
int reactor_select::reactor_select_rebuild_fd_set(){
	FD_ZERO(fd_set_read);        /* ������ɶ���д�ļ����������� */
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
*   ���ܣ��ҵ�ָ���׽ӿ�����λ��
*   @param :
*            int fd                    ---                �׽ӿ�
*   @return��
*           -1            ---            ʧ��
*            0            ---            �ɹ�
************************************************************************************/
int reactor_select::reactor_select_find_fd(int fd){
	if( fd < max_fd_size ){
		return fd;
	}
	return -1;
}
/***********************************************************************************
*   ���ܣ�ע������¼����ɶ���д�¼����ȣ�
*   @param :
*            event_handler_t* ev        ---            �¼�������
*   @return��
*           -1            ---            ʧ��
*            0            ---            �ɹ�
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
	//��ʹ����Сλ��
	if(fd_slot_used_min > slot){       
		fd_slot_used_min = slot;
	}
	//��ʹ�����λ��
	if(fd_slot_used_max <= slot){
		fd_slot_used_max = slot+1;
	}
	return reactor_select_rebuild_fd_set();
}
/***********************************************************************************
*   ���ܣ��Ƴ��¼��������ٹ������д
*   @param :
*            event_handler_t* ev        ---            �¼�������
*   @return��
*           -1            ---            ʧ��
*            0            ---            �ɹ�
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
*   ���ܣ�����Ƿ���ʱ������������򴥷���Ӧ�Ĵ�����
*   @param :
*            const timeval_t* tv    ---            ��ʱʱ��
*   @return��
*           -1            ---            ʧ��
*            0            ---            �ɹ�
************************************************************************************/
int reactor_select::reactor_imp_dispath_event(const timeval_t *tv){
	struct timeval tv_temp;
	if( tv == NULL )
	{
		tv_temp.tv_sec = 0;        /* ���� */
		tv_temp.tv_usec = 0;    /* ΢�� */
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
				//�ɶ��¼�
				if(events[i].ev_read != NULL && FD_ISSET(events[i].fd,fd_set_used_read)){
					event_cb(events[i].ev_read,EVENT_READ);
				}
				//��д�¼�
				if(events[i].ev_write != NULL && FD_ISSET(events[i].fd,fd_set_used_write)){
					event_cb(events[i].ev_write,EVENT_WRITE);
				}
				//�쳣�¼�
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
