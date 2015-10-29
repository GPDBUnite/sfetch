#include "HTTPClient.h"
#include <cstring>
#include <sstream>

static SIZE_T WriterCallback(void *contents, SIZE_T size, SIZE_T nmemb, void *userp)
{
  SIZE_T realsize = size * nmemb;
  Bufinfo *p = (Bufinfo*) userp;
  //std::cout<<"in writer"<<std::endl;
  // assert p->len + realsize < p->maxsize
  memcpy(p->buf + p->len, contents, realsize);
  p->len += realsize;
  return realsize;
}

HTTPClient::HTTPClient(const char* url, SIZE_T cap, OffsetMgr* o)
:BlockingBuffer(url, cap, o)
,urlparser(url)
{
    this->curl = curl_easy_init();
    curl_easy_setopt(this->curl, CURLOPT_VERBOSE, 1L);
    curl_easy_setopt(curl, CURLOPT_PROXY, "127.0.0.1:8080"); 
    curl_easy_setopt(this->curl, CURLOPT_WRITEFUNCTION, WriterCallback);
    //curl_easy_setopt(this->curl, CURLOPT_FORBID_REUSE, 1L);
    this->AddHeaderField(HOST,urlparser.Host());
}

HTTPClient::~HTTPClient() {
    curl_easy_cleanup(this->curl);
}

bool HTTPClient::SetMethod(Method m) {
    this->method = m;
    return true;
}

bool HTTPClient::AddHeaderField(HeaderField f, const char* v) {
    if(v == NULL) {
        // log warning
        return false;
    } 
    this->fields[f] = std::string(v);
    return true;
}

const char* GetFieldString(HeaderField f) {
    switch(f) {
    case HOST: return "host";
    case RANGE: return "range";
    case DATE:  return "date";
    case CONTENTLENGTH: return "content-length";
    case AUTHORIZATION: return "authorization";
    default:
        return "unknown";
    }
}

// buffer size should be at lease len
// read len data from offest
SIZE_T HTTPClient::fetchdata(SIZE_T offset, char* data, SIZE_T len) {
    if(len == 0)  return 0;

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

    sprintf(rangebuf, "bytes=%lld-%lld", offset, offset + len - 1);
    this->AddHeaderField(RANGE, rangebuf);
    
    
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
        if(! ((res == 200) || (res == 206)) ){
            fprintf(stderr, "%.*s", data, len);
            bi.len = -1;
        }
    }

    return bi.len;
}
