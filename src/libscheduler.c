#include <libscheduler.h>
#include <stdlib.h>
#include <stdio.h>

void QueueInsert(Process *p){
	struct _Node *N = malloc(sizeof(struct _Node));
	if(N == NULL){
		printf("NULL MALLOC!!!!\n");
	}
	N->Next = NULL;
	
	N->Value = p;
	if(Queue.End != NULL){
		(Queue.End)->Next = N;
	}else{
		Queue.Head = N;
	}
	Queue.End = N;
}

Process *Dequeue(){
	struct _Node *N = Queue.Head;
	if(N == NULL){
		return NULL;
	}else{
	}
	Queue.Head = N->Next;
	if(N->Next == NULL){
		Queue.End  = NULL;
		Queue.Head = NULL;
	}else{
	}
	Process *Out = N->Value;
	free(N);
	return Out;
}
