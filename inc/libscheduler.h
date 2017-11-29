struct _Node{
	Process *Value;
	struct _Node *Next;
}

struct _Queue{
	struct _Node *Head;
	struct _Node *End;
} Queue;

void QueueInsert(Process *p){
	struct _Node *N = malloc(sizeof(struct _Node));
	N->Value = p;
	(Queue.End)->Next = N;
	Queue.End = N;
}

Process *Dequeue(){
	struct _Node *N = Queue.Head;
	Queue.Head = N->Next;
	return N->value;
}
