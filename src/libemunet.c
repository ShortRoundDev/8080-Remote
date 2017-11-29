#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>
#include <crypt.h>
#include <sys/ioctl.h>
#include <math.h>

#include "lib8080.h"
#include "libemunet.h"
#include "libhttp.h"
#include "libemulator.h"

#define PORT 7070

void *Listen(){

	int ServerFd, NewSocket;
	struct sockaddr_in Address;
	int Opt = 1;
	int AddrLen = sizeof(Address);
	char *Hello = "Hello from server!";
	if((ServerFd = socket(AF_INET, SOCK_STREAM, 0)) == 0){
		perror("Socket failed\n");
		exit(1);
	}
	
	if(setsockopt(ServerFd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &Opt, sizeof(Opt))){
		perror("Set Sock Opt\n");
		exit(1);
	}
	Address.sin_family 	= AF_INET;
	Address.sin_addr.s_addr = INADDR_ANY;
	Address.sin_port = htons(PORT);

	if(bind(ServerFd, (struct sockaddr *)&Address, sizeof(Address)) < 0){
		perror("Bind failure\n");
		exit(1);
	}

	if(listen(ServerFd, 3) < 0){
		perror("Listen\n");
		exit(1);
	}
	
	while(1){
		if((NewSocket = accept(ServerFd, (struct sockaddr *)&Address, (socklen_t*)&AddrLen)) < 0){
			perror("Accept\n");
			exit(1);
		}
		pthread_t HandleThread;
		pthread_create(&HandleThread, NULL, HandleRequest, (void *)&NewSocket);
		pthread_detach(HandleThread);
	}

	return 0;
}

void *HandleRequest(void *_ServerFd){
	int ValRead;
	char Buffer[10000] = {0};
	int ServerFd = *((int*)_ServerFd);
	char ServerTime[1024] = {0};
	time_t RawTime;
	struct tm *Info;
	time(&RawTime);

	Info = gmtime(&RawTime);
	strftime(ServerTime, 1024, "%a, %d %b %Y %H:%M:%S GMT", Info);
	
	ValRead = read(ServerFd, Buffer, 10000);
	printf("BufferLength: %ld\n",strlen(Buffer));
	printf("-----\n%s\n-----\n", Buffer);
	char Method[16] = {0};
	char Endpoint[16] = {0};
	char Args[128] = {0};
	char *Cookies = malloc(5000);
		printf("root\n");
	GetCookies(Cookies, Buffer);
	GetEndpoint(Buffer, Method, Endpoint, Args);

	if(!strcmp(Endpoint, "/")){
		GetFile("html/index.html", ServerFd);
		//Root(ServerFd, ServerTime);
	}else if(!strcmp(Method, "POST") && !strcmp(Endpoint, "/Login")){
		Login(ServerFd, ServerTime, Method, Args);	
	}else if(!strcmp(Method, "POST") && !strcmp(Endpoint, "/Register")){
		Register(ServerFd, ServerTime, Method, Args);
	}else if(!strcmp(Method, "GET") && !strcmp(Endpoint, "/Terminal")){
		printf("Getting Terminal\n");
		if(!strcmp(Cookies, "Not Found!")){
			printf("Couldn't Find Cookie\n");
			NotFound(ServerFd, ServerTime);
		}else{
			printf("Fetching Terminal\n");
			Terminal(ServerFd, Cookies);
		}
	}else if(!strcmp(Method, "GET") && !strcmp(Endpoint, "/ProcessStream")){
		ReadProcessStream(ServerFd, Args, ServerTime);
	}else{
		NotFound(ServerFd, ServerTime);
	}
	free(Cookies);
	close(ServerFd);
	return NULL;
}

char *GetEndpoint(char *Request, char *Method, char *Destination, char *Params){
	char *Buffer = malloc(strlen(Request));

	strncpy(Buffer, Request, strlen(Request));
	
	char *_Method 		= strtok(Buffer, " ");
	char *_Destination 	= strtok(NULL, " ?");
	
	strncpy(Destination, 	_Destination, 	strlen(_Destination));
	strncpy(Method, 	_Method, 	strlen(_Method));

	if(!strcmp(Method, "POST")){
		char *_Params = strstr(Request, "\r\n\r\n") + 4;
		strncpy(Params, _Params, strlen(_Params));

	}else if(!strcmp(Method, "GET")){
		char *_Params = strtok(NULL, " ");
		strncpy(Params, _Params, strlen(_Params));
	}

	free(Buffer);
	return NULL;
}

