#pragma once
struct CurlGlobal
{
    CurlGlobal();
    ~CurlGlobal();
    void link();
};

/*static*/ extern CurlGlobal g_curlGlobal;
#define LOAD_CURL static void* CURL_PTR = &g_curlGlobal;