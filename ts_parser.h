#ifndef _TS_PARSER_H_
#define _TS_PARSER_H_

#include <stdio.h>
#include "userlib_type.h"


typedef struct file_info_s{
	unsigned int time_begin; //文件内容的第一个数据包时间,MS
	unsigned int time_end;   //文件内容的最后一个数据包时间,MS
	unsigned int time_len;   //文件总时长,MS
	DWORD file_len;    //文件总长
	unsigned int byte_rate;    //码率
}file_info_t;

class ts_parser{
public:
	ts_parser();
public:
	
	DWORD ts_parser_file_seek_by_time(HANDLE fd, unsigned int seek_ms);
	uint64 ts_parser_get_first_pts(char *p,int len);
	unsigned int ts_parser_get_last_pcr_ms(char *p,int len);
	unsigned int ts_parser_get_first_pcr_ms(char *p,int len,DWORD*pos=NULL);
	unsigned int ts_parser_get_first_pcr_ms_by_file(HANDLE fd,DWORD file_pos,DWORD* time_pos);
	int ts_parser_file_parse_info(HANDLE fd,file_info_t *info);
public:
	unsigned int ts_num;
};



#endif
