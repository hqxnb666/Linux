#define _CRT_SECURE_NO_WARNINGS
#include <iostream>

using namespace std;

//����Ĭ�ϳ�Ա����

// ���캯��  ��������  ��������  ��ֵ����
//  ȡ��ַ���غ��ٻ��Լ�ʵ��


//���캯�� �ǳ�ʼ������
//��������������ͬ
//�޷���ֵ         ������void�����ǲ���Ҫд��
//����ʵ�����������Զ����ö�Ӧ�Ĺ��캯��
//���캯����������

//class Date //Ĭ�ϳ�Ա���������ǲ�д������ȥ������һ��
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
////��������/��������    int char double
////�Զ������� struct class
//// ��������ʲô������   �Զ����ȥ��������Ĭ�Ϲ���
//
//
//int main()
//{
//	Date d1;
//	d1.Print();
//
//	//��������
//	Date d2(2024, 1,23);//����ʵ�����Զ�����
//	d2.Print();
//	return 0;
//}



//��������   ������Դ  ������ڴ�й©

//����������������ǰ�����ַ�~
//�޲����޷���ֵ
//һ�����ֻ����һ���������� �����ûд��ϵͳ���Զ�����һ��Ĭ�ϵ��������� ������������������
//�����������ڽ���ʱ��C++����ϵͳ�Զ�������������
class Date //Ĭ�ϳ�Ա���������ǲ�д������ȥ������һ��
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

	~Date() //��������
	{

	}
private:
	int _year;
	int _month;
	int _day;
};