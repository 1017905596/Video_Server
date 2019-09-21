#include "app_test_server_rtmp.h"
/***************************************************************
//构造函数，创建httpserver
//
***************************************************************/
app_test_server_rtmp::app_test_server_rtmp(destroy_rtmp_cb_t cb,event_connection_t *ec,char *message,int len){
	memset(&app_test_rtmp,0,sizeof(app_test_rtmp_data_t));
	app_test_rtmp.destroy_cb = cb;
	app_test_rtmp.rtmp = this;
	app_test_rtmp.ec = ec;
	app_test_rtmp.audio_time = 27;
	app_test_rtmp.video_time = 0;
	app_test_rtmp.state = APP_TEST_SERVER_RTMP_WAIT_C0C1;
	app_test_rtmp.client_chunk_size = 128;
	app_test_rtmp.server_chunk_size = 4096;
	app_test_rtmp.recv_data = new message_block(1024*1024);
	app_test_rtmp.send_data = new message_block(SEND_DATA_MAX);
	//拷贝数据
	app_test_rtmp.recv_data->message_block_write(message,len);
	app_test_rtmp.ec->es->event_server_set_event_cb(app_test_rtmp.ec,app_test_server_rtmp_event_cb,&app_test_rtmp);
}

/***************************************************************
//加入事件
//
***************************************************************/
int app_test_server_rtmp::app_test_server_rtmp_impl_run(){
	int ret = 0;
	ret = app_test_rtmp.ec->es->event_server_connection_add_event(app_test_rtmp.ec,EVENT_READ|EVENT_IO_PERSIST, 0);
	if(ret != 0){
		return -1;
	}
	ret = app_test_rtmp.ec->es->event_server_connection_add_event(app_test_rtmp.ec,EVENT_TIMER, 10);
	if(ret != 0){
		return -1;
	}
	return 0;
}
/***************************************************************
//清除http server 处理数据
//
***************************************************************/
void app_test_server_rtmp::app_test_server_rtmp_clean(app_test_rtmp_data_t *atud){
	if(atud != NULL){
		if(atud->ec != NULL){
			atud->ec->es->event_server_connection_remove_event(atud->ec,EVENT_EMASK);
			atud->ec->es->event_server_destroy_connection(atud->ec);
		}
		if(atud->recv_data != NULL){
			delete atud->recv_data;
			atud->recv_data = NULL;
		}
		if(atud->send_data != NULL){
			delete atud->send_data;
			atud->send_data = NULL;
		}
		if(atud->destroy_cb != NULL){
			atud->destroy_cb(this);
		}
		atud = NULL;
	}
	user_log_printf("...............out rtmp.............\n");
}
/***************************************************************
//send函数，给客户端发送数据
//
***************************************************************/
int app_test_server_rtmp::app_test_server_rtmp_check_send(app_test_rtmp_data_t *atud){
	int ret = 0;
	if(atud == NULL || atud->ec->s == NULL){
		return -1;
	}

	ret = atud->ec->s->Send(atud->send_data->message_block_get_rd_ptr(),atud->send_data->message_block_get_data_len());
	if(ret >= 0){
		atud->send_data->message_block_rd_pos_add(ret);
		if(atud->send_data->message_block_get_data_len() > 0){
			return 1;
		}else{
			return 0;
		}
	}
	return -1;
}
/***************************************************************
//设置客户端参数
//
***************************************************************/
int app_test_server_rtmp::app_test_server_rtmp_set_win_ack(char *pag_buf,int pag_len){
	unsigned int win_size = 5000000;
	int message_len;
	const char header_len = 12;
	char *data_ptr = pag_buf;

	if(pag_len < (header_len + 32)){
		return -1;
	}
	//fmt 2 1个字节
	*data_ptr = 0x02;
	data_ptr ++;
	//timestamp 3个字节
	data_ptr += 3;
	//size 
	data_ptr += 3;
	//type id 
	*data_ptr = 0x05;
	data_ptr ++;
	//stream id 4个字节
	data_ptr += 4;
	//body
	win_size = SWAP_4BYTE(win_size);
	memcpy(data_ptr, &win_size, 4 );
    data_ptr += 4;

	message_len = data_ptr - pag_buf - header_len;
	message_len = SWAP_4BYTE(message_len);
	memcpy( pag_buf+4, ((char *)(&message_len)) + 1, 3 );

	return data_ptr - pag_buf;

}
int app_test_server_rtmp::app_test_server_rtmp_set_bandwidth(char *pag_buf,int pag_len){
	unsigned int win_size = 5000000;
	int message_len;
	const char header_len = 12;
	char *data_ptr = pag_buf;

	if(pag_len < (header_len + 32)){
		return -1;
	}
	//fmt 2 1个字节
	*data_ptr = 0x02;
	data_ptr ++;
	//timestamp 3个字节
	data_ptr += 3;
	//size 
	data_ptr += 3;
	//type id 
	*data_ptr = 0x06;
	data_ptr ++;
	//stream id 4个字节
	data_ptr += 4;
	//body
	win_size = SWAP_4BYTE(win_size);
	memcpy(data_ptr, &win_size, 4 );
    data_ptr += 4;
	//Dynamic
	*data_ptr = 0x02;
	data_ptr ++;

	message_len = data_ptr - pag_buf - header_len;
	message_len = SWAP_4BYTE(message_len);
	memcpy( pag_buf+4, ((char *)(&message_len)) + 1, 3 );

	return data_ptr - pag_buf;

}

