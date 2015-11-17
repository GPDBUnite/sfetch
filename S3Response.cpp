#include "S3Response.h"
#include "HTTPCommon.h"
#include <sstream>
#include<algorithm>
using std::stringstream;
using std::sort;

#include <cstring>
#include <cstdlib>
#include <cstdint>

#include <curl/curl.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#include "utils.h"

typedef uint64_t uint64_t;

//CreateBucketContentItem
BucketContent::~BucketContent() {
    if(this->key) {
        free((void*)this->key);
    }
}

BucketContent::BucketContent()
    :key(NULL)
    ,size(0)
{
}

BucketContent* CreateBucketContentItem(const char* key, uint64_t size) {
    if(!key)
        return NULL;
    const char* tmp = strdup(key);
    if(!tmp)
        return NULL;
    BucketContent* ret = new BucketContent();
    if(!ret) {
        free((void*)tmp);
        return NULL;
    }
    ret->key = tmp;
    ret->size = size;
    return ret;
}

struct XMLInfo {
    xmlParserCtxtPtr ctxt;
};



//void S3fetch_curl(const char* url, )
static uint64_t ParserCallback(void *contents, uint64_t size, uint64_t nmemb, void *userp)
{
    uint64_t realsize = size * nmemb;
    int res;
    //printf("%.*s",realsize, (char*)contents);
    struct XMLInfo *pxml = (struct XMLInfo*)userp;
    if(!pxml->ctxt) {
        pxml->ctxt = xmlCreatePushParserCtxt(NULL, NULL,
                                             (const char*)contents, realsize, "resp.xml");
        return realsize;
    } else {
        res = xmlParseChunk(pxml->ctxt, (const char*) contents, realsize, 0);
    }
    return realsize;
}

// require curl 7.17 higher
xmlParserCtxtPtr DoGetXML(const char* host, const char* bucket, const char* url, S3Credential &cred) {
    CURL *curl = curl_easy_init();

    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        curl_easy_setopt(curl, CURLOPT_FORBID_REUSE, 1L);

    } else {
        return NULL;
    }
    std::stringstream sstr;
    XMLInfo xml;
    xml.ctxt = NULL;

    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&xml);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ParserCallback);

    HeaderContent* header = new HeaderContent();
    sstr<<bucket<<".s3.amazonaws.com";
    header->Add(HOST, host);
	UrlParser p(url);
    SignGetV2(header, p.Path(), cred);

    struct curl_slist * chunk = header->GetList();

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);

    CURLcode res = curl_easy_perform(curl);

    if(res != CURLE_OK)
        fprintf(stderr, "curl_easy_perform() failed: %s\n",
                curl_easy_strerror(res));
    xmlParseChunk(xml.ctxt, "", 0, 1);
    curl_slist_free_all(chunk);
    curl_easy_cleanup(curl);

    return xml.ctxt;
}


bool BucketContentComp(BucketContent* a,BucketContent* b)
{
    return strcmp(a->Key(), b->Key()) > 0;
}


ListBucketResult*
ListBucket(const char* host, const char* bucket, const char* prefix, S3Credential &cred) {
    std::stringstream sstr;
    if(prefix) {
        sstr<<"http://"<<host<<"/"<<bucket<<"?prefix="<<prefix;
        //sstr<<"http://"<<bucket<<"."<<host<<"?prefix="<<prefix;
    } else {
        sstr<<"http://"<<bucket<<"."<<host;
    }

    xmlParserCtxtPtr xmlcontext = DoGetXML(host, bucket, sstr.str().c_str(), cred);
    xmlNode *root_element = xmlDocGetRootElement(xmlcontext->myDoc);
    ListBucketResult* result = new ListBucketResult();

    xmlNodePtr cur;
    cur = root_element->xmlChildrenNode;
    while(cur != NULL) {
        if(!xmlStrcmp(cur->name, (const xmlChar *)"Name")) {
            result->Name = (const char*)xmlNodeGetContent(cur);
        }

        if(!xmlStrcmp(cur->name, (const xmlChar *)"Prefix")) {
            result->Prefix = (const char*)xmlNodeGetContent(cur);
        }

        if(!xmlStrcmp(cur->name, (const xmlChar *)"Contents")) {
            xmlNodePtr contNode = cur->xmlChildrenNode;
            const char* key;
			uint64_t size;
            while(contNode != NULL) {
                if(!xmlStrcmp(contNode->name, (const xmlChar *)"Key")) {
                    key = (const char*)xmlNodeGetContent(contNode);
                }
                if(!xmlStrcmp(contNode->name, (const xmlChar *)"Size")) {
                    xmlChar* v = xmlNodeGetContent(contNode);
                    size = atoll((const char*)v);
                }
                contNode = contNode->next;
            }
			BucketContent *item = CreateBucketContentItem(key, size);
			if(item)
				result->contents.push_back(item);
			else {
				// log error here
			}
        }
        cur = cur->next;
    }

    /* always cleanup */
    xmlFreeParserCtxt(xmlcontext);
	sort(result->contents.begin(), result->contents.end(), BucketContentComp);

    return result;
}

