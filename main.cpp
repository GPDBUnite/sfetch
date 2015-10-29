#include "BlockingBuffer.h"
#include <pthread.h>

#define PARALLELNUM 5
//#define CHUNKSIZE   7*1034*125
//#define CHUNKSIZE   64*1024*1024
#define CHUNKSIZE   1233497

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