int app_test_server_rtmp::app_test_server_rtmp_set_chunk_size(char *pag_buf,int pag_len){
	unsigned int chunk_size = 4096;
	int message_len;
	const char header_len = 12;
	char *data_ptr = pag_buf;

	if(pag_len < (header_len + 32)){
		return -1;
	}
	//fmt 2 1个字节
	*data_ptr = 0x02;
	data_ptr ++;
	//timestamp 3个字节
	data_ptr += 3;
	//size 
	data_ptr += 3;
	//type id 
	*data_ptr = 0x01;
	data_ptr ++;
	//stream id 4个字节
	data_ptr += 4;
	//body
	chunk_size = SWAP_4BYTE(chunk_size);
	memcpy(data_ptr, &chunk_size, 4 );
    data_ptr += 4;

	message_len = data_ptr - pag_buf - header_len;
	message_len = SWAP_4BYTE(message_len);
	memcpy( pag_buf+4, ((char *)(&message_len)) + 1, 3 );

	return data_ptr - pag_buf;

}
int app_test_server_rtmp::app_test_server_rtmp_connect_result(char *pag_buf,int pag_len){
	int message_len = 0;
	int  amf0_ret = 0;
	const char header_len = 12;
	char *data_ptr = pag_buf;
	char *data_end = pag_buf + pag_len;

	//fmt 2 1个字节
	*data_ptr = 0x03;
	data_ptr ++;
	//timestamp 3个字节
	data_ptr += 3;
	//size 
	data_ptr += 3;
	//type id 
	*data_ptr = 0x14;
	data_ptr ++;
	//stream id 4个字节
	data_ptr += 4;

	amf0_ret = amf0_process.rtmp_amf0_push_string(data_ptr,data_end-data_ptr,"_result");
	data_ptr += amf0_ret;
	//Number 1 
	amf0_ret = amf0_process.rtmp_amf0_push_number(data_ptr,data_end-data_ptr,1);
	data_ptr += amf0_ret;
	//object header
	amf0_ret = amf0_process.rtmp_amf0_push_object_header(data_ptr,data_end-data_ptr);
	data_ptr += amf0_ret;
	//object 子目录 
	amf0_ret = amf0_process.rtmp_amf0_push_object_prop_name(data_ptr,data_end-data_ptr,"fmsVer");
	data_ptr += amf0_ret;
	//object 子目录 
	amf0_ret = amf0_process.rtmp_amf0_push_string(data_ptr,data_end-data_ptr,"FMS/3,0,1,123");
	data_ptr += amf0_ret;
	//object 子目录 
	amf0_ret = amf0_process.rtmp_amf0_push_object_prop_name(data_ptr,data_end-data_ptr,"capabilities");
	data_ptr += amf0_ret;
	//object 子目录
	amf0_ret = amf0_process.rtmp_amf0_push_number(data_ptr,data_end-data_ptr,31);
	data_ptr += amf0_ret;
	//object end
	amf0_ret = amf0_process.rtmp_amf0_push_object_ender(data_ptr,data_end-data_ptr);
	data_ptr += amf0_ret;

	//object header
	amf0_ret = amf0_process.rtmp_amf0_push_object_header(data_ptr,data_end-data_ptr);
	data_ptr += amf0_ret;
	//object 子目录 
	amf0_ret = amf0_process.rtmp_amf0_push_object_prop_name(data_ptr,data_end-data_ptr,"level");
	data_ptr += amf0_ret;
	//object 子目录 
	amf0_ret = amf0_process.rtmp_amf0_push_string(data_ptr,data_end-data_ptr,"status");
	data_ptr += amf0_ret;
	//object 子目录 
	amf0_ret = amf0_process.rtmp_amf0_push_object_prop_name(data_ptr,data_end-data_ptr,"code");
	data_ptr += amf0_ret;
	//object 子目录 
	amf0_ret = amf0_process.rtmp_amf0_push_string(data_ptr,data_end-data_ptr,"NetConnection.Connect.Success");
	data_ptr += amf0_ret;
	//object 子目录 
	amf0_ret = amf0_process.rtmp_amf0_push_object_prop_name(data_ptr,data_end-data_ptr,"description");
	data_ptr += amf0_ret;
	//object 子目录
	amf0_ret = amf0_process.rtmp_amf0_push_string(data_ptr,data_end-data_ptr,"Connection succeeded.");
	data_ptr += amf0_ret;
	//object 子目录
	amf0_ret = amf0_process.rtmp_amf0_push_object_prop_name(data_ptr,data_end-data_ptr,"objectEncoding");
	data_ptr += amf0_ret;
	//object 子目录
	amf0_ret = amf0_process.rtmp_amf0_push_number(data_ptr,data_end-data_ptr,0);
	data_ptr += amf0_ret;
	//object end
	amf0_ret = amf0_process.rtmp_amf0_push_object_ender(data_ptr,data_end-data_ptr);
	data_ptr += amf0_ret;

	//添加message len
	message_len = data_ptr - pag_buf - header_len;
	message_len = SWAP_4BYTE(message_len);
    memcpy( pag_buf + 4, ((char *)(&message_len)) + 1, 3 );

	return amf0_process.rtmp_amf0_splite_pack(0xc3,header_len,pag_buf,data_ptr - pag_buf);
}

int app_test_server_rtmp::app_test_server_rtmp_create_stream_result(char *pag_buf,int pag_len){
	int message_len = 0;
	int  amf0_ret = 0;
	const char header_len = 12;
	char *data_ptr = pag_buf;
	char *data_end = pag_buf + pag_len;

	//fmt 2 1个字节
	*data_ptr = 0x03;
	data_ptr ++;
	//timestamp 3个字节
	data_ptr += 3;
	//size 
	data_ptr += 3;
	//type id 
	*data_ptr = 0x14;
	data_ptr ++;
	//stream id 4个字节
	data_ptr += 4;

	amf0_ret = amf0_process.rtmp_amf0_push_string(data_ptr,data_end-data_ptr,"_result");
	data_ptr += amf0_ret;
	//Number 2 
	amf0_ret = amf0_process.rtmp_amf0_push_number(data_ptr,data_end-data_ptr,2);
	data_ptr += amf0_ret;
	//null
	amf0_ret = amf0_process.rtmp_amf0_push_null(data_ptr,data_end-data_ptr);
	data_ptr += amf0_ret;
	//Number 1 
	amf0_ret = amf0_process.rtmp_amf0_push_number(data_ptr,data_end-data_ptr,1);
	data_ptr += amf0_ret;

	//添加message len
	message_len = data_ptr - pag_buf - header_len;
	message_len = SWAP_4BYTE(message_len);
    memcpy( pag_buf + 4, ((char *)(&message_len)) + 1, 3 );

	return data_ptr - pag_buf;
}

