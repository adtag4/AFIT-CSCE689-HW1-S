#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include "Conn.h"

Conn::Conn(int socketFD)
{
        socketFD_ = socketFD;
}

Conn::~Conn()
{
        
}

int Conn::readIn()
{
        char *in_buffer = (char *) calloc(0x1000, 1); // if there's a packet larger, then oh well
        int valread = read(socketFD_, in_buffer, 0x1000-1); // because I'm only reading that much
        data_ += std::string(in_buffer);
        free(in_buffer);
        return valread;
}

std::string Conn::dataUntil(char term)
{
        auto subString_loc = data_.find_first_of(term, 0);
        std::string ret;
        if(subString_loc == std::string::npos)
        {
                return "";
        }
        else // return substring until terminator, keep rest (auto kill terminator)
        {
                ret = data_.substr(0, subString_loc);
                data_ = data_.substr(subString_loc + 1);
        }
        return ret;
}

bool Conn::operator==(const Conn& c)
{
        if(socketFD_ == c.socketFD_)
        {
                return true;
        }
        return false;
}

int Conn::getFD()
{
        return socketFD_;
}