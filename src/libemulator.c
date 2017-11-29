#include <errno.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "lib8080.h"
#include "libemulator.h"
#include "libheap.h"

char *AllPages[32768];
int PageNum = 0;
extern int MemUsed;

int GetRegister(char operand, int start, int end){
	char himask = (char) pow(2, (char)start+1)-1;
	operand &= himask;
	operand = operand >> end;
	return operand;
}
short Concatenate(char high, char low, struct Process* Pinfo){
	return (Pinfo->registers[high] << 8) | Pinfo->registers[low];
}


//char Find(

Process* NewProcess(char *Program, int Binary){
	if(Binary == 1){
			//copy program
		FILE *ProgramFile = fopen(Program, "rb");
			//get program size;
		
	}
}
int Initialize(){
	BreakHead = calloc(MEM_SIZE, 1);
	if(BreakHead == NULL)
		return -1;
		Pages = PageAlloc();
	if(Pages == NULL){
		return -2;
	}
	

	Processes = PageAlloc();
	if(Processes == NULL){
		return -3;
	}
		
	return 1;
}

void* PageAlloc(){
		/*start at beginning*/
	MemUsed += sizeof(Page) + 4096;
	char *Cursor = BreakHead;
	Page *Header = (Page *)BreakHead;
	while(Cursor < MEM_LIMIT){
		if(Header->Type == FREE){
			Header->Type = 1;
			break;
		}
		Cursor += 4096 + sizeof(Page);
		Header = (Page *)Cursor;
	}

	if(Cursor >= MEM_LIMIT){
		return NULL;
	}
	AllPages[PageNum++] = Cursor;
	return Cursor;
}

void* PageDealloc(void *PagePtr){
	MemUsed -= sizeof(Page) + 4096;
	Page *Header = (Page *)PagePtr;
	memset(Header, 0, sizeof(Page) + 4096);
	Header->Type = FREE;
	return PagePtr;
}

void* PageTableAlloc(){
		/*Page cursor*/
	Page *Cursor = (Page*) Pages;	
	MemUsed += sizeof(PageTable);
		/*Intra-Page cursor*/
	int offset = 0;
	Page *Header = (Page*) Pages;
	PageTable *PageTableHeader = (PageTable*)(Cursor + 1);

	while(1){
		
			//End of Page
		PageTable *PageTableHeader = (PageTable*)(((char*)(Cursor + 1)) + offset);
		if(offset >= 4096){
			if(Header->Next == NULL){
				Header->Next = PageAlloc();
				if(Header->Next == NULL){
					break;
				}
			}
			offset = 0;
			Cursor = (Page*) Header->Next;
			Header = (Page*) Header->Next;
		}
			//Found Spot, fits
		else if(PageTableHeader->Type == FREE && offset <= (4096 - sizeof(HighPageTable))){
			PageTableHeader->Type = PAGE_TABLE;
			PageTableHeader->Pages[0] = PageAlloc();
			return PageTableHeader;

			//Found spot, but not enough space
		}else if(PageTableHeader->Type == FREE && offset > (4096 - sizeof(HighPageTable))){
			if(Header->Next == NULL){
				Header->Next = PageAlloc();
				if(Header->Next == NULL){
					break;
				}
			}
			offset = sizeof(Page);
			Cursor = (Page*) Header->Next;
			Header = (Page*) Header->Next;
			//Iterate
		}else if(!(PageTableHeader->Type == FREE)){
			if(PageTableHeader->Type == PAGE_TABLE){
				offset += sizeof(PageTable);
			}else if(PageTableHeader->Type == HIGH_PAGE_TABLE){
				offset += sizeof(HighPageTable);
			}
		}
	}
	return NULL;
}

void* PageTableDealloc(void *PageTablePtr){
	PageTable *Header = (PageTable*) PageTablePtr;
	MemUsed -= sizeof(PageTable);
	for(int i = 0; (i < 16) && Header->Pages[i] != NULL; i++){
		PageDealloc(Header->Pages[i]);
	}
	memset(Header, 0, sizeof(PageTable));
	return PageTablePtr;
}

