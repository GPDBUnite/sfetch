#include <cstring>
#include <sstream>

#include "utils.h"
#include "S3Fetcher.h"

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

static uint64_t WriterCallback(void *contents, uint64_t size, uint64_t nmemb, void *userp)
{
    uint64_t realsize = size * nmemb;
    Bufinfo *p = (Bufinfo*) userp;
    std::cout<<contents<<" "<<size<<" "<<nmemb<<" "<<userp<<std::endl;
    //std::cout<<"in writer"<<std::endl;
    // assert p->len + realsize < p->maxsize
    memcpy(p->buf + p->len, contents, realsize);
    p->len += realsize;
    return realsize;
}


S3Fetcher::S3Fetcher(const char* url, OffsetMgr* o)
    :HTTPFetcher(url, o)
{
    curl_easy_setopt(this->curl, CURLOPT_WRITEFUNCTION, WriterCallback);
}

S3Fetcher::~S3Fetcher() {
}

// buffer size should be at lease len
// read len data from offest
uint64_t S3Fetcher::fetchdata(uint64_t offset, char* data, uint64_t len) {
    if(len == 0)  return 0;

RETRY:
    Bufinfo bi;
    bi.buf = data;
    bi.maxsize = len;
    bi.len = 0;

    CURL *curl_handle = this->curl;
    CURLcode res;
    struct curl_slist *chunk = NULL;
    char rangebuf[128];

    curl_easy_setopt(curl_handle, CURLOPT_URL, this->sourceurl);

    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&bi);

    sprintf(rangebuf, "bytes=%" PRIu64 "-%" PRIu64, offset, offset + len - 1);
    this->AddHeaderField(RANGE, rangebuf);

    this->SignV2();

    std::map<HeaderField, std::string>::iterator it;
    for(it = this->fields.begin(); it != this->fields.end(); it++) {
        std::stringstream sstr;
        sstr<<GetFieldString(it->first)<<": "<<it->second;
        std::cout<<sstr.str().c_str()<<std::endl;
        chunk = curl_slist_append(chunk, sstr.str().c_str());
    }

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);

    res = curl_easy_perform(curl_handle);

    if(res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n",
                curl_easy_strerror(res));
        bi.len = -1;
    } else {
        curl_easy_getinfo (curl_handle, CURLINFO_RESPONSE_CODE, &res);
        if( res == 403) {
            goto RETRY;
        }
        if(! ((res == 200) || (res == 206)) ) {
            bi.len = -1;
        }
    }

    return bi.len;
}

bool S3Fetcher::SignV2() {
    char time[64];
    char line[256];
    gethttpnow(time);
    this->AddHeaderField(CONTENTLENGTH, "0");
    char* tmp;
    this->AddHeaderField(DATE,time);
    tmp = SignatureV2(time, this->urlparser.Path(), "BLkT9BWkXCmQT0P1PAriPf3K6ygJorxAD1n/4Tgk");
    sprintf(line, "AWS AKIAJDNIZCZXSKXVP5PA:%s",tmp);
    this->AddHeaderField(AUTHORIZATION, line);
    free(tmp);
}
