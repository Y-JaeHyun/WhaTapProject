#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "linkedList.h"

LinkedList_t* initLinkedList(int dsize, int (*comp)(void *, void *)) {
	LinkedList_t *list = (LinkedList_t *)malloc(sizeof(LinkedList_t));
	list->dsize = dsize;
	list->count = 0;
	list->compFunc = comp;
}

void* appendLinkedList(LinkedList_t *list, void *data) {
	Node *new = (Node *)calloc(1, sizeof(Node));
	if (list->head) {
		list->head->prev = new;
		new->next = list->head;
		list->head = new;
	
	} else {
		list->head = new;
	}

	list->head->data = (void *)malloc(list->dsize);
	return memcpy(list->head->data, data, list->dsize);
}

int destroyLinkedList(LinkedList_t *list) {
	Node *temp;
	while (list->head) {
		temp = list->head;
		list->head = temp->next;
		free(temp->data);
		free(temp);	
	}
	free(list);
	return 1;
}

void* searchNodeData(LinkedList_t *list, void *data) {
	Node *temp = list->head;
	while (temp) {
		if (list->compFunc(temp->data, data) == 1) {
			return temp->data;
		}
		temp = temp->next;
	}

	return NULL;
}

int circuitLinkedList(LinkedList_t *list, int(*func)(LinkedList_t *, void *, void *), void *etc) {
	Node *temp = list->head;
	while (temp) {
		func(list, temp, etc);
		temp = temp->next;
	}
	return 1;
}



//#define LINKED_LIST_TEST
#ifdef LINKED_LIST_TEST

int compfunc (void *nodeData, void *data) {
	if (*(int *)nodeData == *(int*)data) {
		return 1;
	}
	return 0;
}

int printIntFunc(void *node, void *etc) {
	printf("%d\n", *(int *)node);
}
int main () {
	LinkedList_t *list = initList(sizeof(int), compfunc);
	int data = 0;
	int *pd;

	appendList(list, &data);
	data = 1;
	appendList(list, &data);
	data = 2;
	appendList(list, &data);
	data = 3;
	appendList(list, &data);
	data = 4;
	appendList(list, &data);

	circuitList(list, printIntFunc, NULL);

	data = 3;
	pd = searchNodeData(list, &data);
	printf("%d\n", *pd);

	destroyList(list);

	

}
#endif
