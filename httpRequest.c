

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "httpRequest.h" 
#include "url.h"


#define MSGINDEX_ISOUTSIZE(INDEX,ALLSIZE) \
        do                                \
        {                                 \
            if (INDEX > ALLSIZE)          \
            {                             \
                return -1;                \
            }                             \
        }while(0)                         



int parserUrl(const char *url, struct parsedUrl *myParsedUrl)
{
    url_field_t *myUrlData;
    struct hostent *destination;

    myUrlData = url_parse (url);
    if (myUrlData)
    {
        if (myUrlData->host)
        {
            strcpy(myParsedUrl->host, myUrlData->host); 
            
            /* 判断 */
            if (HOST_IPV4 != myUrlData->host_type)
            {
                destination = gethostbyname(myUrlData->host);
                inet_ntop(AF_INET, destination->h_addr_list[0], myParsedUrl->ipAddr, INET_ADDRSTRLEN);
            }
            else
            {
                strcpy(myParsedUrl->ipAddr, myUrlData->host);
            }

            if (myUrlData->port)
            {
                strcpy(myParsedUrl->port, myUrlData->port);
            }
            else
            {
                strcpy(myParsedUrl->port, "80");
            }

            if (myUrlData->path)
            {
                if ('/' == myUrlData->path[0])
                {
                    strcpy(myParsedUrl->path, myUrlData->path);
                }
                else
                {
                    myParsedUrl->path[0] = '/';
                    strcpy(myParsedUrl->path + 1, myUrlData->path);
                }

            }
            else
            {
                strcpy(myParsedUrl->path, "/");
            }
            url_free (myUrlData);
            return 0;
        }
        url_free (myUrlData);
        return -1;
    } 
    
    return -1;
}


int httpHeaderInsert(struct httpHeader *header, const char name[], const char value[])
{
    strcpy(header->name, name);
    strcpy(header->value, value);
    return 0;
}


int httpRequest(char *msg, int msgLength, uint8_t method, const char *path ,struct httpHeader *header, uint16_t headNum, const char *body, int bodyLength)
{
     int msgIndex = 0;

    /* request line */
    if (0 ==method)
    {
        msgIndex += strlen(METHOD_GET) + 1;
        MSGINDEX_ISOUTSIZE(msgIndex,msgLength);
        memcpy(msg, METHOD_GET, strlen(METHOD_GET));
        
    }
    else if (1 == method)
    {
        msgIndex += strlen(METHOD_POST) + 1;
        MSGINDEX_ISOUTSIZE(msgIndex,msgLength);
        memcpy(msg, METHOD_POST, strlen(METHOD_POST));
        
    }
    else
    {
        return -2;
    }
    msgIndex++;
    MSGINDEX_ISOUTSIZE(msgIndex, msgLength);
    strcat(msg, " ");
    
    if(NULL != path)
    {
        msgIndex += strlen(path);
        MSGINDEX_ISOUTSIZE(msgIndex,msgLength);
        strcat(msg, path);
        msgIndex++;
        MSGINDEX_ISOUTSIZE(msgIndex,msgLength);
        strcat(msg, " ");
    }
    msgIndex += strlen(HTTP_VERSION);
    MSGINDEX_ISOUTSIZE(msgIndex,msgLength);
    strcat(msg, HTTP_VERSION);

    msgIndex += strlen(CRCF);
    MSGINDEX_ISOUTSIZE(msgIndex,msgLength);
    strcat(msg,CRCF);


    /* headers */
    if (header && headNum > 0)
    {
        for (uint16_t idx = 0;idx < headNum;idx++)
        {
            msgIndex += strlen(header[idx].name);
            MSGINDEX_ISOUTSIZE(msgIndex,msgLength);
            strcat(msg, header[idx].name);

            msgIndex += strlen(": ");
            MSGINDEX_ISOUTSIZE(msgIndex,msgLength);
            strcat(msg, ": ");

            msgIndex += strlen(header[idx].value);
            MSGINDEX_ISOUTSIZE(msgIndex,msgLength);
            strcat(msg, header[idx].value);
           
            msgIndex += strlen(CRCF);
            MSGINDEX_ISOUTSIZE(msgIndex,msgLength);
            strcat(msg, CRCF);
        }
        msgIndex += strlen(CRCF);
        MSGINDEX_ISOUTSIZE(msgIndex,msgLength);
        strcat(msg, CRCF);
    }
    else
    {
        return -2;
    }

    /* body */
    if (body && bodyLength > 0)
    {
        msgIndex += bodyLength;
        MSGINDEX_ISOUTSIZE(msgIndex,msgLength);
        msgIndex -= bodyLength;
        memcpy(msg + msgIndex, body, bodyLength);
    }
    else if (NULL == body && bodyLength > 0)
    {
        return -2;
    }

    return 0;
}
