#include <iostream>
#include <sstream>
#include <string>
#include <cstring>

#include <filesystem>

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

using namespace std;
void error(const char *msg) {perror(msg); exit(0); }

void snd(int sock, const char *msg) {
  if (send(sock, msg, strlen(msg), 0) == -1)
    error("Could not send message to server");
}

int handleClient(int sock) {
  int connected = 1;
  int shutdown = 0;
  
  while (connected) {
    char buffer[1024] = {0};
    int ret = recv(sock, buffer, sizeof(buffer), 0);
    stringstream msg(buffer);
    string line(buffer);
    string cmd, arg;
    msg >> cmd >> arg; 
    
    if (ret == 0) //client disconnected
      break;
    else if (ret == -1) error("Socket read failed");
    cout << "Message from client: " << line << endl; //delete later
    
    if (cmd == "quit")
      connected = 0;
    else if (cmd == "shutdown") {
      shutdown = 1;
      connected = 0;
    } else if (cmd == "get") {
      cout << "TODO" << endl;
    } else if (cmd == "put") {
      cout << "TODO" << endl;
    } else if (cmd == "delete") {
      if (remove(arg.c_str()) != 0) {
        snd(sock, "ERR File could not be deleted");
      } else {
        snd(sock, "MSG File successfully deleted");
      }
    } else if (cmd == "cd") {
      try {
        std::filesystem::current_path(arg);
        snd(sock, "MSG cd success");
      } catch (const filesystem::filesystem_error &e) {
        snd(sock, "ERR Error: cd: path not found");
      }
    } else if (cmd == "mkdir") {
      try {
        filesystem::create_directory(arg);
        snd(sock, "MSG Directory successfully created");
      } catch (const std::exception &e) {
        char err_msg[1024];
        snprintf(err_msg, sizeof(err_msg), "ERR %s", e.what());
        snd(sock, err_msg);
      }
    } else if (cmd == "ls") {
      string files = "MSG ";
      for (const auto & entry : filesystem::directory_iterator(".")) {
        string path = entry.path();
        string name = path.substr(path.find("/")+1, path.length()-1);
        if (entry.is_directory())
          name.append("/");
        files.append(name).append("\n");
      }
      files = files.substr(0, files.length()-1);
      snd(sock, files.c_str());
    } else if (cmd == "pwd") {
      try {
        string path(filesystem::current_path());
        char path_msg[1024];
        snprintf(path_msg, sizeof(path_msg), "MSG %s", path.c_str());
        
        snd(sock, path_msg);
      } catch (const std:: exception &e) {
        char err_msg[1024];
        snprintf(err_msg, sizeof(err_msg),  "ERR %s", e.what());

        snd(sock, err_msg);
        error(e.what());
      }
    } else {snd(sock, "ERR Unknown data received");}
    
  }
  
  close(sock);
  return shutdown;
}

int main (int argc, char * argv[]) {
  if (argc != 2) error("Correct usage: ./myftpserver <port number>");
  
  int port = stoi(argv[1]);
  
  int servSock = socket(AF_INET, SOCK_STREAM, 0);
  if (servSock == -1) error("Socket creation failed");
  
  sockaddr_in serverAddress;
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_port = htons(port);
  serverAddress.sin_addr.s_addr = INADDR_ANY;
  
  bind(servSock, (struct sockaddr*)&serverAddress, sizeof(serverAddress));
  
  int serving = 1;
  while (serving) {
    listen(servSock, 5);
  
    int clientSock = accept(servSock, nullptr, nullptr);
    if (clientSock == -1) 
      cout << "Client connection failed";
    else
      serving = !handleClient(clientSock);
    
  }
  
  close(servSock);
  
  exit(1);

}
