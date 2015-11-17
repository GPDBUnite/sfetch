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

BucketContent* CreateBucketContentItem(const char* key, uint64_t size);

struct BucketContent
{
	friend BucketContent* CreateBucketContentItem(const char* key, uint64_t size);    
    BucketContent();
    ~BucketContent();
	const char* Key() const {return this->key;};
	uint64_t Size() const {return this->size;};
private:
	BucketContent(const BucketContent& b){};
	BucketContent operator=(const BucketContent& b){};

    const char* key;
    //const char* etags;
    uint64_t size;
	
};

// need free
ListBucketResult*  ListBucket(const char* host, const char* bucket, const char* path, S3Credential &cred);

//bool BucketContentComp(const BucketContent& a,const BucketContent& b);

#endif // __S3RESPONSE_H_
