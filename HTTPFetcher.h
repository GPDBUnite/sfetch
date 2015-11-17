#ifndef __HTTPFETCHER__
#define __HTTPFETCHER__

#include <map>
#include <string>

#include "BlockingBuffer.h"
#include "HTTPCommon.h"

#include <curl/curl.h>


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
    uint64_t fetchdata(uint64_t offset, char* data, uint64_t len);
	virtual bool processheader(){return true;};
	virtual bool retry(CURLcode c){return false;};
    CURL *curl;
    Method method;
    HeaderContent headers;
    UrlParser urlparser;
	
};


#endif//__HTTPCLIENT__