void Register(int ServerFd, char *ServerTime, char *Method, char *Params){
	FILE *Passwords = fopen("server/passwords", "r");
	if(Passwords == NULL){
		printf("couldnt open passwords\n");
		return;
	}
	char Line[256] = {0};
	char *User;
	
	char *Username = strtok(Params, ";");
	char *Password = Username + 2;
	if(Password == NULL || Username == NULL){
		printf("no password\n");
		NotFound(ServerFd, ServerTime);
		return;
	}

	int AlreadyRegistered = 0;
	do{
		fscanf(Passwords, "%s\n", Line);
//		fscanf(Passwords, "%s\n", User);
		User = strtok(Line, "=");
		printf("User: |%s|\nUsername: |%s|\n", User, Username);
		if(!strcmp(User, Username)){
			AlreadyRegistered = 1;
			break;
		}
	}while(!feof(Passwords));

	fclose(Passwords);
	if(AlreadyRegistered){
		printf("Already registered\n");
		NotFound(ServerFd, ServerTime);
		return;
	}
	Passwords = fopen("server/passwords", "a");
	
	time_t CurrentTime = time(NULL);
	
	char Salt[32] = {0};
	sprintf(Salt, "$1$%ld", CurrentTime);
	char *Hash = crypt(Password, Salt);
	fprintf(Passwords, "%s=%s\n", Username, Hash);
	fclose(Passwords);
	return;
}

void Login(int ServerFd, char *ServerTime, char *Method, char *Params){
	FILE *Passwords = fopen("server/passwords", "r");
	if(Passwords == NULL){
		printf("couldnt open passwords\n");
		return;
	}
	printf("Trying to log in...\n");
	char Line[256] = {0};
	char *User;
	
	char *Username = strtok(Params + strlen("LoginString="), ";");
	if(Username == NULL){
		printf("No username\n");
		NotFound(ServerFd, ServerTime);
		return;
	}
	char *Password = Username + strlen(Username) + 1;
	if(Password == NULL){
		printf("no password\n");
		NotFound(ServerFd, ServerTime);
		return;
	}

	int AlreadyRegistered = 0;
	do{
		fscanf(Passwords, "%s\n", Line);
//		fscanf(Passwords, "%s\n", User);
		User = strtok(Line, "=");
		if(!strcmp(User, Username)){
			AlreadyRegistered = 1;
			break;
		}
	}while(!feof(Passwords));

	fclose(Passwords);
	if(!AlreadyRegistered){
		printf("Not registered\n");
		NotFound(ServerFd, ServerTime);
		return;
	}

	char *Salt = User + strlen(User) + 1;
	char *Hash = Salt + 12;//strtok(Hash + 4, "$");
	Salt[11] = 0;
//	Hash = Salt + strlen(Salt) + 1;

	char *NewHash = crypt(Password, Salt);
	if(!strcmp(Hash, NewHash + 12))
		LoginUser(ServerFd);		
	else
		NotFound(ServerFd, ServerTime);

}

void LoginUser(int ServerFd){
	char Buffer[10000];
	char Time[100];
	GetServerTime(Time);
	SetReasonCode(Buffer, 200);

	char *SessionId;
	printf("Generating SID...");
	GenerateSession(&SessionId);
	long int Expiration_t = time(NULL) + 7200;
	printf("...done\n");
	char SessionIdHex[34] = {0};
	for(int i = 0; i < 16; i++){
		char HexTmp[3];
		sprintf(HexTmp, "%02x", (unsigned char)SessionId[i]);
		strcat(SessionIdHex, HexTmp);
	}
	printf("%s\n", SessionIdHex);
	AddSession(SessionIdHex, Expiration_t);

	char ExpirationString[128] = {0};
	FormatDate(Expiration_t, ExpirationString);
	char SessionCookie[1024] = {0};

	sprintf(SessionCookie, "SessionId=%s; Expires=%s", SessionIdHex, ExpirationString);
	printf("Cookie::: %s\n", SessionCookie);

	SetResponseHeader(Buffer, "Date", Time);
	SetResponseHeader(Buffer, "Server", "8080 Remote v0.1");
	SetResponseHeader(Buffer, "Content-Length", "1");
	SetResponseHeader(Buffer, "Set-Cookie", SessionCookie);
	strcat(Buffer, "\r\n");
	strcat(Buffer, "1");
	send(ServerFd, Buffer, strlen(Buffer), 0);
	printf("Logged in:\n---------\n%s\n----------\n", Buffer);
}

