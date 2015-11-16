#ifndef __S3FETCHER__
#define __S3FETCHER__

#include "HTTPFetcher.h"

class S3Fetcher : public HTTPFetcher
{
public:
    S3Fetcher(const char* url, OffsetMgr* o);
    ~S3Fetcher();
protected:
    virtual uint64_t fetchdata(uint64_t offset, char* data, uint64_t len);

private:
    bool SignV2();
};


#endif // __S3FETCHER__
