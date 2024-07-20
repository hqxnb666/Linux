#include <stdio.h>
#include <stdlib.h>

typedef struct TreeNode {
    int val;
    struct TreeNode* left, * right;
} TreeNode;

// Morris �������
void morrisInorder(TreeNode* root) {
    TreeNode* cur, * pre;
    cur = root;
    while (cur != NULL) {
        if (cur->left == NULL) {
            printf("%d ", cur->val);  // ��ӡ�ڵ�ֵ
            cur = cur->right;         // �������ӽڵ�
        }
        else {
            // ���ҵ�ǰ�ڵ��ǰ���ڵ�
            pre = cur->left;
            while (pre->right != NULL && pre->right != cur) {
                pre = pre->right;
            }

            // ����ǰ�ڵ���Ϊ��ǰ���ڵ���Һ���
            if (pre->right == NULL) {
                pre->right = cur;
                cur = cur->left;
            }
            else {
                // �ָ�����ԭʼ�ṹ
                pre->right = NULL;
                printf("%d ", cur->val);  // ��ӡ�ڵ�ֵ
                cur = cur->right;
            }
        }
    }
}

// �����½ڵ�
TreeNode* newNode(int value) {
    TreeNode* node = (TreeNode*)malloc(sizeof(TreeNode));
    node->val = value;
    node->left = NULL;
    node->right = NULL;
    return node;
}

int main() {
    TreeNode* root = newNode(1);
    root->left = newNode(2);
    root->right = newNode(3);
    root->left->left = newNode(4);
    root->left->right = newNode(5);

    printf("����������: ");
    morrisInorder(root);
    printf("\n");

    return 0;
}