void AddSession(char *SessionId, time_t Expiration){
	FILE *Sessions = fopen("server/sessions", "a");
	fprintf(Sessions, "%s | %ld\n", SessionId, Expiration);
	fclose(Sessions);
}


void NotFound(int ServerFd, char *ServerTime){

	char Pong[10000] = {0};
	sprintf(Pong, "HTTP/1.1 404 Not Found\r\nDate: %s\r\nServer: 8080 Remote v0.1\r\nContent-Length: 13\r\nConnection:Closed\r\nContent-Type text/html; charset=iso-8859\r\n\r\n404 Not Found", ServerTime);
	send(ServerFd, Pong, strlen(Pong), 0);
}

int ValidateSession(char *SessionId){
	FILE *Sessions = fopen("server/sessions", "r");
	char _SessionId[50];
	time_t Expiration;
	char NotExpired = 0;

	while(fscanf(Sessions, "%s | %lu\n", _SessionId, &Expiration) != EOF){
		printf("Checking %s\n", _SessionId);
		if(!strcmp(SessionId, _SessionId)){
			NotExpired = 1;
			printf("Found token\n");
			if(Expiration < time(NULL)){
				printf("Expired token\n");
				NotExpired = 0;
			}break;
		}
	}
	return NotExpired;
}

void GetCookies(char *Destination, char *Request){
	char *Tmp = malloc(strlen(Request));
	strcpy(Tmp, Request);
	char *CookieHeader = strstr(Tmp, "Cookie: ");
	printf("finding cookies\n");
	if(CookieHeader == NULL){
		printf("not found!\n");
		strcpy(Destination, "Not Found!");
		return;
	}else{
		printf("Found cookies\n");
		strcpy(Destination, strtok(CookieHeader + 8, "\r\n"));
	}
//	free(Tmp);
}

void Terminal(int ServerFd, char *Request){

	char ServerTime[1024];
	FormatDate(time(NULL), ServerTime);

	printf("Getting Session ID\n");
	char *SessionHeader = strstr(Request, "SessionId=");

	if(SessionHeader == NULL){
		printf("no cookie provided\n");
		NotFound(ServerFd, ServerTime);
		return;

	}
	char *SessionId = strtok(SessionHeader + strlen("SessionId="), "; \r");
	printf("Successfully found Session Header\n");

	if(!ValidateSession(SessionId)){
		printf("Cannot validate\n");
		NotFound(ServerFd, ServerTime);
		return;
	}
	printf("Validated Session\n");
	GetFile("html/desktop.html", ServerFd);
}

void ReadProcessStream(int ServerFd, char *Args, char *ServerTime){
	int Pid = atoi(Args + strlen("pid="));
	Process *Pinfo = FindInProcessTable(Pid);
	if(Pinfo == NULL){
		NotFound(ServerFd, ServerTime);
		return;
	}
	char Buffer[10000];
	int BytesAvailable = 0;

	int err = ioctl(Pinfo->Out, FIONREAD, &BytesAvailable);
	BytesAvailable = fmin(BytesAvailable, 9000);
	char *Output = calloc(BytesAvailable, 1);
	
	read(Pinfo->Out, Output, BytesAvailable);
	char SizeStr[100];
	sprintf(SizeStr, "%d", BytesAvailable);
	
	SetReasonCode(Buffer, 200);
	SetResponseHeader(Buffer, "Date", ServerTime);
	SetResponseHeader(Buffer, "Server", "8080 Remote v0.1");
	SetResponseHeader(Buffer, "Content-Length", SizeStr);
	SetResponseHeader(Buffer, "Connection", "Closed");
	SetResponseHeader(Buffer, "Content-Type", "text/html; charset=iso-8859-1");
	strcat(Buffer, "\r\n");
	strcat(Buffer, Output);
	
	send(ServerFd, Buffer, strlen(Buffer), 0);
	free(Output);
}

void WriteProcessStream(int ServerFd, char *Args, char *ServerTime){
}

void GetAllPrograms(int ServerFd, char *ServerTime){
	
}
