#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <string>
#include <netdb.h>
#include <sstream>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>
#include <vector>

#define _BUFSIZE 1024

int handle_connection(int);
void init_addr(sockaddr_in &, uint16_t);

int main(int argc, char const *argv[])
{
    if (argc < 2)
    {
        std::cout << "Missing port number." << std::endl;
        return 1;
    }

    uint16_t portno = std::atoi(argv[1]);

    int listener_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listener_sockfd < 0)
    {
        perror("Error opening socket");
    }

    struct sockaddr_in serv_addr;
    init_addr(serv_addr, portno);

    if (bind(listener_sockfd, reinterpret_cast<sockaddr *>(&serv_addr), sizeof(serv_addr)) < 0)
    {
        perror("Error binding socket");
    }

    if (listen(listener_sockfd, 5) < 0)
    {
        perror("Error while attempting to listen to socket");
    }
    std::vector<std::thread> handlers;
    while (1)
    {
        sockaddr_in cli_addr;
        socklen_t cli_len;

        int conn_fd = accept(listener_sockfd, reinterpret_cast<sockaddr *>(&cli_addr), &cli_len);
        if (conn_fd < 0)
        {
            perror("Error accepting connection");
        }

        handlers.emplace_back(std::thread(handle_connection, conn_fd));
    }
}

int handle_connection(int conn_fd)
{
    char buffer[_BUFSIZE];
    while (1)
    {
        bzero(buffer, _BUFSIZE);
        if (read(conn_fd, buffer, 255) < 0)
        {
            perror("Error reading from socket");
        }
        std::stringstream command;
        command.getline(buffer, 255);
        std::string cmd_name;
        command >> cmd_name;

        if (cmd_name == "get")
        {
            std::string arg;
            command >> arg;
        }
        else if (cmd_name == "put")
        {
            /* code */
        }
        else if (cmd_name == "delete")
        {
        }
        else if (cmd_name == "ls")
        {
        }
        else if (cmd_name == "cd")
        {
        }
        else if (cmd_name == "mkdir")
        {
        }
        else if (cmd_name == "pwd")
        {
        }
        else if (cmd_name == "quit")
        {
            close(conn_fd);
            return 0;
        }
        else
        {
            /* code */
        }
    }

    printf("%s\n", buffer);

    return 0;
}

void init_addr(sockaddr_in &addr, uint16_t portno)
{
    bzero(reinterpret_cast<char *>(&addr), sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(portno);
    addr.sin_addr.s_addr = INADDR_ANY;
}