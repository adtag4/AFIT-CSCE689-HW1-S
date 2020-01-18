#ifndef TCPSERVER_H
#define TCPSERVER_H

#include "Server.h"
#include <memory>
#include <vector>
#include "Conn.h"

class TCPServer : public Server 
{
public:
        TCPServer();
        ~TCPServer();

        void bindSvr(const char *ip_addr, unsigned short port);
        void listenSvr();
        void shutdown();

private:
        std::vector<Conn> activeConnections_;
        int listenFD_;
        int maxClients_;
        
        void sendHelp(int fd);
        void processCommand(Conn c, std::string command);

};


#endif
