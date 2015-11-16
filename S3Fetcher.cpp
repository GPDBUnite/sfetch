#include <cstring>
#include <sstream>

#include "utils.h"
#include "S3Fetcher.h"

#define __STDC_FORMAT_MACROS
#include <inttypes.h>



S3Fetcher::S3Fetcher(const char* url, OffsetMgr* o)
    :HTTPFetcher(url, o)
{

}

bool S3Fetcher::processheader() {
	return this->SignV2();
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
