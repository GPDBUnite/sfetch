#ifndef __S3RESPONSE_H_
#define __S3RESPONSE_H_

#include <cstdint>
#include <vector>
using std::vector;

#include "S3Common.h"

struct BucketContent;

struct ListBucketResult
{
    const char* Name;
    const char* Prefix;
    unsigned int MaxKeys;
    vector<BucketContent*> contents;
};

struct BucketContent
{
    const char* key;
    //const char* etags;
    uint64_t size;
    static BucketContent* CreateBucketContentItem(const char* key, uint64_t size);
    BucketContent();
    ~BucketContent();
};

ListBucketResult*  ListBucket(const char* bucket, const char* path, S3Credential &cred);

#endif // __S3RESPONSE_H_
