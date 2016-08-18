/*
    Adms Bulletin Board System
    Copyright (C) 2013, Mo Norman, LTaoist6@gmail.com

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 1, or (at your option)
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
*/

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include "curlbuild32.h"
#include <curl/curl.h>
#include "libSystem.h"

/* For bbs_singal */
#define BUF_MAX_SIZE 512
#define BBSS_PREFIX "http://localhost:5000/_hfb"

int BBS_SINGAL(const char *s, ...)
{
    char *fields=NULL;
    char *value=NULL;
    CURL *curl;
    CURLcode res;
    struct curl_httppost *formpost=NULL;
    struct curl_httppost *lastptr=NULL;

    char host[BUF_MAX_SIZE] = BBSS_PREFIX;
    char buf[BUF_MAX_SIZE];
    char buf_err[CURL_ERROR_SIZE];
    
    va_list ap;

    va_start(ap, s);
    curl_global_init(CURL_GLOBAL_ALL);
    
    while((fields = va_arg(ap, char *))!=NULL){
        curl_formadd(&formpost,
                     &lastptr,
                     CURLFORM_COPYNAME, fields,
                     CURLFORM_COPYCONTENTS, va_arg(ap, char*),
                     CURLFORM_END);
    }
    va_end(ap);

    strncat(host, s, BUF_MAX_SIZE);
    curl = curl_easy_init();
    if(curl)
    {
        curl_easy_setopt(curl, CURLOPT_URL, host);
        curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);
        curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, buf_err);
        res = curl_easy_perform(curl);
        if(res != CURLE_OK)
        {
            curl_easy_cleanup(curl);
            curl_formfree(formpost);
            return 2;
        }
        curl_easy_cleanup(curl);
        curl_formfree(formpost);
        return 0;
    }
    return 1;
}
