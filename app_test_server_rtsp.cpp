#include "app_test_server_rtsp.h"
/***************************************************************
//构造函数，创建httpserver
//
***************************************************************/
app_test_server_rtsp::app_test_server_rtsp(destroy_rtsp_cb_t cb,event_connection_t *ec,char *message,int len){
	memset(&app_test_rtsp,0,sizeof(app_test_rtsp_data_t));
	app_test_rtsp.destroy_cb = cb;
	app_test_rtsp.rtsp = this;
	app_test_rtsp.ec = ec;
	app_test_rtsp.packet_size = 7*188;
	app_test_rtsp.rtp_ssid = get_millisecond64();
	app_test_rtsp.play_data = data_start;
	app_test_rtsp.state = APP_TEST_SERVER_RTSP_START;
	app_test_rtsp.ts = new ts_parser();
	app_test_rtsp.parser_url = new url_parser();
	app_test_rtsp.parser = new head_parser();
	app_test_rtsp.rtsp_req = new head_parser();
	app_test_rtsp.recv_data = new message_block(1024*512);
	app_test_rtsp.send_data = new message_block(1024*512);
	for(int i = 0; i < SESSION_LEN; i++){
		app_test_rtsp.rtsp_session[i] = '0' + rand()%10;
	}
	app_test_rtsp.rtsp_session[SESSION_LEN] = '\0';
	app_test_rtsp.rtp_seq = 1;
	//拷贝数据
	app_test_rtsp.recv_data->message_block_write(message,len);
	app_test_rtsp.ec->es->event_server_set_event_cb(app_test_rtsp.ec,app_test_server_rtsp_event_cb,&app_test_rtsp);
	user_log_printf("session:%s\n",app_test_rtsp.rtsp_session);
}

/***************************************************************
//加入事件
//
***************************************************************/
int app_test_server_rtsp::app_test_server_rtsp_impl_run(){
	int ret = 0;
	ret = app_test_rtsp.ec->es->event_server_connection_add_event(app_test_rtsp.ec,EVENT_READ|EVENT_IO_PERSIST, 0);
	if(ret != 0){
		return -1;
	}
	ret = app_test_rtsp.ec->es->event_server_connection_add_event(app_test_rtsp.ec,EVENT_TIMER, 10);
	if(ret != 0){
		return -1;
	}
	return 0;
}
/***************************************************************
//清除http server 处理数据
//
***************************************************************/
void app_test_server_rtsp::app_test_server_rtsp_clean(app_test_rtsp_data_t *atud){
	if(atud != NULL){
		if(atud->ec != NULL){
			atud->ec->es->event_server_connection_remove_event(atud->ec,EVENT_EMASK);
			atud->ec->es->event_server_destroy_connection(atud->ec);
		}
		if(atud->parser != NULL){
			delete atud->parser;
			atud->parser = NULL;
		}
		if(atud->parser_url != NULL){
			delete atud->parser_url;
			atud->parser_url = NULL;
		}
		if(atud->rtsp_req != NULL){
			delete atud->rtsp_req;
			atud->rtsp_req = NULL;
		}
		if(atud->recv_data != NULL){
			delete atud->recv_data;
			atud->recv_data = NULL;
		}
		if(atud->send_data != NULL){
			delete atud->send_data;
			atud->send_data = NULL;
		}
		if(atud->ts != NULL){
			delete atud->ts;
			atud->ts = NULL;
		}

		if(atud->destroy_cb != NULL){
			atud->destroy_cb(this);
		}
		atud = NULL;
	}
	user_log_printf("...............out rtsp.............\n");
}
/***************************************************************
//http 响应头初始化
//
***************************************************************/
int app_test_server_rtsp::app_test_server_rtsp_init_req(app_test_rtsp_data_t *atud){
	const char *cseq = atud->parser->head_parser_get_value_by_name("CSeq");
	atud->rtsp_req->head_parser_set_version(atud->parser->head_parser_get_version());
	atud->rtsp_req->head_parser_set_headers("Server","test_server:1.0.0");
	atud->rtsp_req->head_parser_set_headers("CSeq",cseq);
	//atud->rtsp_cseq.clear();
	//atud->rtsp_cseq.append(cseq);
	atud->rtsp_req->head_parser_set_headers("Session",atud->rtsp_session);
	char date[128] = {0};
	http_get_date(date);
	atud->rtsp_req->head_parser_set_headers("Date",date);
	return 0;
}
/***************************************************************
//获取视频文件路径
//
***************************************************************/
int app_test_server_rtsp::app_test_server_rtsp_get_content_path(app_test_rtsp_data_t *atud,string &play_path){
	const char *play_id = atud->parser_url->url_parser_get_value_byname("id");//id作为2级目录
	const char *play_mod = atud->parser_url->url_parser_get_path(2);
	//拼接文件地址
	play_path.append(content_path);
	if(play_id != NULL){
		play_path.append(play_id);
		play_path.append("/");
	}
	play_path.append(play_mod);
	return 0;
}
/***************************************************************
//解析range请求
//
***************************************************************/
int app_test_server_rtsp::app_test_server_rtsp_parse_range(app_test_rtsp_data_t *atud,int*start_time){
	const char *range = atud->parser->head_parser_get_value_by_name("Range");
	*start_time = 0;
	if(range == NULL){
		return -1;
	}
	const char *pe = NULL;
	const char *pb = strstr( range, "npt=");
	if(pb == NULL){
		return -1;
	}
	pb += strlen("npt=");
    while(*pb == ' ')
           ++pb;
	pe = strchr(pb, '-');
	if(pe == NULL){
		pe = range + strlen(range);
	}
	*start_time = atoi(pb);
	return 0;
}
/***************************************************************
//获取start之后第一个pts
//
***************************************************************/
uint64 app_test_server_rtsp::app_test_server_rtsp_get_first_pts(app_test_rtsp_data_t *atud){
	unsigned int cache_len = 512*1024;
	uint64 pts = 0;
	char *cache_ptr = new char[cache_len];
	if(cache_ptr == NULL){
		return 0;
	}
	file_lseek(atud->file_id,atud->file_start,SEEK_SET);

	while(1){
		if(file_read(atud->file_id,cache_ptr,cache_len) != cache_len){
			break;
		}

		pts = atud->ts->ts_parser_get_first_pts(cache_ptr,cache_len);

		if(pts != 0){
			break;
		}
	}
	delete cache_ptr;
	return pts;
}

