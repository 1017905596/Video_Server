#ifndef __MESSAGE_BLOCK_H__
#define __MESSAGE_BLOCK_H__
#include <stdio.h>
#include "userlib_type.h"

class message_block{
public:
	message_block(int size);
	~message_block();
public:
	int message_block_write(const void *data,int len);
	int message_block_read(void *data,int len);
	unsigned char *message_block_get_rd_ptr();
	unsigned char *message_block_get_wr_ptr();
	int message_block_rd_pos_add(int len);
	int message_block_wr_pos_add(int len);
	int message_block_get_data_len();
	int message_block_get_space();
	int message_block_truncate();//Ëõ½ôÏûÏ¢¿ì
private:
	unsigned char *db_base;
	int db_size;
	int rd_pos;
	int wr_pos;
};


#endif




