#ifndef __HTTPCOMMON_H_
#define __HTTPCOMMON_H_

#include <curl/curl.h>
#include "http_parser.h"
#include <map>
#include <string>


enum Method
{
    GET,
    POST,
    DELETE,
    PUT,
    HEAD
};

enum HeaderField {
    HOST,
    RANGE,
    DATE,
    CONTENTLENGTH,
    AUTHORIZATION,
};

static const char* DATE_HEADER = "date";
static const char* AWS_DATE_HEADER = "X-Amz-Date";
static const char* AWS_SECURITY_TOKEN = "X-Amz-Security-Token";
static const char* ACCEPT_HEADER = "accept";
static const char* ACCEPT_CHAR_SET_HEADER = "accept-charset";
static const char* ACCEPT_ENCODING_HEADER = "accept-encoding";
static const char* AUTHORIZATION_HEADER = "authorization";
static const char* AWS_AUTHORIZATION_HEADER = "authorization";
static const char* COOKIE_HEADER = "cookie";
static const char* CONTENT_LENGTH_HEADER = "content-length";
static const char* CONTENT_TYPE_HEADER = "content-type";
static const char* USER_AGENT_HEADER = "user-agent";
static const char* VIA_HEADER = "via";
static const char* HOST_HEADER = "host";
static const char* AMZ_TARGET_HEADER = "x-amz-target";
static const char* X_AMZ_EXPIRES_HEADER = "X-Amz-Expires";



class UrlParser
{
public:
    UrlParser(const char* url);
    ~UrlParser();
    const char* Schema() {
        return this->schema;
    };
    const char* Host() {
        return this->host;
    };
    const char* Path() {
        return this->path;
    };
    /* data */
private:
    char* extract_field(const struct http_parser_url *u, http_parser_url_fields i);
    char* schema;
    char* host;
    char* path;
    char* fullurl;
};


class HeaderContent
{
public:
    HeaderContent() {};
    ~HeaderContent() {};
    bool Add(HeaderField f, const std::string& value);
    struct curl_slist * GetList();
private:
    std::map<HeaderField, std::string> fields;
};

const char* GetFieldString(HeaderField f);
CURL* CreateCurlHandler(const char* path);


#endif // __HTTPCOMMON_H_
