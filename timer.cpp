#include "timer.h"

head_timer::head_timer(int max_heap){
	int index = 0;
	int max_num = ALIGN(max_heap,128);//对2对齐

	max_heap_numer = max_num;
	now_heap_numer = 0;
	timer_array = new timer_queue_t[sizeof(timer_queue_t) * max_num];
	heap_array = new int[sizeof(int) * max_num];
	//初始化
	for(index = 0; index < max_num; index ++){
		timer_array[index].user_data = NULL;
		timer_array[index].heap_index = -1;

		heap_array[index] = -1;
	}
}

head_timer::~head_timer(){
	delete[] timer_array;
	delete[] heap_array;
	user_log_printf("~head_timer\n");
}

/************************************************************************/
/*功能：修正时间                                                        */
/*timeval_t * tv : 需要修正的时间戳                                     */
/************************************************************************/
void head_timer::timer_timeval_normalize(timeval_t * tv )
{
	if( tv == NULL )
		return ;
	
	if( tv->usec >= 1000000 ){
		tv->sec += tv->usec/1000000;
		tv->usec = tv->usec%1000000;
	}else if( tv->usec <= -1000000 ){
		tv->sec += tv->usec/1000000;
		tv->usec = tv->usec%1000000;
	}else if( tv->usec < 0 && tv->sec > 0 ){
		tv->sec--;
		tv->usec += 1000000;
	}else if( tv->usec > 0 &&  tv->sec < 0 ){
		tv->sec++;
		tv->usec += 1000000;
	}
}
/************************************************************************/
/*功能：时间戳相加                                                      */
/*timeval_t * left : 待处理时间戳                                       */
/*timeval_t * right : 待处理时间戳                                      */
/*timeval_t * result : 相加后返回的时间戳                               */
/************************************************************************/
void head_timer::timer_timeval_add(timeval_t * left,timeval_t * right, timeval_t * result )
{
	result->sec = left->sec+right->sec;
	result->usec = left->usec+right->usec;
	timer_timeval_normalize( result );
}
/************************************************************************/
/*功能：时间戳相加                                                      */
/*timeval_t * left : left时间戳                                         */
/*timeval_t * right : right时间戳                                       */
/*return ： 0    left =  right                                          */
/*          -1   left <  right                                          */
/*          1    left >  right                                          */
/************************************************************************/
int head_timer::timer_timeval_cmp(timeval_t* left,timeval_t* right )
{
	if( left != NULL && right == NULL ){
		return 1;
	}

	if( left == NULL && right != NULL ){
		return -1;
	}
	
	if( left == NULL || right == NULL ){
		return 0;
	}

	if( left->sec > right->sec ){
		return 1;
	}else if( left->sec == right->sec ){
		if( left->usec > right->usec ){
			return 1;
		}else if( left->usec == right->usec ){
			return 0;
		}
	}
	return -1;
}
/************************************************************************/
/*功能：从下往上移为最小堆                                              */
/*int timer_id : 当前需要排序的timer_array 下标                         */
/*int slot : 当前排序的   heap_array 的下标                             */
/************************************************************************/
void head_timer::timer_percolate_up(int timer_id , int slot){
	int parent = QUEUE_HEAP_PARENT(slot);

	while(slot > 0){
		//当前timer与父节点timer比较
		if(timer_timeval_cmp(&timer_array[timer_id].expire_tv,&timer_array[heap_array[parent]].expire_tv) < 0){
			heap_array[slot] = heap_array[parent];
			timer_array[heap_array[parent]].heap_index = slot;
			slot = parent;
			parent = QUEUE_HEAP_PARENT(slot);
		}else{
			break;
		}
	}
	timer_array[timer_id].heap_index = slot;
	heap_array[slot] = timer_id;
}
/************************************************************************/
/*功能：从上往下移为最小堆                                             */
/*int timer_id : 当前需要排序的timer_array 下标                         */
/*int slot : 当前排序的   heap_array 的下标                             */
/************************************************************************/
void head_timer::timer_percolate_down(int timer_id , int slot){
	int child = QUEUE_HEAP_LCHILD(slot);

	while(child < now_heap_numer){
		//选择最小的子节点
		if(child + 1  < now_heap_numer && 
			timer_timeval_cmp(&timer_array[heap_array[child + 1]].expire_tv,&timer_array[heap_array[child]].expire_tv) < 0){
				child ++;
		}
		//当前timer与子节点timer比较
		if(timer_timeval_cmp(&timer_array[heap_array[child]].expire_tv,&timer_array[timer_id].expire_tv) < 0){
			heap_array[slot] = heap_array[child];
			timer_array[heap_array[slot]].heap_index = slot;

			slot = child;
			child = QUEUE_HEAP_LCHILD(slot);
		}else{
			break;
		}
	}
	timer_array[timer_id].heap_index = slot;
	heap_array[slot] = timer_id;
}
/************************************************************************/
/*功能：增加定时器到队列中                                              */
/*void *data : 用户数据                                                 */
/*int interval : 定时时间                                               */
/*return ： timer_id    定时器下标                                      */
/************************************************************************/
int head_timer::timer_add_queue(void *data,int interval){
	int timer_id = 0;
	timeval_t now;
	timeval_t delay;
	while(timer_id <= max_heap_numer){
		if(timer_array[timer_id].heap_index == -1){
			break;
		}
		timer_id ++;
	}
	//计算时间
	delay.sec = interval / 1000;
	delay.usec = 1000 * (interval % 1000);
	gettimeofday(&now);
	timer_array[timer_id].interval_tv = delay;
	timer_timeval_add(&now,&delay,&timer_array[timer_id].expire_tv);
	timer_array[timer_id].heap_index = now_heap_numer;
	timer_array[timer_id].user_data = data;
	timer_percolate_up(timer_id,now_heap_numer);
	now_heap_numer ++;
	return timer_id;
}
/************************************************************************/
/*功能：删除指定定时器 一般为堆顶                                       */
/*int timer_id : 需要删除的定时器下标                                   */
/************************************************************************/
void head_timer::timer_del_queue(int timer_id){
	int slot = timer_array[timer_id].heap_index;
	int parent = QUEUE_HEAP_PARENT(slot);
	//默认将最后一个节点放到堆顶，然后再排序

	if(heap_array[slot] != timer_id){
		return ;
	}
	
	now_heap_numer --;
	if(now_heap_numer <= 0){
		now_heap_numer = 0;
	}
	if(slot < now_heap_numer){
		heap_array[slot] = heap_array[now_heap_numer];
		timer_array[heap_array[slot]].heap_index = slot;

		if(timer_timeval_cmp(&timer_array[heap_array[slot]].expire_tv,&timer_array[heap_array[parent]].expire_tv) >= 0){
			timer_percolate_down(heap_array[slot],slot);
		}else{
			timer_percolate_up(heap_array[slot],slot);
		}
	}
	timer_array[timer_id].user_data = NULL;
	timer_array[timer_id].heap_index = -1;
}
/************************************************************************/
/*功能：获取到期时间的定时器回调给用户                                  */
/*void ** pdata : 定时到了的用户数据                                    */
/*return ：  0     有定时器触发                                         */
/*           -1    没有触发                                             */
/************************************************************************/
int head_timer::timer_get_expire(void ** pdata){
	timeval_t now;
	if(now_heap_numer > 0){
		gettimeofday(&now);
		if(timer_timeval_cmp(&timer_array[heap_array[0]].expire_tv,&now) <= 0){
			*pdata = timer_array[heap_array[0]].user_data;
			timer_del_queue(heap_array[0]);
			return 0;
		}
	}
	
	return -1;
}
/************************************************************************/
/*功能：返回当前定时器中有效得timer个数                                 */
/*return ：  当前有效个数                                               */
/************************************************************************/
int head_timer::timer_queue_size(){
	return now_heap_numer;
}


