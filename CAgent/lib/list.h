#ifndef __LIST_H_
#define __LIST_H_


typedef struct list {
	void *node;
	uint32_t dataSize;
	uint32_t nodeCount;
	uint32_t dataCount;
	int (*appendFunc)(void*, void *, size_t);
	int (*compFunc)(void*, void *);
}List_t;

List_t* initList(size_t size, int (*func)(void*, void*, size_t), int (*comp)(void*, void*));
int appendList(List_t *list, void *data);
int destroyList(List_t *list);
void* searchNode(List_t *list, void *data);
int circuitList(List_t *list, int (*func)(void *, void *), void * etc);


#endif //__LIST_H_
