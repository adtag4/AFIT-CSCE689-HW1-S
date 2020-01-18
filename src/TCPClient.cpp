#include "TCPClient.h"
#include "Conn.h"
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
#include <fcntl.h>
#include <string>
#include <iostream>
#include <errno.h>
#include "strfuncts.h"

/**********************************************************************************************
 * TCPClient (constructor) - Creates a Stdin file descriptor to simplify handling of user input. 
 *
 **********************************************************************************************/

TCPClient::TCPClient() 
{
        consoleIn_ = 0; // 0 is stdin
        int tmp = fcntl(consoleIn_, F_SETFL, fcntl(consoleIn_, F_GETFL, 0) | O_NONBLOCK);
        if(tmp == -1)
        {
                throw std::runtime_error("Could not set non-blocking on stdin");
        }
}

/**********************************************************************************************
 * TCPClient (destructor) - No cleanup right now
 *
 **********************************************************************************************/

TCPClient::~TCPClient() 
{

}

/**********************************************************************************************
 * connectTo - Opens a File Descriptor socket to the IP address and port given in the
 *             parameters using a TCP connection.
 *
 *    Throws: socket_error exception if failed. socket_error is a child class of runtime_error
 **********************************************************************************************/

void TCPClient::connectTo(const char *ip_addr, unsigned short port) 
{
        activeConnection_ = Conn(socket(AF_INET, SOCK_STREAM, 0));
        if(activeConnection_.getFD() < 0)
        {
                throw std::runtime_error("Could not open socket");
        }
        
        struct sockaddr_in address;
        struct sockaddr_in serv_addr;
        
        memset(&serv_addr, '\0', sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(port);
        // convert convert printable to numerical
        int tmp = inet_pton(AF_INET, ip_addr, &serv_addr.sin_addr);
        if(tmp <= 0)
        {
                throw std::runtime_error("IP not found");
        }
        
        tmp = connect(activeConnection_.getFD(), (struct sockaddr *)&serv_addr, sizeof(serv_addr));
        if(tmp < 0)
        {
                printf("%s\n", strerror(errno));
                throw std::runtime_error("Could not connect");
        }
        
        // make non-blocking, but preserve all other flags
        tmp = fcntl(activeConnection_.getFD(), F_SETFL, fcntl(activeConnection_.getFD(), F_GETFL, 0) | O_NONBLOCK);
        if(tmp == -1)
        {
                throw std::runtime_error("Could not set non-blocking");
        }
}

/**********************************************************************************************
 * handleConnection - Performs a loop that checks if the connection is still open, then 
 *                    looks for user input and sends it if available. Finally, looks for data
 *                    on the socket and sends it.
 * 
 *    Throws: socket_error for recoverable errors, runtime_error for unrecoverable types
 **********************************************************************************************/

void TCPClient::handleConnection() 
{
        // prepare for select() to non-block stdin 
        fd_set readfds;
        
        struct timeval delay;
        int index = 0;
        bool active = true;
        while(active)
        {
                // check for socket response
                char *output = (char *) calloc(1, 100);
                int valread = -1;
                while((valread = read(activeConnection_.getFD(), output, 99)))
                {
                        for(int i = 0; i < valread; i++)
                        {
                                if(output[i] != '\x03')
                                        std::cout << output[i];
                                else
                                        std::cout << std::endl;
                                output[i] = '\0';
                        }
                        if(valread == -1)
                        {
                                if((errno == EAGAIN) || (errno == EWOULDBLOCK)) // nothing to read, but still open
                                        break; // taken from linux "man 2 read"
                                else
                                        throw std::runtime_error("Sockets dislike you");
                                        break; // actual error
                        }
                }
                fflush(stdout);
                if(valread == 0) // connection closed 
                        active = false;
                
                FD_ZERO(&readfds);
                FD_SET(consoleIn_, &readfds);
                
                delay.tv_sec = 0;
                delay.tv_usec = 10; // very small delay :)
                
                int activity = select(consoleIn_ + 1, &readfds, NULL, NULL, &delay);
                if(activity < 0)
                {
                        throw std::runtime_error("The OS decided to hate you");
                }
                
                if(FD_ISSET(consoleIn_, &readfds))
                {
                        char *input = (char *) calloc(1, 100);
                        char c;
                        while(read(consoleIn_, &c, 1) > 0)
                        {
                                input[index] = c;
                                if(c == '\n' || c == '\r')
                                {
                                        index = 0;
                                        break;
                                }
                                index++;
                        }
                        
                        std::string data(input);
                        clrNewlines(data);
                        data += "\x03"; // EndOfText
                        write(activeConnection_.getFD(), data.c_str(), data.length());
                        free(input);
                }
                
                fflush(stdin);
                fflush(stdout);
                
                
        }
}

/**********************************************************************************************
 * closeConnection - Your comments here
 *
 *    Throws: socket_error for recoverable errors, runtime_error for unrecoverable types
 **********************************************************************************************/

void TCPClient::closeConn() 
{
        close(activeConnection_.getFD());
}


