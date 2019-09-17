#include "Pthread.h"

CPthread::CPthread(){

}

CPthread::~CPthread(){

}


int CPthread::Create(LPTHREAD_START_ROUTINE thr_func, LPVOID arg_list){
    pthread_create( &thr_id, NULL, thr_func, arg_list );
	return 0;
}