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
	http_range_type_start,            //RANGEֻ�п�ʼ,û��ָ������
	http_range_type_start_end,        //RANGEָ���˿�ʼ�����
	http_range_type_end_start,        //RANGEָ���˴ӽ���������������
	http_range_type_auto_full        //������ȫ������,���Բ���Ҫָ����ʼ�����        
}head_parser_range_type_t;

typedef struct head_parser_range_t
{
	head_parser_range_type_t type;    /* range������ */
	unsigned int start;        /* range�Ŀ�ʼ */
	unsigned int end;        /* range�Ľ��� */
	unsigned int content;    /* range�ֶ���ָ�ĳ��� */
}head_parser_range_t;

class head_parser{
public:
	head_parser();

public:
	//��������
	int parse(const char *buf,int len,int* parse_len);
	const char *head_parser_get_method();
	const char *head_parser_get_url();
	const char *head_parser_get_version();
	const char *head_parser_get_value_by_name(const char *name);
	//����
	int head_parser_set_version(const char *buf);
	int head_parser_set_code(const char *buf);
	int head_parser_set_code_string(const char *buf);
	int head_parser_set_headers(const char *name,const char *value);
	int head_parser_build_packet(char *buf,int *len);
	//����
	int head_parser_clear();
	const char *head_parser_get_build_url();
	int head_parser_get_range(head_parser_range_t *range);
private:

private:
	//����
	string method;
	string url;
	string build_url;
	string version;
	//����
	string code;
	string code_string;
	string content_len;

	vector<HeaderItem> headers;
	head_parser_state_t state;
};



#endif

