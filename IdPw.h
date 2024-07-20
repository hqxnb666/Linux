#ifndef IDPW_H_INCLUDED
#define IDPW_H_INCLUDED
#include <iostream>
using namespace std;
//ÕËºÅÃÜÂë
class IdPw
{
public:
    string id; //ÕËºÅ
    string password; //ÃÜÂë
    IdPw(){}
    ~IdPw(){}
    IdPw * read(string fname){;}//¶ÁÈ¡ÕËºÅÃÜÂëÎÄ¼ş
    void write(IdPw *head_ip, string fname){;} //Ğ´ÈëÕËºÅÃÜÂëÎÄ¼ş
};


#endif // IDPW_H_INCLUDED
