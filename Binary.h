#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <stdbool.h>

typedef int BTDataType;
typedef struct BinaryTreeNode
{
	BTDataType data;
	struct BinaryTreeNode* left;
	struct BinaryTreeNode* right;
}TreeNode;

TreeNode* BuyTreeNode(int x);
TreeNode* CreateNode();
void PrevOrder(TreeNode* root);//前序遍历  --深度优先遍历 dfs
void InOrder(TreeNode* root);
void PostOrder(TreeNode* root);
int TreeSize(TreeNode* root);
int TreeLeafSize(TreeNode* root);//叶子节点个数
int TreeHeight(TreeNode* root);//高度
int TreeLeveLK(TreeNode* root, int k);//第k层节点个数
TreeNode* TreeCreate(char* a, int* pi);//构建二叉树
void DestoryTree(TreeNode** root);//销毁二叉树
void LevelOrder(TreeNode* root);//层序遍历  --广度优先遍历bfs
bool BinaryTreeComplete(TreeNode* root);