#ifndef CONN_H
#define CONN_H 

#include <string>

class Conn
{
public:
        Conn(int socketFD);
        Conn()
        {};
        ~Conn();
        int readIn();
        int getFD();
        std::string dataUntil(char term);
        bool operator==(const Conn& c);
        
private:
        int socketFD_;
        std::string data_;
};















#endif