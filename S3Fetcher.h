#ifndef __S3FETCHER__
#define __S3FETCHER__

#include "HTTPFetcher.h"

class S3Fetcher : public HTTPFetcher
{
public:
    S3Fetcher(const char* url, SIZE_T cap, OffsetMgr* o);
    ~S3Fetcher();
protected:
    virtual SIZE_T fetchdata(SIZE_T offset, char* data, SIZE_T len);

private:
    bool SignV2();
};


#endif // __S3FETCHER__