int app_test_server_rtmp::app_test_server_rtmp_play_result(char *pag_buf,int pag_len){
	int message_len = 0;
	int  amf0_ret = 0;
	const char header_len = 8;
	char *data_ptr = pag_buf;
	char *data_end = pag_buf + pag_len;

	//fmt 2 1个字节
	*data_ptr = 0x43;
	data_ptr ++;
	//timestamp 3个字节
	data_ptr += 3;
	//size 
	data_ptr += 3;
	//type id 
	*data_ptr = 0x14;
	data_ptr ++;

	amf0_ret = amf0_process.rtmp_amf0_push_string(data_ptr,data_end-data_ptr,"_result");
	data_ptr += amf0_ret;
	//Number 3 
	amf0_ret = amf0_process.rtmp_amf0_push_number(data_ptr,data_end-data_ptr,3);
	data_ptr += amf0_ret;
	//null
	amf0_ret = amf0_process.rtmp_amf0_push_null(data_ptr,data_end-data_ptr);
	data_ptr += amf0_ret;

	//添加message len
	message_len = data_ptr - pag_buf - header_len;
	message_len = SWAP_4BYTE(message_len);
    memcpy( pag_buf + 4, ((char *)(&message_len)) + 1, 3 );

	return data_ptr - pag_buf;
}

int app_test_server_rtmp::app_test_server_rtmp_play_stream_begin(char *pag_buf,int pag_len){
	int message_len = 0;
	const char header_len = 12;
	char *data_ptr = pag_buf;

	//fmt 2 1个字节
	*data_ptr = 0x02;
	data_ptr ++;
	//timestamp 3个字节
	data_ptr += 3;
	//size 
	data_ptr += 3;
	//type id 
	*data_ptr = 0x04;
	data_ptr ++;
	//stream id
	data_ptr += 4;
	//body
	data_ptr += 2;

	//添加message len
	message_len = data_ptr - pag_buf - header_len;
	message_len = SWAP_4BYTE(message_len);
    memcpy( pag_buf + 4, ((char *)(&message_len)) + 1, 3 );

	return data_ptr - pag_buf;
}

int app_test_server_rtmp::app_test_server_rtmp_play_onstatus_stream_start(char *pag_buf,int pag_len){
	int message_len = 0;
	int  amf0_ret = 0;
	const char header_len = 8;
	char buf[128] = {0};
	char *data_ptr = pag_buf;
	char *data_end = pag_buf + pag_len;

	//fmt 2 1个字节
	*data_ptr = 0x44;
	data_ptr ++;
	//timestamp 3个字节
	data_ptr += 3;
	//size 
	data_ptr += 3;
	//type id 
	*data_ptr = 0x14;
	data_ptr ++;

	amf0_ret = amf0_process.rtmp_amf0_push_string(data_ptr,data_end-data_ptr,"onStatus");
	data_ptr += amf0_ret;
	//Number 0 
	amf0_ret = amf0_process.rtmp_amf0_push_number(data_ptr,data_end-data_ptr,0);
	data_ptr += amf0_ret;
	//null
	amf0_ret = amf0_process.rtmp_amf0_push_null(data_ptr,data_end-data_ptr);
	data_ptr += amf0_ret;

	//object header
	amf0_ret = amf0_process.rtmp_amf0_push_object_header(data_ptr,data_end-data_ptr);
	data_ptr += amf0_ret;
	//object 子目录 
	amf0_ret = amf0_process.rtmp_amf0_push_object_prop_name(data_ptr,data_end-data_ptr,"level");
	data_ptr += amf0_ret;
	//object 子目录 
	amf0_ret = amf0_process.rtmp_amf0_push_string(data_ptr,data_end-data_ptr,"status");
	data_ptr += amf0_ret;
	//object 子目录 
	amf0_ret = amf0_process.rtmp_amf0_push_object_prop_name(data_ptr,data_end-data_ptr,"code");
	data_ptr += amf0_ret;
	//object 子目录 
	amf0_ret = amf0_process.rtmp_amf0_push_string(data_ptr,data_end-data_ptr,"NetStream.Play.Start");
	data_ptr += amf0_ret;
	//object 子目录 
	amf0_ret = amf0_process.rtmp_amf0_push_object_prop_name(data_ptr,data_end-data_ptr,"description");
	data_ptr += amf0_ret;
	//object 子目录
	snprintf(buf,sizeof(buf),"Start %s",rtmp_app.c_str());
	amf0_ret = amf0_process.rtmp_amf0_push_string(data_ptr,data_end-data_ptr,buf);
	data_ptr += amf0_ret;
	//object end
	amf0_ret = amf0_process.rtmp_amf0_push_object_ender(data_ptr,data_end-data_ptr);
	data_ptr += amf0_ret;

	//添加message len
	message_len = data_ptr - pag_buf - header_len;
	message_len = SWAP_4BYTE(message_len);
    memcpy( pag_buf + 4, ((char *)(&message_len)) + 1, 3 );

	return data_ptr - pag_buf;
}

int app_test_server_rtmp::app_test_server_rtmp_play_rtmpsampleaccess(char *pag_buf,int pag_len){
	int message_len = 0;
	int  amf0_ret = 0;
	const char header_len = 8;
	char *data_ptr = pag_buf;
	char *data_end = pag_buf + pag_len;

	//fmt 2 1个字节
	*data_ptr = 0x44;
	data_ptr ++;
	//timestamp 3个字节
	data_ptr += 3;
	//size 
	data_ptr += 3;
	//type id 
	*data_ptr = 0x12;
	data_ptr ++;

	amf0_ret = amf0_process.rtmp_amf0_push_string(data_ptr,data_end-data_ptr,"|RtmpSampleAccess");
	data_ptr += amf0_ret;
	//BOOL true 
	amf0_ret = amf0_process.rtmp_amf0_push_bool(data_ptr,data_end-data_ptr,1);
	data_ptr += amf0_ret;
	//BOOL true 
	amf0_ret = amf0_process.rtmp_amf0_push_bool(data_ptr,data_end-data_ptr,1);
	data_ptr += amf0_ret;

	//添加message len
	message_len = data_ptr - pag_buf - header_len;
	message_len = SWAP_4BYTE(message_len);
    memcpy( pag_buf + 4, ((char *)(&message_len)) + 1, 3 );

	return data_ptr - pag_buf;
}
int app_test_server_rtmp::app_test_server_rtmp_play_onstatus_data_start(char *pag_buf,int pag_len){
	int message_len = 0;
	int  amf0_ret = 0;
	const char header_len = 8;
	char *data_ptr = pag_buf;
	char *data_end = pag_buf + pag_len;

	//fmt 2 1个字节
	*data_ptr = 0x44;
	data_ptr ++;
	//timestamp 3个字节
	data_ptr += 3;
	//size 
	data_ptr += 3;
	//type id 
	*data_ptr = 0x12;
	data_ptr ++;

	amf0_ret = amf0_process.rtmp_amf0_push_string(data_ptr,data_end-data_ptr,"onStatus");
	data_ptr += amf0_ret;
	//object header
	amf0_ret = amf0_process.rtmp_amf0_push_object_header(data_ptr,data_end-data_ptr);
	data_ptr += amf0_ret;
	//object 子目录 
	amf0_ret = amf0_process.rtmp_amf0_push_object_prop_name(data_ptr,data_end-data_ptr,"code");
	data_ptr += amf0_ret;
	//object 子目录 
	amf0_ret = amf0_process.rtmp_amf0_push_string(data_ptr,data_end-data_ptr,"NetStream.Data.Start");
	data_ptr += amf0_ret;
	//object end
	amf0_ret = amf0_process.rtmp_amf0_push_object_ender(data_ptr,data_end-data_ptr);
	data_ptr += amf0_ret;

	//添加message len
	message_len = data_ptr - pag_buf - header_len;
	message_len = SWAP_4BYTE(message_len);
    memcpy( pag_buf + 4, ((char *)(&message_len)) + 1, 3 );

	return data_ptr - pag_buf;
}

