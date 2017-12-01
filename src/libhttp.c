#include <libhttp.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/socket.h>

void GenerateSession(char **Destination){
	int URand = open("/dev/urandom", O_RDONLY);
	*Destination = calloc(17, 1);
	read(URand, *Destination, 16);
}

void SetResponseHeader(char *Destination, char *Header, char *Value){
	strcat(Destination, Header);
	strcat(Destination, ": ");
	strcat(Destination, Value);
	strcat(Destination, "\r\n");
}

void SetReasonCode(char *Destination, int Code){
	strcat(Destination, "HTTP/1.1 ");
	char ReasonPhrase[80];
	switch(Code){
		case 200:
			strcat(ReasonPhrase, "OK");
			break;
		case 201:
			strcat(ReasonPhrase, "Created");
		case 400:
			strcat(ReasonPhrase, "Bad Request");
			break;
		case 401:
			strcat(ReasonPhrase, "Unauthorized");
			break;
		case 403:
			strcat(ReasonPhrase, "Forbidden");
			break;
		case 404:
			strcat(ReasonPhrase, "Not Found");
			break;
		case 408:
			strcat(ReasonPhrase, "Request Time-out");
			break;
		case 500:
			strcat(ReasonPhrase, "Internal Server Error");
			break;
		case 503:
			strcat(ReasonPhrase, "Service Unavailable");
			break;
	}
	char _Code[5];
	sprintf(_Code, "%d ", Code);
	strcat(Destination, _Code);
	strcat(Destination, ReasonPhrase);
	strcat(Destination, "\r\n");
} 

void GetFile(char *Path, int ServerFd){
	char Destination[10000];
	char *Message;
	int Size = 0;
	int err = GetFileContents(Path, &Message, &Size);
	
	if(err == 0){
		char Length[16];
		char Time[100];
		sprintf(Length, "%d", Size);
		GetServerTime(Time);

		SetReasonCode(Destination, 200);
		SetResponseHeader(Destination, "Date", Time);
		SetResponseHeader(Destination, "Server", "8080 Remote v0.1");
		SetResponseHeader(Destination, "Content-Length", Length);
		SetResponseHeader(Destination, "Connection", "Closed");
		SetResponseHeader(Destination, "Content-Type", "text/html; charset=iso-8859-1");
		strcat(Destination, "\r\n");
		strcat(Destination, Message);

		
		send(ServerFd, Destination, strlen(Destination), 0);
	}else{
		printf("Couldn't open file");
	}
	free(Message);
}

int GetFileContents(char *Path, char **Destination, int *Size){
		/*Open file*/
	FILE *Fp = fopen(Path, "r");
	if(Fp == NULL){
		return -1;
	}
	
		/*Get size*/
	fseek(Fp, 0, SEEK_END);
	unsigned long int size = ftell(Fp);
	rewind(Fp);

		/*Malloc buffer*/
	(*Destination) = malloc(size);
	char c = 0;
	for(int i = 0; i < size; i++){
		(*Destination)[i] = fgetc(Fp);
	}

		/*Exit*/
	*Size = size;
	fclose(Fp);
	return 0;
}

void GetServerTime(char *Destination){
	time_t RawTime;
	struct tm *Info;
	time(&RawTime);
	Info = gmtime(&RawTime);
	strftime(Destination, 1024, "%a, %d %b %Y %H:%M:%S GMT", Info);
}

void FormatDate(long int Date, char *Destination){
	struct tm *Info;
	Info = gmtime(&Date);
	strftime(Destination, 1024, "%a, %d %b %Y %H:%M:%S GMT", Info);
}
