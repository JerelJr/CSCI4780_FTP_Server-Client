#include <iostream>
#include <sstream>
#include <string>
#include <fstream>

using namespace std;

int main (int argc, char * argv[]) {
  if (argc != 3) //bad number of parameters
  {
    printf("Correct usage: ./myftp <server address> <port number>\n");
    exit(0);
  }
  
  printf("Attempting connection to: %s at port %s\n\n", argv[1], argv[2]);
  
  //TODO: socket connection to server w/ appropriate error checking
  
  while(1) { //user control loop
    string cmdline;
    string cmd, tmp;
    string arg = "";
    
    printf("myftp> ");
    getline(cin, cmdline);
    stringstream ss(cmdline);
    ss >> cmd >> arg;
    
    if (cmd == "get") {
      if (arg.empty()) {
        cout << "Invalid command. Usage: get <remote_filename>";
      } else {
        cout << "Downloading file " << arg;
        //TODO: implement
        ofstream getFile(arg);
        getFile << "Test get file" << endl;
        getFile.close();
      }
    
    } else if (cmd == "put") {
      if (arg.empty()) {
        cout << "Invalid command. Usage: put <local_filename>";
      } else {
        cout << "Uploading file " << arg;
        
        ifstream putFile(arg);
        //inFile.open(argv[1]);
        if(putFile.fail()) {
          cout << endl << "File could not be opened. Make sure the file name is correct.";
        } else {
          cout << endl << "File contents: " << putFile.rdbuf();
        }
        putFile.close();
        //TODO: implement
      }
    
    } else if (cmd == "delete") {
      if (arg.empty()) {
        cout << "Invalid command. Usage: delete <remote_filename>";
      } else {
        cout << "Deleting file " << arg;
        //TODO: implement
      }
    
    } else if (cmd == "cd") {
      if (arg.empty()) {
        cout << "Invalid command. Usage: cd <remote_directory_name>";
      } else {
        cout << "Moving to " << arg;
        //TODO: implement
      }
    
    } else if (cmd == "mkdir") {
      if (arg.empty()) {
        cout << "Invalid command. Usage: mkdir <remote_directory_name>";
      } else {
        cout << "Making directory " << arg;
        //TODO: implement
      }
    
    } else if (cmd == "ls") {
      cout << "Files in this directory: ";
      //TODO: implement
    } else if (cmd == "pwd") {
      cout << "Current directory: ";
      //TODO: implement
    } else if (cmd == "quit") {
      cout << "Program exiting..." << endl << endl;
      //TODO: end connection and close socket
      exit(1);
    } else {
      cout << "Unknown command";
    }
    
    cout << endl << endl;
  
  }
  
  exit(1);
}