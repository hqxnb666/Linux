#define _CRT_SECURE_NO_WARNINGS
给你一个由数字和运算符组成的字符串 expression ，按不同优先级组合数字和运算符，计算并返回所有可能组合的结果。你可以 按任意顺序 返回答案。

生成的测试用例满足其对应输出值符合 32 位整数范围，不同结果的数量不超过 104 。



// 动态数组结构，用于存储中间结果
typedef struct {
    int size;
    int capacity;
    int* data;
} vector;

// 初始化动态数组
void vector_init(vector* v) {
    v->size = 0;
    v->capacity = 10;
    v->data = (int*)malloc(v->capacity * sizeof(int));
}

// 向动态数组添加元素
void vector_add(vector* v, int value) {
    if (v->size == v->capacity) {
        v->capacity *= 2;
        v->data = (int*)realloc(v->data, v->capacity * sizeof(int));
    }
    v->data[v->size++] = value;
}

// 释放动态数组内存
void vector_free(vector* v) {
    free(v->data);
    v->data = NULL;
    v->size = v->capacity = 0;
}

// 主要递归函数
void compute(const char* input, int start, int end, vector* result) {
    int i, j, k;
    int left, right;
    int isNumber = 1;

    for (i = start; i < end; ++i) {
        if (input[i] == '+' || input[i] == '-' || input[i] == '*') {
            isNumber = 0;
            vector leftResults, rightResults;
            vector_init(&leftResults);
            vector_init(&rightResults);

            compute(input, start, i, &leftResults);
            compute(input, i + 1, end, &rightResults);

            for (j = 0; j < leftResults.size; ++j) {
                for (k = 0; k < rightResults.size; ++k) {
                    left = leftResults.data[j];
                    right = rightResults.data[k];
                    if (input[i] == '+') {
                        vector_add(result, left + right);
                    }
                    else if (input[i] == '-') {
                        vector_add(result, left - right);
                    }
                    else if (input[i] == '*') {
                        vector_add(result, left * right);
                    }
                }
            }

            vector_free(&leftResults);
            vector_free(&rightResults);
        }
    }

    if (isNumber) {
        char* temp = (char*)malloc((end - start + 1) * sizeof(char));
        memcpy(temp, &input[start], end - start);
        temp[end - start] = '\0';
        vector_add(result, atoi(temp));
        free(temp);
    }
}

// API 函数
int* diffWaysToCompute(char* expression, int* returnSize) {
    vector result;
    vector_init(&result);

    compute(expression, 0, strlen(expression), &result);

    *returnSize = result.size;
    return result.data;
}
