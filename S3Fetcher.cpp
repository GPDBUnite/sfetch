#include <cstring>
#include <sstream>

#include "utils.h"
#include "S3Fetcher.h"

#define __STDC_FORMAT_MACROS
#include <inttypes.h>



S3Fetcher::S3Fetcher(const char* url, OffsetMgr* o, const S3Credential cred)
    :HTTPFetcher(url, o)
{
	this->cred = cred;
}

bool S3Fetcher::processheader() {
	return SignGetV2(&this->headers, this->urlparser.Path(), this->cred);	
}

bool S3Fetcher::retry(CURLcode c) {
	if(c == 403)
		return true;
	else
		return false;	
}
