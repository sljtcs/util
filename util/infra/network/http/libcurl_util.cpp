#include "libcurl_util.h"
#include <curl/curl.h>

CurlGlobal::CurlGlobal()
{
    curl_global_init(CURL_GLOBAL_DEFAULT);
}
CurlGlobal::~CurlGlobal()
{
    curl_global_cleanup();
}

void CurlGlobal::link(){}

CurlGlobal g_curlGlobal;