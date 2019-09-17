#ifndef __LXLIST_H__
#define __LXLIST_H__

#include <stdio.h>
#include "userlib_type.h"

typedef struct lxlist_head_s{
	struct lxlist_head_s *next;
	struct lxlist_head_s *prev;
}lxlist_head_t;

/* 返回第一个结点指针 */
#define LXLIST_FIRST_ENTRY( l, type, member) \
(((l)->next != (l) && (l)->next != NULL)?(CONTAINER_OF( (l)->next, type, member )): NULL)

class lxlist{
public:
	lxlist();
	~lxlist();
public:
	int lxlist_get_user_num();
	//从头开始插入
	int lxlist_add_head(lxlist_head_t *new_n);
	//从尾部开始插入
	int lxlist_add_tail(lxlist_head_t *new_n);
	//删除节点
	int lxlist_del(lxlist_head_t *del);
	//是否为空
	int lxlist_is_empty(){return (head.next == &head || head.prev == &head);}
	//获取链表头
	lxlist_head_t * lxlist_get_head(){return &head;}

private:
	lxlist_head_t head;
	int user_num;
};

//测试代码
int test_list_main();
#endif
