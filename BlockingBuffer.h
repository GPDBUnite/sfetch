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

typedef  uint64_t  SIZE_T;

class BlockingBuffer
{
public:
    static BlockingBuffer* CreateBuffer(const char* url, OffsetMgr* o);
    BlockingBuffer(const char* url, SIZE_T cap, OffsetMgr* o);
    virtual ~BlockingBuffer();
    bool Init();
    bool EndOfFile() {
        return this->eof;
    };
    bool Error() {
        return this->error;
    };

    SIZE_T Read(char* buf, SIZE_T len);
    SIZE_T Fill();

    static const int STATUS_EMPTY = 0;
    static const int STATUS_READY = 1;

    /* data */
protected:
    const char* sourceurl;
    const SIZE_T bufcap;
    virtual SIZE_T fetchdata(SIZE_T offset, char* data, SIZE_T len) = 0;
private:
    int status;
    bool eof;
    bool error;
    pthread_mutex_t stat_mutex;
    pthread_cond_t   stat_cond;
    SIZE_T readpos;
    SIZE_T realsize;
    char* bufferdata;
    OffsetMgr* mgr;
    Range nextpos;
};

#endif//__BLOCKINH_BUFFER_

