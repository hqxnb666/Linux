#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
typedef int STDataType;

typedef struct Stack
{
    STDataType* a;
    int top;
    int capacity;
}ST;

void STInit(ST* pst);
void STDestroy(ST* pst);
void STPush(ST* pst, STDataType x);
void STPop(ST* pst);
bool STEmpty(ST* pst);
STDataType STTop(ST* pst);
STDataType STSize(ST* pst);
void STInit(ST* pst)
{
    assert(pst);
    pst->a = (STDataType*)malloc(sizeof(STDataType) * 1);
    if (pst->a == NULL)
    {
        perror("malloc fail");
        exit(EXIT_FAILURE);
    }
    pst->capacity = 1;
    pst->top = -1;
}

void STDestroy(ST* pst)
{
    assert(pst);
    free(pst->a);
    pst->a = NULL;
    pst->capacity = pst->top = 0;
}
void STPush(ST* pst, STDataType x)
{
    assert(pst);
    if (pst->top == pst->capacity - 1)
    {
        int newcapacity = pst->capacity * 2;
        STDataType* tmp = (STDataType*)realloc(pst->a, sizeof(STDataType) * newcapacity);
        if (tmp == NULL)
        {
            perror("realloc fail");
            exit(EXIT_FAILURE);
        }
        pst->a = tmp;
        pst->capacity = newcapacity;
    }
    pst->top++;
    pst->a[pst->top] = x;
}

void STPop(ST* pst)
{
    assert(pst);
    assert(!STEmpty(pst));
    pst->top--;
}
bool STEmpty(ST* pst)
{
    assert(pst);
    return pst->top == -1;
}
STDataType STTop(ST* pst)
{
    assert(pst);
    if (!STEmpty(pst)) {
        return pst->a[pst->top];
    }
    else {
        fprintf(stderr, "Error: Stack is empty.\n");
        exit(EXIT_FAILURE);
    }
}


STDataType STSize(ST* pst)
{
    assert(pst);
    return pst->top;
}


typedef struct {
    ST q1;
    ST q2;
} MyQueue;


MyQueue* myQueueCreate() {
    MyQueue* pst = (MyQueue*)malloc(sizeof(MyQueue));
    if (pst == NULL)
    {
        perror("malloc fail");
    }
    STInit(&(pst->q1));
    STInit(&(pst->q2));
    return pst;
}
void myQueuePush(MyQueue* obj, int x) {

    assert(obj);
    // ��Ԫ���Ƶ���һ��ջ
    STPush(&obj->q1, x);

    // ����һ��ջ��Ԫ������������Ƶ��ڶ���ջ
    if (STEmpty(&obj->q2)) {
        while (!STEmpty(&obj->q1)) {
            STPush(&obj->q2, STTop(&obj->q1));
            STPop(&obj->q1);
        }
    }
}






int myQueuePop(MyQueue* obj) {
    assert(obj);
    int frontElement;

    // ����ڶ���ջΪ�գ�����һ��ջ��Ԫ������������Ƶ��ڶ���ջ
    if (STEmpty(&obj->q2)) {
        while (!STEmpty(&obj->q1)) {
            STPush(&obj->q2, STTop(&obj->q1));
            STPop(&obj->q1);
        }
    }

    // �����ڶ���ջ��ջ��Ԫ��
    if (!STEmpty(&obj->q2)) {
        frontElement = STTop(&obj->q2);
        STPop(&obj->q2);
        return frontElement;
    }
    else {
        // �������ջ��Ϊ�գ�˵��������û��Ԫ��
        return -1; // ���Ը���ʵ���������һ�����ʵ�ֵ
    }
}

int myQueuePeek(MyQueue* obj) {
    assert(obj);
    int frontElement;

    // ����ڶ���ջΪ�գ�����һ��ջ��Ԫ������������Ƶ��ڶ���ջ
    if (STEmpty(&obj->q2)) {
        while (!STEmpty(&obj->q1)) {
            STPush(&obj->q2, STTop(&obj->q1));
            STPop(&obj->q1);
        }
    }

    // ��ȡ�ڶ���ջ��ջ��Ԫ�أ���������
    if (!STEmpty(&obj->q2)) {
        frontElement = STTop(&obj->q2);
        return frontElement;
    }
    else {
        // �������ջ��Ϊ�գ�˵��������û��Ԫ��
        return -1; // ���Ը���ʵ���������һ�����ʵ�ֵ
    }
}

bool myQueueEmpty(MyQueue* obj) {
    assert(obj);
    // ����Ϊ�յ�����������ջ��Ϊ��
    return STEmpty(&obj->q1) && STEmpty(&obj->q2);
}

void myQueueFree(MyQueue* obj) {
    assert(obj);
    STDestroy(&obj->q1);
    STDestroy(&obj->q2);
    free(obj);
}


int main() {
    MyQueue* myQueue = myQueueCreate();

    // ��������Ԫ��
    printf("Pushing elements into the queue:\n");
    for (int i = 1; i <= 5; ++i) {
        myQueuePush(myQueue, i);
        printf("Pushed %d. Front element: %d\n", i, myQueuePeek(myQueue));
    }

    // ������е�״̬
    printf("\nQueue after pushing elements:\n");
    printf("Is the queue empty? %s\n", myQueueEmpty(myQueue) ? "Yes" : "No");
    printf("Front element: %d\n", myQueuePeek(myQueue));

    // �ͷŶ���
    myQueueFree(myQueue);

    return 0;
}
