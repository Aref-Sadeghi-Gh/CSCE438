# makefile

all: server client

client: crc.cpp 
	g++ -g -w -std=c++17 -o client crc.cpp

server: crsd.cpp
	g++ -g -w -std=c++17 -o server crsd.cpp

clean:
	rm -rf *.o fifo* server client 