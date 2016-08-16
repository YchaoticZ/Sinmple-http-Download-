OBJECT=picohttpparser.o \
       httpRequest.o \
	   url.o\
	   multiThreadTCPClient.o\

EXECUTABLE= multiClient\

CFLAGS= -std=c99 -g -Wall
CC=gcc

all:$(EXECUTABLE)

multiClient:picohttpparser.o url.o httpRequest.o multiThreadTCPClient.o 
	$(CC) $(CFLAGS) -o $@ $^ -pthread


picohttpparser.o:picohttpparser.c picohttpparser.h 
	$(CC) $(CFLAGS) -c $<  
httpRequest.o:httpRequest.c httpRequest.h url.h
	$(CC) $(CFLAGS) -c $<  
multiThreadTCPClient.o:multiThreadTCPClient.c 
	$(CC) $(CFLAGS) -c $< -pthread
url.o:url.c url.h
	$(CC) $(CFLAGS) -c $<

.PHONY:clean
clean:
	rm -r $(OBJECT) $(EXECUTABLE) 
