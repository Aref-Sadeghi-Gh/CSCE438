#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "interface.h"
#include <iostream>
#include <string>
#include <arpa/inet.h>
#include <sstream>
#include <map>
#include <algorithm>
#include <thread>

using namespace std;


/*
 * TODO: IMPLEMENT BELOW THREE FUNCTIONS
 */
int connect_to(const char* host, const int port);
struct Reply process_command(const int sockfd, char* command);
void process_chatmode(const char* host, const int port);
void send_message(int room_socket);
void recv_message(int room_socket);

Reply reply;
thread t_send, t_recv;
bool exit_flag = false;

int main(int argc, char** argv)
{
	if (argc != 3) {
		fprintf(stderr,
			"usage: enter host address and port number\n");
		exit(1);
	}

	reply.num_member = 0;
	display_title();

	while (1) {
		exit_flag = false;
		int sockfd = connect_to(argv[1], atoi(argv[2]));
		char command[MAX_DATA];
		get_command(command, MAX_DATA);

		struct Reply reply = process_command(sockfd, command);
		display_reply(command, reply);

		touppercase(command, strlen(command) - 1);
		if (strncmp(command, "JOIN", 4) == 0) {
			process_chatmode(argv[1], reply.port);
		}
		close(sockfd);
	}

	return 0;
}

/*
 * Connect to the server using given host and port information
 *
 * @parameter host    host address given by command line argument
 * @parameter port    port given by command line argument
 *
 * @return socket fildescriptor
 */
int connect_to(const char* host, const int port)
{
	// ------------------------------------------------------------
	// GUIDE :
	// In this function, you are suppose to connect to the server.
	// After connection is established, you are ready to send or
	// receive the message to/from the server.
	// 
	// Finally, you should return the socket fildescriptor
	// so that other functions such as "process_command" can use it
	// ------------------------------------------------------------

	// below is just dummy code for compilation, remove it.

	//create a message buffer 
	//setup a socket and connection tools 
	struct hostent* serverIp = gethostbyname(host);
	sockaddr_in sendSockAddr;
	bzero((char*)&sendSockAddr, sizeof(sendSockAddr));
	sendSockAddr.sin_family = AF_INET;
	sendSockAddr.sin_addr.s_addr =
		inet_addr(inet_ntoa(*(struct in_addr*)*serverIp->h_addr_list));
	//inet_pton(AF_INET, host, &sendSockAddr.sin_addr);
	sendSockAddr.sin_port = htons(port);
	int clientSd = socket(AF_INET, SOCK_STREAM, 0);
	//try to connect...
	int status = connect(clientSd,
		(sockaddr*)&sendSockAddr, sizeof(sendSockAddr));
	if (status < 0)
	{
		cout << "Error connecting to socket!" << endl;
	}

	return clientSd;
}

/*
 * Send an input command to the server and return the result
 *
 * @parameter sockfd   socket file descriptor to commnunicate
 *                     with the server
 * @parameter command  command will be sent to the server
 *
 * @return    Reply
 */
struct Reply process_command(const int sockfd, char* command)
{
	// ------------------------------------------------------------
	// GUIDE 1:
	// In this function, you are supposed to parse a given command
	// and create your own message in order to communicate with
	// the server. Surely, you can use the input command without
	// any changes if your server understand it. The given command
	// will be one of the followings:
	//
	// CREATE <name>
	// DELETE <name>
	// JOIN <name>
	// LIST
	//
	// -  "<name>" is a chatroom name that you want to create, delete,
	// or join.
	// 
	// - CREATE/DELETE/JOIN and "<name>" are separated by one space.
	// ------------------------------------------------------------


	// ------------------------------------------------------------
	// GUIDE 2:
	// After you create the message, you need to send it to the
	// server and receive a result from the server.
	// ------------------------------------------------------------


	// ------------------------------------------------------------
	// GUIDE 3:
	// Then, you should create a variable of Reply structure
	// provided by the interface and initialize it according to
	// the result.
	//
	// For example, if a given command is "JOIN room1"
	// and the server successfully created the chatroom,
	// the server will reply a message including information about
	// success/failure, the number of members and port number.
	// By using this information, you should set the Reply variable.
	// the variable will be set as following:
	//
	// Reply reply;
	// reply.status = SUCCESS;
	// reply.num_member = number;
	// reply.port = port;
	// 
	// "number" and "port" variables are just an integer variable
	// and can be initialized using the message fomr the server.
	//
	// For another example, if a given command is "CREATE room1"
	// and the server failed to create the chatroom becuase it
	// already exists, the Reply varible will be set as following:
	//
	// Reply reply;
	// reply.status = FAILURE_ALREADY_EXISTS;
	// 
	// For the "LIST" command,
	// You are suppose to copy the list of chatroom to the list_room
	// variable. Each room name should be seperated by comma ','.
	// For example, if given command is "LIST", the Reply variable
	// will be set as following.
	//
	// Reply reply;
	// reply.status = SUCCESS;
	// strcpy(reply.list_room, list);
	// 
	// "list" is a string that contains a list of chat rooms such 
	// as "r1,r2,r3,"
	// ------------------------------------------------------------

	// REMOVE below code and write your own Reply.


	bool valid = false;

	char msg[1025];
	string command_cpy = command;

