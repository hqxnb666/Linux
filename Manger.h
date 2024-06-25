#ifndef MANGER_H_INCLUDED
#define MANGER_H_INCLUDED
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <string>
#include <iomanip>
#include "Manger_Student.h"
#include "Manger_Teacher.h"
#include "Subject.h"
using namespace std;

// ����Ա��
class Manger : public Manger_Student, public Manger_Teacher
{
    vector<Subject> subList;
public:
    void login_admin(); // ����Ա��¼
    void menu_main(); // ���˵�
    void menu_stu(); // ѧ���˵�
    void menu_sub(); // �γ̲˵�
    void menu_tea(); // ��ʦ�˵�
    void menu_admin(); // ����Ա�˵�
    void selectCourse();
    void withdrawCourse();
    void insertList_sub();
    void deleteList_sub();
    void updateList_sub();
    void selectList_sub();
    void displaySub();
    void insertList_tea();
    void deleteList_tea();
    void updateList_tea();
    void selectList_tea();
    void displayTea();
    void writeFile_sub();
    void readFile_sub();
    void writeFile_tea(); // ȷ������������
    void readFile_tea(); // ȷ������������
    void selectCourse_tea();
    void withdrawCourse_tea();
};

#endif // MANGER_H_INCLUDED
