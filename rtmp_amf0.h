#ifndef __RTMP_AMF0_H_
#define __RTMP_AMF0_H_

#include "userlib_type.h"

class rtmp_amf0{
public:
	//数据切chunk
	int rtmp_amf0_splite_pack(char ct, const int header_len, char *pag_buf, const int pag_len);
	//构造数据
	int rtmp_amf0_push_string(char *dst, int size, const char* str_value);
	int rtmp_amf0_push_number(char *dst, int size, const double value);
	int rtmp_amf0_push_object_header(char *dst, int size);
	int rtmp_amf0_push_object_ender(char *dst, int size);
	int rtmp_amf0_push_object_prop_name(char *dst, int size, const char* name);
	int rtmp_amf0_push_bool(char *dst, int size,const int vlaue);
	int rtmp_amf0_push_null(char *dst, int size);
	//解析数据
	int rtmp_amf0_pop_string(char *dst, int dst_size, unsigned char *src, int src_len );
	int rtmp_amf0_pop_number(double *dst, unsigned char *src, int src_len);
	int rtmp_amf0_pop_null( unsigned char *src, int src_len );
	int rtmp_amf0_pop_object_begin( unsigned char *src, int src_len );
	int rtmp_amf0_pop_object_end( unsigned char *src, int src_len );
	int rtmp_amf0_pop_object_prop_name( char *dst, int dst_size, unsigned char *src, int src_len );
};

#endif