/***************************************************************
//send函数，给客户端发送数据
//
***************************************************************/
int app_test_server_rtsp::app_test_server_rtsp_check_send(app_test_rtsp_data_t *atud){
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
//服务器响应
//
***************************************************************/
int app_test_server_rtsp::app_test_server_rtsp_do_rep_cmd(app_test_rtsp_data_t *atud,const char*code,const char*code_string){
	atud->rtsp_req->head_parser_set_code(code);
	atud->rtsp_req->head_parser_set_code_string(code_string);
	//send
	int len = atud->send_data->message_block_get_space();
	atud->rtsp_req->head_parser_build_packet((char *)atud->send_data->message_block_get_wr_ptr(),&len);

	atud->send_data->message_block_wr_pos_add(len);
	user_log_printf("req cmd:%.*s\n",len,atud->send_data->message_block_get_rd_ptr());
	if(app_test_server_rtsp_check_send(atud) < 0){
		return -1;
	}
	atud->state = APP_TEST_SERVER_RTSP_START;
	return 0;
}
/***************************************************************
//服务器响应+content
//
***************************************************************/
int app_test_server_rtsp::app_test_server_rtsp_do_rep_cmd_and_content(app_test_rtsp_data_t *atud,const char*code,const char*code_string,char*buf,int buf_len){
	atud->rtsp_req->head_parser_set_code(code);
	atud->rtsp_req->head_parser_set_code_string(code_string);
	char file_len[32] = {0};
	snprintf(file_len,sizeof(file_len),"%u",buf_len);
	atud->rtsp_req->head_parser_set_headers("Content-Length",file_len);

	//send
	int len = atud->send_data->message_block_get_space();
	atud->rtsp_req->head_parser_build_packet((char *)atud->send_data->message_block_get_wr_ptr(),&len);
	atud->send_data->message_block_wr_pos_add(len);

	if(buf != NULL && buf_len != 0){
		atud->send_data->message_block_write(buf,buf_len);
	}
	user_log_printf("req cmd:%.*s\n",len + buf_len,atud->send_data->message_block_get_rd_ptr());
	if(app_test_server_rtsp_check_send(atud) < 0){
		return -1;
	}
	atud->state = APP_TEST_SERVER_RTSP_START;
	return 0;
}
/***************************************************************
//更新大ts文件的pcr缓存，用于限速
//
***************************************************************/
int app_test_server_rtsp::app_test_server_rtsp_updata_file_pcr(app_test_rtsp_pcr_t *pcr,unsigned int cur_pcr,unsigned int now_ms){

	if( pcr->send_begin_ms == 0 ){
		pcr->send_begin_ms = now_ms;
		pcr->send_cur_ms = now_ms;
		pcr->content_begin_ms = cur_pcr;
		pcr->content_cur_ms = cur_pcr;
	}else{
		pcr->send_cur_ms = now_ms;
		pcr->content_cur_ms = cur_pcr;
	}
	return 0;
}
/***************************************************************
//检测是否发的过快
//
***************************************************************/
int app_test_server_rtsp::app_test_server_rtsp_check_send_too_fast(app_test_rtsp_pcr_t *pcr,unsigned int now_ms){
	unsigned int e_range = 1000;
	unsigned int content_ms = 0;
	unsigned int send_ms = 0;

	if(pcr->content_cur_ms >= pcr->content_begin_ms){
		content_ms = pcr->content_cur_ms - pcr->content_begin_ms;
	}else{
		content_ms = pcr->content_begin_ms - pcr->content_cur_ms;
	}
	content_ms = content_ms - content_ms/5;
	send_ms = now_ms - pcr->send_begin_ms;

	if(content_ms >= send_ms && (content_ms < (send_ms + e_range))){
		return 1;
	}

	if((content_ms >= send_ms && (content_ms >= send_ms + e_range)) || 
		(send_ms >= content_ms && (send_ms >= content_ms+e_range))){
		pcr->send_begin_ms = 0;
	}

	return 0;
}
/***************************************************************
//解析transport信息，优先使用udp
//
***************************************************************/
int app_test_server_rtsp::app_test_server_rtsp_process_setup_parse_transport(app_test_rtsp_data_t *atud,char *trans_port,int trans_port_len){
	const char * trans = atud->parser->head_parser_get_value_by_name("Transport");
	int v = 0;
	unsigned int vl = 0;
	if(trans == NULL){
		return -1;
	}
	string tp(trans);

	v = tp.find(",");
	if(v == -1){
		//最后一次
		v = tp.size();
	}
	
	while(1){
		string sub_trans = tp.substr(vl,v - vl);
		//优先udp？
		if(trans_port[0] == 0 || 
			strstr(sub_trans.c_str(),"UDP") != NULL){
			strncpy(trans_port,sub_trans.c_str(),trans_port_len);
		}
		vl = v + 1;
		if(vl > tp.size()){
			break;
		}
		v = tp.find(",",vl);
		if(v == -1){
			//最后一次
			v = tp.size();
		}
	}

	return 0;
}
/***************************************************************
//解析transport用户信息
//
***************************************************************/
int app_test_server_rtsp::app_test_server_rtsp_process_setup_parse_transport_info(app_test_rtsp_data_t *atud,char *trans_port){
	string trans(trans_port);
	int v = 0;
	unsigned int vl = 0;
	
	if(trans.empty()){
		return -1;
	}
	v = trans.find(";");
	if(v == -1){
		v = trans.size();
	}
	while(1){
		string sub_trans = trans.substr(vl,v - vl);
		if(strstr(sub_trans.c_str(),"destination=") != NULL){
			strcpy(atud->udp_client_ip,sub_trans.c_str() + strlen("destination="));
		}else if(strstr(sub_trans.c_str(),"client_port=") != NULL){
			atud->udp_client_port = atoi(sub_trans.c_str() + strlen("client_port="));
		}
		vl = v + 1;
		if(vl > trans.size()){
			break;
		}
		v = trans.find(';',vl);
		if(v == -1){
			v = trans.size();
		}
	}
	return 0;
}
/***************************************************************
//udp,客户端连接响应，用户获取用户ip端口
//
***************************************************************/
void app_test_server_rtsp::app_test_server_rtsp_udp_read(Socket *s, int mask, void *arg, struct event_handler_s * ev){
	app_test_rtsp_data_t *atud = (app_test_rtsp_data_t *) arg;
	char buf[1024*64] = {0};
	unsigned short port = 0;
	char ip[32] = {0};
	if(BIT_DISABLED( mask,EVENT_READ)){
		return ;
	}

	int ret = atud->udp_s->RecvFrom(buf,sizeof(buf),&port,ip);
	if(ret > 0){
		strcpy(atud->udp_client_ip,ip);
		atud->udp_client_port = port;
		user_log_printf("recv udp:%s:%d,buf:%s\n",atud->udp_client_ip,atud->udp_client_port,buf);
		atud->wiat_udp_client = 1;
	}
}
/***************************************************************
//创建udp通道
//
***************************************************************/
int app_test_server_rtsp::app_test_server_rtsp_process_setup_create_udp(app_test_rtsp_data_t *atud,char *source_ip,unsigned short* source_port){

	if(atud->udp_ev_read != NULL){
		atud->ec->r->reactor_remove_event(atud->udp_ev_read);
		atud->ec->r->reactor_destroy_event(atud->udp_ev_read);
	}
	if(atud->udp_s != NULL){
		delete atud->udp_s;
		atud->udp_s = NULL;
	}

	atud->udp_s = new Socket();
	if(atud->udp_s->Create(AF_INET,SOCK_DGRAM) == -1){
		return -1;
	}

	atud->udp_s->set_buf_size();
	atud->udp_s->set_socket_nonblock(1);
	atud->udp_ev_read = atud->ec->r->reactor_create_event();
	if(atud->udp_ev_read == NULL){
		return -1;
	}
	atud->ec->r->reactor_set_event(atud->udp_ev_read,atud->udp_s,EVENT_READ|EVENT_IO_PERSIST,app_test_server_rtsp_udp_read,atud);
	atud->ec->r->reactor_add_event(atud->udp_ev_read);
	//端口0.自动分配
	atud->udp_s->Bind(0);

	atud->udp_s->get_sock_name(source_port,source_ip);

	return 0;
}
/***************************************************************
//拼接rtp头，ortcp头，并发送
//
***************************************************************/
int app_test_server_rtsp::app_test_server_rtsp_file_send(app_test_rtsp_data_t *atud,char *file_buf,int file_buf_len){
	char send_cache[4*1024] = {0};
	size_t send_packet_len = 0; 

	if( atud->is_tcp ){
		send_cache[0] = '$';
		send_cache[1] = 0;
		send_packet_len += 4;
	}

	if(atud->is_rtp){
		send_cache[send_packet_len]= 0x80;
		send_cache[send_packet_len+1]= 0x21;
		// 序列号 2byte
		send_cache[send_packet_len+2]= (atud->rtp_seq>>8) & 0xFF;
		send_cache[send_packet_len+3]= atud->rtp_seq & 0xFF;

		// 时间戳 4byte    
		send_cache[send_packet_len+4]= (atud->last_data_pts>>24) & 0xFF;
		send_cache[send_packet_len+5]= (atud->last_data_pts>>16) & 0xFF;
		send_cache[send_packet_len+6]= (atud->last_data_pts>>8) & 0xFF;
		send_cache[send_packet_len+7]= atud->last_data_pts & 0xFF;

		// SSID 4byte    
		send_cache[send_packet_len+8]= (atud->rtp_ssid>>24) & 0xFF;
		send_cache[send_packet_len+9]= (atud->rtp_ssid>>16) & 0xFF;
		send_cache[send_packet_len+10]= (atud->rtp_ssid>>8) & 0xFF;
		send_cache[send_packet_len+11]= atud->rtp_ssid & 0xFF;

		send_packet_len += 12;
		atud->rtp_seq++;
	}

	memcpy( send_cache + send_packet_len, file_buf, file_buf_len );
	send_packet_len += file_buf_len;

	if(atud->is_tcp){
		send_cache[2] = ( (send_packet_len-4) >> 8 ) & 0xFF;
		send_cache[3] = (send_packet_len-4) & 0xFF;

		atud->send_data->message_block_write(send_cache, send_packet_len);
		return app_test_server_rtsp_check_send(atud);
	}else{
		atud->udp_s->SendTo(send_cache, send_packet_len,atud->udp_client_ip,atud->udp_client_port);
	}    
	return 0;
}

/***************************************************************
//发送文件数据
//
***************************************************************/
int app_test_server_rtsp::app_test_server_rtsp_file_data(app_test_rtsp_data_t *atud)
{
	int ret = 0;
	int is_can_send = 1;
	unsigned int packet_len = atud->packet_size;
	char *cache_ptr = NULL;
	int thiz_send_bytes = 0;
	unsigned int content_ms = 0;
	unsigned int now_ms = get_millisecond32();

	//上一次的没有发送完成，继续发送

	if(atud->is_tcp){
		if(atud->send_data->message_block_get_data_len() > 0){
			ret = atud->rtsp->app_test_server_rtsp_check_send(atud);
			if(ret < 0){
				return -1;
			}else if (ret > 0){
				return 0;
			}
		}
		//清除buf消息块，偏移读指针到初始位置
		atud->send_data->message_block_truncate();
		//发送完成，退出
	}

	if(atud->file_cur >= atud->file_end){
		return 1;
	}

	while(atud->file_cur < atud->file_end && (thiz_send_bytes < 2048*1024/10) && is_can_send){
		packet_len = MIN(packet_len,atud->file_end - atud->file_cur);
		
		if(atud->file_cache_data_len == 0){
			if(app_test_server_rtsp_check_send_too_fast(&atud->file_pcr_info,now_ms)){
				return 0;
			}
			
			atud->file_cache_data_pos = 256;
			int len = file_read(atud->file_id, atud->file_cache_data + atud->file_cache_data_pos
				, sizeof(atud->file_cache_data) - atud->file_cache_data_pos);

			atud->file_cache_data_len += (unsigned int)len;

			content_ms = atud->ts->ts_parser_get_last_pcr_ms(atud->file_cache_data + atud->file_cache_data_pos, atud->file_cache_data_len);
			if(content_ms != 0){
				app_test_server_rtsp_updata_file_pcr(&atud->file_pcr_info,content_ms,now_ms);
				if(atud->first_pcr == 0){
					atud->first_pcr = content_ms;
				}
				atud->cur_pcr = content_ms;
			}
		}
		if(packet_len > atud->file_cache_data_len){
			packet_len = atud->file_cache_data_len;
		}
		cache_ptr = atud->file_cache_data + atud->file_cache_data_pos;
		atud->file_cache_data_len -= packet_len;
		atud->file_cache_data_pos += packet_len;

		uint64 pts = atud->ts->ts_parser_get_first_pts(cache_ptr,packet_len);
		if(pts != 0){
			atud->last_data_pts = pts;
		}
		int send_ret = app_test_server_rtsp_file_send(atud,cache_ptr,packet_len);
		if(send_ret < 0){
			is_can_send = 0;
		}
		atud->file_cur += packet_len;
		thiz_send_bytes += packet_len;
	}
	return 0;
}
/*OPTIONS rtsp://127.0.0.1:7000/play/11.ts RTSP/1.0
CSeq: 2
User-Agent: LibVLC/3.0.5 (LIVE555 Streaming Media v2016.11.28)

RTSP/1.0 200 OK
Server: test_server:1.0.0
CSeq: 2
Session: 1740948824
Date: 2019-09-06 03:57:2
Public: OPTIONS, DESCRIBE, SETUP, TEARDOWN, PLAY, PAUSE, GET_PARAMETER, SET_PARAMETER
*/
int app_test_server_rtsp::app_test_server_rtsp_process_options(app_test_rtsp_data_t *atud){
	user_log_printf("recv cmd:OPTIONS\n");
	
	atud->rtsp_req->head_parser_set_headers("Public","OPTIONS, DESCRIBE, SETUP, TEARDOWN, PLAY, PAUSE, GET_PARAMETER, SET_PARAMETER");
	return app_test_server_rtsp_do_rep_cmd(atud,"200","OK");
}
/*DESCRIBE rtsp://127.0.0.1:7000/play/11.ts RTSP/1.0
CSeq: 3
User-Agent: LibVLC/3.0.5 (LIVE555 Streaming Media v2016.11.28)
Accept: application/sdp

RTSP/1.0 200 OK
Server: test_server:1.0.0
CSeq: 3
Session: 1740948824
Date: 2019-09-06 03:57:2
Content-Type: application/sdp
x-Burst: yes
x-Retrans: no
Timeshift-Status: 0
Content-Length: 204

v=0
a=range:npt=0-10719.000
o=1567742242124522 1567742242124 IN IP4 127.0.0.1
s=RTSP Session
t=0 0
c=IN IP4 0.0.0.0
a=control:*
m=video 0 RTP/AVP 33
a=control:trackID=1
a=rtpmap:33 MP2T/90000
*/
int app_test_server_rtsp::app_test_server_rtsp_process_describe(app_test_rtsp_data_t *atud){
	user_log_printf("recv cmd:DESCRIBE\n");
	string ts_path;
	timeval_t tv;
	char content_data[16*1024] = {0};
	char session_own1[64] = {0};
	char session_own2[64] = {0};
	unsigned short server_port = 0;
	char server_ip[32] = {0};
	
	app_test_server_rtsp_get_content_path(atud,ts_path);
	if(strstr(ts_path.c_str(),"m3u8") != NULL){
		return -1;
	}
	user_log_printf("ts path:%s\n",ts_path.c_str());
	//打开文件
	atud->file_id = file_open(ts_path.c_str(), O_RDONLY, FILE_PERMS_ALL,NULL);
	if(atud->file_id == INVALID_HANDLE_VALUE){
		return -1;
	}

	atud->ts->ts_parser_file_parse_info(atud->file_id,&atud->file_info);
	atud->file_cur = atud->file_start = 0;
	atud->file_size = atud->file_info.file_len;
	atud->file_end = atud->file_size - (atud->file_size%188);
	
	atud->rtsp_req->head_parser_set_headers("Content-Type","application/sdp");
	atud->rtsp_req->head_parser_set_headers("Cache-Control","no-cache");
	//atud->rtsp_req->head_parser_set_headers("x-Retrans","no");
	//atud->rtsp_req->head_parser_set_headers("Timeshift-Status","0");

	gettimeofday( &tv );
	snprintf( session_own1,sizeof(session_own1),"%d%d", (int)(tv.sec), (int)(tv.usec));
	snprintf( session_own2,sizeof(session_own2), "%llu", get_millisecond64());
	atud->ec->s->get_sock_name(&server_port,server_ip);
	if(server_ip[0] == 0){
		snprintf(server_ip,sizeof(server_ip),"0.0.0.0");
	}

	snprintf( content_data,sizeof(content_data),
		"v=0\r\n"
		"a=range:npt=0-%d.000\r\n"
		"o=- %s %s IN IP4 %s\r\n"
		"s=RTSP Session\r\n"
		"t=0 0\r\n"
		"c=IN IP4 0.0.0.0\r\n"
		"a=control:*\r\n"
		"m=video 0 RTP/AVP 33\r\n"
		"a=control:trackID=1\r\n"
		"a=rtpmap:33 MP2T/90000\r\n"
		"\r\n",
		atud->file_info.time_len/1000,
		session_own1,
		session_own2,
		server_ip);
	
	atud->play_mode = rtsp_describe;
	return app_test_server_rtsp_do_rep_cmd_and_content(atud,"200","OK",content_data,strlen(content_data));
}
/*
SETUP rtsp://127.0.0.1:7000/play/11.ts/trackID=1 RTSP/1.0
CSeq: 4
User-Agent: LibVLC/3.0.5 (LIVE555 Streaming Media v2016.11.28)
Transport: RTP/AVP;unicast;client_port=56674-56675

RTSP/1.0 200 OK
Server: test_server:1.0.0
CSeq: 4
Session: 3675356291
Date: 2019-09-09 07:41:0
Transport: RTP/AVP;unicast;client_port=56674-56675;server_port=41659-41660;source=192.168.174.128
Session: 3675356291
Timeshift-Status: 0
*/
int app_test_server_rtsp::app_test_server_rtsp_process_setup(app_test_rtsp_data_t *atud){
	user_log_printf("recv cmd:SETUP\n");
	char transport[128] = {0};
	char transport_data[128] = {0};
	unsigned short udp_source_port = 0;
	char udp_source_ip[64] = {0};
	
	app_test_server_rtsp_process_setup_parse_transport(atud,transport,sizeof(transport));

	if(transport[0] == 0){
		return -1;
	}
	
	if(strstr(transport,"RTP") != NULL){
		atud->is_rtp = 1;
	}
	if(strstr(transport,"TCP") != NULL){
		atud->is_tcp = 1;
		snprintf(transport_data,sizeof(transport_data),"%s;ssrc=%s",transport,atud->rtsp_session);
	}else{
		app_test_server_rtsp_process_setup_parse_transport_info(atud,transport);
		app_test_server_rtsp_process_setup_create_udp(atud,udp_source_ip,&udp_source_port);

		if(strcmp(udp_source_ip,"0.0.0.0") == 0){
			atud->ec->s->get_sock_name(NULL,udp_source_ip);
		}
		snprintf(transport_data,sizeof(transport_data),"%s;server_port=%d-%d;source=%s",
			transport,
			udp_source_port, udp_source_port+1,
			udp_source_ip);
	}

	atud->rtsp_req->head_parser_set_headers("Transport",transport_data);
	atud->rtsp_req->head_parser_set_headers("Session",atud->rtsp_session);
	//atud->rtsp_req->head_parser_set_headers("Timeshift-Status","0");
	
	atud->play_mode = rtsp_setup;
	return app_test_server_rtsp_do_rep_cmd(atud,"200","OK");
}
/*PLAY rtsp://192.168.174.128:7000/play/11.ts RTSP/1.0
CSeq: 5
User-Agent: LibVLC/3.0.5 (LIVE555 Streaming Media v2016.11.28)
Session: 3675356291
Range: npt=0.000-

RTSP/1.0 200 OK
Server: test_server:1.0.0
CSeq: 5
Session: 3675356291
Date: 2019-09-09 09:33:2
Range: npt=0.00000-
*/
int app_test_server_rtsp::app_test_server_rtsp_process_play(app_test_rtsp_data_t *atud){
	user_log_printf("recv cmd:PLAY\n");
	int seek_time = 0;
	char coship_position[128] = {0};
	unsigned int ms = atud->cur_pcr - atud->first_pcr;
	const char *url = atud->parser->head_parser_get_build_url();

	atud->file_cur = atud->file_start = 0;
	atud->file_size = file_size(atud->file_id);
	atud->file_end = atud->file_size - (atud->file_size%188);

	app_test_server_rtsp_parse_range(atud,&seek_time);
	if(seek_time != 0){
		DWORD cur_pos = atud->ts->ts_parser_file_seek_by_time(atud->file_id,seek_time*1000);
		if(cur_pos != 0){
			ms = seek_time*1000;
			atud->file_start = atud->file_cur = cur_pos;
		}
		user_log_printf("check seek:%d,%lu\n",seek_time,cur_pos);
	}
	
	snprintf(coship_position,sizeof(coship_position), "npt=%d.%03d00-",ms/1000, ms%1000 );
	atud->rtsp_req->head_parser_set_headers("Range",coship_position);
	if( url != NULL ){
    	char rtp_info[1024*4] = {0};
		atud->last_data_pts = app_test_server_rtsp_get_first_pts(atud);
    	snprintf(rtp_info,sizeof(rtp_info),"url=%s/trackID=1;seq=%d;rtptime=%llu", url,atud->rtp_seq,atud->last_data_pts);
    	atud->rtsp_req->head_parser_set_headers("RTP-Info",rtp_info);
	}

	file_lseek(atud->file_id,atud->file_start,SEEK_SET);
	atud->play_data = data_play;
	atud->play_mode = rtsp_play;

	return app_test_server_rtsp_do_rep_cmd(atud,"200","OK");
}
/*PAUSE rtsp://192.168.3.45:7000/play/11.ts RTSP/1.0
CSeq: 6
User-Agent: LibVLC/3.0.5 (LIVE555 Streaming Media v2016.11.28)
Session: 1740948824

RTSP/1.0 200 OK
Server: test_server:1.0.0
CSeq: 16
Session: 3675356291
Date: Tue, 17 Sep 2019 03:17:12 GM
Range: npt=4694.50000-
*/
int app_test_server_rtsp::app_test_server_rtsp_process_pause(app_test_rtsp_data_t *atud){
	user_log_printf("recv cmd:PAUSE\n");
	char coship_position[128] = {0};
	unsigned int ms = atud->cur_pcr - atud->first_pcr;
	
	if(atud->play_mode != rtsp_play && atud->play_mode != rtsp_pause && atud->play_mode != rtsp_setup){
		return app_test_server_rtsp_do_rep_cmd(atud,"560","invalid state");
	}

	atud->wiat_udp_client = 0;
	atud->play_data = data_pause;
	atud->play_mode = rtsp_pause;
	snprintf(coship_position,sizeof(coship_position), "npt=%d.%03d00-",ms/1000, ms%1000 );
	atud->rtsp_req->head_parser_set_headers("Range",coship_position);
	
	return app_test_server_rtsp_do_rep_cmd(atud,"200","OK");
}
/*TEARDOWN rtsp://192.168.174.128:7000/play/11.ts RTSP/1.0
CSeq: 7
User-Agent: LibVLC/3.0.8 (LIVE555 Streaming Media v2016.11.28)
Session: 3675356291

RTSP/1.0 200 OK
Server: test_server:1.0.0
CSeq: 7
Session: 3675356291
Date: Mon, 16 Sep 2019 06:35:23 GM
*/
int app_test_server_rtsp::app_test_server_rtsp_process_teardown(app_test_rtsp_data_t *atud){
	user_log_printf("recv cmd:TEARDOWN\n");
	atud->play_data = data_end;
	atud->play_mode = rtsp_teardown;
	return app_test_server_rtsp_do_rep_cmd(atud,"200","OK");
}
/*GET_PARAMETER rtsp://192.168.174.128:7000/play/11.ts RTSP/1.0
CSeq: 6
User-Agent: LibVLC/3.0.8 (LIVE555 Streaming Media v2016.11.28)
Session: 3675356291

RTSP/1.0 200 OK
Server: test_server:1.0.0
CSeq: 6
Session: 3675356291
Date: Mon, 16 Sep 2019 06:35:19 GM
Range: npt=58.00200-
*/
int app_test_server_rtsp::app_test_server_rtsp_process_getparameter(app_test_rtsp_data_t *atud){
	user_log_printf("recv cmd:GET_PARAMETER\n");
	char coship_position[128] = {0};
	unsigned int ms = atud->cur_pcr - atud->first_pcr;
	snprintf(coship_position,sizeof(coship_position),"npt=%d.%03d00-",ms/1000, ms%1000);

	atud->rtsp_req->head_parser_set_headers("Range",coship_position);
	return app_test_server_rtsp_do_rep_cmd(atud,"200","OK");
}

int app_test_server_rtsp::app_test_server_rtsp_process_setparameter(app_test_rtsp_data_t *atud){
	user_log_printf("recv cmd:SET_PARAMETER\n");
	return app_test_server_rtsp_do_rep_cmd(atud,"200","OK");
}
/***************************************************************
//处理rtsp信令
//
***************************************************************/
int app_test_server_rtsp::app_test_server_rtsp_process(app_test_rtsp_data_t *atud){
	const char *method = atud->parser->head_parser_get_method();

	if(method == NULL){
		return -1;
	}

	if(strcmp(method,"OPTIONS") == 0){
		return app_test_server_rtsp_process_options(atud);
	}else if(strcmp(method,"DESCRIBE") == 0){
		return app_test_server_rtsp_process_describe(atud);
	}else if(strcmp(method,"SETUP") == 0){
		return app_test_server_rtsp_process_setup(atud);
	}else if(strcmp(method,"PLAY") == 0){
		return app_test_server_rtsp_process_play(atud);
	}else if(strcmp(method,"PAUSE") == 0){
		return app_test_server_rtsp_process_pause(atud);
	}else if(strcmp(method,"TEARDOWN") == 0){
		return app_test_server_rtsp_process_teardown(atud);
	}else if(strcmp(method,"GET_PARAMETER") == 0){
		return app_test_server_rtsp_process_getparameter(atud);
	}else if(strcmp(method,"SET_PARAMETER") == 0){
		return app_test_server_rtsp_process_setparameter(atud);
	}
	user_log_printf("CMD error:%s\n",method);
	return app_test_server_rtsp_do_rep_cmd(atud,"200","OK");
}



/***************************************************************
//数据处理回调
//
***************************************************************/
void app_test_server_rtsp::app_test_server_rtsp_event_cb(struct event_connection_s *ec, int event_task, void *user_data ){
	int ret = 0;
	app_test_rtsp_data_t *atud = (app_test_rtsp_data_t *)user_data;

	if(event_task == EVENT_READ){
		if(ec->s != NULL){
			ret = ec->s->Receive(atud->recv_data->message_block_get_wr_ptr(),atud->recv_data->message_block_get_space());
			if(ret == 0){
				atud->state = APP_TEST_SERVER_RTSP_ERROR;
			}else if(ret < 0){
				if(get_errno() != EWOULDBLOCK && get_errno() != EINPROGRESS){
					atud->state = APP_TEST_SERVER_RTSP_ERROR;
					user_log_printf("recv error:%d\n",get_errno());
				}
			}else{
				atud->recv_data->message_block_wr_pos_add(ret);
				user_log_printf("rtsp user request:%.*s\n",ret,atud->recv_data->message_block_get_rd_ptr());
			}
		}
	}
	if(event_task == EVENT_TIMER){
		if(atud->rtsp->app_test_server_rtsp_check_send(atud) < 0)
		{
			atud->state = APP_TEST_SERVER_RTSP_ERROR;
		}
		ec->es->event_server_connection_add_event(ec,EVENT_TIMER, 50);
	}
	//开始处理http请求
	if(atud->state == APP_TEST_SERVER_RTSP_START){
		atud->parser->head_parser_clear();
		atud->parser_url->url_parser_clear();
		atud->rtsp_req->head_parser_clear();
		atud->state = APP_TEST_SERVER_RTSP_REQ_HEAD;
	}
	//解析http、url
	if(atud->state == APP_TEST_SERVER_RTSP_REQ_HEAD){
		int parse_len = 0;
		if(atud->recv_data->message_block_get_data_len() != 0){
			int ret = atud->parser->parse((const char *)atud->recv_data->message_block_get_rd_ptr(),atud->recv_data->message_block_get_data_len(), &parse_len);
			if(ret == 0){
				const char *url = atud->parser->head_parser_get_build_url();
				user_log_printf("head_parser_get_build_url:%s\n",url);
				//解析url
				if(atud->parser_url->parse(url,strlen(url)) == 0){
					atud->state = APP_TEST_SERVER_RTSP_PROCESS;
				}else{
					user_log_printf("url parser error\n");
					atud->state = APP_TEST_SERVER_RTSP_ERROR;
				}
				atud->recv_data->message_block_rd_pos_add(parse_len);
			}else if(ret < 0){
				atud->state = APP_TEST_SERVER_RTSP_ERROR;
			}else{
				atud->recv_data->message_block_rd_pos_add(parse_len);
			}
		}
	}
	//处理任务
	if(atud->state == APP_TEST_SERVER_RTSP_PROCESS){
		atud->rtsp->app_test_server_rtsp_init_req(atud);
		if(atud->rtsp->app_test_server_rtsp_process(atud) != 0){
			atud->state = APP_TEST_SERVER_RTSP_ERROR;
		}
	}
	//发送数据
	if(atud->play_data == data_play && atud->state != APP_TEST_SERVER_RTSP_ERROR ){
		if(atud->wiat_udp_client){
			atud->rtsp->app_test_server_rtsp_file_data(atud);
		}
	}
	//请求结束
	if(atud->state == APP_TEST_SERVER_RTSP_ERROR || atud->play_data == data_end){
		atud->rtsp->app_test_server_rtsp_clean(atud);
	}
}