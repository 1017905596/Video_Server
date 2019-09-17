#include "url_parser.h"


url_parser::url_parser(){
	state = Start;

}

int url_parser::url_parser_clear(){
	scheme.clear();
	hostname.clear();
	file_name.clear();
	path.clear();
	values.clear();
	fragment.clear();
	state = Start;
	return 0;
}

int url_parser::parse(const char *buf,int len){
	const char *begin = buf;
	const char *end = buf + len;

	while(begin != end){
		char input = *begin++;

		switch(state){
			case Start:
				if(input == '/'){
					//仅有一个字符
					if(begin + 1 == end){
						return 0;
					}
					state = UrlPath;
				}else{
					scheme.push_back(input);
					state = Scheme;
				}
				break;
			case Scheme:
				if(input == ':'){
					state = SlashAfterScheme1;
				}else{
					scheme.push_back(input);
				}
				break;
			//两个//
			case SlashAfterScheme1:
				if(input == '/'){
					state = SlashAfterScheme2;
				}else{
					return -1;
				}
				break;
			case SlashAfterScheme2:
				if(input == '/'){
					state = HostPort;
				}else{
					return -1;
				}
				break;
			case HostPort:
				if(begin + 1 == end){
					hostname.push_back(input);
					hostname.push_back(*begin);
					return 0;
				}else if(input == '/' || input == '\\'){
					state = UrlPathStart;
				}else if(input == '?' || input == '&'){
					state = QueryStart;
				}else if(input == '#'){
					state = Fragment;
				}else{
					hostname.push_back(input);
				}
				break;
			case UrlPathStart:
				if(begin + 1 == end){
					return 0;
				}else if(input != '/'){
					path.push_back(string());
					path.back().reserve(16);
					path.back().push_back(input);
					state = UrlPath;
				}else{
					return -1;
				}
				break;
			case UrlPath:
				if(begin + 1 == end){
					path.back().push_back(input);
					path.back().push_back(*begin);
					return 0;
				}else if(input == '/'){
					state = UrlPathStart;
				}else if(input == '?' || input == '&'){
					state = QueryStart;
				}else if(input == '#'){
					state = Fragment;
				}else{
					path.back().push_back(input);
				}
				break;
			case QueryStart:
				if(input == '&'){
					state = QueryStart;
				}else if(begin + 1 == end){
					return 0;
				}else{
					values.push_back(url_key_val());
					values.back().key.reserve(16);
					values.back().value.reserve(16);
					values.back().key.push_back(input);
					state = QueryName;
				}
				break;
			case QueryName:
				if(begin + 1 == end){
					values.back().key.push_back(input);
					values.back().key.push_back(*begin);
					return 0;
				}else if(input == '?' || input == '&'){
					state = QueryStart;
				}else if(input == '#'){
					state = Fragment;
				}else if(input == '='){
					state = QueryValue;
				}else{
					values.back().key.push_back(input);
				}
				break;
			case QueryValue:
				if(begin + 1 == end){
					values.back().value.push_back(input);
					values.back().value.push_back(*begin);
					return 0;
				}else if(input == '?' || input == '&'){
					state = QueryStart;
				}else if(input == '#'){
					state = Fragment;
				}else{
					values.back().value.push_back(input);
				}
				break;
			case Fragment:
				if(begin + 1 == end){
					fragment.push_back(input);
					fragment.push_back(*begin);
					return 0;
				}else{
					fragment.push_back(input);
				}
				break;
		}
	}
	return -1;
}


const char *url_parser::url_parser_get_file_name(){
	vector<string>::iterator it;

	it = path.end() - 1;
	return it->c_str();
}

const char*url_parser::url_parser_get_scheme(){
	if(!scheme.empty()){
		return scheme.c_str();
	}else{
		return NULL;
	}
}

const char *url_parser::url_parser_get_path(int num){
	vector<string>::iterator it;
	int i = 0;
	for( it = path.begin();it != path.end(); it++)
	{
		i ++;
		if(i == num){
			return it->c_str();
		}
	}
	return NULL;
}

const char *url_parser::url_parser_get_value_byname(const char *name){
	vector<url_key_val>::iterator it;

	for( it = values.begin();it != values.end(); it++)
	{
		if(it->key.compare(name) == 0){
			return it->value.c_str();
		}
	}
	return NULL;
}