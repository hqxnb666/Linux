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
void PrevOrder(TreeNode* root);//ǰ�����  --������ȱ��� dfs
void InOrder(TreeNode* root);
void PostOrder(TreeNode* root);
int TreeSize(TreeNode* root);
int TreeLeafSize(TreeNode* root);//Ҷ�ӽڵ����
int TreeHeight(TreeNode* root);//�߶�
int TreeLeveLK(TreeNode* root, int k);//��k��ڵ����
TreeNode* TreeCreate(char* a, int* pi);//����������
void DestoryTree(TreeNode** root);//���ٶ�����
void LevelOrder(TreeNode* root);//�������  --������ȱ���bfs
bool BinaryTreeComplete(TreeNode* root);