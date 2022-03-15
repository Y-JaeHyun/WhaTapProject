#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "list.h"

#define MIN_SIZE 10

List_t* initList(size_t size, int (*func)(void*, void*, size_t), int (*comp)(void*, void*)) {
	List_t *list = (List_t *)malloc(sizeof(List_t));
	list->appendFunc = func;
	list->compFunc = comp;
	list->node = calloc(MIN_SIZE, size);
	list->dataSize = size;
	list->nodeCount = MIN_SIZE;
	list->dataCount = 0;
	return list;
}

int appendList(List_t *list, void *data) {
	if (list->nodeCount == list->dataCount) {
		list->nodeCount += MIN_SIZE;
		list->node = (void *)realloc(list->node, list->dataSize * list->nodeCount);
	}
	if (list->appendFunc != NULL) {
		list->appendFunc(list->node + (list->dataSize * list->dataCount++), data, list->dataSize);
//		list->dataCount++;	
	}
	return 1;
}

int destroyList(List_t *list) {
	if (list) {
		if (list->node) free(list->node);
		free(list);
	}
}

void* searchNode(List_t *list, void *data) {
	int i;
	int ret;
	void *node;

	if (list) {
		for (i = 0; i < list->dataCount; i++) {
			node = list->node + (list->dataSize * i);
			ret = list->compFunc(node, data);
			if (ret == 1) return node;
		}
	}
	return NULL;
}

int circuitList(List_t *list, int (*func)(void *, void *), void *etc) {
	int i;
	int ret;
	for (i = 0; i < list->dataCount; i++) {
		ret = func(list->node + (list->dataSize * i), etc);
	}
	return ret;
}


//#define LIST_TEST
#ifdef LIST_TEST



int appendIntFunc(void *node, void *data, size_t size) {
	memcpy(node, data, size);
}

int printIntFunc(void *node, void *etc) {
	printf("%d\n", *(int *)node);
}


int main() {
	List_t* list = initList(sizeof(int), appendIntFunc, NULL);
	int a = 0;


	for (; a < 100; a++) {
		appendList(list, &a);
	}

	circuitList(list, printIntFunc, NULL);

}
#endif //LIST_TEST
