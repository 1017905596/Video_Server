#include "app_test_server.h"

int main(){
	app_test_server *server = new app_test_server();

	server->app_test_server_init();
	server->app_test_server_begin_thread();
	while(1){
		sleep_millisecond(200);
	}
}