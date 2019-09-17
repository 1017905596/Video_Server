#ifndef __URL_PARSER_H__
#define __URL_PARSER_H__

#include <string>
#include <vector>

using namespace std;

typedef enum url_parser_state_s{
	Start,
	Scheme,
	SlashAfterScheme1,
	SlashAfterScheme2,
	HostPort,
	UrlPathStart,
	UrlPath,
	QueryStart,
	QueryName,
	QueryValue,
	Fragment
} url_parser_state_t;

struct url_key_val
{
	string key;
	string value;
};

class url_parser{
public:
	url_parser();

public:
	int parse(const char *buf,int len);
	int url_parser_clear();
	const char *url_parser_get_scheme();
	const char *url_parser_get_file_name();
	const char *url_parser_get_path(int num);
	const char *url_parser_get_value_byname(const char *name);
private:
	string scheme;//协议方式
	string hostname;//host
	string file_name;
	vector<string> path;
	vector<url_key_val> values;
	string fragment;

	url_parser_state_t state;
};



#endif
