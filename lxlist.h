#ifndef __LXLIST_H__
#define __LXLIST_H__

#include <stdio.h>
#include "userlib_type.h"

typedef struct lxlist_head_s{
	struct lxlist_head_s *next;
	struct lxlist_head_s *prev;
}lxlist_head_t;

/* ���ص�һ�����ָ�� */
#define LXLIST_FIRST_ENTRY( l, type, member) \
(((l)->next != (l) && (l)->next != NULL)?(CONTAINER_OF( (l)->next, type, member )): NULL)

class lxlist{
public:
	lxlist();
	~lxlist();
public:
	int lxlist_get_user_num();
	//��ͷ��ʼ����
	int lxlist_add_head(lxlist_head_t *new_n);
	//��β����ʼ����
	int lxlist_add_tail(lxlist_head_t *new_n);
	//ɾ���ڵ�
	int lxlist_del(lxlist_head_t *del);
	//�Ƿ�Ϊ��
	int lxlist_is_empty(){return (head.next == &head || head.prev == &head);}
	//��ȡ����ͷ
	lxlist_head_t * lxlist_get_head(){return &head;}

private:
	lxlist_head_t head;
	int user_num;
};

//���Դ���
int test_list_main();
#endif