int app_test_server_rtmp::app_test_server_rtmp_onmetadata_result(app_test_rtmp_data_t * atud){
	int message_len = 0;
	int  amf0_ret = 0;
	const char header_len = 12;
	char *pag_buf = (char *)atud->send_data->message_block_get_wr_ptr();
	char *data_ptr = pag_buf;
	char *data_end = pag_buf + atud->send_data->message_block_get_space();

	//fmt 2 1个字节
	*data_ptr = 0x05;
	data_ptr ++;
	//timestamp 3个字节
	data_ptr += 3;
	//size 
	data_ptr += 3;
	//type id 
	*data_ptr = 0x12;
	data_ptr ++;

	data_ptr += 4;

	amf0_ret = amf0_process.rtmp_amf0_push_string(data_ptr,data_end-data_ptr,"onMetaData");
	data_ptr += amf0_ret;
	//object header
	amf0_ret = amf0_process.rtmp_amf0_push_object_header(data_ptr,data_end-data_ptr);
	data_ptr += amf0_ret;
	//拷贝flv中的数据
	memcpy(data_ptr,atud->flv_info,atud->flv_info_len);
	data_ptr += atud->flv_info_len;
	
	//object end
	amf0_ret = amf0_process.rtmp_amf0_push_object_ender(data_ptr,data_end-data_ptr);
	data_ptr += amf0_ret;

	//添加message len
	message_len = data_ptr - pag_buf - header_len;
	message_len = SWAP_4BYTE(message_len);
    memcpy( pag_buf + 4, ((char *)(&message_len)) + 1, 3 );

	atud->send_data->message_block_wr_pos_add(data_ptr - pag_buf);

	return app_test_server_rtmp_check_send(atud);
}

/***************************************************************
//解析客户端请求
//
***************************************************************/
int app_test_server_rtmp::app_test_server_rtmp_parser_message(app_test_rtmp_data_t *atud){
	unsigned char *message_buf = atud->recv_data->message_block_get_rd_ptr();
	unsigned int message_len = atud->recv_data->message_block_get_data_len();
	rtmp_chunk_info_t chunk_info_back = {0};
	int fmt = -1;
	int chunk_id = 0;
	int is_new_pack = 0;
	unsigned int chunk_len = 0;
	unsigned int chunk_header_len = 0;

	if(message_len == 0){
		return 1;
	}
	//区分message类型
	fmt = (message_buf[0]&0xc0) >> 6;
	chunk_id = message_buf[0]&0x3f;
	switch(fmt){
		case 0:
			chunk_header_len = 12;
			if(message_len < chunk_header_len){
				return 1;
			}
			chunk_info_back.message_size = \
							(((unsigned int)(message_buf[4]))<<16) + \
							(((unsigned int)(message_buf[5]))<<8) + \
							message_buf[6];
			chunk_info_back.message_type = message_buf[7];
			break;
		case 1:
			chunk_header_len = 8;
			if(message_len < chunk_header_len){
				return 1;
			}
			chunk_info_back.message_size = \
							(((unsigned int)(message_buf[4]))<<16) + \
							(((unsigned int)(message_buf[5]))<<8) + \
							message_buf[6];
			chunk_info_back.message_type = message_buf[7];
			break;
		case 2:
			chunk_header_len = 4;
			if(message_len < chunk_header_len){
				return 1;
			}
			break;
		case 3:
			chunk_header_len = 1;
			if(message_len < chunk_header_len){
				return 1;
			}
			break;
		default:
			return -1;
	}
	//chunk包分类
	if(atud->chunk_info[chunk_id].last_message_size == 0){
		is_new_pack = 1;
		chunk_len = MIN(chunk_info_back.message_size,atud->client_chunk_size);
	}else{
		chunk_len = MIN(atud->chunk_info[chunk_id].last_message_size,atud->client_chunk_size);
	}

	if(message_len < chunk_len + chunk_header_len){
		return 1;
	}
	//新包
	if(is_new_pack){
		atud->chunk_info[chunk_id].last_message_size = chunk_info_back.message_size;
		atud->chunk_info[chunk_id].message_size = chunk_info_back.message_size;
		atud->chunk_info[chunk_id].message_type = chunk_info_back.message_type;

		if(atud->chunk_info[chunk_id].chunk_data){
			delete atud->chunk_info[chunk_id].chunk_data;
			atud->chunk_info[chunk_id].chunk_data = NULL;
		}
		atud->chunk_info[chunk_id].chunk_data = new message_block((chunk_info_back.message_size + 64*1024)/(64*1024)*(64*1024));
	}
	
	atud->chunk_info[chunk_id].chunk_data->message_block_write(message_buf + chunk_header_len, chunk_len);
	atud->recv_data->message_block_rd_pos_add(chunk_len + chunk_header_len);
	atud->chunk_info[chunk_id].last_message_size -= chunk_len;
	if(chunk_len == atud->chunk_info[chunk_id].message_size){
		return app_test_server_rtmp_parser_process(atud,&atud->chunk_info[chunk_id]);
	}else if(atud->chunk_info[chunk_id].chunk_data->message_block_get_data_len() < (int)atud->chunk_info[chunk_id].message_size){
		return 1;
	}

	return app_test_server_rtmp_parser_process(atud,&atud->chunk_info[chunk_id]);
}

