#include <iostream>
#include <sstream>
#include <string>
#include <cstring>

#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h> //IP addresses
#include <arpa/inet.h> //inet_addr()
#include <netdb.h> //struct hostent, gethostbyname

using namespace std;

int sock;
char buffer[1024];

void error(const char *msg) {perror(msg); exit(0); }
void zerobuf() {  memset(buffer, 0, sizeof(buffer));}

int code(const char *msg) {
  if (memcmp(msg, "ERR", 3) == 0) return -1;
  if (memcmp(msg, "MSG", 3) == 0) return 1;
  if (memcmp(msg, "FIL", 3) == 0) return 2;
  return -2;
}

/**
*
* Normal reply messages are assumed to be less than 1024 characters
*
**/
char* rcv() {
  zerobuf();
    
  int size = recv(sock, buffer, sizeof(buffer), 0); //receive data from server
  if (size < 0)
    error("Error receiving data from server");
    
  return buffer;
}

void snd(const char *msg) {
  if (send(sock, msg, strlen(msg), 0) == -1)
    error("Could not send message to server");
}

int main (int argc, char * argv[]) {
  if (argc != 3) error("Correct usage: ./myftp <server address> <port number>");
  
  string addr = argv[1];
  int port = stoi(argv[2]);
  
  sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock == -1) error("Socket creation failed.\n");
  
  sockaddr_in serverAddress;
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_port = htons(port);
  if (addr == "self")
    serverAddress.sin_addr.s_addr = INADDR_ANY;
  else
    serverAddress.sin_addr.s_addr = inet_addr(argv[1]);
  
  if (connect(sock, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) != 0)
    error("Could not connect to server");
    
  cout << "Connection to server established" << endl;
  
  int running = 1;
  while(running) { //user control loop
    string cmdline;
    string cmd, tmp;
    string arg = "";
    
    cout << "myftp> ";
    getline(cin, cmdline);
    stringstream ss(cmdline);
    ss >> cmd >> arg;
    
    if (cmd == "get") {
      if (arg.empty()) {
        cout << "Invalid command. Usage: get <remote_filename>";
      } else {
        cout << "Downloading file " << arg;
        //TODO: implement

        FILE *file = fopen(arg.c_str(), "wb");
        
        snd(cmdline.c_str());
        sleep(1);
        zerobuf();
        
        size_t rec_len = -1;
        while((rec_len = recv(sock, buffer, sizeof(buffer), 0)) > 0) {
          fwrite(buffer, 1, rec_len, file);
        }
        fclose(file);
      }
    
    } else if (cmd == "put") {
      if (arg.empty()) {
        cout << "Invalid command. Usage: put <local_filename>";
      } else {
        cout << "Uploading file " << arg;
        
        FILE *file = fopen(arg.c_str(), "rb");
        if(0){
          cout << endl << "File could not be opened. Make sure the file name is correct.";
        } else {
          snd(cmdline.c_str());
          sleep(1);
          zerobuf();
          
          size_t rec_len = -1;
          while((rec_len = fread(buffer, 1, 1024, file)) > 0) {
            snd(buffer);
          }
          
          cout << "File Uploaded";
          fclose(file);
        }
      }
    
    } else if (cmd == "delete") {
      if (arg.empty()) {
        cout << "Invalid command. Usage: delete <remote_filename>";
      } else {
        //TODO: implement
        snd(cmdline.c_str());
        
        char *reply = rcv();
        cout << reply+4;
      }
    
    } else if (cmd == "cd") {
      if (arg.empty()) {
        cout << "Invalid command. Usage: cd <remote_directory_name>";
      } else {
        //TODO: implement
        snd(cmdline.c_str());
        
        char *reply = rcv();
        cout << reply+4;
      }
    
    } else if (cmd == "mkdir") {
      if (arg.empty()) {
        cout << "Invalid command. Usage: mkdir <remote_directory_name>";
      } else {
        //TODO: implement
        snd(cmdline.c_str());
        
        char *reply = rcv();
        cout << reply+4;
      }
    
    } else if (cmd == "ls") {
      snd("ls");
      
      char *reply = rcv();
      cout << reply;
    } else if (cmd == "pwd") {
      snd("pwd");
      
      char *reply = rcv();
      cout << reply;
    } else if (cmd == "quit" || cmd == "exit") {
      snd("quit");
      cout << "Program exiting...";
      running = 0;
    } else if (cmd == "shutdown") { //used to close the server
      snd("shutdown");
      cout << "Exiting program and shutting down server...";
      running = 0;
    } else {
      cout << "Unknown command";
    }
    
    cout << endl << endl;
  
  }
  
  close(sock);
  exit(1);
}
