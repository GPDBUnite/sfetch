#ifndef __HTTPFETCHER__
#define __HTTPFETCHER__

#include <map>
#include <string>

#include "BlockingBuffer.h"
#include "HTTPCommon.h"

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

struct Bufinfo
{
    /* data */
    char* buf;
    uint64_t maxsize;
    uint64_t len;
};


class HTTPFetcher : public BlockingBuffer
{
public:
    HTTPFetcher(const char* url, OffsetMgr* o);
    ~HTTPFetcher();
    bool SetMethod(Method m);
    bool AddHeaderField(HeaderField f, const char* v);
protected:
    virtual uint64_t fetchdata(uint64_t offset, char* data, uint64_t len);
    CURL *curl;
    Method method;
    std::map<HeaderField, std::string> fields;
    UrlParser urlparser;
};


#endif//__HTTPCLIENT__
