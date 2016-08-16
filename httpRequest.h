

#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H


#include <stdint.h>


#define URLMAXSIZE      (500)
#define METHOD_GET      "GET"
#define METHOD_POST     "POST"
#define HTTP_VERSION    "HTTP/1.1"
#define CRCF            "\r\n"

struct parsedUrl
{
    char host[URLMAXSIZE/2];
    char ipAddr[URLMAXSIZE/2];
    char port[8];
    char path[URLMAXSIZE/2];
};

struct httpHeader
{
    char name[URLMAXSIZE/2];
    char value[URLMAXSIZE/2];
};


int parserUrl(const char *url, struct parsedUrl *myParsedUrl);

int httpHeaderInsert(struct httpHeader *header, const char name[], const char value[]);

int httpRequest(char *msg, int msgLength, uint8_t method, const char *path ,struct httpHeader *header, uint16_t headNum, const char *body, int bodyLength);

#endif
