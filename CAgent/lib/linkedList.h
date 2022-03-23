#ifndef __LINKEDLIST_H_
#define __LINKEDLIST_H_

typedef struct node {
	struct node *prev;
	struct node *next;
	void *data;
}Node;


typedef struct linkedList {
	struct node *head;
	int dsize;
	int count;
	int (*appendFunc)(void*, void *, size_t);
	int (*compFunc)(void*, void *);
}LinkedList_t;

LinkedList_t* initLinkedList(int dsize, int (*comp)(void *, void *));
void* appendLinkedList(LinkedList_t *list, void *data);
int destroyLinkedList(LinkedList_t *list);
void* searchNodeData(LinkedList_t *list, void *data);
int circuitLinkedList(LinkedList_t *list, int(*func)(LinkedList_t *, void *, void *), void *etc);


#endif //__LINKEDLIST_H_
