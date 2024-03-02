#include <iostream>
#include <sstream>
#include <string>
#include <cstring>

#include <filesystem> //file rename
#include <random> //random number gen for download

#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h> //IP addresses
#include <arpa/inet.h> //inet_addr()
#include <netdb.h> //struct hostent, gethostbyname
#include <thread>

using namespace std;

default_random_engine generator;
uniform_int_distribution<> distribution(100000,999999);

int sock;
string addr;
int termPort;
char buffer[1024];
int running;
int terminating = 0;
int activeThreads;

void error(const char *msg) {perror(msg); exit(0); }
void zerobuf() {  memset(buffer, 0, sizeof(buffer));}

int code(const char *msg) {
  if (memcmp(msg, "ERR", 3) == 0) return -1;
  if (memcmp(msg, "MSG", 3) == 0) return 1;
  if (memcmp(msg, "FIL", 3) == 0) return 2;
  return -2;
}

void snd(const char *msg) {
  if (send(sock, msg, strlen(msg), 0) == -1)
    error("Could not send message to server");
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
  if (size == 0) {
    cout << "Server has disconnected abruptly. Exiting program...";
    running = 0;
  }
  return buffer;
}

//clear the socket of all remaining data
void flushSock() {
  
  int ret;
  char flushBuffer[1024];
  fd_set set;
  struct timeval  tv;
  tv.tv_sec = 0;
  tv.tv_usec = 100000; //.1 seconds
  while (1) {
  
    FD_ZERO(&set);
    FD_SET(sock, &set);
    ret = select(sock+1, &set, NULL, NULL, &tv);
    if (ret)
      recv(sock, flushBuffer, sizeof(flushBuffer), 0);
    else
      break;
  }
}

void handleCommand(string cmdline, string cmd, string arg, int useThread) {

    if (cmd == "terminate") {
      terminating = 1;
      int termSock = socket(AF_INET, SOCK_STREAM, 0);
      if (termSock == -1) error("Socket creation failed.\n");
  
      sockaddr_in serverAddress;
      serverAddress.sin_family = AF_INET;
      serverAddress.sin_port = htons(termPort);
      if (addr == "self")
        serverAddress.sin_addr.s_addr = INADDR_ANY;
      else
        serverAddress.sin_addr.s_addr = inet_addr(addr.c_str());
  
      if (connect(termSock, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) != 0)
        error("Could not connect to server");
      
      send(termSock, arg.c_str(), strlen(arg.c_str()), 0);
      close(termSock);
    
    } else if ((activeThreads - useThread) > 0)
      cout << "Server connection in use. Please wait until other commands have finished.";
    else if (cmd == "get") {//arg
      if (arg.empty()) {
        cout << "Invalid command. Usage: get <remote_filename>";
      } else {
        
        snd(cmdline.c_str());
        char *reply = rcv();
        
        if (code(reply) == -1) {
          cout << reply+4;
        } else {
          //use temporary name while downloading so original file is
          //preserved in case command is terminated
          string tempNum = to_string(distribution(generator));
          string tempName = arg + "-tmp" + tempNum;
          FILE *file = fopen(tempName.c_str(), "wb");
        
          //parse reply for termination code and file size
        
          stringstream msg(reply);
          string cmd, termCode, fileSizeStr;
          msg >> cmd >> termCode >> fileSizeStr; 
        
          if (useThread)
            cout << "Download termination code: " << termCode << endl;
        
          zerobuf();
          int fileSize = stoi(fileSizeStr);
          int written = 0;
          size_t rec_len = -1;
          while(written < fileSize) {
            rec_len = recv(sock, buffer, sizeof(buffer), 0);
            if (terminating) {
              flushSock();
              remove(tempName.c_str());
              break;
            }
            fwrite(buffer, 1, rec_len, file);
            written += rec_len;
          }
          
          if (!terminating)
            filesystem::rename(tempName, arg);
        
          fclose(file);
          
          snd("Get command complete");
          
          if (terminating) {
            cout << "Download Terminated" << endl;
            terminating = 0;
          } else
            cout << "Download complete";
        }
      }
    
    } else if (cmd == "put") {
      if (arg.empty()) {
        cout << "Invalid command. Usage: put <local_filename>";
      } else {
        
        FILE *file = fopen(arg.c_str(), "rb");
        if(file == NULL){
          cout << endl << "File could not be opened. Make sure the file name is correct.";
        } else {
        
        
          string filesize = to_string(filesystem::file_size(arg));
          snd((cmd+" "+arg+" "+filesize).c_str());
          char *reply = rcv();
          if ((strlen(reply) > 0) && useThread)
            cout << "Upload termination code: " << reply+4 << endl;
          
          zerobuf();
          size_t rec_len = -1;
          
          while((rec_len = fread(buffer, 1, 1024, file)) > 0) {
            if (terminating) {
              terminating = 0;
              break;
            }
            send(sock, buffer, rec_len, 0);
            //usleep(300); //used to slow down upload to test termination
          }
          
          reply = rcv();
          cout << reply+4 << endl;
          
          fclose(file);
        }
      }
    
    } else if (cmd == "delete") {
      if (arg.empty()) {
        cout << "Invalid command. Usage: delete <remote_filename>";
      } else {
        snd(cmdline.c_str());
        
        char *reply = rcv();
        if (strlen(reply) > 0)
          cout << reply+4;
      }
    
    } else if (cmd == "cd") {
      if (arg.empty()) {
        cout << "Invalid command. Usage: cd <remote_directory_name>";
      } else {
        snd(cmdline.c_str());
        
        char *reply = rcv();
        if (strlen(reply) > 0)
          cout << reply+4;
      }
    
    } else if (cmd == "mkdir") {
      if (arg.empty()) {
        cout << "Invalid command. Usage: mkdir <remote_directory_name>";
      } else {
        snd(cmdline.c_str());
        
        char *reply = rcv();
        if (strlen(reply) > 0)
          cout << reply+4;
      }
    
    } else if (cmd == "ls") {
      snd("ls");
      
      char *reply = rcv();
      if (strlen(reply) > 0)
        cout << reply+4;
    } else if (cmd == "pwd") {
      snd("pwd");
      
      char *reply = rcv();
      if (strlen(reply) > 0)
        cout << reply+4;
    } else if (cmd == "quit" || cmd == "exit") {
      snd("quit");
      cout << "Program exiting...";
      running = 0;
    } else if (cmd == "shutdown") { //used to close the server
      snd("shutdown");
      cout << "Exiting program and shutting down server...";
      running = 0;
    } else
      cout << "Unknown command";
    
    
    if (useThread)
      activeThreads--;

}

int main (int argc, char * argv[]) {
  if (argc != 4) error("Correct usage: ./myftp <server address> <port number> <terminate port number>");
  
  addr = argv[1];
  int port = stoi(argv[2]);
  termPort = stoi(argv[3]);
  
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
  
  running = 1;
  while(running) { //user control loop
    string cmdline, cmd;
    string arg = "";
    string useThread = "";
    
    cout << "myftp> ";
    getline(cin, cmdline);
    stringstream ss(cmdline);
    ss >> cmd >> arg >> useThread;
    
    if (useThread == "&") {
      thread threadCommand(handleCommand, cmdline, cmd, arg, 1);
      activeThreads++;
      threadCommand.detach();
    } else
      handleCommand(cmdline, cmd, arg, 0);
    
    cout << endl << endl;
  
  }
  
  while (activeThreads > 0)
    sleep(1);
  
  close(sock);
  exit(1);
}