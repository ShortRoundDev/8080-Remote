#include <lib8080.h>

#ifndef __LIBSCHEDULER_H
#define __LIBSCHEDULER_H

struct _Node{
	Process *Value;
	struct _Node *Next;
};

struct _Queue{
	struct _Node *Head;
	struct _Node *End;
} Queue;

void QueueInsert(Process *p);
Process *Dequeue();
#endif
