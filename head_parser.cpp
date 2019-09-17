#include "head_parser.h"



head_parser::head_parser(){
	state = RequestMethodStart;
}

int head_parser::parse(const char *buf,int len,int* parse_len){
	const char *begin = buf;
	const char *end = buf + len;
	if(parse_len){
		*parse_len = 0;
	}
	while(begin != end){
		char input = *begin++;

		if(parse_len){
			(*parse_len) ++;
		}
		switch (state){
			case RequestMethodStart:
				if(input == ' ' || input == '\r' || input == '\n'){
					return -1;
				}else{
					state = RequestMethod;
					method.push_back(input);
				}
				break;
			case RequestMethod:
				if(input == ' '){
					state = RequestUriStart;
				}else if(input == '\r' || input == '\n'){
					return -1;
				}else{
					method.push_back(input);
				}
				break;
			case RequestUriStart:
				if(input == '\r' || input == '\n'){
					return -1;
				}else{
					state = RequestUri;
					url.push_back(input);
				}
				break;
			case RequestUri:
				if(input == ' '){
					state = RequestHttpVersion_start;
				}else if(input == '\r' || input == '\n'){
					return -1;
				}else{
					url.push_back(input);
				}
				break;
			case RequestHttpVersion_start:
				if(input == '\r' || input == '\n'){
					return -1;
				}else{
					state = RequestHttpVersion;
					version.push_back(input);
				}
				break;
			case RequestHttpVersion:
				if(input == '\r'){
					state = RequestHttpVersion_new_line;
				}else if(input == '\n'){
					state = HeaderLineStart;
				}else{
					version.push_back(input);
				}
				break;
			case RequestHttpVersion_new_line:
				if(input == '\n'){
					state = HeaderLineStart;
				}else{
					return -1;
				}
				break;
			//head
			case HeaderLineStart:
				//不用解析body
				if(input == '\r'){
					if(parse_len){
						(*parse_len) ++;
					}
					return 0;
				}else if(input == '\n'){
					return 0;
				}else{
					headers.push_back(HeaderItem());
					headers.back().name.reserve(16);
					headers.back().value.reserve(16);
					headers.back().name.push_back(input);
					state = HeaderName;
				}
				break;
			case HeaderName:
				if(input == ':'){
					state = HeaderValueStart;
				}else if(input == '\r' || input == '\n'){
					return -1;
				}else{
					headers.back().name.push_back(input);
				}
				break;
			case HeaderValueStart:
				if(input == ' '){
					state = HeaderValue;
				}else{
					return -1;
				}
				break;
			case HeaderValue:
				if(input == '\r'){
					state = ExpectingNewline_2;
				}else if(input == '\n'){
					return 0;
				}else{
					headers.back().value.push_back(input);
				}
				break;
			case ExpectingNewline_2:
				if(input == '\n'){
					state = HeaderLineStart;
				}else{
					return 0;
				}
				break;
		}
	}
	return 1;
}

int head_parser::head_parser_clear(){
	headers.clear();
	method.clear();
	url.clear();
	version.clear();
	state = RequestMethodStart;

	code.clear();
	code_string.clear();
	content_len.clear();
	return 0;
}

const char *head_parser::head_parser_get_method(){
	if(!method.empty()){
		return method.c_str();
	}
	return NULL;
}

const char *head_parser::head_parser_get_url(){
	if(!url.empty()){
		return url.c_str();
	}
	return NULL;
}

const char *head_parser::head_parser_get_build_url(){
	const char *host = head_parser_get_value_by_name("Host");
	if(!url.empty()){
		build_url.clear();
		if(url.find("://",0) == string::npos){
			if(version.find_first_of('r') != string::npos || version.find_first_of('R') != string::npos){
				build_url.append("rtsp://");
			}else{
				build_url.append("http://");
			}
			build_url.append(host);
			build_url.append(url);
		}else{
			build_url.append(url);
		}
		return build_url.c_str();
	}
	return NULL;
}

