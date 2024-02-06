#include <iostream>
#include <sstream>
#include <string>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

using namespace std;
void error(const char *msg)
{
  perror(msg);
  exit(0);
}

void snd(int sock, const char *msg)
{
  if (send(sock, msg, strlen(msg), 0) == -1)
    error("Could not send message to server");
}

int handleClient(int sock)
{
  int connected = 1;
  int shutdown = 0;

  while (connected)
  {
    char buffer[1024] = {0};
    int ret = recv(sock, buffer, sizeof(buffer), 0);
    stringstream command(buffer);
    string msg, arg;
    command >> msg >> arg;

    if (ret == 0) // client disconnected
      break;
    else if (ret == -1)
      error("Socket read failed");

    cout << "Message from client: " << msg << endl; // delete later

    // TODO: handle client requests

    if (msg == "quit")
      connected = 0;
    else if (msg == "shutdown")
    {
      shutdown = 1;
      connected = 0;
    }
    else if (msg == "get")
    {
      if (!arg.empty())
      {
        std::ifstream file(arg, ios::binary);
        bzero(buffer, sizeof(buffer));

        while (file.read(buffer, sizeof(buffer)))
        {
          send(sock, buffer, sizeof(buffer), 0);
        }
        file.close();
      }
      else
      {
        snd(sock, "ERR Filename argument required");
      }
    }
    else if (msg == "put")
    {
      if (!arg.empty())
      {
        std::ofstream file(arg, ios::binary);
        bzero(buffer, sizeof(buffer));
        size_t size_recvd;

        while ((size_recvd = recv(sock, buffer, sizeof(buffer), 0)) > 0)
        {
          file.write(buffer, size_recvd);
        }
        if (size_recvd < 0)
        {
          std::cerr << "Error receiving file" << std::endl;
        }
        std::cerr << size_recvd << " bytes read" << std::endl;

        file.close();
      }
      else
      {
        snd(sock, "ERR Filename argument required");
      }
    }
    else if (msg == "delete")
    {
      if (remove(arg.c_str()))
      {
        snd(sock, "ERR Filename argument required");
      }
    }
    else if (msg == "cd")
    {
      if (!arg.empty())
      {
        filesystem::current_path(arg);
        snd(sock, "MSG");
      }
      else
      {
        snd(sock, "ERR Filename argument required");
      }
    }
    else if (msg == "mkdir")
    {
      try
      {
        filesystem::create_directory(arg);
      }
      catch (const std::exception &e)
      {
        char err_msg[1024];
        snprintf(err_msg, sizeof(err_msg), "ERR %s", e.what());
        snd(sock, err_msg);

        error(e.what());
      }
    }
    else if (msg == "ls")
    {
    }
    else if (msg == "pwd")
    {
      try
      {
        string path(filesystem::current_path());
        char path_msg[1024];
        snprintf(path_msg, sizeof(path_msg), "MSG %s", path.c_str());

        snd(sock, path_msg);
      }
      catch (const std::exception &e)
      {
        char err_msg[1024];
        snprintf(err_msg, sizeof(err_msg), "ERR %s", e.what());

        snd(sock, err_msg);
        error(e.what());
      }
    }
    else
    {
      snd(sock, "ERR Unknown data received");
    }
  }

  close(sock);
  return shutdown;
}

int main(int argc, char *argv[])
{
  if (argc != 2)
    error("Correct usage: ./myftpserver <port number>");

  int port = stoi(argv[1]);

  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock == -1)
    error("Socket creation failed");

  sockaddr_in serverAddress;
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_port = htons(port);
  serverAddress.sin_addr.s_addr = INADDR_ANY;

  bind(sock, (struct sockaddr *)&serverAddress, sizeof(serverAddress));

  int serving = 1;
  while (serving)
  {
    listen(sock, 5);

    int clientSock = accept(sock, nullptr, nullptr);
    if (clientSock == -1)
      error("Client connection failed");

    serving = !handleClient(clientSock);
  }

  close(sock);
}
