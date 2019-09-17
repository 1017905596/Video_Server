#ifndef _USERLIB_TYPE_H_
#define _USERLIB_TYPE_H_
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <string>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <errno.h>
#include <sys/vfs.h>
#include <pthread.h>
#include <unistd.h>
using namespace std;

#define content_path "/home/share/"

typedef unsigned long long int uint64;

#define user_log_printf(fmt,...) printf("[%s][%d]"fmt,__FUNCTION__,__LINE__,##__VA_ARGS__)

#define MIN( a, b ) ((a)>=(b)?(b):(a))         /* 返回两者之间最小值 */
#define MAX( a, b ) ((a)>=(b)?(a):(b))         /* 返回两者之间最大值 */
#define ABS( a, b ) ((a)>=(b)?((a)-(b)):((b)-(a)))    /* 返回两者之间的绝对值 */

#define BIT_ENABLED(WORD, BIT) (((WORD) & (BIT)) != 0)
#define BIT_DISABLED(WORD, BIT) (((WORD) & (BIT)) == 0)
#define SET_BITS(WORD, BITS) (WORD |= (BITS))
#define CLR_BITS(WORD, BITS) (WORD &= ~(BITS))

#define ALIGN(value, align)  (((value) + (align - 1)) & ~(align - 1)) /* 对齐， a必须是2的指数值(内存分配那里最体现)*/
#define QUEUE_HEAP_PARENT(X) (X == 0 ? 0 : (((X) - 1) / 2))/*获取其父节点*/
#define QUEUE_HEAP_LCHILD(X) (((X)+(X))+1)/*获取其子节点*/

#define SWAP_2BYTE(L) ((((L) & 0x00FF) << 8) | (((L) & 0xFF00) >> 8))
#define SWAP_4BYTE(L) ((SWAP_2BYTE ((L) & 0xFFFF) << 16) | SWAP_2BYTE(((L) >> 16) & 0xFFFF))
#define SWAP_8BYTE(L) ((SWAP_4BYTE (((uint64)(L)) & 0xFFFFFFFF) << 32) | SWAP_4BYTE((((uint64)(L)) >> 32) & 0xFFFFFFFF))

typedef unsigned long DWORD,*LPDWORD;
typedef int HANDLE;
#define INVALID_HANDLE_VALUE -1
#define FILE_PERMS_ALL  0777
typedef void * LPSECURITY_ATTRIBUTES;

typedef pthread_t thr_id_t;
typedef pthread_t thr_handle_t;
typedef pthread_mutex_t thr_lock;

/** 
*    (TYPE *)0 表示数据段基址强制转化成TYPE类型，也就是TYPE的起始地址
*    &((TYPE *)0->MEMBER）得到MEMBER成员距离起始地址的偏移地址，也等于
*   MEMBER的地址（相对值，0~N）
*/
#define OFFSETOF(TYPE, MEMBER) ((size_t) &(((TYPE *)0)->MEMBER))   /* 取结构体成员到结构体起始地址的偏移量 */
 /** 
 *   (char *)(ptr)- OFFSETOF(TYPE,MEMBER)其中(char *)是为了将步长置为一个字节，
 *    形如: PTR = &a.member; b = YY_P_CONTAINER_OF(PTR,TYPE,member), 最后b就指向a，
 *    将a.member的绝对地址减去相对地址就是a的起始地址（内存地址）
 */
#define CONTAINER_OF(PTR, TYPE, MEMBER) (TYPE *)( (char *)(PTR) - OFFSETOF(TYPE,MEMBER) ) /* 从一个结构的成员指针找到其容器的指针 */

class CCritSec
{
public:
	CCritSec(){
		pthread_mutexattr_t attr;
		pthread_mutexattr_init(&attr);
		pthread_mutexattr_settype(&attr, 0);//PTHREAD_MUTEX_TIMED_NP
		pthread_mutex_init(&mtx, &attr);
	}
	~CCritSec(){
		pthread_mutex_destroy(&mtx);
	}
	void Enter(){

		pthread_mutex_lock(&mtx);
	}

	void Leave(){
		pthread_mutex_unlock(&mtx);
	}
	operator CCritSec*(){
		return this;
	}
protected:
	thr_lock mtx;
};

class mutex_lock{
public:
	mutex_lock(CCritSec *lock){
		m_plock = lock;
		lock->Enter();
	}
	virtual ~mutex_lock(){
		m_plock->Leave();
	}

	CCritSec *m_plock;
};

typedef struct timeval_s{
    long sec;   // 秒数
    long usec;  //1sec = 1000000usec
}timeval_t;

HANDLE file_open(const char *filename, int flag, mode_t perms, LPSECURITY_ATTRIBUTES sa );
int file_close( HANDLE fd );
DWORD file_read( HANDLE fd,  void *buf, DWORD len );
DWORD file_write( HANDLE fd,  const void *buf, DWORD len );
DWORD file_lseek (HANDLE fd, DWORD offset, int whence);
DWORD file_size( HANDLE fd );
uint64 get_millisecond64();
unsigned int  get_millisecond32();
int gettimeofday( timeval_t * t );
int sleep_millisecond( unsigned int ms );
int rtsp_get_date( char * str);
int http_get_date( char * str);
string& replace_all(string& str, const string& old_value, const string& new_value);
DWORD ato_dword_t( const char * s );
int get_errno();

#endif