void* HighPageTableAlloc(){
		/*Page cursor*/
	Page *Cursor = (Page*) Pages;	
	MemUsed += sizeof(HighPageTable);

		/*Intra-Page cursor*/
	int offset = 0;
	Page *Header = (Page*) Pages;
	HighPageTable *HighPageTableHeader = (HighPageTable*)(((char*)(Cursor + 1)) + offset);
	while(1){
		
			//End of Page
		HighPageTableHeader = (HighPageTable*)(((char*)(Cursor + 1)) + offset);
		if(offset >= 4096){
			if(Header->Next == NULL){
				Header->Next = PageAlloc();
				if(Header->Next == NULL){
					break;
				}
			}
			offset = 0;
			Cursor = (Page*) Header->Next;
			Header = (Page*) Header->Next;
		}
			//Found Spot, fits
		else if(HighPageTableHeader->Type == FREE && offset <= (4096 - sizeof(HighPageTable))){
			HighPageTableHeader->Type = HIGH_PAGE_TABLE;
			HighPageTableHeader->PageTables[0] = PageTableAlloc();
			return HighPageTableHeader;

			//Found spot, but not enough space
		}else if(HighPageTableHeader->Type == FREE && offset > (4096 - sizeof(HighPageTable))){
			if(Header->Next == NULL){
				Header->Next = PageAlloc();
				if(Header->Next == NULL){
					break;
				}
			}
			offset = sizeof(Page);
			Cursor = (Page*) Header->Next;
			Header = (Page*) Header->Next;
			//Iterate
		}else if(!(HighPageTableHeader->Type == FREE)){
			if(HighPageTableHeader->Type == PAGE_TABLE){
				offset += sizeof(PageTable);
			}else if(HighPageTableHeader->Type == HIGH_PAGE_TABLE){
				offset += sizeof(HighPageTable);
			}
		}
	}
	return NULL;
}

void* HighPageTableDealloc(void *HighPageTablePtr){
	MemUsed -= sizeof(HighPageTable);
	HighPageTable *Header = (HighPageTable *)HighPageTablePtr;
	for(int i = 0; (i < 16) && Header->PageTables[i] != NULL; i++){
		PageTableDealloc(Header->PageTables[i]);
	}
	memset(Header, 0, sizeof(HighPageTable));
	return Header;
}

void* ProcessAlloc(){
	Page *Cursor = (Page*) PageAlloc();

	if(Cursor == NULL)
		return Cursor;	

	Process *Header = (Process *)Cursor;

	Header->free = !FREE;
	Header->pc = 0;
	Header->sp = 0x0f;
	memset(Header->registers, 0, sizeof(unsigned char) * 9);
	Header->PageTables = (HighPageTable*) HighPageTableAlloc();
	
	if(Header->PageTables == NULL)
		return NULL;
	char InPath[80] = {0};
	char OutPath[80] = {0};

	Header->Pid = ProcessId++;
	
	sprintf(InPath, "/tmp/%d-In.fifo", Header->Pid);
	sprintf(OutPath, "/tmp/%d-Out.fifo", Header->Pid);
	
	mkfifo(InPath, 0777);
	mkfifo(OutPath, 0777);
		//IO fifo queues
	Header->In = open(InPath, O_RDWR | O_NONBLOCK);
	if(Header->In == -1){
		printf("Creation error: IN: %s\n", strerror(errno));
		ProcessDealloc(Header);
		return NULL;
	}
	Header->Out = open(OutPath, O_RDWR | O_NONBLOCK);
	if(Header->Out == -1){
		printf("Creation error: OUT: %s\n", strerror(errno));
		ProcessDealloc(Header);
		return NULL;
	}

	return Header;	
}