int head_parser::head_parser_get_range(head_parser_range_t *range){
	
	vector<HeaderItem>::const_iterator it;
	string rang_value;
	if(!headers.empty()){
		for( it = headers.begin();it != headers.end(); it++)
		{
			if(it->name.compare("Range") == 0){
				rang_value = it->value;
			}
		}
	}
	
	if(rang_value.empty()){
		return -1;
	}
	
	const char *range_len = rang_value.c_str() + rang_value.length();
	const char *p = rang_value.c_str() + 6 ;//bytes= 字符之后
	range->type = http_range_type_null;
	
	while( p < range_len && *p == ' '){
		p++;
	}
	
	if(p >= range_len){
		return -1;
	}
	if(*p == '-'){
		return -1;
	}else if(isdigit(*p)){//是否为数字
		range->type = http_range_type_start;
		while(p < range_len && isdigit(*p)){
			range->start = 10 * range->start + *p-'0';
			++p;
		}
		//跳过空格
		while( p < range_len && *p == ' '){
			p++;
		}
		if(p >= range_len || *p != '-'){
			range->type = http_range_type_null;
			return -1;
		}
		//仅有start
		if(++p >= range_len){
			return 0;
		}

		//跳过空格
		while( p < range_len && *p == ' '){
			p++;
		}

		if(p >= range_len || *p == '*'){
			return 0;
		}

		if(isdigit(*p)){
			while(p < range_len && isdigit(*p)){
				range->end = 10 * range->end + *p-'0';
				++p;
			}
			range->type = http_range_type_start_end;
			return 0;
		}
	}
	return -1;
}

const char *head_parser::head_parser_get_version(){
	if(!version.empty()){
		return version.c_str();
	}
	return NULL;
}

const char *head_parser::head_parser_get_value_by_name(const char *name){
	vector<HeaderItem>::const_iterator it;
	if(!headers.empty()){
		for( it = headers.begin();it != headers.end(); it++)
		{
			if(it->name.compare(name) == 0){
				return it->value.c_str();
			}
		}
	}
	
	return NULL;
}

int head_parser::head_parser_set_version(const char *buf){
	if(buf != NULL){
		version.append(buf);
		return 0;
	}
	return -1;
}

int head_parser::head_parser_set_code(const char *buf){
	if(buf != NULL){
		code.append(buf);
		return 0;
	}
	return -1;
}

int head_parser::head_parser_set_code_string(const char *buf){
	if(buf != NULL){
		code_string.append(buf);
		return 0;
	}
	return -1;
}

int head_parser::head_parser_set_headers(const char *name,const char *value){
	if(name == NULL){
		return -1;
	}
	headers.push_back(HeaderItem());
	headers.back().name.reserve(16);
	headers.back().value.reserve(16);
	headers.back().name.append(name);
	headers.back().value.append(value);

	return 0;
}

int head_parser::head_parser_build_packet(char *buf,int *len){
	if(version.empty() || code.empty() || code_string.empty()){
		return -1;
	}

	int vl = 0;
	int header_len = 0;
	vl = version.length();
	if( buf != NULL && header_len+vl+1 < *len )    /* 获取http消息第一行第一个字段的值 */
	{
		memcpy(buf+header_len, version.c_str(),vl);
		*(buf+header_len+vl) = ' ';
	}
	header_len +=vl+1;

	vl = code.length();
	if( buf != NULL && header_len+vl+2 < *len )    /* 获取http消息第一行第二个字段的值 */
	{
		memcpy(buf+header_len, code.c_str(),vl);
		*(buf+header_len+vl) = ' ';
	}
	header_len +=vl+1;

	vl = code_string.length();
	if( buf != NULL && header_len+vl+1 < *len )    /* 获取http消息第一行第三个字段的值 */
	{
		memcpy(buf+header_len, code_string.c_str(),vl);
		*(buf+header_len+vl) = '\r';
		*(buf+header_len+vl+1) = '\n';
	}
	header_len +=vl+2;

	vector<HeaderItem>::const_iterator it;

	for(it = headers.begin();it != headers.end(); it++){
		vl = it->name.length();
		if( buf != NULL && header_len+vl+2 < *len )    /* ·获取头域 */
		{
			memcpy( buf+header_len,it->name.c_str(), vl );
			*(buf+header_len+vl) = ':';    /* 添加上字符 ':'以分割头域和值 */
			*(buf+header_len+vl+1) = ' ';
		}
		header_len +=vl+2;

		if(it->value.empty()){
			vl = 0;
			if( buf != NULL && header_len+vl+2 < *len )
			{                        
				*(buf+header_len+vl) = '\r';
				*(buf+header_len+vl+1) = '\n';
			}
			header_len +=vl+2;
		}else{
			vl = it->value.length();
			if( buf != NULL && header_len+vl+2 < *len )    /* 获取头域对应的值 */
			{
				memcpy( buf+header_len, it->value.c_str(), vl );
				*(buf+header_len+vl) = '\r';
				*(buf+header_len+vl+1) = '\n';    /* 每一行以\r\n结尾 */
			}
			header_len +=vl+2;
		}
	}

	vl = 0;
	if( buf != NULL && header_len+vl+2 < *len)
	{                
		*(buf+header_len+vl) = '\r';
		*(buf+header_len+vl+1) = '\n';    /* http消息结尾是以\r\n\r\n结尾 */
	}
	header_len +=vl+2;

	*len = header_len;

	return 0;
}