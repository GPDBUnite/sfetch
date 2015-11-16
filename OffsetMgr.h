#ifndef _OFFSET_MANAGER_
#define _OFFSET_MANAGER_
#include <cstddef>
#include <cstdlib>
#include <cstdint>
#include <pthread.h>

typedef  uint64_t  uint64_t;

struct Range
{
    /* data */
    uint64_t offset;
    uint64_t len;
};

class OffsetMgr {
public:
    OffsetMgr(uint64_t maxsize, uint64_t chunksize);
    ~OffsetMgr() {
        pthread_mutex_destroy(&this->offset_lock);
    };
    Range NextOffset(); // ret.len == 0 means EOF
    void Reset(uint64_t n);
	uint64_t Chunksize() {return this->chunksize;};
	uint64_t Size() { return this->maxsize;};
private:
    pthread_mutex_t offset_lock;
    uint64_t maxsize;
    uint64_t chunksize;
    uint64_t curpos;
};

#endif //_OFFSET_MANAGER_
