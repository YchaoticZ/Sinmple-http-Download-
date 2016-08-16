

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>    // bzero
#include <stdint.h>     // uint32_t
#include <unistd.h>
#include <pthread.h>
#include <sys/select.h>   
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/time.h>

#include "httpRequest.h"
#include "picohttpparser.h"


#define MAXSIZE             2048 


void *thread_main(void *argv);
int getFileNmae(const char str[], char *fileName);
int saveFiles(const char *fileName, uint8_t buffer[], uint32_t bufferLength);
int ContentLength(struct phr_header *headers, int headNum);
int downloadFiles(const char *fileName, int sockeFd);


int main(int argc, char *argv[])
{
    int    cliFd;
    struct parsedUrl myParsedUrl;
    pthread_t threadID[2];
    /* URL解析 */
    if (argc > 1)
    {
        parserUrl(argv[1], &myParsedUrl);
    }
    else
    {
        printf ("Error, None URL\r\n");
        return 1;
    }
    pthread_create(&threadID[0], NULL, thread_main, (void*)argv[1]);
    pthread_create(&threadID[1], NULL, thread_main, (void*)argv[1]);
    pthread_join(threadID[0], NULL);
    pthread_join(threadID[1], NULL);
    
    return 0;
}

void *thread_main(void *argv)
{

    int    cliFd;
    struct parsedUrl myParsedUrl;
    char   urlStr[128];
    pthread_t myThreadID;
    //size_t argv_length;
    char g_sendData[MAXSIZE];
    
    //argv_length = strlen(argv);
    strcpy(urlStr, argv);
    /* URL解析 */
    parserUrl(urlStr, &myParsedUrl);

    /* 创建套接字 */
    cliFd = socket(AF_INET, SOCK_STREAM, 0);
    if(cliFd < 0)
    {
        printf ("Create Client Socket File Descriptor Error\r\n");
        //return 1;
        pthread_exit(1);
    }
    printf ("Create Client Socket Success\r\n");

    /* 连接服务器 */
    struct sockaddr_in serveAddr;
    uint32_t addrIP;
    uint16_t port;
	
    if (inet_pton(AF_INET, myParsedUrl.ipAddr, &addrIP) != 1)
    {
        printf ("Convet IP addr Failed\r\n");
        //return 1;
        pthread_exit(1);
    }
    port = atoi(myParsedUrl.port);
    bzero (&serveAddr, sizeof(serveAddr));
    serveAddr.sin_family = AF_INET;
    serveAddr.sin_addr.s_addr = addrIP; 
    serveAddr.sin_port = htons(port);
    if (connect (cliFd, (struct sockaddr *)&serveAddr, sizeof(serveAddr)) < 0)
    {
        printf ("Connect Serve Error\r\n");
        //return 1;
        pthread_exit(1);
    }
    printf ("Connect Serve Success\r\n");

    ssize_t writeRetn, readRetn;
    /* HTTP Request Header  */
    struct httpHeader myHttpHeader[10];
    int httpHeaderNum = 0;
	
    httpHeaderInsert(&myHttpHeader[0], "Accept","text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8");
    httpHeaderNum++;
    httpHeaderInsert(&myHttpHeader[1], "User-Agent","Mozilla/5.0 (X11; Ubuntu; Linux i686; rv:31.0) Gecko/20100101 Firefox/31.0");
    httpHeaderNum++;
    httpHeaderInsert(&myHttpHeader[2], "Host", myParsedUrl.host);
    httpHeaderNum++;
    httpHeaderInsert(&myHttpHeader[3], "Connection","keep-alive");
    httpHeaderNum++;

    httpRequest(g_sendData, MAXSIZE, 0, myParsedUrl.path, myHttpHeader, httpHeaderNum, NULL, 0);
    printf ("Send Http Request:\r\n%s", g_sendData);
    writeRetn = write (cliFd, g_sendData, strlen(g_sendData) + 1);
    if (writeRetn > 0)
    {   
        char fileName [64];
        if (getFileNmae(myParsedUrl.path, fileName) == -1)
        {
            printf ("File Name Error\r\n");
            pthread_exit(-1);
           
        }

        if (downloadFiles(fileName, cliFd) == 0)
        {
            printf ("%s File Download Success\r\n", fileName);
            shutdown(cliFd, 0);
			pthread_exit(0);
            

        }
        else
        {
            remove(fileName);
            printf ("%s File Download Error\r\n", fileName);
            shutdown(cliFd, 0);
            pthread_exit(-1);
            
        }
    }
}

