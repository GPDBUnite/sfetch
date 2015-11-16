#ifndef __S3FETCHER__
#define __S3FETCHER__

#include "HTTPFetcher.h"

#include "S3Common.h"


class S3Fetcher : public HTTPFetcher
{
public:
    S3Fetcher(const char* url, OffsetMgr* o,const S3Credential cred);
    ~S3Fetcher(){};
protected:
    virtual bool processheader();
	virtual bool retry(CURLcode c);
 private:
	S3Credential cred;
};

#endif // __S3FETCHER__
