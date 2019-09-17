#ifndef __REACTOR_SELECT_H__
#define __REACTOR_SELECT_H__

#include <stdio.h>
#include "lxlist.h"
#include "timer.h"
#include "Pthread.h"
#include "Socket.h"
#include "userlib_type.h"



#define EVENT_READ             1    /* ���¼� */
#define EVENT_WRITE            2    /* д�¼� */
#define EVENT_TIMER            4    /* ��ʱ���¼� */
#define EVENT_IO_PERSIST       8    /* ������IO�����׽ӿڳ�������д */
#define EVENT_EMASK    (EVENT_READ|EVENT_WRITE|EVENT_TIMER)
#define EVENT_FLAG_IO_LIST      0x00000100    /* �¼���־���¼���IO������ */
#define EVENT_FLAG_ACTIVE_LIST  0x00000200    /* �¼���־���¼��ڻ�Ծ������ */
#define EVENT_FLAG_TIMER_LIST   0x00000400    /* �¼���־���¼��ڶ�ʱ�������� */
#define EVENT_FLAG_ALL_LIST (EVENT_FLAG_IO_LIST|EVENT_FLAG_ACTIVE_LIST|EVENT_FLAG_TIMER_LIST)

struct event_handler_s;
class reactor;

/* �¼��ص����� */
typedef void (*event_cb_t)(Socket *s, int mask, void *arg, struct event_handler_s * ev);
/* �����¼��ص���������Ҫ�ǽ��¼����ص���Ӧ���Ļ�Ծ���� */
typedef int (*reactor_active_event_t)(struct event_handler_s * ev, int mask );


/* �¼��������ṹ�� */
typedef struct event_handler_s{
	volatile unsigned int flag;        /* �¼����������־ */
	int interval_ms;                  //���ʱ��

	Socket *s;                          /* �׽ӿڶ��� */
	reactor *r;                       //���浱ǰ��Ӧ������
	int timerid;                     //��ʱ��id
	int event_mask;                 /* ��Ծ�¼����ͣ�����д����ʱ������ */
	int active_mask;                 /* ��Ծ�¼����ͣ�����д����ʱ������ */
	lxlist_head_t list_active;      /* װ�ػ�Ծ�¼����� */

	//���ڻص�
	event_cb_t cb_func;        /* �¼��ص����� */
	void * cb_arg;                /* �ص��������� */
}event_handler_t;

/* �¼��ṹ */
typedef struct reactor_select_event_s{
	int fd;                                    /* �׽ӿ� */
	event_handler_t *ev_read;                /* �ɶ��¼������� */
	event_handler_t *ev_write;                /* ��д�¼������� */
}reactor_select_event_t;


class reactor_select{
public:
	reactor_select(int max_size,reactor_active_event_t cb);
	~reactor_select();
public:
	int reactor_imp_dispath_event(const timeval_t *tv);//����Ƿ���ʱ������������򴥷���Ӧ�Ĵ�����
	int reactor_imp_remove_event(event_handler_t *ev);//�Ƴ��¼��������ٹ������д
	int reactor_imp_add_event(event_handler_t *ev);//ע������¼����ɶ���д�¼���
private:
	int reactor_select_find_empty(int fd);//��һ��λ�ã�ע���׽ӿڣ���ע�����ã�
	int reactor_select_find_fd(int fd);//�ҵ�ָ���׽ӿ�����λ��
	int reactor_select_rebuild_fd_set();//�ؽ����ļ�����������
	int reactor_select_fd_set_copy(fd_set *t ,const fd_set * s);//��¡�ļ�����������
private:
	reactor_active_event_t event_cb;   //�ص�
	int max_fd_size;                   //��󻺴����������ļ�������������
	reactor_select_event_t *events;    //����ʱ��
	fd_set * fd_set_read;              //�ɶ��¼��ļ�����������
	fd_set * fd_set_write;             //��д�¼��ļ�����������
	fd_set * fd_set_used_read;         //��ǰ��ע��Ŀɶ��¼��ļ�����������
	fd_set * fd_set_used_write;        //��ǰ��ע��Ŀ�д�¼��ļ����������� 
	fd_set * fd_set_used_except;       //��ǰ��ע����쳣�¼��ļ�����������
	int fd_slot_used_min;              //��ע�����С�ļ�������
	int fd_slot_used_max;              //��ע�������ļ������� 
	int fd_width;                      //�ļ���������Χ����ע���ļ�����������
};

#endif