	while (!valid) {
		touppercase(command, strlen(command) - 1);
		if (strncmp(command, "JOIN", 4) == 0 || strncmp(command, "CREATE", 6) == 0 || strncmp(command, "DELETE", 6) == 0 || strncmp(command, "LIST", 4) == 0) {
			valid = true;
		}
		if (!valid) {
			cout << "invalid command" << endl;
			get_command(command, MAX_DATA);

		}

	}
	memset(&msg, 0, sizeof(msg));//clear the buffer
	//strcpy(msg, command);


	//struct Reply reply;
	//reply.status = SUCCESS;
	bool found = false;

	if (strncmp(command, "CREATE", 6) == 0) {
		command_cpy.erase(0, 7);
		msg[0] = 'C';
		int j = 1;
		for (int i = 0; i < command_cpy.size(); i++) {
			msg[j] = command_cpy[i];
			j++;
		}

		send(sockfd, (char*)&msg, strlen(msg), 0);
		memset(&msg, 0, sizeof(msg));//clear the buffer
		recv(sockfd, (char*)&msg, sizeof(msg), 0);
		string portStr;
		string socketStr;
		string ptr(msg);
		stringstream parser(ptr);
		int portNum;
		int socketFD;
		parser >> socketFD >> portNum;
		if (parser.fail()) {
			reply.status = FAILURE_ALREADY_EXISTS;
		}
		else {
			reply.status = SUCCESS;
		}

	}
	else if (strncmp(command, "JOIN", 4) == 0) {
		//string key;
		command_cpy.erase(0, 5);
		msg[0] = 'J';
		int j = 1;
		for (int i = 0; i < command_cpy.size(); i++) {
			msg[j] = command_cpy[i];
			j++;
		}

		send(sockfd, (char*)&msg, strlen(msg), 0);
		memset(&msg, 0, sizeof(msg));//clear the buffer
		recv(sockfd, (char*)&msg, sizeof(msg), 0);
		int portNum;
		string message(msg);
		stringstream parser(msg);
		parser >> portNum;
		if (parser.fail()) {
			reply.status = FAILURE_NOT_EXISTS;
		}
		else {
			reply.status = SUCCESS;
			reply.port = portNum;
			reply.num_member++;
		}
	}

	else if (strncmp(command, "LIST", 4) == 0) {
		command_cpy.erase(0, 5);
		msg[0] = 'L';

		send(sockfd, (char*)&msg, strlen(msg), 0);
		memset(&msg, 0, sizeof(msg));//clear the buffer
		recv(sockfd, (char*)&msg, sizeof(msg), 0);
		string message(msg);

		for (int i= 0; i < 256; i++)
			reply.list_room[i] = msg[i];
		reply.status = SUCCESS;
	}

	else if (strncmp(command, "DELETE", 6) == 0) {
		command_cpy.erase(0, 7);
		msg[0] = 'D';
		int j = 1;
		for (int i = 0; i < command_cpy.size(); i++) {
			msg[j] = command_cpy[i];
			j++;
		}

		send(sockfd, (char*)&msg, strlen(msg), 0);
		memset(&msg, 0, sizeof(msg));//clear the buffer
		recv(sockfd, (char*)&msg, sizeof(msg), 0);
		string message(msg);
		if (message.compare("1") == 0) {
			reply.status = SUCCESS;
		}
		else {
			reply.status = FAILURE_NOT_EXISTS;
		}
	}

	return reply;
}

/*
 * Get into the chat mode
 *
 * @parameter host     host address
 * @parameter port     port
 */
void process_chatmode(const char* host, const int port)
{
	// ------------------------------------------------------------
	// GUIDE 1:
	// In order to join the chatroom, you are supposed to connect
	// to the server using host and port.
	// You may re-use the function "connect_to".
	// ------------------------------------------------------------

	// ------------------------------------------------------------
	// GUIDE 2:
	// Once the client have been connected to the server, we need
	// to get a message from the user and send it to server.
	// At the same time, the client should wait for a message from
	// the server.
	// ------------------------------------------------------------

	// ------------------------------------------------------------
	// IMPORTANT NOTICE:
	// 1. To get a message from a user, you should use a function
	// "void get_message(char*, int);" in the interface.h file
	// 
	// 2. To print the messages from other members, you should use
	// the function "void display_message(char*)" in the interface.h
	//
	// 3. Once a user entered to one of chatrooms, there is no way
	//    to command mode where the user  enter other commands
	//    such as CREATE,DELETE,LIST.
	//    Don't have to worry about this situation, and you can 
	//    terminate the client program by pressing CTRL-C (SIGINT)
	// ------------------------------------------------------------

	if (reply.status == SUCCESS) {
		//cout << "I am here" << endl;

		printf("Now you are in the chatmode\n");
		int room_socket = connect_to(host, port);
		thread t1(send_message, room_socket);
		thread t2(recv_message, room_socket);

		t_send = move(t1);
		t_recv = move(t2);

		if (t_send.joinable())
			t_send.join();
		if (t_recv.joinable())
			t_recv.join();
	}

}

void send_message (int room_socket) {

	while (true) {
		if (exit_flag)
			break;
		char str[MAX_DATA];
		get_message(str, MAX_DATA);
		send(room_socket, str, sizeof(str), 0);
	}

}

void recv_message(int room_socket) {
	while (true) {
		char str[MAX_DATA];
		int bytes_received = recv(room_socket, str, sizeof(str), 0);
		if (bytes_received <= 0)
			continue;
		display_message(str);
		cout << endl;
		if (strncmp(str, "Warning: the chatting room is going to be closed...", 51) == 0) {
			exit_flag = true;
			//close(room_socket);
			break;
		}
	}
}

