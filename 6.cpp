#define _CRT_SECURE_NO_WARNINGS
#include <iostream>

using namespace std;

//六个默认成员函数

// 构造函数  析构函数  拷贝构造  赋值重载
//  取地址重载很少会自己实现


//构造函数 是初始化对象
//函数名和类名相同
//无返回值         （不是void，就是不需要写）
//对象实例化编译器自动调用对应的构造函数
//构造函数可以重载

//class Date //默认成员函数，我们不写编译器去会生成一个
//{
//public:
//	Date()
//	{
//		_year = 1;
//		_month = 1;
//		_day = 1;
//	}
//
//	Date(int year, int month, int day)
//	{
//		_year = year;
//		_month = month;
//		_day = day;
//	}
//
//	void Print()
//	{
//		cout << _year << "-" << _month << "-" << _day << endl;
//	}
//private:
//	int _year;
//	int _month;
//	int _day;
//};
//
////内置类型/基本类型    int char double
////自定义类型 struct class
//// 内置类型什么都不做   自定义会去调用他的默认构造
//
//
//int main()
//{
//	Date d1;
//	d1.Print();
//
//	//函数重载
//	Date d2(2024, 1,23);//对象实例化自动调用
//	d2.Print();
//	return 0;
//}



//析构函数   清理资源  否则会内存泄漏

//析构函数是在类名前加上字符~
//无参数无返回值
//一个类就只能有一个析构函数 。如果没写吗，系统会自动生成一个默认的析构函数 ：析构函数不能重载
//对象生命周期结束时。C++编译系统自动调用析构函数
class Date //默认成员函数，我们不写编译器去会生成一个
{
public:
	

	Date(int year = 1, int month = 1, int day = 1)
	{
		_year = year;
		_month = month;
		_day = day;
	}

	void Print()
	{
		cout << _year << "-" << _month << "-" << _day << endl;
	}

	~Date() //析构函数
	{

	}
private:
	int _year;
	int _month;
	int _day;
};