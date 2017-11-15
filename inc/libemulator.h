#ifndef __LIBEMULATOR_H
#define __LIBEMULATOR_H

#include <math.h>

#define FREE 		0
#define PAGE_TABLE 	1 
#define HIGH_PAGE_TABLE 2

#define MODE_BINARY	0
#define MODE_HEX	1

#define MEM_LIMIT (BreakHead + 0x8000000)
#define MEM_SIZE 0x8000000

typedef struct _Page{
	char Type;
	char *Next;
} Page;

typedef struct _PageTable{
	char Type;
	Page *Pages[16];
} PageTable;

typedef struct _HighPageTable{
	char Type;
	PageTable *PageTables[16];
} HighPageTable;

struct Process;
typedef struct Process Process;
extern char* BreakHead;
extern char* Pages;
extern char* Processes;

	//gets bitrange between start and end, inclusive
int GetRegister(char operand, int start, int end);
short Concatenate(char high, char low, struct Process* Pinfo);

struct Process* NewProcess(char *Program, int Binary);
int Initialize();

void* PageAlloc();
void* PageDealloc(void* PagePtr);

void* PageTableAlloc();
void* PageTableDealloc(void* PageTablePtr);

void* HighPageTableAlloc();
void* HighPageTableDealloc(void* HighPageTablePtr);

void* ProcessAlloc();
void* ProcessDealloc(void* ProcessPtr);

Page* GetPage(short address, Process *p);
int SetMemory(char value, unsigned short address, Process *p);
char GetMemory(unsigned short address, Process *p);
Process *CreateProcess(char *Program, char mode);
#endif
