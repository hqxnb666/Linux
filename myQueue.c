#define _CRT_SECURE_NO_WARNINGS
#include "myQueue.h"
void QueueInit(Queue* pq)
{
	assert(pq);
	pq->head = pq->tail = NULL;
	pq->size = 0;
}
void QueueDestory(Queue* pq)
{
	assert(pq);
	QNode* cur = pq->head;
	while (cur)
	{
		QNode* next = cur->next;
		free(cur);
		cur = next;
	}
	pq->head = pq->tail = NULL;
	pq->size = 0;
}
void QueuePush(Queue* pq, QDataType x)
{
	assert(pq);
	QNode* newnode = (QNode*)malloc(sizeof(QNode));
	if (newnode == NULL)
	{
		perror("malloc fail");
		return;
	}
	newnode->val = x;
	newnode->next = NULL;
	if (pq->tail == NULL)
	{
		pq->head = pq->tail = newnode;
	}
	else
	{
		
		pq->tail->next = newnode;
		pq->tail = newnode;
	}
	pq->size++;
}
void QueuePop(Queue* pq)
{
	assert(pq);
	assert(pq->head);
	QNode* del = pq->head;
	pq->head = pq->head->next;
	free(del);
	del = NULL;

	if (pq->head == NULL)
		pq->tail = NULL;
	pq->size--;
}
QDataType QueueFront(Queue* pq)
{
	assert(pq);
	assert(pq->head);
	return pq->head->val;
}
QDataType QueueBack(Queue* pq)
{
	assert(pq);
	assert(pq->tail);
	return pq->tail->val;
}
bool QueueEmpty(Queue* pq)
{
	return pq->head == NULL;
}
int QueueSize(Queue* pq)
{
	assert(pq);
	return pq->size;
}