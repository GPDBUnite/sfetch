#include <pthread.h>
#include <cstddef>
#include <cstdlib>
#include <algorithm>    // std::min
#include <cstring>

#include "BlockingBuffer.h"
#include "HTTPFetcher.h"
#include "S3Fetcher.h"


BlockingBuffer::BlockingBuffer(const char* url, OffsetMgr* o)
    :sourceurl(url)
    ,readpos(0)
    ,realsize(0)
    ,status(BlockingBuffer::STATUS_EMPTY)
    ,eof(false)
    ,mgr(o)
    ,error(false)
{
    this->nextpos = o->NextOffset();
	this->bufcap = o->Chunksize();
}

BlockingBuffer::~BlockingBuffer() {
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
uint64_t BlockingBuffer::Read(char* buf, uint64_t len) {
    // assert buf not null
    // assert len > 0, len < this->bufcap
    pthread_mutex_lock(&this->stat_mutex);
    while (this->status == BlockingBuffer::STATUS_EMPTY) {
        pthread_cond_wait(&this->stat_cond, &this->stat_mutex);
    }
    uint64_t left_data_length = this->realsize - this->readpos;
    int length_to_read = std::min(len, left_data_length);

    memcpy(buf, this->bufferdata + this->readpos, length_to_read);
    if(left_data_length >= len) {
        this->readpos += len; // not empty
    } else { // empty, reset everything
        this->readpos = 0;
        if(this->status == BlockingBuffer::STATUS_READY)
            this->status = BlockingBuffer::STATUS_EMPTY;
        if(!this->EndOfFile()) {
            this->nextpos = this->mgr->NextOffset();
            pthread_cond_signal(&this->stat_cond);
        }
    }
    pthread_mutex_unlock(&this->stat_mutex);
    return length_to_read;
}

uint64_t BlockingBuffer::Fill() {
    // assert offset > 0, offset < this->bufcap
    pthread_mutex_lock(&this->stat_mutex);
    while (this->status == BlockingBuffer::STATUS_READY) {
        pthread_cond_wait(&this->stat_cond, &this->stat_mutex);
    }
    uint64_t offset = this->nextpos.offset;
    uint64_t leftlen = this->nextpos.len;
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
            if(this->realsize == 0) {
                this->eof = true;
            }
            std::cout<<"reach end of file ?"<<std::endl;
            break;
        } else if (readlen == -1) { // Error, network error or sth.
            // perror, retry
            this->error = true;
            // Ensure status is still empty
            //this->status = BlockingBuffer::STATUS_READY;
            //pthread_cond_signal(&this->stat_cond);
            break;
        } else { // > 0
            offset += readlen;
            leftlen -= readlen;
            this->realsize += readlen;
            // this->status = BlockingBuffer::STATUS_READY;
        }
    }
    this->status = BlockingBuffer::STATUS_READY;
    if(this->realsize >= 0) {
        pthread_cond_signal(&this->stat_cond);
    }

    pthread_mutex_unlock(&this->stat_mutex);
    return (readlen == -1) ? -1 : this->realsize ;
}


BlockingBuffer* BlockingBuffer::CreateBuffer(const char* url, OffsetMgr* o, S3Credential* pcred) {
	BlockingBuffer* ret = NULL;
	if ( url == NULL )
		return NULL;

	if(pcred) {
		S3Credential cred;
		cred.keyid = pcred->keyid;
		cred.secret = pcred->secret;
		ret = new S3Fetcher(url, o, cred);
	} else {
		ret = new HTTPFetcher(url, o);
	}

	return ret;
}


void* DownloadThreadfunc(void* data) {
    BlockingBuffer* buffer = (BlockingBuffer*)data;
    size_t filled_size = 0;
    void* pp = (void*)pthread_self();
    // assert offset > 0
    do {
        filled_size = buffer->Fill();
        std::cout<<"Fillsize is "<<filled_size<<" of "<<pp<<std::endl;
        if(buffer->EndOfFile())
            break;
        if (filled_size == -1) { // Error
            // retry?
            if(buffer->Error()) {
                break;
            } else
                continue;
        }
    } while(1);
    std::cout<<"quit\n";
    return NULL;
}

Downloader::Downloader(uint8_t part_num) 
	:num(part_num)
{
	this->threads = (pthread_t*) malloc(num * sizeof(pthread_t));
	this->buffers = (BlockingBuffer**) malloc (num * sizeof(BlockingBuffer*));
}

bool Downloader::init(const char* url, uint64_t size, uint64_t chunksize, S3Credential* pcred) {
	this->o = new OffsetMgr(size, chunksize);
	for(int i = 0; i < this->num; i++) {
		this->buffers[i] = BlockingBuffer::CreateBuffer(url, o, pcred);  // decide buffer according to url
		if(!this->buffers[i]->Init()) {
			std::cerr<<"init fail"<<std::endl;
		}
		pthread_create(&this->threads[i], NULL, DownloadThreadfunc, this->buffers[i]);
	}
	readlen = 0;
	chunkcount = 0;
}

bool Downloader::get(char* data, uint64_t& len) {
	uint64_t filelen = this->o->Size();
	BlockingBuffer* buf = buffers[this->chunkcount % this->num];
	uint64_t tmplen = buf->Read(data, len);
    this->readlen += tmplen;
	if(tmplen < len) {
		this->chunkcount++;
		len = tmplen;
		if(buf->Error()) {
			return false;
		}
	}
	if(this->readlen ==  filelen) {
		if(buf->EndOfFile())
			return false;
	}
	return true;
}

void Downloader::destroy() {
	// if error
	for(int i = 0; i < this->num; i++) {
		pthread_cancel(this->threads[i]);
	}
    for(int i = 0; i < this->num; i++) {
        pthread_join(this->threads[i],NULL);
		delete this->buffers[i];
				
    }
	delete this->o;
}

Downloader::~Downloader() {
	free(this->threads);
	free(this->buffers);
}

