#include "S3Common.h"

#include "utils.h"

#include <sstream>
#include <cstring>
using std::stringstream;

bool SignGetV2(HeaderContent* h, const char* path, const S3Credential cred) {
    char timestr[64];
    char line[256];
    gethttpnow(timestr);
    h->Add(DATE ,timestr);
    h->Add(CONTENTLENGTH, "0");
    stringstream sstr;
    sstr<<"GET\n\n\n"<<timestr<<"\n"<<path;
    char* tmpbuf = sha1hmac(sstr.str().c_str(), cred.secret);
    int len  = strlen(tmpbuf);
    char* signature = Base64Encode(tmpbuf, len);
    sstr.clear();
    sstr.str("");
    sstr<<"AWS "<<cred.keyid<<":"<<signature;
    free(signature);
    h->Add(AUTHORIZATION,sstr.str().c_str());
    return true;
}
