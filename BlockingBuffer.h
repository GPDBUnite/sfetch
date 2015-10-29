#ifndef __BLOCKINH_BUFFER_
#define __BLOCKINH_BUFFER_


#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <curl/curl.h>
#include <iostream>

#include "OffsetMgr.h"

class BlockingBuffer
{
public:
    static BlockingBuffer* CreateBuffer(const char* url, OffsetMgr* o);
    BlockingBuffer(const char* url, size_t cap, OffsetMgr* o);
    virtual ~BlockingBuffer();
    bool Init();
    bool EndOfFile(){return this->eof;};

    size_t Read(char* buf, size_t len);
    size_t Fill();

    static const int STATUS_EMPTY = 0;
    static const int STATUS_READY = 1;

    /* data */
protected:
    const char* sourceurl;
    const size_t bufcap;
    virtual size_t fetchdata(size_t offset, char* data, size_t len) = 0;
private:
    int status;
    bool eof;
    pthread_mutex_t stat_mutex;
    pthread_cond_t   stat_cond;
    size_t readpos;
    size_t realsize;
    char* bufferdata;
    OffsetMgr* mgr;
    Range nextpos;
};

#endif//__BLOCKINH_BUFFER_

