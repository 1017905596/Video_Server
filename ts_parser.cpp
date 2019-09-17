#include "ts_parser.h"

ts_parser::ts_parser(){
	ts_num = 0;
}


unsigned int ts_parser::ts_parser_get_last_pcr_ms(char *p,int len){
	if( p == NULL || len < 188 )
		return 0;

	/* len数据必须为188的倍数 */
	len = len - len%188;

	/* 调到最后一个ts包 */
	p += len-188;
	while( len >= 188 )
	{
		unsigned char adaptation_field_control = ( *(p+3) )&0x30;
		if( p[0] != 0x47 ){
			p --;
			len --;
			continue;
		}
		if( adaptation_field_control == 0x30 || adaptation_field_control == 0x20 )
		{
			unsigned char adaptation_field_length = *(p+4);
			unsigned char adaptation_field_flag = *(p+5);
			if( adaptation_field_length >= 1 && adaptation_field_length != 0xff)
			{
				if( adaptation_field_flag & 0x10 )
				{
					//have time
					unsigned int pcr_base = *(unsigned int *)( p+6 );        
					pcr_base = SWAP_4BYTE( pcr_base );
					pcr_base /= 45;
					return pcr_base;
				}
			}
		}

		p -= 188;
		len -= 188;

	}
	return 0;
}

unsigned int ts_parser::ts_parser_get_first_pcr_ms(char *p,int len,DWORD*pos){
	char *ptr = p;
	if(ptr == NULL || len < 188 )
		return 0;
	/* len数据必须为188的倍数 */
	len = len - len%188;

	while( len >= 188 )
	{
		unsigned char adaptation_field_control = ( *(ptr+3) )&0x30;
		if( adaptation_field_control == 0x30 || adaptation_field_control == 0x20 )
		{
			unsigned char adaptation_field_length = *(ptr+4);
			unsigned char adaptation_field_flag = *(ptr+5);
			if( adaptation_field_length >= 1)
			{
				if( adaptation_field_flag & 0x10 )
				{
					unsigned int pcr_base = *(unsigned int *)(ptr+6 );        
					pcr_base = SWAP_4BYTE( pcr_base );
					pcr_base /= 45;
					if(pos){
						*pos = ptr - p;
					}
					return pcr_base;
				}
			}
		}

		ptr += 188;
		len -= 188;
	}
	return 0;
}
unsigned int ts_parser::ts_parser_get_first_pcr_ms_by_file(HANDLE fd,DWORD file_pos,DWORD* time_pos){
	unsigned int  cache_size = 512*1024;
	unsigned int  ts_read_size = cache_size - (cache_size%188);
	unsigned int pcr_ms = 0;
	DWORD file_cur = 0;
	DWORD in_cache_pos = 0;
	DWORD size = file_size(fd);
	char *file_cache = (char *)malloc(cache_size);

	file_cur = file_pos - file_pos%188;
    while( file_cur < size && file_cur < file_pos+4*1024*1024 )
    {
        DWORD read_size = MIN(ts_read_size, size-file_cur );
		read_size = read_size - read_size%188;

        if( file_lseek(fd, file_cur, SEEK_SET ) != file_cur)
            goto label_error;

        if( file_read(fd, file_cache, read_size ) != read_size )
            goto label_error;

        //        
        pcr_ms = ts_parser_get_first_pcr_ms( file_cache,read_size,&in_cache_pos);
        if( pcr_ms != 0 )
        {
            if( time_pos != NULL )
                *time_pos = file_cur+in_cache_pos;
            break; //跳出循环
        }
        else
        {
            file_cur += MAX( read_size, 188 );
        }
    }

    if( file_cache != NULL )
    {
        free(file_cache);
        file_cache = NULL;
    }
    return pcr_ms;
label_error:
    if( file_cache != NULL )
    {
        free(file_cache);
        file_cache = NULL;
    }
    return 0;
}
int ts_parser::ts_parser_file_parse_info(HANDLE fd,file_info_t *info){
	unsigned int  cache_size = 512*1024;
	unsigned int  ts_read_size = cache_size - (cache_size%188);
	DWORD mfile_len = 0;
	DWORD file_cur = 0;
	char *file_cache = (char *)malloc(cache_size);

	info->time_begin = info->time_end = 0;
	info->file_len = mfile_len = file_size(fd);

	file_cur = 0;

	while(file_cur < mfile_len && file_cur < 4*1024*1024){
		DWORD read_size = 0;
		if(mfile_len - file_cur >= ts_read_size){
			read_size = ts_read_size;
		}else{
			read_size = mfile_len - file_cur;
		}

		read_size = read_size - read_size%188;

		file_lseek(fd,file_cur,SEEK_SET);

		file_read(fd,file_cache,read_size);

		info->time_begin = ts_parser_get_first_pcr_ms(file_cache,read_size);

		if(info->time_begin != 0){
			break;
		}else{
			file_cur += read_size;
		}
	}

	file_cur = mfile_len - mfile_len%188;

	while(file_cur >= 0 && file_cur + 4*1024*1024 > mfile_len){
		DWORD read_size = 0;
		if(mfile_len - file_cur >= ts_read_size){
			read_size = ts_read_size;
		}else{
			read_size = mfile_len - file_cur;
			if(read_size <= 188){
				read_size = 188;
			}
		}

		file_cur -= read_size;
		file_lseek(fd,file_cur,SEEK_SET);
		file_read(fd,file_cache,read_size);
		info->time_end = ts_parser_get_last_pcr_ms(file_cache,read_size);

		if(info->time_end != 0){
			break;
		}
	}

	if(info->time_begin == 0 || info->time_end == 0){
		if(file_cache != NULL){
			free(file_cache);
			file_cache = NULL;
		}
		file_lseek(fd,0,SEEK_SET);
		return -1;
	}

	if(info->time_end >= info->time_begin){
		info->time_len = info->time_end - info->time_begin;
	}else{
		info->time_len = UINT_MAX/45 + info->time_end - info->time_begin;
	}

	if(info->time_end > 0){
		info->byte_rate = (unsigned long long)info->file_len*1000/info->time_len;
	}

	if(file_cache != NULL){
		free(file_cache);
		file_cache = NULL;
	}
	file_lseek(fd,0,SEEK_SET);
	return 0;
}