void *ProcessDealloc(void* ProcessPtr){
	Process* Header = (Process*) ProcessPtr;
	HighPageTableDealloc(Header->PageTables);
	memset(Header, 0, sizeof(Page));
	return ProcessPtr;
}

/*Memory Access functions*/

Page* GetPage(short address, Process *p){
	if((p->PageTables->PageTables[p->bank]) == NULL){
		p->PageTables->PageTables[p->bank] = PageTableAlloc();
		if(p->PageTables->PageTables[p->bank] == NULL)
			return NULL;
	}

	int PageNum = address/4096;
	
	if(p->PageTables->PageTables[p->bank]->Pages[PageNum] == NULL){
		p->PageTables->PageTables[p->bank]->Pages[PageNum] = PageAlloc();
		if(p->PageTables->PageTables[p->bank]->Pages[PageNum] == NULL)
			return NULL;
	}
	
	return (Page *)p->PageTables->PageTables[p->bank]->Pages[PageNum];
}

int SetMemory(char value, unsigned short address, Process *p){
	Page *MyPage = GetPage(address, p);

		//TODO: Figure out Pointer arithmetic to offset page
	char *MemoryPtr = (char*)(MyPage + 1);
	if(MyPage == NULL)
		return 0;
	
	int _address = (address % 4096);
	MemoryPtr[_address] = value;
	return 1;
}

char GetMemory(unsigned short address, Process *p){
	Page *MyPage = GetPage(address, p);
	if(MyPage == NULL)
		return 0;
	char *MemoryPtr = (char *)(MyPage + 1);
		//TODO: Add better error handling
		//TODO: FIgure out Pointer arithmetic to offset page
	int _address = (address % 4096);

	return MemoryPtr[_address];
}

Process *CreateProcess(char *Program, char mode){
	Process *p = (Process *)ProcessAlloc();
	if(p == NULL)
		return NULL;
	AddToProcessTable(p);
	
	FILE *FProgram = NULL;
	if(mode == MODE_BINARY){
		FProgram = fopen(Program, "rb");
		if(FProgram == NULL){
			return NULL;
		}
		unsigned short i = 0;
		char c = 0;
		while((c = fgetc(FProgram)) != EOF && i < 0xffff){
			if(c != '\0'){
				SetMemory(c, i, p);
			}
			i++;
		}
	}else if(mode == MODE_HEX){
		FProgram = fopen(Program, "r");
		if(FProgram == NULL){
			return NULL;
		}
		unsigned short i = 0;
		while(!feof(FProgram)){
			char Octet[2] = {'\0', '\0'};
			fscanf(FProgram, "%c%c", &(Octet[0]), &(Octet[1]));
			if(Octet[0] == '\n' || (Octet[0] == '\r' && Octet[1] == '\n'))
				break;
			char Byte = (char)strtol(Octet, NULL, 16);
			if(Byte != '\0'){
				SetMemory(Byte, i, p);
			}
			i++;
		}
	}else{
		return NULL;
	}
	fclose(FProgram);
	return p;
}


void AddToProcessTable(Process *p){
	int Index = p->Pid % 127;
	if(ProcessTable[Index] == NULL){
		ProcessTable[Index] = malloc(sizeof(ProcessBucket));
		ProcessTable[Index]->Value = p;
	}else{
		ProcessBucket *Cursor = ProcessTable[Index];
		while(Cursor->Next != NULL){
			Cursor = Cursor->Next;
		}
		Cursor->Next = malloc(sizeof(ProcessBucket));
		Cursor->Next->Value = p;
	}
}

Process *FindInProcessTable(int pid){
	int Index = pid % 127;
	ProcessBucket *Cursor = ProcessTable[Index];
	if(ProcessTable[Index] == NULL){
		return NULL;
	}else{
		while(Cursor->Next != NULL){
			if(Cursor->Value->Pid == pid)
				break;
			Cursor = Cursor->Next;
		}if(Cursor->Next == NULL && Cursor->Value->Pid != pid){
			return NULL;
		}else{
			return Cursor->Value;
		}
	}
}

