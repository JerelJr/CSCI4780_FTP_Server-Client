#include <iostream>
#include <sstream>
#include <string>
#include <cstring>

#include <filesystem>
#include <random>

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <thread>

using namespace std;

int serving = 1;
int taskList[100][2] = {0};
int testing;
int consoleSocket;

default_random_engine generator;
uniform_int_distribution<> distribution(100000,999999);

void error(const char *msg) {perror(msg); exit(0); }

void snd(int sock, const char *msg) {
  if (send(sock, msg, strlen(msg), 0) == -1)
    error("Could not send message to server");
}

void fullClose(int sock) {
  shutdown(sock, SHUT_RDWR);
  close(sock);
}

//clear the socket of all remaining data
void flushSock(int sock) {
  
  int ret;
  char flushBuffer[1024];
  fd_set set;
  struct timeval  tv;
  tv.tv_sec = 0;
  tv.tv_usec = 100000;
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

void handleClient(int sock, int tid) {
  cout << "Client " << tid << " connected." << endl;
  int connected = 1;
  
  while (connected && serving) {
    char buffer[1024] = {0};
    int ret = recv(sock, buffer, sizeof(buffer), 0);
    stringstream msg(buffer);
    string line(buffer);
    string cmd, arg, arg2;
    msg >> cmd >> arg >> arg2; 
    
    if (ret == 0) //client disconnected
      break;
    else if (ret == -1) {
      perror("Socket read failed");
      cmd = "quit";
    }
    cout << "Client " << tid << ": " << line << endl;
    
    if (cmd == "quit")
      connected = 0;
    else if (cmd == "shutdown") {
      serving = 0;
      connected = 0;
    } else if (cmd == "get") {
    
      FILE *file = fopen(arg.c_str(), "rb");
      if(file == NULL){
        snd(sock, "ERR File could not be found on server");
        continue;
      }
      
      //send termination code and filesize to client
      string filesize = to_string(filesystem::file_size(arg));
      string startGet = "MSG " + to_string(tid) + " " + filesize;
      snd(sock, startGet.c_str());
      //set a task number in the array
      int taskNum = -1;
      while (taskNum == -1) { //loop in case taskList is full
        for (taskNum = 0; taskNum < 100; taskNum++) {
          if (taskList[taskNum][0] == 0) {
            taskList[taskNum][0] = tid;
            taskList[taskNum][1] = 0;
            break;
          }
        }
        sleep(1);
      }
      
      
      char getBuf[1024] = {0};
      
      size_t rec_len = -1;
      
      while((rec_len = fread(getBuf, 1, 1024, file)) > 0) {
        if (taskList[taskNum][1] == 1) { //operation terminated
          taskList[taskNum][1] = 0; //reset
          break;
        }
        send(sock, getBuf, rec_len, 0);
      }
      
      recv(sock, getBuf, sizeof(getBuf), 0); //wait for acknowledgement from client
      
      fclose(file);
      taskList[taskNum][0] = 0; //clear task from list
      
    } else if (cmd == "put") {
    
      string terminationMessage = "MSG " + to_string(tid);
      snd(sock, terminationMessage.c_str());
      //set a task number in the array
      int taskNum = -1;
      while (taskNum == -1) { //loop in case taskList is full
        for (taskNum = 0; taskNum < 100; taskNum++) {
          if (taskList[taskNum][0] == 0) {
            taskList[taskNum][0] = tid;
            taskList[taskNum][1] = 0;
            break;
          }
        }
        sleep(1);
      }
      
      //use temporary name while downloading so original file is
      //preserved in case command is terminated
      string tempNum = to_string(distribution(generator));
      string tempName = arg + "-temp" + tempNum;
      FILE *file = fopen(tempName.c_str(), "wb");
      
      char putBuf[1024] = {0};
      
      int fileSize = stoi(arg2);
      int written = 0;
      int i = 0;
      size_t rec_len = -1;
      while (written < fileSize) {
        rec_len = recv(sock, putBuf, sizeof(putBuf), 0);
        if (taskList[taskNum][1] == 1) { //operation terminated
          cout << "Command terminated" << endl;
          flushSock(sock);
          break;
        } else if (rec_len == 0) { //client disconnected
          taskList[taskNum][1] = 1;
          break;
        }
        fwrite(putBuf, 1, rec_len, file);
        written += rec_len;
        if (testing) {
          i++;
          if (i % 500 == 0)
            cout << "Written " << i << " kilobytes.\n";
        }
      }
      if (taskList[taskNum][1] != 1)
        snd(sock, "MSG File upload successful");
      else
        snd(sock, "MSG File upload terminated");
      if (taskList[taskNum][1] != 1)
        filesystem::rename(tempName, arg);
      else
        filesystem::remove(tempName);
      
      if(testing) cout << "Write done.\n";
      fclose(file);
      taskList[taskNum][0] = 0; //clear task from list
      taskList[taskNum][1] = 0;
      
    } else if (cmd == "delete") {
      try {
        if (filesystem::remove(arg))
          snd(sock, "MSG File successfully deleted");
        else {
          string err_msg = "ERR File '";
          err_msg.append(arg).append("' not found");
          snd(sock, err_msg.c_str());
        }
      } catch (const std::exception &e) {
        char err_msg[1024];
        snprintf(err_msg, sizeof(err_msg), "ERR %s", e.what());
        snd(sock, err_msg);
      }
    } else if (cmd == "cd") {
      try {
        filesystem::current_path(arg);
        snd(sock, "MSG cd success");
      } catch (const exception &e) {
        char err_msg[1024];
        snprintf(err_msg, sizeof(err_msg), "ERR %s", e.what());
        snd(sock, err_msg);
      }
    } else if (cmd == "mkdir") {
      try {
        filesystem::create_directory(arg);
        snd(sock, "MSG Directory successfully created");
      } catch (const exception &e) {
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
  
  fullClose(sock);
}

void termFunc(int termSock, int portNo) {
  char buffer[1024] = {0};
  
  if (termSock == -1) error("Socket creation failed");
  sockaddr_in termAddress;
  termAddress.sin_family = AF_INET;
  termAddress.sin_port = htons(portNo);
  termAddress.sin_addr.s_addr = INADDR_ANY;
  
  bind(termSock, (struct sockaddr*)&termAddress, sizeof(termAddress));
  
  while (serving) {
    listen(termSock, 5);

    int terminator = accept(termSock, nullptr, nullptr);
    if (terminator != -1) {
      
      memset(buffer, 0, sizeof(buffer));
      int size = recv(terminator, buffer, sizeof(buffer), 0); //receive data from server
      if (size < 0)
        error("Error receiving data from server");
      
      char* p_end{};
      int taskNum = strtol(buffer, &p_end, 10);
      
      if (taskNum == 0) {
        cout << "Invalid termination task number" << endl;
        fullClose(terminator);
        continue;
      }
      
      cout << "Terminating task " << taskNum << endl;
      int taskFound = 0;
      for (int i = 0; i < 100; i++) {
        if (taskList[i][0] == taskNum) { //task to be terminated found
          taskList[i][1] = 1;
          taskFound = 1;
          break;
        }
      }
      if (!taskFound)
        cout << "Task could not be found.\n";
      fullClose(terminator);
    }
  }
}

int main (int argc, char * argv[]) {
  if (argc < 3) error("Correct usage: ./myftpserver <port number> <terminate port number>");
  
  int port = stoi(argv[1]);
  int termPort = stoi(argv[2]);
  if (argc == 4) testing = stoi(argv[3]);
  
  //create server socket with address
  int servSock = socket(AF_INET, SOCK_STREAM, 0);
  if (servSock == -1) error("Socket creation failed");
  sockaddr_in serverAddress;
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_port = htons(port);
  serverAddress.sin_addr.s_addr = INADDR_ANY;
  
  bind(servSock, (struct sockaddr*)&serverAddress, sizeof(serverAddress));
  
  int termSock = socket(AF_INET, SOCK_STREAM, 0);
  thread termThread(termFunc, termSock, termPort);
  
  int tid = 1;
  while (serving) {
    listen(servSock, 5);
    
    fd_set set;
    FD_ZERO(&set);
    FD_SET(servSock, &set);
    FD_SET(0, &set);
    
    select(servSock+1, &set, NULL, NULL, NULL);
    if(FD_ISSET(STDIN_FILENO, &set)){
      string input;
      getline(cin,input);
      if (input == "shutdown") {
        serving = 0;
        cout<<"\nServer shutting down.\n";
      }
    } else if(FD_ISSET(servSock, &set)) {
      int clientSock = accept(servSock, nullptr, nullptr);
      if (clientSock == -1) 
        cout << "Client connection failed";
      else {
        handleClient(clientSock, tid);
        tid++;
      }
    }
  }
  
  fullClose(servSock);
  fullClose(termSock);
  termThread.join();
  
  
  exit(1);

}