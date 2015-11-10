#include "HTTPCommon.h"
#include <sstream>
#include <map>

const char* GetFieldString(HeaderField f) {
    switch(f) {
    case HOST:
        return "Host";
    case RANGE:
        return "Range";
    case DATE:
        return "Date";
    case CONTENTLENGTH:
        return "Content-Length";
    case AUTHORIZATION:
        return "Authorization";
    default:
        return "unknown";
    }
}

bool HeaderContent::Add(HeaderField f, const std::string& v)
{
    if(!v.empty()) {
        this->fields[f] = std::string(v);
        return true;
    } else {
        return false;
    }

}

struct curl_slist *HeaderContent::GetList()
{
    struct curl_slist * chunk = NULL;
    std::map<HeaderField, std::string>::iterator it;
    for(it = this->fields.begin(); it != this->fields.end(); it++) {
        std::stringstream sstr;
        sstr<<GetFieldString(it->first)<<": "<<it->second;
        chunk = curl_slist_append(chunk, sstr.str().c_str());
    }
    return chunk;
}
