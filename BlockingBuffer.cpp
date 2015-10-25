#include <pthread.h>
#include <cstddef>
#include <cstdlib>
#include <algorithm>    // std::min
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <curl/curl.h>
#include <iostream>

#include "OffsetMgr.h"

#define PARALLELNUM 5
//#define CHUNKSIZE   7*1034*125
//#define CHUNKSIZE   64*1024*1024
#define CHUNKSIZE   1233497

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


BlockingBuffer::BlockingBuffer(const char* url, size_t cap, OffsetMgr* o)
:sourceurl(url)
,bufcap(cap)
,readpos(0)
,realsize(0)
,status(BlockingBuffer::STATUS_EMPTY)
,eof(false)
,mgr(o)
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
size_t BlockingBuffer::Read(char* buf, size_t len) {
    // assert buf not null
    // assert len > 0, len < this->bufcap
    pthread_mutex_lock(&this->stat_mutex);
    while (this->status == BlockingBuffer::STATUS_EMPTY) {
        pthread_cond_wait(&this->stat_cond, &this->stat_mutex);
    }
    size_t left_data_length = this->realsize - this->readpos;
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

size_t BlockingBuffer::Fill() {
    // assert offset > 0, offset < this->bufcap
    pthread_mutex_lock(&this->stat_mutex);
    while (this->status == BlockingBuffer::STATUS_READY) {
        pthread_cond_wait(&this->stat_cond, &this->stat_mutex);
    }
    size_t offset = this->nextpos.offset;
    size_t leftlen = this->nextpos.len;
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

class HTTPBuffer : public BlockingBuffer
{
public:
    HTTPBuffer(const char* url, size_t cap, OffsetMgr* o);
    ~HTTPBuffer(){};

protected:
    virtual size_t fetchdata(size_t offset, char* data, size_t len);
};

HTTPBuffer::HTTPBuffer(const char* url, size_t cap, OffsetMgr* o)
:BlockingBuffer(url, cap, o)
{

}
BlockingBuffer* BlockingBuffer::CreateBuffer(const char* url, OffsetMgr* o) {
    return url == NULL ? NULL : new HTTPBuffer(url, CHUNKSIZE, o);
}
struct Bufinfo
{
    /* data */
    char* buf;
    int maxsize;
    int len;
};

size_t
WriterCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  Bufinfo *p = (Bufinfo*) userp;
  //std::cout<<"in writer"<<std::endl;
  // assert p->len + realsize < p->maxsize
  memcpy(p->buf + p->len, contents, realsize);
  p->len += realsize;
  return realsize;
}

// buffer size should be at lease len
// read len data from offest
size_t HTTPBuffer::fetchdata(size_t offset, char* data, size_t len) {
    Bufinfo bi;
    bi.buf = data;
    bi.maxsize = len;
    bi.len = 0;
    if(len == 0)
        return 0;
    CURL *curl_handle = curl_easy_init();
    CURLcode res;
    char rangebuf[128];
    //curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_URL, this->sourceurl);
    /* send all data to this function  */
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriterCallback);

    /* we pass our 'chunk' struct to the callback function */
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&bi);

    sprintf(rangebuf, "%d-%d", offset, offset + len - 1);
    curl_easy_setopt(curl_handle, CURLOPT_RANGE, rangebuf);
    curl_easy_setopt(curl_handle, CURLOPT_FORBID_REUSE, 1L);
    /* get it! */
    res = curl_easy_perform(curl_handle);

    /* check for errors */
    if(res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n",
        curl_easy_strerror(res));
    }

    /* cleanup curl stuff */
    curl_easy_cleanup(curl_handle);
    return bi.len;
}

void* DownloadThreadfunc(void* data) {

    BlockingBuffer* buffer = (BlockingBuffer*)data;
    size_t filled_size = 0;
    // assert offset > 0
    do {
        filled_size = buffer->Fill();
        //std::cout<<"Fillsize is "<<filled_size<<" "<<data<<std::endl;
        if(buffer->EndOfFile() == true)
            break;
        if (filled_size == -1) { // Error
            // retry?
            continue;
        }
    } while(1);
    std::cout<<"quit\n";
    return NULL;
}

int main(int argc, char const *argv[])
{
    /* code */
    //InitRB();
    //InitOffset(601882624, CHUNKSIZE);
    pthread_t threads[PARALLELNUM];
    BlockingBuffer* buffers[PARALLELNUM];
    OffsetMgr* o = new OffsetMgr(1016517804,CHUNKSIZE);
    for(int i = 0; i < PARALLELNUM; i++) {
        // Create
        buffers[i] = BlockingBuffer::CreateBuffer(argv[1], o);
        if(!buffers[i]->Init())
            std::cerr<<"init fail"<<std::endl;
        pthread_create(&threads[i], NULL, DownloadThreadfunc, buffers[i]);
    }

    int i = 0;
    BlockingBuffer *buf = NULL;
    char* data = (char*) malloc(4096);
    size_t len;
    size_t totallen = 0;

    int fd = open("/home/jasper/work/s3/fetcher/data.bin", O_RDWR | O_CREAT, S_IRUSR);
    if(fd == -1) {
        perror("create file error");
    }
    while(true) {
        buf = buffers[i%PARALLELNUM];
        len = buf->Read(data, 4096);

        write(fd, data, len);
        totallen += len;
        if(len < 4096)
            i++;
        if(totallen ==  1016517804) {
            if(buf->EndOfFile())
                break;
        }
    }
    std::cout<<"exiting"<<std::endl;
    free(data);
    for(i = 0; i < PARALLELNUM; i++) {
        pthread_join(threads[i],NULL);
    }
    for(i = 0; i < PARALLELNUM; i++) {
        delete buffers[i];
    }
    close(fd);
    delete o;
    return 0;
}
