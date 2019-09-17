#ifndef __PTHREAD_H__
#define __PTHREAD_H__

#include "Socket.h"

#include <pthread.h>
typedef void* LPVOID;
typedef void* Dthr_ret;
typedef pthread_t Dthr_id;
#define WINAPI
typedef  Dthr_ret ( WINAPI *LPTHREAD_START_ROUTINE)(LPVOID lParam);


class CPthread{
public:
	CPthread();
	virtual ~CPthread();
public:
	int Create(LPTHREAD_START_ROUTINE thr_func, LPVOID arg_list);
protected:
	Dthr_id thr_id;
};

#endif