int app_test_server_rtmp::app_test_server_rtmp_parser_process(app_test_rtmp_data_t * atud,rtmp_chunk_info_t * info){
	int ret = 0;
	unsigned char *messsge_data = info->chunk_data->message_block_get_rd_ptr();
	user_log_printf("recv message_type:0x%x\n",info->message_type);
	switch(info->message_type){
		// Set Chunk Size (0x01)
		case 0x01:
			atud->client_chunk_size = (((unsigned int)(messsge_data[0]))<<24) + (((unsigned int)(messsge_data[1]))<<16)
									+ (((unsigned int)(messsge_data[2]))<<8) + messsge_data[3];
			break;
		//User Control Message (0x04)
		case 0x04:
			
			break;
		//AMF0 Command (0x14)
		case 0x14:
			ret = app_test_server_rtmp_parser_process_cmd(atud,info);
			break;
		default:
			break;
	}

	if(info->chunk_data){
		delete info->chunk_data;
		info->chunk_data = NULL;
	}
	return ret;
}

int app_test_server_rtmp::app_test_server_rtmp_parser_process_cmd(app_test_rtmp_data_t * atud,rtmp_chunk_info_t * info){
	char command_name[256]={0};
	int ret = amf0_process.rtmp_amf0_pop_string(command_name,sizeof(command_name)\
		,info->chunk_data->message_block_get_rd_ptr(),info->chunk_data->message_block_get_data_len());
	if(ret < 0){
		return -1;
	}
	user_log_printf("now command_name:%s,state:%d\n",command_name,atud->state);
	switch(atud->state){
		case APP_TEST_SERVER_RTMP_WAIT_CONNECT:
			if(strcmp(command_name,"connect") == 0){
				user_log_printf("start parser\n");
				app_test_server_rtmp_parser_connect(atud,info);
				user_log_printf("start connect result\n");
				return app_test_server_rtmp_connect(atud);
			}
			break;
		case APP_TEST_SERVER_RTMP_WAIT_CREATESTREAM:
			if(strcmp(command_name,"createStream") == 0){
				return app_test_server_rtmp_create_stream(atud);
			}
			break;
		case APP_TEST_SERVER_RTMP_WAIT_PLAY:
			if(strcmp(command_name,"play") == 0){
				app_test_server_rtmp_parser_play(atud,info);
				return app_test_server_rtmp_play(atud);
			}
			break;
		case APP_TEST_SERVER_RTMP_FILE_SEND:
			if(strcmp(command_name,"deleteStream") == 0){
				atud->state = APP_TEST_SERVER_RTMP_FILE_END;
			}
			break;
		default:
			break;
	}
	return 0;
}
/*
String 'connect'
    AMF0 type: String (0x02)
    String length: 7
    String: connect
Number 1
    AMF0 type: Number (0x00)
    Number: 1
Object (8 items)
    AMF0 type: Object (0x03)
    Property 'app' String 'live'
    Property 'flashVer' String 'LNX 9,0,124,2'
    Property 'tcUrl' String 'rtmp://192.168.174.129:1935/live'
    Property 'fpad' Boolean false
    Property 'capabilities' Number 15
    Property 'audioCodecs' Number 4071
    Property 'videoCodecs' Number 252
    Property 'videoFunction' Number 1
    End Of Object Marker
*/
int app_test_server_rtmp::app_test_server_rtmp_parser_connect(app_test_rtmp_data_t * atud,rtmp_chunk_info_t * info){
	int ret = 0;
	double trans_id = 0.0;
	char command_name[256]={0};
	char prop_name[32] = {0};
	char prop_value[256] = {0};
	unsigned char *messsge_data = info->chunk_data->message_block_get_rd_ptr();
	int data_len = info->chunk_data->message_block_get_data_len();
	
	ret = amf0_process.rtmp_amf0_pop_string(command_name,sizeof(command_name),messsge_data,data_len);
	if(ret < 0){
		return -1;
	}
	messsge_data += ret;
	data_len -= ret;

	ret = amf0_process.rtmp_amf0_pop_number(&trans_id, messsge_data, data_len );
    if( ret < 0 ){
        return -1;
	}
	messsge_data += ret;
	data_len -= ret;

	ret = amf0_process.rtmp_amf0_pop_object_begin(messsge_data, data_len);
	if( ret < 0 ){
        return -1;
	}
	messsge_data += ret;
	data_len -= ret;

	ret = amf0_process.rtmp_amf0_pop_object_prop_name(prop_name,sizeof(prop_name),messsge_data, data_len);
	if( ret < 0 ){
        return -1;
	}
	messsge_data += ret;
	data_len -= ret;

	ret = amf0_process.rtmp_amf0_pop_string(prop_value,sizeof(prop_value),messsge_data, data_len);
	if( ret < 0 ){
        return -1;
	}
	messsge_data += ret;
	data_len -= ret;
	//仅需要分析到app
	if(strcmp(prop_name,"app") == 0){
		rtmp_app.clear();
		rtmp_app.append(prop_value);
		user_log_printf("rtmp_app:%s\n",prop_value);
	}
	return 0;
}
/*
String 'play'
    AMF0 type: String (0x02)
    String length: 4
    String: play
Number 4
    AMF0 type: Number (0x00)
    Number: 4
Null
    AMF0 type: Null (0x05)
String '11.ts'
    AMF0 type: String (0x02)
    String length: 5
    String: 11.ts
Number -2000
    AMF0 type: Number (0x00)
    Number: -2000
*/
int app_test_server_rtmp::app_test_server_rtmp_parser_play(app_test_rtmp_data_t * atud,rtmp_chunk_info_t * info){
	int ret = 0;
	double trans_id = 0.0;
	char command_name[256]={0};
	char prop_value[256] = {0};
	unsigned char *messsge_data = info->chunk_data->message_block_get_rd_ptr();
	int data_len = info->chunk_data->message_block_get_data_len();
	
	ret = amf0_process.rtmp_amf0_pop_string(command_name,sizeof(command_name),messsge_data,data_len);
	if(ret < 0){
		return -1;
	}
	messsge_data += ret;
	data_len -= ret;

	ret = amf0_process.rtmp_amf0_pop_number(&trans_id, messsge_data, data_len);
    if( ret < 0 ){
        return -1;
	}
	messsge_data += ret;
	data_len -= ret;

	ret = amf0_process.rtmp_amf0_pop_null(messsge_data, data_len);
    if( ret < 0 ){
        return -1;
	}
	messsge_data += ret;
	data_len -= ret;

	ret = amf0_process.rtmp_amf0_pop_string(prop_value,sizeof(prop_value),messsge_data,data_len);
	if(ret < 0){
		return -1;
	}
	messsge_data += ret;
	data_len -= ret;

	rtmp_stream.clear();
	rtmp_stream.append(prop_value);
	user_log_printf("rtmp_stream:%s\n",prop_value);
	return 0;
}

