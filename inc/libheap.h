#ifndef __LIBHEAP_H
#define __LIBHEAP_H
struct Process;
typedef struct Process Process;

struct _Page;
typedef struct _Page Page;

extern char* BreakHead;
extern char* Pages;
extern char* Processes;

char *HeapAddProcess(Process *p);
Process **HeapGetNode(int Node);
void HeapInsertNode(Process *p);
void MaxHeapify(Page *Head, int Nodes, int Index);
void HeapIncreaseKey(Page *Head, int Nodes, int Index, int Key);
void HeapBubbleFrom(Process **Head, int Index);


#endif
