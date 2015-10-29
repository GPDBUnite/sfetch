#ifndef _OFFSET_MANAGER_
#define _OFFSET_MANAGER_
#include <cstddef>
#include <cstdlib>

#include <pthread.h>

typedef  unsigned long long  SIZE_T;

struct Range
{
    /* data */
    size_t offset;
    size_t len;
};

class OffsetMgr {
public:
    OffsetMgr(SIZE_T maxsize, SIZE_T chunksize);
    ~OffsetMgr(){pthread_mutex_destroy(&this->offset_lock);};
    Range NextOffset(); // ret.len == 0 means EOF
    void Reset(SIZE_T n);
private:
    pthread_mutex_t offset_lock;
    size_t maxsize;
    size_t chunksize;
    size_t curpos;
};

#endif //_OFFSET_MANAGER_