int app_test_server_rtmp::app_test_server_rtmp_connect(app_test_rtmp_data_t * atud){
	user_log_printf("start connect\n");
	char buf[1024] = {0};
	int  ret_len = 0;
	int  data_pos = 0;

	ret_len = app_test_server_rtmp_set_win_ack(buf + data_pos,sizeof(buf) - data_pos);
	if(ret_len == -1){
		return -1;
	}
	data_pos += ret_len;
	ret_len = app_test_server_rtmp_set_bandwidth(buf + data_pos,sizeof(buf) - data_pos);
	if(ret_len == -1){
		return -1;
	}
	data_pos += ret_len;
	ret_len = app_test_server_rtmp_set_chunk_size(buf + data_pos,sizeof(buf) - data_pos);
	if(ret_len == -1){
		return -1;
	}
	data_pos += ret_len;

	ret_len = app_test_server_rtmp_connect_result(buf + data_pos,sizeof(buf) - data_pos);
	if(ret_len == -1){
		return -1;
	}
	data_pos += ret_len;

	atud->send_data->message_block_write(buf,data_pos);
	if(app_test_server_rtmp_check_send(atud) < 0){
		return -1;
	}
	atud->state = APP_TEST_SERVER_RTMP_WAIT_CREATESTREAM;
	return 0;
}
int app_test_server_rtmp::app_test_server_rtmp_create_stream(app_test_rtmp_data_t * atud){
	user_log_printf("start create stream\n");
	char buf[1024] = {0};
	int  ret_len = 0;
	
	ret_len = app_test_server_rtmp_create_stream_result(buf,sizeof(buf));
	if(ret_len == -1){
		return -1;
	}
	atud->send_data->message_block_write(buf,ret_len);

	atud->state = APP_TEST_SERVER_RTMP_WAIT_PLAY;
	return app_test_server_rtmp_check_send(atud);
}

int app_test_server_rtmp::app_test_server_rtmp_play(app_test_rtmp_data_t * atud){
	user_log_printf("start play\n");
	char buf[2048] = {0};
	int  ret_len = 0;
	int  data_pos = 0;

	ret_len = app_test_server_rtmp_play_result(buf + data_pos,sizeof(buf) - data_pos);
	if(ret_len == -1){
		return -1;
	}
	data_pos += ret_len;
	ret_len = app_test_server_rtmp_play_stream_begin(buf + data_pos,sizeof(buf) - data_pos);
	if(ret_len == -1){
		return -1;
	}
	data_pos += ret_len;
	ret_len = app_test_server_rtmp_play_onstatus_stream_start(buf + data_pos,sizeof(buf) - data_pos);
	if(ret_len == -1){
		return -1;
	}
	data_pos += ret_len;
	ret_len = app_test_server_rtmp_play_rtmpsampleaccess(buf + data_pos,sizeof(buf) - data_pos);
	if(ret_len == -1){
		return -1;
	}
	data_pos += ret_len;
	ret_len = app_test_server_rtmp_play_onstatus_data_start(buf + data_pos,sizeof(buf) - data_pos);
	if(ret_len == -1){
		return -1;
	}
	data_pos += ret_len;

	atud->send_data->message_block_write(buf,data_pos);
	if(app_test_server_rtmp_check_send(atud) < 0){
		return -1;
	}
	atud->state = APP_TEST_SERVER_RTMP_FILE_START;
	return 0;
}


