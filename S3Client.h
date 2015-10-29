#ifndef __S3CLIENT__
#define __S3CLIENT__

#include "HTTPClient.h"

class S3Client : public HTTPClient
{
public:
    S3Client(const char* url, SIZE_T cap, OffsetMgr* o);
    ~S3Client();
protected:
    virtual SIZE_T fetchdata(SIZE_T offset, char* data, SIZE_T len);

private:
    bool SignV2();
};


#endif//__S3CLIENT__
