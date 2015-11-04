#include "S3Response.h"
#include "HTTPCommon.h"
#include <sstream>

using std::stringstream;

#include <cstring>
#include <cstdlib>
#include <cstdint>

#include <curl/curl.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#include "utils.h"

typedef uint64_t SIZE_T;

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

BucketContent* BucketContent::CreateBucketContentItem(const char* key, uint64_t size) {
    if(!key)
        return NULL;
    const char* tmp = strdup(key);
    if(!tmp)
        return NULL;
    BucketContent* ret = new BucketContent();
    if(!ret)
        return NULL;
    ret->key = tmp;
    ret->size = size;
    return ret;
}

struct XMLInfo {
    xmlParserCtxtPtr ctxt;
    xmlDocPtr doc;
};


//void S3fetch_curl(const char* url, )
static SIZE_T ParserCallback(void *contents, SIZE_T size, SIZE_T nmemb, void *userp)
{
    SIZE_T realsize = size * nmemb;
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

static void
print_element_names(xmlNode * a_node)
{
    xmlNode *cur_node = NULL;
    xmlChar* v = NULL;
    for (cur_node = a_node; cur_node; cur_node = cur_node->next) {
        if (cur_node->type == XML_ELEMENT_NODE) {
            v = xmlNodeGetContent(cur_node);
            printf("node type: Element, name: %s\t", cur_node->name);
            printf("value: %s\n", v);
            if(v) {
                xmlFree(v);
                v = NULL;
            }
        }

        print_element_names(cur_node->children);
    }
}

static char* CreateListBucketURL() {
    return NULL;
}

ListBucketResult*  
ListBucket(const char* host, const char* bucket, const char* prefix, S3Credential &cred) {
    std::stringstream sstr;
    if(prefix) {
        sstr<<"http://"<<bucket<<"."<<host<<"?prefix="<<prefix;
    } else {
        sstr<<"http://"<<bucket<<"."<<host;
    }
    char* url = strdup(sstr.str().c_str());
    //CURL* curl = CreateCurlHandler(sstr.str().c_str());
    CURL *curl = NULL;
    if(!path) {
        return NULL;
    } else {
        curl = curl_easy_init();
    }

    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

    } else {
        return NULL;
    }
    //free(url);
    sstr.clear();
    sstr.str("");
    XMLInfo xml;
    xml.ctxt = NULL;
    xml.doc = NULL;
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&xml);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ParserCallback);
    // header content
    HeaderContent* header = new HeaderContent();
    sstr<<bucket<<".s3.amazonaws.com";
    header->Add(HOST, sstr.str().c_str());
    SignGetV2(header, "/metro.pivotal.io/", cred);
    // set header
    struct curl_slist * chunk = header->GetList();
    // perform curl
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);

    CURLcode res = curl_easy_perform(curl);
    /* Check for errors */
    if(res != CURLE_OK)
        fprintf(stderr, "curl_easy_perform() failed: %s\n",
                curl_easy_strerror(res));
    xmlParseChunk(xml.ctxt, "", 0, 1);
    xml.doc = xml.ctxt->myDoc;

    /*Get the root element node */
    xmlNode *root_element = xmlDocGetRootElement(xml.doc);

    print_element_names(root_element);

    // xml doc is ready, parse it
    
    /* always cleanup */
    xmlFreeParserCtxt(xml.ctxt);
    curl_slist_free_all(chunk);
    curl_easy_cleanup(curl);
}
#define ASMAIN
#ifdef ASMAIN

int main() 
{
    S3Credential cred;
    cred.keyid = "AKIAJDNIZCZXSKXVP5PA";
    cred.secret = "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";

    ListBucketResult* r = ListBucket("s3-us-west-2.amazonaws.com", "metro.pivotal.io", "data", cred);
    return 0;
}


#endif // ASMAIN
