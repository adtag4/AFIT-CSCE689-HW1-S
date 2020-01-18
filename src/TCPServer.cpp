#include "TCPServer.h"
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
#include <stdexcept>
#include "exceptions.h"
#include <algorithm>

TCPServer::TCPServer() 
        : Server() 
{
        maxClients_ = 10;
}


TCPServer::~TCPServer() {
// nada
}

/**********************************************************************************************
 * bindSvr - Creates a network socket and sets it nonblocking so we can loop through looking for
 *           data. Then binds it to the ip address and port
 *
 *    Throws: socket_error for recoverable errors, runtime_error for unrecoverable types
 **********************************************************************************************/

void TCPServer::bindSvr(const char *ip_addr, short unsigned int port) 
{
        int tmp;
        
        listenFD_ = socket(AF_INET, SOCK_STREAM, 0);
        if(listenFD_ == 0)
        {
                throw std::runtime_error("Could not create socket.");
        }
        
        int opt = 1;
        tmp = setsockopt(listenFD_, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));
        if(tmp < 0)
        {
                throw std::runtime_error("Could not set socket options");
        }
        
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);
        
        tmp = bind(listenFD_, (struct sockaddr *)&addr, sizeof(addr));
        if(tmp < 0)
        {
                throw std::runtime_error("Could not bind socket");
        }
        
        return;
}

/**********************************************************************************************
 * listenSvr - Performs a loop to look for connections and create TCPConn objects to handle
 *             them. Also loops through the list of connections and handles data received and
 *             sending of data. 
 *
 *    Throws: socket_error for recoverable errors, runtime_error for unrecoverable types
 **********************************************************************************************/

void TCPServer::listenSvr() 
{
        int tmp;
        fd_set readfds;
        int max_fd;
        
        tmp = listen(listenFD_, maxClients_); 
        if(tmp < 0)
        {
                throw std::runtime_error("Could not listen.");
        }
        
        while(1)
        {
                FD_ZERO(&readfds);
                FD_SET(listenFD_, &readfds);
                
                
                max_fd = listenFD_;
                
                for(auto conn : activeConnections_)
                {
                        if(conn.getFD() > 0)
                        {
                                FD_SET(conn.getFD(), &readfds);
                        }
                        
                        // find the highest socket fd 
                        if(conn.getFD() > max_fd)
                        {
                                max_fd = conn.getFD();
                        }
                }
                
                // waits for activity on a client socket, or a new connection
                int activity = select(max_fd + 1, &readfds, NULL, NULL, NULL);
                
                /* Reference note:
                 *      This code was taken and modified from existing code in a project 
                 *        I wrote in C during the summer of 2018 to create a logging 
                 *        server for the USAFA summer cyber course.  I do not recall the 
                 *        references used for the creation of that code, but did rely 
                 *        heavily on the linux man pages.  
                 */
                
                if ((activity < 0) && (errno!=EINTR))  
                {  
                        //throw std::socket_error("Select problems");
                        continue;
                }
                
                // an activity on listening socket = new connection
                if(FD_ISSET(listenFD_, &readfds))
                {
                        struct sockaddr_in addr;
                        int addrlen = sizeof (addr);
                        int new_conn = accept(listenFD_, (struct sockaddr *)&addr, (socklen_t *)&addrlen);
                        
                        if(new_conn < 0)
                        {
                                //throw std::socket_error("Invalid connection");
                                continue;
                        }
                        
                        if(activeConnections_.size() < maxClients_)
                        {
                                Conn nc(new_conn);
                                activeConnections_.push_back(nc);
                                sendHelp(new_conn);
                        }
                }
                
                // otherwise, client is doing IO
                for(auto conn : activeConnections_)
                {
                        if(FD_ISSET(conn.getFD(), &readfds))
                        {
                                tmp = conn.readIn();
                                if(tmp == 0) // closed socket
                                {
                                        close(conn.getFD());
                                        auto it = std::find(activeConnections_.begin(), activeConnections_.end(), conn);
                                        if(it != activeConnections_.end())
                                                activeConnections_.erase(it);
                                        //activeConnections_.erase(std::remove(activeConnections_.begin(), activeConnections_.end(), conn), activeConnections_.end());
                                        continue;
                                }
                                std::string command = conn.dataUntil('\x03');
                                if(command == "")
                                        continue; // have not received full cmd
                                processCommand(conn, command);
                        }
                }
        }
}

void TCPServer::sendHelp(int fd)
{
        std::string help = "";
        help += "Welcome to the fount of all knowledge.\n";
        help += "\n";
        help += "Your knowledge intake is limited to these options:\n";
        help += "       hello: you will be greeted\n";
        help += "       menu: you will see this text\n";
        help += "       passwd: not implemented\n";
        help += "       exit: your connection will be closed\n";
        help += "       1: ask the ultimate quesiton of life, the universe, and everything\n";
        help += "       2: ask who the greates theologian of the Reformation was\n";
        help += "       3: ask what the best color is\n";
        help += "       4: ask what the average airspeed of an unladen swallow is\n";
        help += "       5: ask where you should go for food\n";
        help += "\n";
        help += "With great knowledge comes great responsibility.\n";
        help += "\x03";
        
        write(fd, help.c_str(), help.length());
}


void TCPServer::processCommand(Conn c, std::string command)
{
        std::string response = "null";
        int x = 0;
        if(command == "hello")
                response = "Sup";
        else if(command == "1")
                response = "42";
        else if(command == "2")
                response = "Calvin";
        else if(command == "3")
                response = "Gold";
        else if(command == "4")
                response = "European or African swallows?";
        else if(command == "5")
                response = "Chick-Fil-A (except on Sunday)";
        else if(command == "passwd")
                response = "Not so fast";
        else if(command == "exit")
                x = 1;
        else if(command == "menu")
                sendHelp(c.getFD());
        else
                response = "I don't understand \"" + command + "\"";
        
        if(response != "null")
        {
                response += "\x03"; // as terminator of message (03 is 'end of text')
                write(c.getFD(), response.c_str(), response.length());
        }
        
        if(x) // x for "x-it"
        {
                close(c.getFD());
                auto it = std::find(activeConnections_.begin(), activeConnections_.end(), c);
                if(it != activeConnections_.end())
                        activeConnections_.erase(it);
                //activeConnections_.erase(std::remove(activeConnections_.begin(), activeConnections_.end(), conn), activeConnections_.end());
        }
}

/**********************************************************************************************
 * shutdown - Cleanly closes the socket FD.
 *
 *    Throws: socket_error for recoverable errors, runtime_error for unrecoverable types
 **********************************************************************************************/

void TCPServer::shutdown() 
{
        
}