//测试代码
int test_timer_main(){
	char data[256] = {0};
	char *ptr = data;
	head_timer *timer = new head_timer(1024);

	timer->timer_add_queue((void *)"timer 1 delay 2s",2000);
	timer->timer_add_queue((void *)"timer 2 delay 1.5s",1500);
	timer->timer_add_queue((void *)"timer 3 delay 1s",1000);
	timer->timer_add_queue((void *)"timer 4 delay 2.5s",2500);
	timer->timer_add_queue((void *)"timer 5 delay 0.1s",100);
	timer->timer_add_queue((void *)"timer 6 delay 0.5s",500);
	timer->timer_add_queue((void *)"timer 7 delay 0.7s",700);
	timer->timer_add_queue((void *)"timer 8 delay 1.3s",1300);
	timer->timer_add_queue((void *)"timer 9 delay 1.1s",1100);
	timer->timer_add_queue((void *)"timer 10 delay 1.9s",1900);
	timer->timer_add_queue((void *)"timer 11 delay 1.7s",1700);
	timer->timer_add_queue((void *)"timer 12 delay 3s",3000);
	timer->timer_add_queue((void *)"timer 13 delay 2.2s",2200);
	timer->timer_add_queue((void *)"timer 14 delay 2.8s",2800);
	timer->timer_add_queue((void *)"timer 15 delay 4s",4000);
	timer->timer_add_queue((void *)"timer 16 delay 3.5s",3500);

	while(1){
		if(timer->timer_get_expire((void **)&ptr) == 0){
			printf("ptr:%s\n",ptr);
		}
		sleep_millisecond(100);
	}
	return 0;
}