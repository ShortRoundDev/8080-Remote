#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <pthread.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>

#include "lib8080.h"
#include "libemulator.h"
#include "libemunet.h"

void *Head(void *x);

int ProcessId = 0;
int MemUsed = 0;

instruction InstructionSet[256];

char *BreakHead = NULL;
char *Pages	= NULL;
char *Processes = NULL;

int main(int argc, char** argv){
	LoadInstructionSet();

	int s = 0;
	if((s = Initialize()) == -1){
		printf("Failed to initialize address space\n\tmalloc returned -1 and set errno to:\t%d\nExiting now\n", errno);
		return -1;
	}else if(s == -2){
		printf("Page table pool failed to allocate\nExiting now\n");
		return -2;
	}else if(s == -3){
		printf("Process pool failed to allocate\nExiting now\n");
		return -3;
	}

	printf("*.....................................*\n");
	printf("| Remote 8080 OS v0.1                 |\n");
	printf("| by Collin Oswalt 2017               |\n");
	printf("| 128MiB RAM                          |\n");
	printf("| Process Pool: %p        |\n", Processes);
	printf("|    Page Pool: %p        |\n", Pages);
	printf("*.....................................*\n");

	pthread_t handle;
	if(!pthread_create(&handle, NULL, Listen, NULL)){
		printf("thread created successfully!\n");
		if(!pthread_detach(handle)){
			printf("detached successfully!\n");
		}
	}

	Process *p = ProcessAlloc();
	p->registers[R_A] = 0x6a;
	printf("6a & 1: %d\n", 0x6a & 1);
	p->registers[R_F] |= 1;
	//p->registers[R_C] = 0x05;
	InstructionSet[0x1f](0x1f, 0x00, p);
	printf("A: %d\n", p->registers[R_A]);
	printf("C: %d\n", (p->registers[R_F] & F_C) && 1);
	printf("Z: %d\n", (p->registers[R_F] & F_Z) && 1);
	printf("a: %d\n", (p->registers[R_F] & F_A) && 1);
	printf("P: %d\n", (p->registers[R_F] & F_P) && 1);
	printf("S: %d\n", (p->registers[R_F] & F_S) && 1);

	//p->registers[R_A]  = 0x3e;

	//p->registers[R_B]  = 0x3e;
//	p->registers[R_F] |= F_C;

//	SetMemory(0x00, 0x88, p);
	
	//InstructionSet[0x90](0x90, 0, p);
	//printf("%u F:%d\n", p->registers[R_A], p->registers[R_F]);

	//printf("%d\n", p->registers[R_F] & F_C);
	//printf("%d\n", p->registers[R_F] & F_P);
//	printf("%d\n", p->registers[R_F] & F_A);
//	printf("%d\n", p->registers[R_F] & F_Z);
//	printf("%d\n", p->registers[R_F] & F_S);

	pthread_exit(NULL);
}

void *Head(void *x){
	char Input[256];
	int delim = 0;
	
	while(1){
		printf("$> ");
		fgets(Input, 256, stdin);
		if(Input == NULL){
			printf("Null input. Out\n");
			break;
		}
		char *Command = strtok(Input, " ");
		if(!strcmp(Input, "run")){
			delim = strlen(Input);

			strtok(Input + delim + 1, " ");
			char *Filename = Input + delim + 1;
			char *Mode = strtok(Input + delim + 1 + strlen(Input + delim + 1) + 1, "\n");
			
			Process *p = NULL;
			if(!strcmp(Mode, "hex")){
				p = CreateProcess(Filename, MODE_HEX);
			}else{
				p = CreateProcess(Filename, MODE_BINARY);
			}

			if(p == NULL){
				break;
			}
			p->registers[R_H] = 0xff;
			p->registers[R_L] = 0xf0;		

			char instruction = GetMemory(p->pc, p);
			InstructionSet[instruction](instruction, 0, p);

			printf("%d\n", GetMemory(0xfff0, p));
			break;
		}else{
			break;
		}
			
	}
}
