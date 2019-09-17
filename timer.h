#ifndef __TIMER_H__
#define __TIMER_H__

#include "userlib_type.h"

//定时器结构体
typedef struct timer_queue_s{
	timeval_t interval_tv;//定时时长
	timeval_t expire_tv;//实际触发时间
	void * user_data;//用户数据
	int heap_index;//对应heap_array坐标
}timer_queue_t;

class head_timer{
public:
	head_timer(int max_heap);
	virtual ~head_timer();
public:
	int timer_add_queue(void *data,int interval);//增加定时器到队列中
	int timer_get_expire(void ** pdata);//获取到期时间的定时器回调给用户
	void timer_del_queue(int timer_id);//删除指定定时器 一般为堆顶
	int timer_queue_size();//获取队列大小
private:
	void timer_percolate_up(int timer_id, int slot);//从下往上移为最小堆
	void timer_percolate_down(int timer_id, int slot);//从上往下移为最小堆
	int timer_timeval_cmp(timeval_t* left,timeval_t* right );//时间戳比较
	void timer_timeval_normalize(timeval_t * tv );//修正时间
	void timer_timeval_add(timeval_t * left,timeval_t * right, timeval_t * result );//时间戳加
private:
	timer_queue_t *timer_array;//定时器队列
	int           *heap_array;//最小堆（对定时器时间进行排序，存储index）
	int           max_heap_numer;//最大堆节点个数
	int           now_heap_numer;//当前堆元素个数
};

#endif
