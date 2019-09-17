#ifndef __TIMER_H__
#define __TIMER_H__

#include "userlib_type.h"

//��ʱ���ṹ��
typedef struct timer_queue_s{
	timeval_t interval_tv;//��ʱʱ��
	timeval_t expire_tv;//ʵ�ʴ���ʱ��
	void * user_data;//�û�����
	int heap_index;//��Ӧheap_array����
}timer_queue_t;

class head_timer{
public:
	head_timer(int max_heap);
	virtual ~head_timer();
public:
	int timer_add_queue(void *data,int interval);//���Ӷ�ʱ����������
	int timer_get_expire(void ** pdata);//��ȡ����ʱ��Ķ�ʱ���ص����û�
	void timer_del_queue(int timer_id);//ɾ��ָ����ʱ�� һ��Ϊ�Ѷ�
	int timer_queue_size();//��ȡ���д�С
private:
	void timer_percolate_up(int timer_id, int slot);//����������Ϊ��С��
	void timer_percolate_down(int timer_id, int slot);//����������Ϊ��С��
	int timer_timeval_cmp(timeval_t* left,timeval_t* right );//ʱ����Ƚ�
	void timer_timeval_normalize(timeval_t * tv );//����ʱ��
	void timer_timeval_add(timeval_t * left,timeval_t * right, timeval_t * result );//ʱ�����
private:
	timer_queue_t *timer_array;//��ʱ������
	int           *heap_array;//��С�ѣ��Զ�ʱ��ʱ��������򣬴洢index��
	int           max_heap_numer;//���ѽڵ����
	int           now_heap_numer;//��ǰ��Ԫ�ظ���
};

#endif
