#ifndef __LIBHTTP_H
#define __LIBHTTP_H

void GenerateSession(char **Destination);
void SetResponseHeader(char *Destination, char *Header, char *Value);
void SetReasonCode(char *Destination, int Code);
void GetFile(char *Path, int ServerFd);
int GetFileContents(char *Path, char **Destination, int *Size);
void GetServerTime(char *Destination);
void FormatDate(long int Date, char *Destination);

#endif
