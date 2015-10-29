#ifndef __HTTPCLIENT__
#define __HTTPCLIENT__

#include <map>
#include <string>

#include "BlockingBuffer.h"
#include "URLParser.h"

#include <curl/curl.h>
// enum OS_type { Linux, Apple, Windows };

// inline const char* ToString(OS_type v)
// {
//     switch (v)
//     {
//         case Linux:   return "Linux";
//         case Apple:   return "Apple";
//         case Windows: return "Windows";
//         default:      return "[Unknown OS_type]";
//     }
// }


// #ifndef GENERATE_ENUM_STRINGS
//     #define DECL_ENUM_ELEMENT( element ) element
//     #define BEGIN_ENUM( ENUM_NAME ) typedef enum tag##ENUM_NAME
//     #define END_ENUM( ENUM_NAME ) ENUM_NAME; \
//             char* GetString##ENUM_NAME(enum tag##ENUM_NAME index);
// #else
//     #define DECL_ENUM_ELEMENT( element ) #element
//     #define BEGIN_ENUM( ENUM_NAME ) char* gs_##ENUM_NAME [] =
//     #define END_ENUM( ENUM_NAME ) ; char* GetString##ENUM_NAME(enum \
//             tag##ENUM_NAME index){ return gs_##ENUM_NAME [index]; }
// #endif

enum Method
{
    GET, 
    POST, 
    DELETE, 
    PUT,
    HEAD
};

enum AWS_Region
{
    US_EAST_1,
    US_WEST_1,
    US_WEST_2,
    EU_WEST_1,
    EU_CENTRAL_1,
    AP_SOUTHEAST_1,
    AP_SOUTHEAST_2,
    AP_NORTHEAST_1,
    SA_EAST_1
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

enum HeaderField {
	HOST,
	RANGE,
	DATE,
	CONTENTLENGTH,
    AUTHORIZATION,
};

struct Bufinfo
{
    /* data */
    char* buf;
    SIZE_T maxsize;
    SIZE_T len;
};

const char* GetFieldString(HeaderField f);

class HTTPClient : public BlockingBuffer
{
public:
    HTTPClient(const char* url, SIZE_T cap, OffsetMgr* o);
    ~HTTPClient();
    bool SetMethod(Method m);
    bool AddHeaderField(HeaderField f, const char* v);
protected:
    virtual SIZE_T fetchdata(SIZE_T offset, char* data, SIZE_T len);
    CURL *curl;
    Method method;
    std::map<HeaderField, std::string> fields;
    UrlParser urlparser;
};


#endif//__HTTPCLIENT__
