#include "userlib_type.h"

HANDLE file_open(const char *filename, int flag, mode_t perms, LPSECURITY_ATTRIBUTES sa )
{
	return open( filename, flag, perms );
}

int file_close( HANDLE fd )
{
	return close( fd );
}

DWORD file_read( HANDLE fd,  void *buf, DWORD len )
{
	return read( fd, buf, len );
}

DWORD file_write( HANDLE fd,  const void *buf, DWORD len )
{
	return write( fd, buf, len );
}
DWORD file_lseek (HANDLE fd, DWORD offset, int whence)
{     
	return lseek( fd, offset, whence );
}


DWORD file_size( HANDLE fd )
{
	DWORD old_pos = file_lseek( fd, 0, SEEK_CUR );
	DWORD len = file_lseek( fd, 0, SEEK_END );
	if( len == 0 ){
		return 0;
	}
	len = file_lseek( fd, 0, SEEK_CUR ); 
	file_lseek( fd, old_pos, SEEK_SET );
	return len;
}


int gettimeofday( timeval_t * t )
{
    if( t != NULL )
    {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        t->sec = ts.tv_sec;
        t->usec = ts.tv_nsec/1000;
        return 0;
    }
	return -1;
}
uint64 get_millisecond64()
{
    uint64 ms = 0;
    timeval_t tv;
    gettimeofday( &tv );
    ms = tv.sec;
    ms *= 1000;
    ms += tv.usec/1000;
    return ms;
}

unsigned int  get_millisecond32()
{ 
	static int is_inited = 0;
	static timeval_t tv_begin;
	timeval_t tv_end;
	if( !is_inited )
	{
		is_inited = 1;
		gettimeofday( &tv_begin );
	}
	gettimeofday( &tv_end );
	tv_end.sec -= tv_begin.sec;
	tv_end.usec -= tv_begin.usec;
	return 9527+( tv_end.sec*1000+tv_end.usec/1000 );
}

int sleep_millisecond( unsigned int ms )
{
	struct timespec req,rem;
	req.tv_sec = ms/1000;
	req.tv_nsec = 1000000*(ms%1000);
	rem = req;
	return nanosleep( &req, &rem );  /* 这个函数功能是暂停某个线程直到你规定的时间后恢复 */
}

int http_get_date( char * str)
{
	static const char *day_of_week_name[] =
	{
		("Sun"),
		("Mon"),
		("Tue"),
		("Wed"),
		("Thu"),
		("Fri"),
		("Sat")
	};

	static const char *month_name[] =
	{
		("Jan"),
		("Feb"),
		("Mar"),
		("Apr"),
		("May"),
		("Jun"),
		("Jul"),
		("Aug"),
		("Sep"),
		("Oct"),
		("Nov"),
		("Dec")
	};

	if( str == NULL)
		return -1;
	*str = '\0';
	time_t t = time( NULL );

	struct tm tn = {0};
	gmtime_r( &t, &tn);
	snprintf( str,strlen(str), "%3s, %02d %3s %04d %02d:%02d:%02d GMT", day_of_week_name[tn.tm_wday], tn.tm_mday ,month_name[tn.tm_mon],
		tn.tm_year+1900, tn.tm_hour, tn.tm_min, tn.tm_sec );
	return 0;
}

int rtsp_get_date( char * str)
{
	if( str == NULL)
		return -1;
	*str = '\0';
	time_t t = time( NULL );

	struct tm tn = {0};
	gmtime_r( &t, &tn);
	snprintf( str,strlen(str), "%04d-%02d-%02d %02d:%02d:%02d",tn.tm_year+1900, tn.tm_mon+1,tn.tm_mday, tn.tm_hour, tn.tm_min, tn.tm_sec );

	return 0;
}

string& replace_all(string& str, const string& old_value, const string& new_value)
{
	string::size_type pos=0;
	while((pos=str.find(old_value))!= string::npos)
	{
		str=str.replace(str.find(old_value),old_value.length(),new_value);
	}
	return str;
}

DWORD ato_dword_t( const char * s )
{
	DWORD v = 0;
	if( s != NULL )
	{
		while( *s == ' ' )        /* 跳过串头部连续空格 */
			++s;

		while( *s != '\0' )
		{
			if( *s >= '0' && *s <= '9' )
			{
				v = v*10+(*s-'0');
				++s;
			}
			else        /* 遇到第一个非数字则结束转换 */
			{
				return v;
			}

		}
	}
	return v;
}

int get_errno()
{
    return errno;
}