uint64 ts_parser::ts_parser_get_first_pts(char *p,int len){
	if( p == NULL || len < 188 )
		return 0;
	/* len数据必须为188的倍数 */
	len = len - len%188;

	while( len >= 188 )
	{
        unsigned char * ts = (unsigned char *)p;
        unsigned char * pes = (unsigned char *)p+4;
		unsigned char adaptation_field_control = ( *(p+3) )&0x30;
		if( adaptation_field_control != 0x30 && adaptation_field_control != 0x10 ){
			p += 188;
			len -= 188;
			continue;
		}
		/* 去除调整字段有效pes的数据 */
        if( adaptation_field_control == 0x30 ){
            unsigned char adaptation_field_length = *(ts+4);
            pes = ts+4+1+adaptation_field_length;
        }

		if( !( *pes == 0 && *(pes+1) == 0 && *(pes+2) == 1 )){
			p += 188;
			len -= 188;
            continue;
        }
		unsigned int PTS_DTS_flags = pes[7]>>6 & 0x03;
		if(PTS_DTS_flags == 0x02 || PTS_DTS_flags == 0x03){
			unsigned char *data = pes + 9;
			return (((((uint64)data[0]) >>1 & 0x07) <<30)
			          | (((uint64)data[1]) << 22)
			         | (((uint64)data[2])>>1 <<15)
			         | (((uint64)data[3]) <<7)
			         | (((uint64)data[4]) >>1));
		}

		p += 188;
		len -= 188;
	}
	return 0;
}


DWORD ts_parser::ts_parser_file_seek_by_time(HANDLE fd, unsigned int seek_ms)
{
    int test_count = 6;
    file_info_t ts_xfi;
    DWORD file_pos = 0;
    DWORD time_file_pos = 0;
    unsigned int cur_ms = 0;

    /* 获取该文件的信息 */
    if( ts_parser_file_parse_info(fd, &ts_xfi) != 0 )
        return 0;

    /* seek到文件最后 */
    if( seek_ms >= ts_xfi.time_len )
        return (ts_xfi.file_len - (ts_xfi.file_len%188) );

    /* < 1s 无效 */
    if( seek_ms < 1000 )
        return 0;

    if( ts_xfi.file_len == 0 )
        return 0;
    file_pos = ( (((DWORD)seek_ms)-900)/1000 )*ts_xfi.byte_rate;
    file_pos -= file_pos%188;

    while( test_count-- > 0 )
    {
        file_pos -= file_pos%188;

        /* 从大概时间位置依次获取pcr */
        cur_ms = ts_parser_get_first_pcr_ms_by_file(fd, file_pos, &time_file_pos);
        //不能读取时间点,则认为已经到正确位置了.
        if( cur_ms == 0 )
            break;
        //cur_ms -= ts_xfi.time_begin;
        if( cur_ms >= ts_xfi.time_begin )
            cur_ms -= ts_xfi.time_begin;
        else
            cur_ms = UINT_MAX/45 + cur_ms - ts_xfi.time_begin;


        //误差小于1秒,认为已经到达最佳计算点
        if(ABS( seek_ms , cur_ms ) < 150)
        {
            file_pos = time_file_pos;
            break;
        }
        else if( cur_ms < seek_ms )
        {
            file_pos = time_file_pos+ (((DWORD)seek_ms)-cur_ms)*(ts_xfi.byte_rate/1000);
            if( file_pos > ts_xfi.file_len )
                file_pos = ts_xfi.file_len;            
        }
        else
        {
            if( time_file_pos > (((DWORD)cur_ms) - seek_ms)*(ts_xfi.byte_rate/1000) )
                file_pos = time_file_pos - (((DWORD)cur_ms) - seek_ms)*(ts_xfi.byte_rate/1000);
            else
                file_pos = 0;
            
        }
    }
    file_pos -= file_pos%188;    
    return file_pos;
}