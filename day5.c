#define _CRT_SECURE_NO_WARNINGS
����һ�������ֺ��������ɵ��ַ��� expression ������ͬ���ȼ�������ֺ�����������㲢�������п�����ϵĽ��������� ������˳�� ���ش𰸡�

���ɵĲ��������������Ӧ���ֵ���� 32 λ������Χ����ͬ��������������� 104 ��



// ��̬����ṹ�����ڴ洢�м���
typedef struct {
    int size;
    int capacity;
    int* data;
} vector;

// ��ʼ����̬����
void vector_init(vector* v) {
    v->size = 0;
    v->capacity = 10;
    v->data = (int*)malloc(v->capacity * sizeof(int));
}

// ��̬�������Ԫ��
void vector_add(vector* v, int value) {
    if (v->size == v->capacity) {
        v->capacity *= 2;
        v->data = (int*)realloc(v->data, v->capacity * sizeof(int));
    }
    v->data[v->size++] = value;
}

// �ͷŶ�̬�����ڴ�
void vector_free(vector* v) {
    free(v->data);
    v->data = NULL;
    v->size = v->capacity = 0;
}

// ��Ҫ�ݹ麯��
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

// API ����
int* diffWaysToCompute(char* expression, int* returnSize) {
    vector result;
    vector_init(&result);

    compute(expression, 0, strlen(expression), &result);

    *returnSize = result.size;
    return result.data;
}
