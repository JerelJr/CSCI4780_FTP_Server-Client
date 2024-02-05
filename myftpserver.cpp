#include <iostream>
#include <sstream>
#include <string>
#include <cstring>
#include <fstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

using namespace std;
void error(const char *msg) {perror(msg); exit(0); }

int handleClient(int sock) {
  int connected = 1;
  int shutdown = 0;
  
  while (connected) {
    char buffer[1024] = {0};
    int ret = recv(sock, buffer, sizeof(buffer), 0);
    string msg(buffer);
    if (ret == 0) //client disconnected
      break;
    else if (ret == -1) error("Socket read failed");
    
    cout << "Message from client: " << msg << endl; //delete later
    
    //TODO: handle client requests
    
    if (msg == "quit")
      connected = 0;
    if (msg == "shutdown") {
      shutdown = 1;
      connected = 0;
    }
    
  }
  
  close(sock);
  return shutdown;
}

int main (int argc, char * argv[]) {
  if (argc != 2) error("Correct usage: ./myftpserver <port number>");
  
  int port = stoi(argv[1]);
  
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock == -1) error("Socket creation failed");
  
  sockaddr_in serverAddress;
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_port = htons(port);
  serverAddress.sin_addr.s_addr = INADDR_ANY;
  
  bind(sock, (struct sockaddr*)&serverAddress, sizeof(serverAddress));
  
  int serving = 1;
  while (serving) {
    listen(sock, 5);
  
    int clientSock = accept(sock, nullptr, nullptr);
    if (clientSock == -1) error("Client connection failed");
    
    serving = !handleClient(clientSock);
  
  }
  
  close(sock);

}