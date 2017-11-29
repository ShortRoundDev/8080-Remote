#ifndef __LIBEMUNET_H
#define __LIBEMUNET_H

void *Listen();
void *HandleRequest(void *_ServerFd);
char *GetEndpoint(char *Request, char *Method, char *Destination, char *Params);
void NotFound(int ServerFd, char *ServerTime);
void Register(int ServerFd, char *ServerTime, char *Method, char *Params);
void Login(int ServerFd, char *ServerTime, char *Method, char *Params);
void LoginUser(int ServerFd);
void AddSession(char *SessionId, time_t Expiration);
int ValidateSession(char *SessionId);
void GetCookies(char *Destination, char *Request);
void Terminal(int ServerFd, char *Request);
void ReadProcessStream(int ServerFd, char *Args, char *ServerTime);
#endif
