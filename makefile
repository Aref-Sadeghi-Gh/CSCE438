# makefile

all: server client

client: crc.cpp 
	g++ -g -w -std=c++17 -o client crc.cpp -lpthread

server: crsd.cpp
	g++ -g -w -std=c++17 -o server crsd.cpp -lpthread

clean:
	rm -rf *.o server client 