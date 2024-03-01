OPTS = -Wall -pedantic-errors -std=c++17 -pthread
N1 = myftp
N2 = myftpserver

compile: myftp.cpp myftpserver.cpp
	g++ $(OPTS) -o client/$(N1) $(N1).cpp
	g++ $(OPTS) -o server/$(N2) $(N2).cpp

client: myftp.cpp
	g++ $(OPTS) -o client/$(N1) $(N1).cpp

server: myftpserver.cpp
	g++ $(OPTS) -o server/$(N2) $(N2).cpp
 
tst: tst.cpp
	g++ $(OPTS) -o tst tst.cpp

clean:
	rm -f client/$(N1)
	rm -f server/$(N2)