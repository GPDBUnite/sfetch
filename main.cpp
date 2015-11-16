#include "BlockingBuffer.h"
#include <pthread.h>

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

#define ASMAIN

#ifdef ASMAIN


struct fetcher {
	fetcher(uint8_t part_num);
	~fetcher();
	bool init(const char* url, uint64_t size, uint64_t chunksize);
	bool get(char* buf, uint64_t& len);
	void destroy();
	//reset
	// init(url)
private:
	const uint8_t num;
	pthread_t* threads;
	BlockingBuffer** buffers;
	OffsetMgr* o;
	uint8_t chunkcount;
	uint64_t readlen;
};

fetcher::fetcher(uint8_t part_num) 
	:num(part_num)
{
	this->threads = (pthread_t*) malloc(num * sizeof(pthread_t));
	this->buffers = (BlockingBuffer**) malloc (num * sizeof(BlockingBuffer*));
}

bool fetcher::init(const char* url, uint64_t size, uint64_t chunksize) {
	this->o = new OffsetMgr(size, chunksize);
	for(int i = 0; i < this->num; i++) {
		this->buffers[i] = BlockingBuffer::CreateBuffer(url, o);  // decide buffer according to url
		if(!this->buffers[i]->Init()) {
			std::cerr<<"init fail"<<std::endl;
		}
		pthread_create(&this->threads[i], NULL, DownloadThreadfunc, this->buffers[i]);
	}
	readlen = 0;
	chunkcount = 0;
}

bool fetcher::get(char* data, uint64_t& len) {
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

void fetcher::destroy() {
    for(int i = 0; i < this->num; i++) {
        pthread_join(this->threads[i],NULL);
		delete this->buffers[i];
				
    }
	delete this->o;
}

fetcher::~fetcher() {
	free(this->threads);
	free(this->buffers);
}

int main(int argc, char const *argv[])
{
    // filepath and file length
    if(argc < 3) {
        printf("not enough parameters\n");
        return 1;
    }
    uint64_t len = atoll(argv[2]);

    /* code */
	fetcher* f = new fetcher(8);
	f->init(argv[1], len, 64 * 1024 * 1024);
    char* data = (char*) malloc(4096);
    if(!data) {
        return 0;
    }
	len     = 4096;
    int fd = open("data.bin", O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if(fd == -1) {
        perror("create file error");
        if(data)
            free(data);
        return 1;
    }
    while(f->get(data, len)) {
        write(fd, data, len);
		len = 4096;
    }
	if(len > 0)
		write(fd, data, len);
    std::cout<<"exiting"<<std::endl;
    free(data);
    close(fd);
	f->destroy();
	delete f;
    return 0;
} 

#endif // ASMAIN
