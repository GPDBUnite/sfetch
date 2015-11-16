#ifndef __S3_COMMON_H__
#define __S3_COMMON_H__

#include "HTTPCommon.h"

struct S3Credential {
    const char* keyid;
    const char* secret;
};


bool SignGetV2(HeaderContent* h, const char* path, const S3Credential cred);




#endif // __S3_COMMON_H__
