#ifndef __S3_COMMON_H__
#define __S3_COMMON_H__

#include "HTTPCommon.h"

struct S3Credential {
    const char* keyid;
    const char* secret;
};

//char* SignatureV2(const char* date, const char* path, const S3Credential &cred){return NULL;};
//char* SignatureV4(const char* date, const char* path, const S3Credential &cred){return NULL;};

bool SignGetV2(HeaderContent* h, const char* path, const S3Credential cred);
//bool SignV4(HeaderContent* h, const S3Credential cred){return false;};



#endif // __S3_COMMON_H__
