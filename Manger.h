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

// 管理员类
class Manger : public Manger_Student, public Manger_Teacher
{
    vector<Subject> subList;
public:
    void login_admin(); // 管理员登录
    void menu_main(); // 主菜单
    void menu_stu(); // 学生菜单
    void menu_sub(); // 课程菜单
    void menu_tea(); // 教师菜单
    void menu_admin(); // 管理员菜单
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
    void writeFile_tea(); // 确保在这里声明
    void readFile_tea(); // 确保在这里声明
    void selectCourse_tea();
    void withdrawCourse_tea();
};

#endif // MANGER_H_INCLUDED
