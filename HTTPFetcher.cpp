#include "HTTPFetcher.h"
#include <cstring>
#include <sstream>

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

static uint64_t WriterCallback(void *contents, uint64_t size, uint64_t nmemb, void *userp)
{
    uint64_t realsize = size * nmemb;
    Bufinfo *p = (Bufinfo*) userp;
    //std::cout<<"in writer"<<std::endl;
    // assert p->len + realsize < p->maxsize
    memcpy(p->buf + p->len, contents, realsize);
    p->len += realsize;
    return realsize;
}

HTTPFetcher::HTTPFetcher(const char* url, OffsetMgr* o)
    :BlockingBuffer(url, o)
    ,urlparser(url)
{
    this->curl = curl_easy_init();
    //curl_easy_setopt(this->curl, CURLOPT_VERBOSE, 1L);
    //curl_easy_setopt(curl, CURLOPT_PROXY, "127.0.0.1:8080");
    curl_easy_setopt(this->curl, CURLOPT_WRITEFUNCTION, WriterCallback);
    curl_easy_setopt(this->curl, CURLOPT_FORBID_REUSE, 1L);
    this->AddHeaderField(HOST,urlparser.Host());
}

HTTPFetcher::~HTTPFetcher() {
    curl_easy_cleanup(this->curl);
}

bool HTTPFetcher::SetMethod(Method m) {
    this->method = m;
    return true;
}

bool HTTPFetcher::AddHeaderField(HeaderField f, const char* v) {
    if(v == NULL) {
        // log warning
        return false;
    }
    return this->headers.Add(f, v);
}

// buffer size should be at lease len
// read len data from offest
uint64_t HTTPFetcher::fetchdata(uint64_t offset, char* data, uint64_t len) {
    if(len == 0)  return 0;

 RETRY:
    Bufinfo bi;
    bi.buf = data;
    bi.maxsize = len;
    bi.len = 0;

    CURL *curl_handle = this->curl;

    char rangebuf[128];

    curl_easy_setopt(curl_handle, CURLOPT_URL, this->sourceurl);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&bi);

    sprintf(rangebuf, "bytes=%" PRIu64 "-%" PRIu64, offset, offset + len - 1);
    this->AddHeaderField(RANGE, rangebuf);

	this->processheader();

	struct curl_slist *chunk = this->headers.GetList();
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);

    CURLcode res = curl_easy_perform(curl_handle);
    if(res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n",
                curl_easy_strerror(res));
        bi.len = -1;
    } else {
        curl_easy_getinfo (curl_handle, CURLINFO_RESPONSE_CODE, &res);
		if(this->retry(res))
			goto RETRY;
        if(! ((res == 200) || (res == 206)) ) {
            fprintf(stderr, "%.*s", (int)len, data);
            bi.len = -1;
        }
    }

    return bi.len;
}
