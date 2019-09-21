#include "rtmp_amf0.h"


int rtmp_amf0::rtmp_amf0_push_string(char *dst, int dst_size, const char *str_value )
{
    int len = 0;
    if( dst == NULL || str_value == NULL )
        return -1;

    len = strlen(str_value);
    if( dst_size < len + 3 || len > (int)(0xffff) )
        return -1;

    //1:type
    *dst = 0x02;
    dst++;

    //2:len
    *(unsigned short *)dst = (unsigned short)len;
	*(unsigned short *)dst = SWAP_2BYTE(*(unsigned short *)dst);
    dst += 2;

    //3:string
    memcpy( dst, str_value, len );

    return len + 3;
}

int rtmp_amf0::rtmp_amf0_push_number(char *dst, int dst_size, const double value )
{
    if( dst_size < 9 || dst == NULL )
        return -1;

    //1:type
    *dst = 0x00;
    dst++;

    //2:number
    memcpy( dst, &value, 8 );
	*(unsigned long long int *)dst = SWAP_8BYTE(*(unsigned long long int *)dst);
    
	return 9;
}

int rtmp_amf0::rtmp_amf0_push_object_header(char *dst, int dst_size )
{
    if( dst_size < 1 || dst == NULL )
        return -1;

    //1:type
    *dst = 0x03;

    return 1;
}

int rtmp_amf0::rtmp_amf0_push_object_ender(char *dst, int dst_size )
{
    if( dst_size < 3 || dst == NULL )
        return -1;

    //0x00,0x00,0x09
    dst[0] = 0x00;
    dst[1] = 0x00;
    dst[2] = 0x09;
    return 3;
}

int rtmp_amf0::rtmp_amf0_push_object_prop_name(char *dst, int dst_size, const char *name )
{
    int len = 0;
    if( dst == NULL || name == NULL )
        return -1;

    len = strlen(name);
    if( dst_size < len + 2 || len > (int)(0xffff) )
        return -1;

    //2:len
    *(unsigned short *)dst = (unsigned short)len;
	*(unsigned short *)dst = SWAP_2BYTE(*(unsigned short *)dst);
    dst+=2;

    //3:string
    memcpy( dst, name, len );

    return len + 2;
}

int rtmp_amf0::rtmp_amf0_push_bool(char *dst, int dst_size, const int value )
{
    if( dst_size < 2 || dst == NULL )
        return -1;

    //1:type
    *dst = 0x01;
    dst++;

    *dst = (value)?1:0;

    return 2;
}

int rtmp_amf0::rtmp_amf0_push_null(char *dst, int dst_size){
	if( dst_size < 1 || dst == NULL )
        return -1;

    //1:type
    *dst = 0x05;
    dst++;

    return 1;
}

int rtmp_amf0::rtmp_amf0_splite_pack(char ct, const int header_len, char *pag_buf, const int pag_len)
{
    int page_count = 0;
    int i = 0;
    char *src = NULL;
    char *dst = NULL;
    int chunk_max_size = 4096;

    if( pag_len <= header_len )
        return pag_len;

    page_count = ( ( pag_len - header_len ) - 1) / chunk_max_size;
    if( page_count == 0 ){
        return pag_len;
	}

    src = (pag_buf + header_len) + page_count * chunk_max_size;
    dst = (pag_buf + header_len) + page_count * (chunk_max_size + 1);
    for( i=0; i<page_count; i++ )
    {
        memmove( dst, src, chunk_max_size );
        *(dst-1) = ct;

        src -= chunk_max_size;
        dst -= (chunk_max_size+1);
    }
    return pag_len + page_count;
}

int rtmp_amf0::rtmp_amf0_pop_string(char *dst, int dst_size,unsigned char *src, int src_len )
{
    int str_len = 0;
    if( src_len < 3 || dst == NULL || src == NULL || dst_size < 1 )
        return -1;
    if( src[0] != 0x02 )
        return -1;

    str_len = (((int)(src[1]))<<8) + src[2];
    if( str_len + 3 > src_len || str_len > (dst_size-1))
        return -1;
    memcpy( dst, src + 3, str_len );
    dst[str_len] = 0;
    return str_len + 3;
}

int rtmp_amf0::rtmp_amf0_pop_number(double *dst,unsigned char *src, int src_len )
{
    unsigned long long int *t = (unsigned long long int *)dst;
    if( src_len < 9 || dst==NULL || src==NULL )
        return -1;
    if( src[0] != 0x00 )
        return -1;

    memcpy( dst, src+1, 8 );
    (*t) = SWAP_8BYTE((*t));
    return 9;
}

int rtmp_amf0::rtmp_amf0_pop_null(unsigned char *src, int src_len )
{
    if( src_len < 1 || src == NULL )
        return -1;
    if( src[0] != 0x05 )
        return -1;
    return 1;
}

int rtmp_amf0::rtmp_amf0_pop_object_begin( unsigned char *src, int src_len )
{
    if( src_len < 1 || src==NULL )
        return -1;
    if( src[0] != 0x03 )
        return -1;
    return 1;
}

int rtmp_amf0::rtmp_amf0_pop_object_end( unsigned char *src, int src_len )
{
    if( src_len < 3 || src==NULL )
        return -1;
    if( src[0] != 0x00 && src[1] != 0x00 && src[2] != 0x09 )
        return -1;
    return 3;
}

int rtmp_amf0::rtmp_amf0_pop_object_prop_name( char *dst, int dst_size, unsigned char *src, int src_len )
{
    int str_len = 0;
    if( src_len < 2 || dst==NULL || src==NULL || dst_size < 1 )
        return -1;

    str_len = (((size_t)(src[0]))<<8) + src[1];
    if( str_len == 0 )
    {
        return 0;
    }
    if( str_len + 2 > src_len || str_len>(dst_size-1))
        return -1;
    memcpy( dst, src+2, str_len );
    dst[str_len] = 0;
    return str_len + 2;
}
