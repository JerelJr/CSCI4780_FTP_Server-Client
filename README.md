# CSCI4780 Project 1: Simple FTP Client and Server

## Members
Jerel Harris,
Hunter Henson, 
Pranay Udhaya

This project was done in its entirety by Jerel Harris, Hunter Henson, and Pranay Udhaya. We hereby
state that we have not received unauthorized help of any form.

Compiling:
Use 'make' to compile both files, or 'make client' and 'make server' to compile the 
client and server files respectively. The files must be compiled with the option
'-std=c++17' to ensure that the compiler uses at least C++17. 

Usage:
```
./myftpserver <port number>
./myftp <server address> <port number>
```

The port numbers must match for the client to connect. The server address can be found
by running 'hostname -i' on the machine running the server. If the client and server
are run on the same machine, <server address> can be replaced with "self".

The shutdown command can be used from the client to shut the server down gracefully.
