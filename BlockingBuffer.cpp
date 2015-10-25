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

size_t GetNextOffset();

class BlockingBuffer
{
public:
    static BlockingBuffer* CreateBuffer(const char* url);
    BlockingBuffer(const char* url, size_t cap);
    virtual ~BlockingBuffer();
    bool Init();
    int Status(){return this->status;};

    size_t Read(char* buf, size_t len);
    size_t Fill(size_t offset, bool init);

    static const int STATUS_EMPTY = 0;
    static const int STATUS_READY = 1;
    static const int STATUS_EOF = 2;

    /* data */
protected:
    const char* sourceurl;
    const size_t bufcap;
    virtual size_t fetchdata(size_t offset, char* data, size_t len) = 0;
private:
    int status;
    pthread_mutex_t stat_mutex;
    pthread_cond_t   stat_cond;
    size_t readpos;
    size_t realsize;
    char* bufferdata;
};


BlockingBuffer::BlockingBuffer(const char* url, size_t cap)
:sourceurl(url)
,bufcap(cap)
,readpos(0)
,realsize(0)
,status(BlockingBuffer::STATUS_EMPTY)
{
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
        pthread_cond_signal(&this->stat_cond);
    }
    pthread_mutex_unlock(&this->stat_mutex);
    return length_to_read;
}

size_t BlockingBuffer::Fill(size_t offset, bool init) {
    // assert offset > 0, offset < this->bufcap
    pthread_mutex_lock(&this->stat_mutex);
    while (this->status == BlockingBuffer::STATUS_READY) {
        pthread_cond_wait(&this->stat_cond, &this->stat_mutex);
    }
    if(init == false) {
        offset = GetNextOffset();
    }
    // assert this->status != BlockingBuffer::STATUS_READY
    int readlen = 0;
    this->realsize = 0;
    while(this->realsize < this->bufcap) {
        if(offset != (size_t) -1 ) {
            readlen = this->fetchdata(offset, this->bufferdata + this->realsize, this->bufcap - this->realsize);
            std::cout<<"return "<<readlen<<" from libcurl\n";
        } else {
            readlen = 0; // EOF
        }
        if(readlen == 0) {// EOF!!
            if(this->realsize == 0) {
                this->status = BlockingBuffer::STATUS_EOF;
                std::cout<<"reach end of file"<<std::endl;
            }
            break;
        } else if (readlen == -1) { // Error, network error or sth.
            // perror
            break;
        } else { // > 0
            offset += readlen;
            this->realsize += readlen;
            this->status = BlockingBuffer::STATUS_READY;
        }
    }
    if(this->realsize > 0) {
        pthread_cond_signal(&this->stat_cond);
    }

    pthread_mutex_unlock(&this->stat_mutex);
    return (readlen == -1) ? -1 : this->realsize ;
}

class HTTPBuffer : public BlockingBuffer
{
public:
    HTTPBuffer(const char* url, size_t cap);
    ~HTTPBuffer(){};

protected:
    virtual size_t fetchdata(size_t offset, char* data, size_t len);
};

HTTPBuffer::HTTPBuffer(const char* url, size_t cap)
:BlockingBuffer(url, cap)
{

}
BlockingBuffer* BlockingBuffer::CreateBuffer(const char* url) {
    return url == NULL ? NULL : new HTTPBuffer(url, 64*1024*1024);
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
// read len data from offert
size_t HTTPBuffer::fetchdata(size_t offset, char* data, size_t len) {
    Bufinfo bi;
    bi.buf = data;
    bi.maxsize = len;
    bi.len = 0;

    CURL *curl_handle = curl_easy_init();
    CURLcode res;
    char rangebuf[128];
    curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 1L);
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

struct FuncArgs
{
    /* data */
    BlockingBuffer* buffer;
    size_t offset;
};

pthread_mutex_t *offset_mutex=NULL;
static size_t maxlen = 0;
static size_t curpos = 0;
static size_t delta = 0;
void InitOffset(size_t max, size_t chucksize = 64*1024*1024) {
    if(!offset_mutex){
        offset_mutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
        pthread_mutex_init(offset_mutex, NULL);
    }
    curpos = 0;
    maxlen = max;
    delta = chucksize;
}

size_t GetNextOffset() {
    // spin lock
    pthread_mutex_lock(offset_mutex);
    int ret = -1;
    if(maxlen == -1) {
        pthread_mutex_unlock(offset_mutex);
        return ret;
    }
    if((curpos + delta) <= maxlen) {
        ret = curpos;
        curpos += delta;
    } else {
        ret = curpos;
        curpos = maxlen;
        maxlen = -1;
    }
    pthread_mutex_unlock(offset_mutex);
    return ret;
}
void* DownloadThreadfunc(void* data) {
    FuncArgs* args = (FuncArgs*) data;
    BlockingBuffer* buffer = args->buffer;
    size_t offset = args->offset;
    size_t filled_size = 0;
    // assert offset > 0
    bool init = true;
    do {
        filled_size = buffer->Fill(offset, init);
        if(filled_size == 0) { // EOF
            // make sure EOF
            std::cout<<"Fill return 0 with stat: "<<buffer->Status()<<std::endl;
            if(buffer->Status() == BlockingBuffer::STATUS_EOF) {
                break;
            } else {
                // log error, why here!
            }
        } else if (filled_size == -1) { // Error
            // retry?
            continue;
        } else
            init = 0;
    } while(1);
    return NULL;
}

int main(int argc, char const *argv[])
{
    /* code */
    //InitRB();
    InitOffset(1016517804, 64*1024*1024);
    pthread_t threads[5];
    BlockingBuffer* buffers[5];
    struct FuncArgs args[5];
    for(int i = 0; i < 5; i++) {
        // Create

        buffers[i] = args[i].buffer = BlockingBuffer::CreateBuffer(argv[1]);
        if(!buffers[i]->Init())
            std::cerr<<"init fail"<<std::endl;
        args[i].offset = GetNextOffset();
        std::cerr<<args[i].offset<<std::endl;
        pthread_create(&threads[i], NULL, DownloadThreadfunc, &args[i]);
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
        buf = buffers[i%5];
        len = buf->Read(data, 4096);

        if(len < 4096) {
            i++;
        }
        totallen += len;
        //std::cout<<"read len is "<<totallen<<std::endl;

        write(fd, data, len);
        if(totallen == 1016517804)
            break;
    }

    free(data);
    for(i = 0; i < 5; i++) {
        pthread_join(threads[i],NULL);
    }
    for(i = 0; i < 5; i++) {
        delete buffers[i];
    }
    close(fd);
    return 0;
}