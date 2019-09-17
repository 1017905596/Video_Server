#include "message_block.h"

message_block::message_block(int size){
	db_size = size;
	if(db_size <= 0){
		db_size = 1024;
	}
	db_base = (unsigned char *)malloc(db_size);
	rd_pos = 0;
	wr_pos = 0;
}

message_block::~message_block(){
	if(db_base != NULL){
		free(db_base);
		db_base = NULL;
	}
}

int message_block::message_block_write(const void *data,int len){

	if(data == NULL || (wr_pos + len) > db_size){
		return -1;
	}
	memcpy(db_base + wr_pos, data, len);
	wr_pos += len;
	return 0;
}

int message_block::message_block_read(void *data,int len){
	if(data == NULL || rd_pos + len > wr_pos){
		return -1;
	}
	memcpy(data,db_base + rd_pos,len);
	rd_pos += len;

	return 0;
}

unsigned char *message_block::message_block_get_rd_ptr(){
	return db_base + rd_pos;
}

unsigned char *message_block::message_block_get_wr_ptr(){
	return db_base + wr_pos;
}

int message_block::message_block_rd_pos_add(int len){
	rd_pos += len;
	return rd_pos;
}

int message_block::message_block_wr_pos_add(int len){
	wr_pos += len;
	return wr_pos;
}

int message_block::message_block_get_data_len(){
	return wr_pos - rd_pos;
}

int message_block::message_block_get_space(){
	if(db_size >= wr_pos){
		return db_size - wr_pos;
	}
	return db_size;
}

int message_block::message_block_truncate(){
	memmove(db_base,db_base+rd_pos,wr_pos-rd_pos);
	wr_pos = wr_pos - rd_pos;
	rd_pos = 0;
	return 0;
}