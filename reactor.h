#ifndef __REACTOR_H__
#define __REACTOR_H__

#include "reactor_select.h"

class reactor{
public:
	reactor(size_t max_event_num);
	~reactor();
public:
	//�����¼�������(�ɶ��¼�����д�¼�����ʱ���¼�)
	event_handler_t * reactor_create_event();
	//�ͷ��¼�������
	void reactor_destroy_event(event_handler_t *ev);

	//��Ӷ�д�¼��������õ���Ӧ����
	int reactor_set_event(event_handler_t *ev,Socket *s,int mask,event_cb_t event_cb,void *arg);
	//��Ӷ�ʱ���¼��������õ���Ӧ����
	int reactor_set_timer(event_handler_t *ev,Socket *s,unsigned int interval_ms,event_cb_t event_cb,void *arg);

	//��ӵ���Ӧ��
	int reactor_add_event(event_handler_t * ev);
	//�ӷ�Ӧ���Ƴ��¼�������
	int reactor_remove_event(event_handler_t * ev);

	//��Ӧ������ѭ����ֱ����Ҫ���˳�
	int reactor_loop_forever();
	//��Ӧ���˳�
	int reactor_exit_loop();
private:
	//����д����ʱ���¼��ص������Ծ����
	static int reactor_active_event_cb(struct event_handler_s * ev, int mask);
	//�ӻ�Ծ�����ж�ȡ�¼��ص����ϲ�
	int reactor_process_active_event();
private:
	//��ʱ������
	head_timer * tq;
	//select ����
	reactor_select *select;
	//��Ծ�¼��������
	lxlist *active_list;
	//�˳���Ӧ����־
	int is_wait_exit;
};

void reactor_validation_test();
#endif
