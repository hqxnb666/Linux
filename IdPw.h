#ifndef IDPW_H_INCLUDED
#define IDPW_H_INCLUDED
#include <iostream>
using namespace std;
//�˺�����
class IdPw
{
public:
    string id; //�˺�
    string password; //����
    IdPw(){}
    ~IdPw(){}
    IdPw * read(string fname){;}//��ȡ�˺������ļ�
    void write(IdPw *head_ip, string fname){;} //д���˺������ļ�
};


#endif // IDPW_H_INCLUDED
