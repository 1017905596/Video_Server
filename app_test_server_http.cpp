
#include "app_test_server_http.h"
/***************************************************************
//���캯��������httpserver
//
***************************************************************/
app_test_server_http::app_test_server_http(destroy_cb_t cb,event_connection_t *ec,char *message,int len){
	memset(&app_test_http,0,sizeof(app_test_http_data_t));
	app_test_http.destroy_cb = cb;
	app_test_http.http = this;
	app_test_http.ec = ec;
	app_test_http.state = APP_TEST_SERVER_HTTP_START;
	app_test_http.ts = new ts_parser();
	app_test_http.parser_url = new url_parser();
	app_test_http.parser = new head_parser();
	app_test_http.http_req = new head_parser();
	app_test_http.recv_data = new message_block(1024*512);
	app_test_http.send_data = new message_block(1024*512);
	//��������
	app_test_http.recv_data->message_block_write(message,len);
	app_test_http.ec->es->event_server_set_event_cb(app_test_http.ec,app_test_server_http_event_cb,&app_test_http);
}
/***************************************************************
//�����¼�
//
***************************************************************/
int app_test_server_http::app_test_server_http_impl_run(){
	int ret = 0;
	ret = app_test_http.ec->es->event_server_connection_add_event(app_test_http.ec,EVENT_READ|EVENT_IO_PERSIST, 0);
	if(ret != 0){
		return -1;
	}
	ret = app_test_http.ec->es->event_server_connection_add_event(app_test_http.ec,EVENT_TIMER, 10);
	if(ret != 0){
		return -1;
	}
	return 0;
}
/***************************************************************
//���http server ��������
//
***************************************************************/
void app_test_server_http::app_test_server_http_clean(app_test_http_data_t *atud){
	if(atud != NULL){
		if(atud->ec != NULL){
			atud->ec->es->event_server_connection_remove_event(atud->ec,EVENT_EMASK);
			atud->ec->es->event_server_destroy_connection(atud->ec);
		}
		if(atud->file_id != INVALID_HANDLE_VALUE){
			file_close(atud->file_id);
			atud->file_id = INVALID_HANDLE_VALUE;
		}
		if(atud->parser != NULL){
			delete atud->parser;
			atud->parser = NULL;
		}
		if(atud->parser_url != NULL){
			delete atud->parser_url;
			atud->parser_url = NULL;
		}
		if(atud->http_req != NULL){
			delete atud->http_req;
			atud->http_req = NULL;
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
	user_log_printf("out http.....................................\n");
}
/***************************************************************
//http ��Ӧͷ��ʼ��
//
***************************************************************/
int app_test_server_http::app_test_server_http_init_req(app_test_http_data_t *atud){
	atud->http_req->head_parser_set_version(atud->parser->head_parser_get_version());
	atud->http_req->head_parser_set_headers("Server","test_server:1.0.0");
	atud->http_req->head_parser_set_headers("Connection","close");
	atud->http_req->head_parser_set_headers("Cache-Control","no-cache");
	atud->http_req->head_parser_set_headers("Access-Control-Allow-Origin","*");
	char date[128] = { 0 };
	http_get_date(date);
	atud->http_req->head_parser_set_headers("Date",date);
	return 0;
}
/***************************************************************
//send���������ͻ��˷�������
//
***************************************************************/
int app_test_server_http::app_test_server_http_check_send(app_test_http_data_t *atud){
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
//���´�ts�ļ���pcr���棬��������
//
***************************************************************/
int app_test_server_http::app_test_server_http_updata_file_pcr(app_test_http_pcr_t *pcr,unsigned int cur_pcr,unsigned int now_ms){
	
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
//����Ƿ񷢵Ĺ���
//
***************************************************************/
int app_test_server_http::app_test_server_http_check_send_too_fast(app_test_http_pcr_t *pcr,unsigned int now_ms){
	unsigned int e_range = 15*1000;
	unsigned int content_ms = 0;
	unsigned int send_ms = 0;

	if(pcr->content_cur_ms >= pcr->content_begin_ms){
		content_ms = pcr->content_cur_ms - pcr->content_begin_ms;
	}else{
		content_ms = pcr->content_begin_ms - pcr->content_cur_ms;
	}

	send_ms = now_ms - pcr->send_begin_ms;

	if(content_ms >= send_ms && (content_ms < send_ms + e_range)){
		return 1;
	}

	if((content_ms >= send_ms && (content_ms >= send_ms + e_range)) ||
		(send_ms >= content_ms && (send_ms >= content_ms+e_range))){
		pcr->send_begin_ms = 0;
	}
	return 0;
}
/***************************************************************
//��ȡ�û�range����
//
***************************************************************/
int app_test_server_http::app_test_server_http_set_file_pos_by_range(app_test_http_data_t *atud){
	switch(atud->range.type){
		case http_range_type_start:
			atud->file_start = atud->range.start;
			break;
		case http_range_type_start_end:
			atud->file_start = atud->range.start;
			atud->file_end = atud->range.end;
			break;
		default:
			return -1;
	}
	return 0;
}
/***************************************************************
//��ȡ�Զ���file_start or file_len
//
***************************************************************/
int app_test_server_http::app_test_server_http_set_file_pos_by_file(app_test_http_data_t *atud){
	const char *file_start = atud->parser_url->url_parser_get_value_byname("file_start");
	const char *file_end = atud->parser_url->url_parser_get_value_byname("file_end");

	if(file_start == NULL || file_end == NULL){
		return -1;
	}

	atud->file_start = ato_dword_t(file_start);
	atud->file_end = ato_dword_t(file_end);
	return 0;
}
/***************************************************************
//�����ļ�����
//
***************************************************************/
int app_test_server_http::app_test_server_http_send_file(app_test_http_data_t *atud)
{
	int ret = 0;
	DWORD len = 0;
	unsigned int pcr_len_ms = 0;
	unsigned int now_ms = get_millisecond32();

	//��һ�ε�û�з�����ɣ���������
	if(atud->send_data->message_block_get_data_len() > 0){
		ret = app_test_server_http_check_send(atud);
		if(ret < 0){
			return -1;
		}else if (ret > 0){
			return 0;
		}
	}
	//���buf��Ϣ�飬ƫ�ƶ�ָ�뵽��ʼλ��
	atud->send_data->message_block_truncate();
	//������ɣ��˳�
	if(atud->file_cur >= atud->file_end){
		return 1;
	}
	//����̫�����෢15s����
	if(!atud->is_m3u8_file && 
		app_test_server_http_check_send_too_fast(&atud->file_pcr_info,now_ms)){
		return 0;
	}

	//�ϴη�����ɣ�������ȡ����
	if(atud->file_cur < atud->file_end){
		//�ж���Ϣ��ʣ���С
		if(atud->file_end - atud->file_cur > (DWORD)atud->send_data->message_block_get_space()){
			len = atud->send_data->message_block_get_space();
		}else{
			len = atud->file_end - atud->file_cur;
		}
		//ts�ļ����Ͷ���
		if(!atud->is_m3u8_file){
			len -= len%188;
		}

		//��ȡ�ļ����ݵ�buf
		if(file_read(atud->file_id, atud->send_data->message_block_get_wr_ptr(), len) != len){
			return -1;
		}

		if(!atud->is_m3u8_file){
			unsigned int content_ms = atud->ts->ts_parser_get_last_pcr_ms((char *)atud->send_data->message_block_get_rd_ptr(),len);
			if(content_ms != 0){
				app_test_server_http_updata_file_pcr(&atud->file_pcr_info,content_ms,now_ms);
				if(atud->first_pcr == 0 || atud->first_pcr > content_ms){
					atud->first_pcr = content_ms;
				}
				atud->cur_pcr = content_ms;
			}
			atud->send_data_byte += len;

			if(atud->cur_pcr >= atud->first_pcr){
				pcr_len_ms = atud->cur_pcr - atud->first_pcr;
			}else{
				pcr_len_ms = UINT_MAX/45 + atud->cur_pcr - atud->first_pcr;
			}
			if(atud->send_data_byte > 1024*1024 && atud->byte_rate == 0 && pcr_len_ms != 0){
				atud->byte_rate = (uint64)atud->send_data_byte *1000/pcr_len_ms;
				printf("byte_rate:%u,time:%u\n",atud->byte_rate,pcr_len_ms);
			}
		}
		//дָ��ƫ��
		atud->send_data->message_block_wr_pos_add(len);
		atud->file_cur += len;
	}
	//�����ļ�����
	if(app_test_server_http_check_send(atud) < 0){
		return -1;
	}

	return 0;
}
/***************************************************************
//��ȡ�ļ�·��
//
***************************************************************/
int app_test_server_http::app_test_server_http_get_content_path(app_test_http_data_t *atud,string &play_path){
	const char *play_id = atud->parser_url->url_parser_get_value_byname("id");//id��Ϊ2��Ŀ¼
	const char *play_mod = atud->parser_url->url_parser_get_path(2);
	//ƴ���ļ���ַ
	play_path.append(content_path);
	if(play_id != NULL){
		play_path.append(play_id);
		play_path.append("/");
	}
	play_path.append(play_mod);
	return 0;
}
/***************************************************************
//����ts�ļ�
//
***************************************************************/
int app_test_server_http::app_test_server_http_play_ts(app_test_http_data_t *atud){
	string play_path;
	const char *hls_ts = atud->parser_url->url_parser_get_value_byname("file_hls_ts");

	//��ȡ�ļ�·��
	app_test_server_http_get_content_path(atud,play_path);
	user_log_printf("ts path:%s\n",play_path.c_str());
	//���ļ�
	atud->file_id = file_open(play_path.c_str(), O_RDONLY, FILE_PERMS_ALL,NULL);
	if(atud->file_id == INVALID_HANDLE_VALUE){
		return -1;
	}
	
	atud->file_cur = atud->file_start = 0;
	atud->file_size = atud->file_end = file_size(atud->file_id);
	//���ж��Ƿ���file start or file end
	if(atud->http->app_test_server_http_set_file_pos_by_file(atud) != 0){
		//����range
		atud->http->app_test_server_http_set_file_pos_by_range(atud);
	}
	user_log_printf("rang:%lu - %lu\n",atud->file_start,atud->file_end);

	if(atud->file_end > atud->file_size || atud->file_start >= atud->file_size){
		printf("file size:%lu\n",atud->file_size);
		return -1;
	}
	//seek��ָ��λ��
	atud->file_cur = atud->file_start;
	file_lseek(atud->file_id,atud->file_start,SEEK_SET);
	//ƴ����Ӧͷ
	char file_len[32] = {0};
	snprintf(file_len,sizeof(file_len),"%lu",(atud->file_end - atud->file_start));
	atud->http_req->head_parser_set_headers("Content-Length",file_len);
	atud->http_req->head_parser_set_headers("Accept-Ranges","bytes");
	atud->http_req->head_parser_set_headers("Content-Type","application/octet-stream");
	if(hls_ts == NULL && atud->range.type != http_range_type_null){
		char range_buf[64];
		atud->http_req->head_parser_set_code("206");
		atud->http_req->head_parser_set_code_string("Partial Content");
		snprintf(range_buf,sizeof(range_buf),"bytes %lu-%lu/%lu",atud->file_start,atud->file_end,atud->file_size);
		atud->http_req->head_parser_set_headers("Content-Range",range_buf);
	}else{
		atud->http_req->head_parser_set_code("200");
		atud->http_req->head_parser_set_code_string("OK");
	}
	//send
	int len = atud->send_data->message_block_get_space();
	atud->http_req->head_parser_build_packet((char *)atud->send_data->message_block_get_wr_ptr(),&len);
	atud->send_data->message_block_wr_pos_add(len);
	user_log_printf("start play ts!!!:%.*s\n",len,atud->send_data->message_block_get_rd_ptr());
	if(app_test_server_http_check_send(atud) < 0){
		return -1;
	}
	atud->state = APP_TEST_SERVER_HTTP_SEND_FILE;
	return 0;
}
/***************************************************************
//����m3u8�ļ�
//
***************************************************************/
int app_test_server_http::app_test_server_http_play_m3u8(app_test_http_data_t *atud){
	string play_path;

	app_test_server_http_get_content_path(atud,play_path);
	user_log_printf("m3u8 path:%s\n",play_path.c_str());
	//���Դ�hls�ļ�
	atud->file_id = file_open(play_path.c_str(), O_RDONLY, FILE_PERMS_ALL,NULL);
	if(atud->file_id != INVALID_HANDLE_VALUE){
		return app_test_server_http_play_m3u8_hls_file(atud);
	}
	
	//�ļ������ڣ��ж�.ts�Ƿ���ڣ�
	string ts_path = replace_all(play_path,(string)"m3u8",(string)"ts");
	printf("ts_path:%s\n",ts_path.c_str());
	//���Դ�ts�ļ�
	atud->file_id = file_open(ts_path.c_str(), O_RDONLY, FILE_PERMS_ALL,NULL);
	if(atud->file_id == INVALID_HANDLE_VALUE){
		return -1;
	}else{
		return app_test_server_http_play_m3u8_ts_file(atud,ts_path);
	}
	return -1;
}
/***************************************************************
//m3u8���ţ�hls�ļ�����
//
***************************************************************/
int app_test_server_http::app_test_server_http_play_m3u8_hls_file(app_test_http_data_t *atud){
	atud->file_cur  = 0;
	atud->file_end = file_size(atud->file_id);
	//ƴ����Ӧͷ
	atud->http_req->head_parser_set_code("200");
	atud->http_req->head_parser_set_code_string("OK");
	atud->http_req->head_parser_set_headers("Content-Type","application/vnd.apple.mpegurl");
	char file_len[32] = {0};
	snprintf(file_len,sizeof(file_len),"%lu",(atud->file_end - atud->file_cur));
	atud->http_req->head_parser_set_headers("Content-Length",file_len);
	//send
	int len = atud->send_data->message_block_get_space();
	atud->http_req->head_parser_build_packet((char *)atud->send_data->message_block_get_wr_ptr(),&len);
	atud->send_data->message_block_wr_pos_add(len);
	user_log_printf("start play m3u8!!!:%.*s\n",len,atud->send_data->message_block_get_rd_ptr());
	if(app_test_server_http_check_send(atud) < 0){
		return -1;
	}
	atud->is_m3u8_file = 1;
	atud->state = APP_TEST_SERVER_HTTP_SEND_FILE;
	return 0;
}
/***************************************************************
//m3u8���ţ�ts�ļ�����
//
***************************************************************/
int app_test_server_http::app_test_server_http_play_m3u8_ts_file(app_test_http_data_t *atud,string &path){
	int ret = 0;
	const char * now_url =  atud->parser->head_parser_get_build_url();
	string now_url_string(now_url);
	string ts_url = replace_all(now_url_string,(string)"m3u8",(string)"ts");
	string m3u8_path = replace_all(path,(string)"ts",(string)"m3u8");

	//����hls�����ļ�
	ret = app_test_server_http_play_m3u8_build_hls_file(atud,ts_url,m3u8_path);
	if(ret != 0){
		return -1;
	}
	//����hls�����ļ�
	atud->file_id = file_open(m3u8_path.c_str(), O_RDONLY, FILE_PERMS_ALL,NULL);
	if(atud->file_id == INVALID_HANDLE_VALUE){
		return -1;
	}
	//����
	return app_test_server_http_play_m3u8_hls_file(atud);
}
/***************************************************************
//m3u8���ţ�ts�ļ����ڣ�����hls�����ļ�
//
***************************************************************/
int app_test_server_http::app_test_server_http_play_m3u8_build_hls_file(app_test_http_data_t *atud,string &ts_path,string &m3u8_path){
	file_info_t info;
	int splice_count = 0;
	DWORD splice_size = 0;
	DWORD cache_size = 512*1024;
	DWORD ts_read_size = 0;
	char *file_cache = NULL;
	string m3u8_info;
	//�����ļ�
	atud->ts->ts_parser_file_parse_info(atud->file_id,&info);
	printf("file_len:%lu,time_len:%u\n",info.file_len,info.time_len);
	printf("pcr:%u-%u\n",info.time_begin,info.time_end);
	//�����Ƭ��С
	splice_count = info.time_len/M3U8_SPLICE_TIME_MS + 1;
	splice_size = (info.file_len/splice_count + 188)/188*188;
	
	if(cache_size > splice_size){
		ts_read_size = splice_size;
	}else{
		ts_read_size = cache_size - cache_size%188;
	}
	//�����ڴ����ڶ�ȡts����
	file_cache = (char *)malloc(cache_size);
	if(file_cache == NULL){
		return -1;
	}
	//ģ����Ƭ
	char temp_buf[1024] = {0};
	unsigned int pcr_ms = 0;
	unsigned int begin_s = (info.time_begin + 100)/1000;//��ֹ����4999���룬Ҳ������Ϊ4������
	unsigned int end_s = 0;
	unsigned int max_time_len = 0;
	DWORD begin_pos = 0;
	DWORD end_pos = splice_size;
	int is_pcr_reset = 0;//�ع�
	int is_pcr_jump = 0;//����
	//ѭ��ÿ����Ƭ��С����ȡpcr�Լ�pos
	while(begin_pos < info.file_len){
		if(end_pos + splice_size/2 > info.file_len){//���һ����Ƭ
			if(!is_pcr_reset){
				end_s = (info.time_end)/1000;
			}else{
				end_s = (info.time_begin+info.time_len)/1000;
			}
			end_pos = info.file_len;
		}else{
			DWORD read_size = ts_read_size;
			if(read_size > info.file_len - end_pos){
				read_size = info.file_len - end_pos;
			}
			memset(file_cache,0,cache_size);
			file_lseek(atud->file_id,end_pos,SEEK_SET);
			file_read(atud->file_id,file_cache,read_size);
			//��ȡ��ǰbuf��һ��pcr
			pcr_ms = atud->ts->ts_parser_get_first_pcr_ms(file_cache,read_size);
			if(pcr_ms != 0){
				if(pcr_ms/1000 > begin_s){
					end_s = pcr_ms/1000;
				}else if((pcr_ms + UINT_MAX/45)/1000 - begin_s <120){//���ǻع�
					end_s = (pcr_ms + UINT_MAX/45)/1000;
					is_pcr_reset = 1;
				}else{//�쳣
					end_s = begin_s = pcr_ms/1000;
					is_pcr_jump = 1;
				}
			}else{
				end_pos += read_size;
				if(end_pos < info.file_len){
					continue;
				}else{//���һ����Ƭ
					if(!is_pcr_reset){
						end_s = (info.time_end)/1000;
					}else{
						end_s = (info.time_begin+info.time_len)/1000;
					}
				}
			}
		}
		//�������ʱ�������������ļ�
		if(end_s - begin_s > max_time_len){
			max_time_len = end_s - begin_s;
		}
		
		if(is_pcr_jump){
			snprintf(temp_buf,sizeof(temp_buf),"#EXTINF:%u,\n",M3U8_SPLICE_TIME_MS/1000);
		}else{
			snprintf(temp_buf,sizeof(temp_buf),"#EXTINF:%u,\n",end_s - begin_s);
		}
		
		m3u8_info.append(temp_buf);
		m3u8_info.append(ts_path);
		if(strstr(ts_path.c_str(),"?") == NULL){
			snprintf(temp_buf,sizeof(temp_buf),"?file_start=%lu&file_end=%lu&file_hls_ts=1\n",begin_pos,end_pos);
		}else{
			snprintf(temp_buf,sizeof(temp_buf),"&file_start=%lu&file_end=%lu&file_hls_ts=1\n",begin_pos,end_pos);
		}
		m3u8_info.append(temp_buf);

		begin_pos = end_pos;
		end_pos += splice_size;
		begin_s = end_s;
		is_pcr_jump = 0;
	}
	m3u8_info.append("#EXT-X-ENDLIST\n");
	//�ͷ�ts
	if(file_cache != NULL){
		free(file_cache);
		file_cache = NULL;
	}
	file_close(atud->file_id);
	atud->file_id = INVALID_HANDLE_VALUE;
	//����hls�����ļ�����д��
	HANDLE m3u8_fd = file_open(m3u8_path.c_str(), O_CREAT|O_RDWR, FILE_PERMS_ALL,NULL);
	if(m3u8_fd == INVALID_HANDLE_VALUE){
		return -1;
	}
	if( max_time_len < 5 ){
		max_time_len = 5;
	}else if(max_time_len > 60 ){
		max_time_len = 60;
	}
	snprintf(temp_buf,sizeof(temp_buf),"#EXTM3U\n#EXT-X-TARGETDURATION:%u\n#EXT-X-MEDIA-SEQUENCE:0\n",max_time_len);
	file_write(m3u8_fd,temp_buf,strlen(temp_buf));
	file_write(m3u8_fd,m3u8_info.c_str(),m3u8_info.length());
	file_close(m3u8_fd);
	return 0;
}

/***************************************************************
//����2��http��ַ
//
***************************************************************/
int app_test_server_http::app_test_server_http_display_mode(app_test_http_data_t *atud){
	const char *path = atud->parser_url->url_parser_get_path(2);
	if(path == NULL){
		return -1;
	}
	user_log_printf("path2:%s\n",path);
	//m3u8�ļ�
	if(strstr(path,"m3u8") != NULL){
		return app_test_server_http_play_m3u8(atud);
	}else if(strstr(path,"ts") != NULL){//ts�ļ�
		return app_test_server_http_play_ts(atud);
	}else{
		return -1;
	}
}
/***************************************************************
//����1��http��ַ
//
***************************************************************/
int app_test_server_http::app_test_server_http_process(app_test_http_data_t *atud){
	const char *path = atud->parser_url->url_parser_get_path(1);
	app_test_server_http_init_req(atud);

	user_log_printf("path1:%s\n",path);
	//play�������չ
	if(strncmp(path,"play",4) == 0){
		return app_test_server_http_display_mode(atud);
	}else{
		return -1;
	}
}
/***************************************************************
//���ݴ���ص�
//
***************************************************************/
void app_test_server_http::app_test_server_http_event_cb(struct event_connection_s *ec, int event_task, void *user_data ){
	int ret = 0;
	app_test_http_data_t *atud = (app_test_http_data_t *)user_data;

	if(event_task == EVENT_READ){
		if(ec->s != NULL){
			ret = ec->s->Receive(atud->recv_data->message_block_get_wr_ptr(),atud->recv_data->message_block_get_space());
			if(ret == 0){
				atud->state = APP_TEST_SERVER_HTTP_ERROR;
			}else if(ret < 0){
				if(get_errno() != EWOULDBLOCK && get_errno() != EINPROGRESS){
					atud->state = APP_TEST_SERVER_HTTP_ERROR;
					user_log_printf("recv error:%d\n",get_errno());
				}
			}else{
				atud->recv_data->message_block_wr_pos_add(ret);
				user_log_printf("rtsp user request:%.*s\n",ret,atud->recv_data->message_block_get_rd_ptr());
			}
		}
	}
	if(event_task == EVENT_TIMER){
		if(atud->http->app_test_server_http_check_send(atud) < 0)
		{
			atud->state = APP_TEST_SERVER_HTTP_ERROR;
		}
		ec->es->event_server_connection_add_event(ec,EVENT_TIMER, 50);
	}
	//��ʼ����http����
	if(atud->state == APP_TEST_SERVER_HTTP_START){
		atud->parser->head_parser_clear();
		atud->parser_url->url_parser_clear();
		atud->http_req->head_parser_clear();
		atud->state = APP_TEST_SERVER_HTTP_REQ_HEAD;
	}
	//����http��url
	if(atud->state == APP_TEST_SERVER_HTTP_REQ_HEAD){
		int parse_len = 0;
		if(atud->parser->parse((const char *)atud->recv_data->message_block_get_rd_ptr(),atud->recv_data->message_block_get_data_len(), &parse_len) == 0){
			const char *url = atud->parser->head_parser_get_build_url();
			user_log_printf("head_parser_get_build_url:%s\n",url);
			//����url
			if(atud->parser_url->parse(url,strlen(url)) == 0){
				atud->state = APP_TEST_SERVER_HTTP_PROCESS;
			}else{
				user_log_printf("url parser error\n");
				atud->state = APP_TEST_SERVER_HTTP_ERROR;
			}
			atud->recv_data->message_block_rd_pos_add(parse_len);
			//��ȡrange
			atud->parser->head_parser_get_range(&atud->range);
		}else{
			atud->state = APP_TEST_SERVER_HTTP_ERROR;
		}
	}
	//��������
	if(atud->state == APP_TEST_SERVER_HTTP_PROCESS){
		if(atud->http->app_test_server_http_process(atud) != 0){
			atud->state = APP_TEST_SERVER_HTTP_ERROR;
		}
	}
	//��������
	if(atud->state == APP_TEST_SERVER_HTTP_SEND_FILE){
		int ret = atud->http->app_test_server_http_send_file(atud);
		
		if(ret < 0){
			atud->state = APP_TEST_SERVER_HTTP_ERROR;
		}else if(ret > 0){
			atud->state = APP_TEST_SERVER_HTTP_SEND_CLOSE;
		}
	}
	//�ļ����ݷ������
	if(atud->state == APP_TEST_SERVER_HTTP_SEND_CLOSE){
		if(atud->send_data->message_block_get_data_len() == 0){
			atud->state = APP_TEST_SERVER_HTTP_END;
		}
	}
	//�������
	if(atud->state == APP_TEST_SERVER_HTTP_ERROR || atud->state == APP_TEST_SERVER_HTTP_END){
		atud->http->app_test_server_http_clean(atud);
	}
}