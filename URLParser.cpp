#include "HTTPCommon.h"


#include <cstring>
#include <cstdlib>
#include <cstdio>

#include <iostream>
UrlParser::UrlParser(const char* url)
{
    if(!url) {
        // throw exception
        return;
    }
    struct http_parser_url u;
    int len, result;
    len = strlen(url);
    this->fullurl = (char*)malloc(len+1);
    sprintf(this->fullurl, "%s", url);
    result = http_parser_parse_url(this->fullurl, len, false, &u);
    if (result != 0) {
        printf("Parse error : %d\n", result);
        return;
    }
    //std::cout<<u.field_set<<std::endl;
    this->host = extract_field(&u,UF_HOST);
    this->schema = extract_field(&u,UF_SCHEMA);
    this->path = extract_field(&u,UF_PATH);
}

UrlParser::~UrlParser() {
    if(host)
        free(host);
    if(schema)
        free(schema);
    if(path)
        free(path);
    if(fullurl)
        free(fullurl);
}

char* UrlParser::extract_field(const struct http_parser_url *u, http_parser_url_fields i) {
    char* ret = NULL;
    if((u->field_set & (1 << i)) != 0) {
        ret = (char*)malloc(u->field_data[i].len+1);
        if(ret) {
            memcpy(ret, this->fullurl + u->field_data[i].off, u->field_data[i].len);
            ret[u->field_data[i].len] = 0;
        }
    }
    return ret;
}
