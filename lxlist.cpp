#include "lxlist.h"

lxlist::lxlist(){
	head.next = head.prev = &head;
	user_num = 0;
}
lxlist::~lxlist(){


}
//从头开始插入
int lxlist::lxlist_add_head(lxlist_head_t *new_n){
	if(new_n == NULL || head.next == NULL){
		return -1;
	}

	head.next->prev = new_n;
	new_n->next = head.next;
	new_n->prev = &head;
	head.next = new_n;
	user_num ++;
	return 0;
}
//从尾部开始插入
int lxlist::lxlist_add_tail(lxlist_head_t *new_n){
	if(new_n == NULL || head.prev == NULL){
		return -1;
	}

	new_n->next = &head;
	new_n->prev = head.prev;
	head.prev->next = new_n;
	head.prev = new_n;
	user_num ++;
	return 0;
}
//删除指定节点
int lxlist::lxlist_del(lxlist_head_t *del){

	if(del == NULL || del->next == NULL || del->prev == NULL){
		return -1;
	}

	del->next->prev = del->prev;
	del->prev->next = del->next;
	user_num --;
	if(user_num <= 0){
		user_num = 0;
	}
	return 0;
}

int lxlist::lxlist_get_user_num(){
	return user_num;
}
//-------------------------test----------------------------

typedef struct test_s{
	int num;
	char buf[32];
	int x;
	int y;
	lxlist_head_t test_list;
}test_t;

int test_list_main(){
	lxlist *list = new lxlist();
	test_t test;
	test_t test1;
	test_t test2;
	test_t test3;
	test_t *test_ev;
	
	test.num = 1;
	test1.num = 2;
	test2.num = 3;
	test3.num = 4;

	list->lxlist_add_tail(&test.test_list);
	list->lxlist_add_tail(&test1.test_list);
	list->lxlist_add_tail(&test2.test_list);
	list->lxlist_add_tail(&test3.test_list);

	while(!list->lxlist_is_empty()){
		test_ev = LXLIST_FIRST_ENTRY(list->lxlist_get_head(),test_t,test_list);
		printf("test_ev:%d\n",test_ev->num);
		list->lxlist_del(&test_ev->test_list);
	}
	return 0;
}