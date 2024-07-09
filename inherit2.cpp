#include <stdio.h>
#include <stdlib.h>

typedef struct TreeNode {
    int val;
    struct TreeNode* left, * right;
} TreeNode;

// Morris 中序遍历
void morrisInorder(TreeNode* root) {
    TreeNode* cur, * pre;
    cur = root;
    while (cur != NULL) {
        if (cur->left == NULL) {
            printf("%d ", cur->val);  // 打印节点值
            cur = cur->right;         // 移至右子节点
        }
        else {
            // 查找当前节点的前驱节点
            pre = cur->left;
            while (pre->right != NULL && pre->right != cur) {
                pre = pre->right;
            }

            // 将当前节点作为其前驱节点的右孩子
            if (pre->right == NULL) {
                pre->right = cur;
                cur = cur->left;
            }
            else {
                // 恢复树的原始结构
                pre->right = NULL;
                printf("%d ", cur->val);  // 打印节点值
                cur = cur->right;
            }
        }
    }
}

// 创建新节点
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

    printf("中序遍历结果: ");
    morrisInorder(root);
    printf("\n");

    return 0;
}