/***************************************************************
//处理rtmp信令
//
***************************************************************/
int app_test_server_rtmp::app_test_server_rtmp_s0s1s2(app_test_rtmp_data_t *atud){
	const int s0s1_len = 1537;
	const int s2_len = 1536;
	char s0s1s2[s0s1_len + s2_len] = {0};
	unsigned int time_now = (unsigned int)time(NULL);
	user_log_printf("start send s0s1s2\n");

	s0s1s2[0] = 0x03;
	memcpy(s0s1s2 + 1,&time_now,sizeof(time_now));
	memcpy(s0s1s2 + s0s1_len ,atud->recv_data->message_block_get_rd_ptr() + 1,atud->recv_data->message_block_get_data_len() - 1);

	atud->send_data->message_block_write(s0s1s2,s0s1_len + s2_len);
	app_test_server_rtmp_check_send(atud);
	atud->state = APP_TEST_SERVER_RTMP_WAIT_C2;
	return 0;
}
/***************************************************************
//文件相关
//
***************************************************************/
int app_test_server_rtmp::app_test_server_rtmp_process_data(app_test_rtmp_data_t * atud){
	unsigned char tag_headr[11] = {0};
	const int rtmp_header_len = 12;
	unsigned char rtmp_headr[rtmp_header_len] = {0};
	unsigned int tag_size = 0;
	unsigned int timestamp = 0;
	unsigned int save_tag_size = 0;
	unsigned int change_size = 0;
	unsigned int send_ms = 0;

	//上一次的没有发送完成，继续发送
	if(atud->send_data->message_block_get_data_len() > 0){
		int ret = app_test_server_rtmp_check_send(atud);
		if(ret < 0){
			user_log_printf("send error\n");
			return -1;
		}else if (ret > 0){
			return 0;
		}
	}
	//清除buf消息块，偏移读指针到初始位置
	atud->send_data->message_block_truncate();
	if(atud->file_cur >= atud->file_len){
		user_log_printf("send is ok\n");
		return -1;
	}
	//限速? 多发10s数据
	send_ms = get_millisecond64() - atud->send_file_time;
	if(atud->video_time >= send_ms && (atud->video_time < send_ms + 10*1000)){
		return 0;
	}

	if(file_read(atud->file_id,tag_headr,sizeof(tag_headr)) != sizeof(tag_headr)){
		user_log_printf("read error:%lu,%lu\n",atud->file_cur,atud->file_len);
		return -1;
	}
	atud->file_cur += sizeof(tag_headr);
	tag_size = (((unsigned int)(tag_headr[1]))<<16) + \
				(((unsigned int)(tag_headr[2]))<<8) + \
				tag_headr[3];
	save_tag_size = tag_size;
//-----------------------------header------------------------------------
	//fmt+id
	if(tag_headr[0] == 0x09){
		rtmp_headr[0] = 0x04;
		timestamp = atud->video_time;
		atud->video_time += atud->video_time_add;
	}else if(tag_headr[0] == 0x08){
		rtmp_headr[0] = 0x05;
		timestamp = (unsigned int)atud->audio_time;
		atud->audio_time += atud->audio_time_add;
	}else{
		user_log_printf("tag_headr type:0x%x\n",tag_headr[0]);
		return -1;
	}
	//timestamp
	timestamp = SWAP_4BYTE(timestamp);
    memcpy( rtmp_headr + 1, ((char *)(&timestamp)) + 1, 3);
	//size
	save_tag_size = SWAP_4BYTE(save_tag_size);
	memcpy( rtmp_headr + 4, ((char *)(&save_tag_size)) + 1, 3);
	//type
	rtmp_headr[7] = tag_headr[0];
	atud->send_data->message_block_write(rtmp_headr,rtmp_header_len);
//-----------------------------body------------------------------------
	if(tag_size >= SEND_DATA_MAX){
		user_log_printf("data full :%u\n",tag_size);
	}
	//可能出现tag_size 比SEND_DATA_MAX大的情况
	while(tag_size > 0){
		unsigned int send_data = MIN(SEND_DATA_MAX,tag_size);
		if(send_data == SEND_DATA_MAX){
			send_data = send_data - send_data%atud->server_chunk_size;
		}

		tag_size -= send_data;
		
		if(file_read(atud->file_id,atud->send_data->message_block_get_wr_ptr(),send_data) != send_data){
			user_log_printf("file_read is error\n");
			return -1;
		}
		atud->file_cur += send_data;

		if(tag_headr[0] == 0x09){
			change_size = amf0_process.rtmp_amf0_splite_pack(0xc4,rtmp_header_len,
				(char *)atud->send_data->message_block_get_rd_ptr(),send_data + rtmp_header_len);
		}else if(tag_headr[0] == 0x08){
			change_size = amf0_process.rtmp_amf0_splite_pack(0xc5,rtmp_header_len,
				(char *)atud->send_data->message_block_get_rd_ptr(),send_data + rtmp_header_len);
		}
		change_size -= rtmp_header_len;
		atud->send_data->message_block_wr_pos_add(change_size);

		app_test_server_rtmp_check_send(atud);

		//清除buf消息块，偏移读指针到初始位置
		atud->send_data->message_block_truncate();
	}

	file_lseek(atud->file_id,TAG_SIZE_LEN,SEEK_CUR);
	atud->file_cur += TAG_SIZE_LEN;
	return 0;
}

int app_test_server_rtmp::app_test_server_rtmp_parser_data_info(app_test_rtmp_data_t * atud){
	unsigned char flv_header[9] = {0};
	unsigned char tag_headr[11] = {0};
	char type_info[32] = {0};
	char prop_name[32] = {0};
	unsigned char *metadata = NULL;
	unsigned char *metadata_ptr = NULL;
	unsigned int array_size = 0;
	unsigned int metadata_size = 0;
	int ret_len = 0;

	file_read(atud->file_id,flv_header,sizeof(flv_header));

	if(flv_header[0] != 'F' && flv_header[1] != 'L' && flv_header[2] != 'V'){
		user_log_printf("file error:0x%x,0x%x,0x%x\n",flv_header[0],flv_header[1],flv_header[2]);
		return -1;
	}
	file_lseek(atud->file_id,TAG_SIZE_LEN,SEEK_CUR);
	file_read(atud->file_id,tag_headr,sizeof(tag_headr));
	
	if(tag_headr[0] != 0x12){
		return -1;
	}
	metadata_size = (((unsigned int)(tag_headr[1]))<<16) + \
					(((unsigned int)(tag_headr[2]))<<8) + \
					tag_headr[3];
	if(metadata_size <= 0 || metadata_size >= 1024*1024){
		return -1;
	}
	user_log_printf("metadata_size:%d\n",metadata_size);
	metadata = new unsigned char[metadata_size];
	metadata_ptr = metadata;
	file_read(atud->file_id,metadata_ptr,metadata_size);

	ret_len = amf0_process.rtmp_amf0_pop_string(type_info,sizeof(type_info),metadata_ptr,metadata_size);
	if(ret_len < 0){
		return -1;
	}

	metadata_ptr += ret_len;
	metadata_size -= ret_len;

	if(strcmp(type_info,"onMetaData") != 0){
		user_log_printf("type_info error:%s\n",type_info);
		delete metadata;
		return -1;
	}

	if(metadata_ptr[0] != 0x08){
		delete metadata;
		return -1;
	}
	metadata_ptr ++;
	metadata_size --;
	array_size = (((unsigned int)(metadata_ptr[0]))<<16) + \
					(((unsigned int)(metadata_ptr[1]))<<16) + \
					(((unsigned int)(metadata_ptr[2]))<<8) + \
					metadata_ptr[3];
	metadata_ptr += 4;
	metadata_size -= 4;
	user_log_printf("array_size:%d\n",array_size);
	//metadata 尾部包含3个字节的结束符
	atud->flv_info_len = metadata_size - 3;
	memcpy(atud->flv_info,metadata_ptr,atud->flv_info_len);

	while(array_size -- > 0 && metadata_size > 3){
		double id;
		char prop_value[128] = {0};
		ret_len = amf0_process.rtmp_amf0_pop_object_prop_name(prop_name,sizeof(prop_name),metadata_ptr,metadata_size);
		if(ret_len < 0){
			break;
		}
		metadata_ptr += ret_len;
		metadata_size -= ret_len;

		if(strcmp(prop_name,"fps") == 0){
			ret_len = amf0_process.rtmp_amf0_pop_string(atud->flv_metadata.fps,sizeof(atud->flv_metadata.fps),metadata_ptr,metadata_size);
			if(ret_len < 0){
				break;
			}
		}else if(strcmp(prop_name,"audiosamplerate") == 0){
			ret_len = amf0_process.rtmp_amf0_pop_number(&atud->flv_metadata.audiosamplerate,metadata_ptr,metadata_size);
			if(ret_len < 0){
				break;
			}
		}else if(metadata_ptr[0] == 0x00){
			ret_len = amf0_process.rtmp_amf0_pop_number(&id,metadata_ptr,metadata_size);
			if(ret_len < 0){
				break;
			}
		}else if(metadata_ptr[0] == 0x01){
			ret_len = 2;
		}else if(metadata_ptr[0] == 0x02){
			ret_len = amf0_process.rtmp_amf0_pop_string(prop_value,sizeof(prop_value),metadata_ptr,metadata_size);
			if(ret_len < 0){
				break;
			}
		}

		metadata_ptr += ret_len;
		metadata_size -= ret_len;
	}
	int fps = atoi(atud->flv_metadata.fps);
	if(fps != 0){
		atud->video_time_add=(int)(1000/fps);
	}else{
		atud->video_time_add = 40;
	}
	if(atud->flv_metadata.audiosamplerate != 0){
		atud->audio_time_add =((1024*1000)/atud->flv_metadata.audiosamplerate);
	}else{
		atud->audio_time_add = 21.0;
	}
	user_log_printf("video_time_add:%d,audio_time_add:%f\n",atud->video_time_add,atud->audio_time_add);
	if(metadata){
		delete metadata;
		metadata = NULL;
	}
	atud->file_cur = file_lseek(atud->file_id,TAG_SIZE_LEN,SEEK_CUR);
	return 0;
}

