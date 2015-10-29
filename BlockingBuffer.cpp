#include <pthread.h>
#include <cstddef>
#include <cstdlib>
#include <algorithm>    // std::min
#include <cstring>

#include "BlockingBuffer.h"
#include "HTTPClient.h"

//#define PARALLELNUM 5
//#define CHUNKSIZE   7*1034*125
//#define CHUNKSIZE   64*1024*1024
#define CHUNKSIZE   1233497


BlockingBuffer::BlockingBuffer(const char* url, SIZE_T cap, OffsetMgr* o)
:sourceurl(url)
,bufcap(cap)
,readpos(0)
,realsize(0)
,status(BlockingBuffer::STATUS_EMPTY)
,eof(false)
,mgr(o)
,error(false)
{
    this->nextpos = o->NextOffset();
}

BlockingBuffer::~BlockingBuffer(){
    if(this->bufferdata) {
        free(this->bufferdata);
        pthread_mutex_destroy(&this->stat_mutex);
        pthread_cond_destroy(&this->stat_cond);
    }

};

bool BlockingBuffer::Init() {
    this->bufferdata = (char*) malloc(this->bufcap);
    if(!this->bufferdata) {
        // malloc fail
        std::cout<<"alloc error"<<std::endl;
        return false;
    }
    pthread_mutex_init(&this->stat_mutex, NULL);
    pthread_cond_init (&this->stat_cond, NULL);
    return true;
}



// ret < len means EMPTY
SIZE_T BlockingBuffer::Read(char* buf, SIZE_T len) {
    // assert buf not null
    // assert len > 0, len < this->bufcap
    pthread_mutex_lock(&this->stat_mutex);
    while (this->status == BlockingBuffer::STATUS_EMPTY) {
        pthread_cond_wait(&this->stat_cond, &this->stat_mutex);
    }
    SIZE_T left_data_length = this->realsize - this->readpos;
    int length_to_read = std::min(len, left_data_length);

    memcpy(buf, this->bufferdata + this->readpos, length_to_read);
    if(left_data_length >= len) {
        this->readpos += len; // not empty
    } else { // empty, reset everything
        this->readpos = 0;
        if(this->status == BlockingBuffer::STATUS_READY)
            this->status = BlockingBuffer::STATUS_EMPTY;
        if(!this->EndOfFile()){
            this->nextpos = this->mgr->NextOffset();
            pthread_cond_signal(&this->stat_cond);
        }
    }
    pthread_mutex_unlock(&this->stat_mutex);
    return length_to_read;
}

SIZE_T BlockingBuffer::Fill() {
    // assert offset > 0, offset < this->bufcap
    pthread_mutex_lock(&this->stat_mutex);
    while (this->status == BlockingBuffer::STATUS_READY) {
        pthread_cond_wait(&this->stat_cond, &this->stat_mutex);
    }
    SIZE_T offset = this->nextpos.offset;
    SIZE_T leftlen = this->nextpos.len;
    // assert this->status != BlockingBuffer::STATUS_READY
    int readlen = 0;
    this->realsize = 0;
    while(this->realsize < this->bufcap) {
        if(leftlen != 0 ) {
            //readlen = this->fetchdata(offset, this->bufferdata + this->realsize, this->bufcap - this->realsize);
            readlen = this->fetchdata(offset, this->bufferdata + this->realsize, leftlen);
            //std::cout<<"return "<<readlen<<" from libcurl\n";
        } else {
            readlen = 0; // EOF
        }
        if(readlen == 0) {// EOF!!
            this->eof = true;
            std::cout<<"reach end of file"<<std::endl;
            break;
        } else if (readlen == -1) { // Error, network error or sth.
            // perror, retry
            this->error = true;
            // Ensure status is still empty
            this->status = BlockingBuffer::STATUS_READY;
            pthread_cond_signal(&this->stat_cond);
            break;
        } else { // > 0
            offset += readlen;
            leftlen -= readlen;
            this->realsize += readlen;
            this->status = BlockingBuffer::STATUS_READY;
        }
    }
    if(this->realsize >= 0) {
        pthread_cond_signal(&this->stat_cond);
    }

    pthread_mutex_unlock(&this->stat_mutex);
    return (readlen == -1) ? -1 : this->realsize ;
}


BlockingBuffer* BlockingBuffer::CreateBuffer(const char* url, OffsetMgr* o) {
    return url == NULL ? NULL : new HTTPClient(url, CHUNKSIZE, o);
}