int getFileNmae(const char str[], char *fileName)
{
    int i, j;
    pthread_t myThreadID;
    uint32_t convetThreadID;

    myThreadID = pthread_self();
    convetThreadID = myThreadID;
    sprintf (fileName, "%lu_", convetThreadID);
#if 0 
    printf (fileName);

#endif
    for (i = 0;str[i] != '\0';i++)
    {
        if (str[i] == '/')
        {
            j = i;
        }
    }

    if (j + 1== i)
    {
        fileName = NULL;
        return -1;
    }

    strcat(fileName, str + j + 1);

    return 0;
}

int saveFiles(const char *fileName, uint8_t buffer[], uint32_t bufferLength)
{
	FILE *myFile;
    myFile = fopen(fileName, "a");
    if (NULL == myFile)
    {
        return -1;
    }
    fwrite(buffer, bufferLength, 1, myFile);
    fclose(myFile);

    return 0;
}

int ContentLength(struct phr_header *headers, int headNum)
{
    uint32_t length = 0;
    char convetChar[20];
    int i;

    if (NULL == headers || 0 == headNum)
    {
        return -1;
    }
    for (i = 0;i < headNum;i++)
    {
        if (memcmp(headers[i].name, "Content-Length", headers[i].name_len) == 0)
        {
            memcpy(convetChar, headers[i].value, headers[i].value_len);
            convetChar[headers[i].value_len] = '\0';
            length = atoi(convetChar);

            return length;
        }
    }

    return -1;
}

int downloadFiles(const char *fileName, int sockeFd)
{
    
    size_t num_headers = 20;
    struct phr_header headers[20];
    char g_recvData[MAXSIZE];

    /* http */
    int recvLength, retnValue = 0, flag = 0;
    /* http header Parser */ 
    int recvParserLength = 0, minor_version, status, parseResRetn;
    /* http body */
    uint32_t allByteLength;
    uint32_t recvBodyLength = 0;
    

    /* select */
    int selectRetnValue = 0;
    fd_set rset;
    struct timeval tv;

    FD_ZERO(&rset);   
    FD_SET(sockeFd, &rset);   

    for (;;)
    {
        tv.tv_sec = 10;
        tv.tv_usec = 0;
        selectRetnValue = select(sockeFd + 1, &rset, NULL, NULL, &tv);

        if (0 == selectRetnValue)
        {
            fprintf (stderr, "Select Time Out\r\n");
            retnValue = -1;
            break;
        }
        else if (selectRetnValue < 0)
        {
            fprintf (stderr, "Select Function Error\r\n");
            retnValue = -1;
            break;
        }
        else
        {
            recvLength = read (sockeFd, g_recvData, MAXSIZE);

            if (recvLength > 0 && 0 == flag)
            {
                for (int i = 0;i < recvLength;i++)
                {
                    printf ("%c", g_recvData[recvParserLength + i]);
                }
                printf ("Parsering Http Response Header\r\n");
                recvParserLength += recvLength;
                parseResRetn = phr_parse_status_headers(g_recvData, recvParserLength, &minor_version, 
                                                        &status, headers, &num_headers, 1);
                if (parseResRetn > 0)
                {
                    int mycontentLength = 0;

                    printf ("Parser Response Done\r\n");

                    if (status == 200)
                    {
                        if ((mycontentLength = ContentLength(headers, num_headers)) > 0)
                        {
                            allByteLength = mycontentLength;
                            flag = 1;

                            printf ("Start Download File****\r\n");

                            if (saveFiles(fileName, g_recvData + parseResRetn, recvParserLength - parseResRetn ) == -1)
                            {
                                return -1;
                            }
                            recvBodyLength += recvParserLength - parseResRetn ;
							continue;
                        }
                    }
                    printf ("Error Status: %d, content length:%d\r\n", status, mycontentLength);

                    return -1;
                }
                else if (parseResRetn == -2)
                {
                    printf ("Continue Read\r\n");
                }
                else
                {
                    printf ("Parser Error\r\n");

                    return -1;
                }
            }
            else if (recvLength > 0 && 1 == flag)
            {
              	recvBodyLength += recvLength;
                if (saveFiles(fileName, g_recvData, recvLength) == -1)
                {
                    return -1;
                }
			//	printf ("Download File %d / %d Bytes\r\n", recvBodyLength, allByteLength);
                if (recvBodyLength >= allByteLength)
                {
                    return 0;
                }
            }
        }
    }
    return retnValue;
}