int app_test_server_rtmp::app_test_server_rtmp_onmetadata(app_test_rtmp_data_t * atud){
	string rtmp_connect_path;
	int  ret_len = 0;
	if(rtmp_app.empty() || rtmp_stream.empty()){
		return -1;
	}
	if(strcmp(rtmp_app.c_str(),"vod") != 0){
		user_log_printf("rtmp_app:%s\n",rtmp_app.c_str());
		return -1;
	}
	rtmp_connect_path.append(content_path);
	rtmp_connect_path.append(rtmp_stream.c_str());
	rtmp_connect_path.append(".flv");

	user_log_printf("open flv:%s\n",rtmp_connect_path.c_str());
	atud->file_id = file_open(rtmp_connect_path.c_str(), O_RDONLY, FILE_PERMS_ALL,NULL);
	if(atud->file_id == INVALID_HANDLE_VALUE){
		user_log_printf("flv error\n");
		return -1;
	}
	atud->file_len = file_size(atud->file_id);
	atud->file_cur = 0;
	atud->send_file_time = get_millisecond64();
	if(app_test_server_rtmp_parser_data_info(atud) < 0){
		user_log_printf("flv parser error\n");
		return -1;
	}

	ret_len = app_test_server_rtmp_onmetadata_result(atud);
	if(ret_len == -1){
		return -1;
	}
	
	atud->state = APP_TEST_SERVER_RTMP_FILE_SEND;
	return 0;
}



/***************************************************************
//数据处理回调
//
***************************************************************/
void app_test_server_rtmp::app_test_server_rtmp_event_cb(struct event_connection_s *ec, int event_task, void *user_data ){
	int ret = 0;
	app_test_rtmp_data_t *atud = (app_test_rtmp_data_t *)user_data;

	if(event_task == EVENT_READ){
		if(ec->s != NULL){
			ret = ec->s->Receive(atud->recv_data->message_block_get_wr_ptr(),atud->recv_data->message_block_get_space());
			if(ret == 0){
				atud->state = APP_TEST_SERVER_RTMP_ERROR;
			}else if(ret < 0){
				if(get_errno() != EWOULDBLOCK && get_errno() != EINPROGRESS){
					atud->state = APP_TEST_SERVER_RTMP_ERROR;
					user_log_printf("recv error:%d\n",get_errno());
				}
			}else{
				atud->recv_data->message_block_wr_pos_add(ret);
			}
		}
	}
	if(event_task == EVENT_TIMER){
		ec->es->event_server_connection_add_event(ec,EVENT_TIMER, 50);
	}
	//rtmp c0c1协议头 1537字节，固定0x03开始
	if(atud->state == APP_TEST_SERVER_RTMP_WAIT_C0C1){
		if(atud->rtmp->app_test_server_rtmp_s0s1s2(atud) < 0){
			atud->state = APP_TEST_SERVER_RTMP_ERROR;
		}else{
			atud->recv_data->message_block_rd_pos_add(1537);
		}
	}
	//rtmp c2协议头 1536字节
	if(atud->state == APP_TEST_SERVER_RTMP_WAIT_C2){
		if(atud->recv_data->message_block_get_data_len() >= 1536){
			atud->recv_data->message_block_rd_pos_add(1536);
			atud->state = APP_TEST_SERVER_RTMP_WAIT_CONNECT;
		}
	}
	//rtmp 处理消息
	if(atud->state == APP_TEST_SERVER_RTMP_WAIT_CONNECT ||\
		atud->state == APP_TEST_SERVER_RTMP_WAIT_CREATESTREAM||\
		atud->state == APP_TEST_SERVER_RTMP_WAIT_PLAY){
		if(atud->rtmp->app_test_server_rtmp_parser_message(atud) < 0){
			atud->state = APP_TEST_SERVER_RTMP_ERROR;
		}
	}

	if(atud->state == APP_TEST_SERVER_RTMP_FILE_START){
		if(atud->rtmp->app_test_server_rtmp_onmetadata(atud) < 0){
			atud->state = APP_TEST_SERVER_RTMP_ERROR;
		}
	}

	if(atud->state == APP_TEST_SERVER_RTMP_FILE_SEND){
		if(atud->rtmp->app_test_server_rtmp_process_data(atud) < 0){
			atud->state = APP_TEST_SERVER_RTMP_ERROR;
		}
	}
	
	//请求结束
	if(atud->state == APP_TEST_SERVER_RTMP_ERROR || atud->state == APP_TEST_SERVER_RTMP_FILE_END){
		atud->rtmp->app_test_server_rtmp_clean(atud);
	}
}
