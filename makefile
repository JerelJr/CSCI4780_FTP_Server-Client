OPTS = -Wall -pedantic-errors -std=c++17 -pthread
N1 = myftp
N2 = myftpserver

compile:
	g++ $(OPTS) -o client/$(N1) $(N1).cpp
	g++ $(OPTS) -o server/$(N2) $(N2).cpp

client:
	g++ $(OPTS) -o client/$(N1) $(N1).cpp

server:
	g++ $(OPTS) -o server/$(N2) $(N2).cpp

clean:
	rm -f client/$(N1)
	rm -f server/$(N2)