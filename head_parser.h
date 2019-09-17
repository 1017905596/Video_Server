#ifndef __HEAD_PARSER_H__
#define __HEAD_PARSER_H__

#include <string.h>
#include <string>
#include <vector>

using namespace std;

struct HeaderItem
{
	string name;
	string value;
};

typedef enum head_parser_state_s{
	RequestMethodStart,
	RequestMethod,
	RequestUriStart,
	RequestUri,
	RequestHttpVersion_start,
	RequestHttpVersion,
	RequestHttpVersion_new_line,

	HeaderLineStart,
	HeaderName,
	HeaderValueStart,
	HeaderValue,
	ExpectingNewline_2,
} head_parser_state_t;

typedef enum head_parser_range_type_e{
	http_range_type_null,
	http_range_type_start,            //RANGE只有开始,没有指定结束
	http_range_type_start_end,        //RANGE指定了开始与结束
	http_range_type_end_start,        //RANGE指定了从结束反向下载内容
	http_range_type_auto_full        //请求了全部数据,所以不需要指定开始与结束        
}head_parser_range_type_t;

typedef struct head_parser_range_t
{
	head_parser_range_type_t type;    /* range的类型 */
	unsigned int start;        /* range的开始 */
	unsigned int end;        /* range的结束 */
	unsigned int content;    /* range字段所指的长度 */
}head_parser_range_t;

class head_parser{
public:
	head_parser();

public:
	//解析代码
	int parse(const char *buf,int len,int* parse_len);
	const char *head_parser_get_method();
	const char *head_parser_get_url();
	const char *head_parser_get_version();
	const char *head_parser_get_value_by_name(const char *name);
	//创建
	int head_parser_set_version(const char *buf);
	int head_parser_set_code(const char *buf);
	int head_parser_set_code_string(const char *buf);
	int head_parser_set_headers(const char *name,const char *value);
	int head_parser_build_packet(char *buf,int *len);
	//操作
	int head_parser_clear();
	const char *head_parser_get_build_url();
	int head_parser_get_range(head_parser_range_t *range);
private:

private:
	//解析
	string method;
	string url;
	string build_url;
	string version;
	//创建
	string code;
	string code_string;
	string content_len;

	vector<HeaderItem> headers;
	head_parser_state_t state;
};



#endif

