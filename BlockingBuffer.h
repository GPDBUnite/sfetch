#ifndef __BLOCKINH_BUFFER_
#define __BLOCKINH_BUFFER_


#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <curl/curl.h>
#include <iostream>

#include "OffsetMgr.h"
#include <cstdint>

typedef  uint64_t  uint64_t;

class BlockingBuffer
{
public:
    static BlockingBuffer* CreateBuffer(const char* url, OffsetMgr* o);
    BlockingBuffer(const char* url,OffsetMgr* o);
    virtual ~BlockingBuffer();
    bool Init();
    bool EndOfFile() {
        return this->eof;
    };
    bool Error() {
        return this->error;
    };

    uint64_t Read(char* buf, uint64_t len);
    uint64_t Fill();

    static const int STATUS_EMPTY = 0;
    static const int STATUS_READY = 1;

    /* data */
protected:
    const char* sourceurl;
    uint64_t bufcap;
    virtual uint64_t fetchdata(uint64_t offset, char* data, uint64_t len) = 0;
private:
    int status;
    bool eof;
    bool error;
    pthread_mutex_t stat_mutex;
    pthread_cond_t   stat_cond;
    uint64_t readpos;
    uint64_t realsize;
    char* bufferdata;
    OffsetMgr* mgr;
    Range nextpos;
};

#endif//__BLOCKINH_BUFFER_

