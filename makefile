OPTS = -Wall -pedantic-errors -std=c++17
N1 = myftp
N2 = myftpserver

compile:
	g++ $(OPTS) -o $(N1) $(N1).cpp
	g++ $(OPTS) -o $(N2) $(N2).cpp

client:
	g++ $(OPTS) -o $(N1) $(N1).cpp

server:
	g++ $(OPTS) -o $(N2) $(N2).cpp

clean:
	rm -f $(N1)
	rm -f $(N2)
	rm -f *.o
	rm -f tes*