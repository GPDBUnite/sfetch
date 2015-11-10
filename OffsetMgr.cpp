#include "OffsetMgr.h"

#include <iostream>

OffsetMgr::OffsetMgr(SIZE_T m, SIZE_T c)
    :maxsize(m)
    ,chunksize(c)
    ,curpos(0)
{
    pthread_mutex_init(&this->offset_lock, NULL);
}

Range OffsetMgr::NextOffset() {
    Range ret;
    pthread_mutex_lock(&this->offset_lock);
    if(this->curpos < this->maxsize) {
        ret.offset = this->curpos;
    } else {
        ret.offset = this->maxsize;
    }

    if(this->curpos + this->chunksize > this->maxsize) {
        ret.len = this->maxsize - this->curpos;
        this->curpos = this->maxsize;
    }
    else {
        ret.len = this->chunksize;
        this->curpos += this->chunksize;
    }
    pthread_mutex_unlock(&this->offset_lock);
    //std::cout<<ret.offset<<std::endl;
    return ret;
}

void OffsetMgr::Reset(SIZE_T n) {
    pthread_mutex_lock(&this->offset_lock);
    this->curpos = n;
    pthread_mutex_unlock(&this->offset_lock);
}
