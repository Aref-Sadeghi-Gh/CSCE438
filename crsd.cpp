#include <stdio.h> 
#include <string.h>   //strlen 
#include <stdlib.h> 
#include <errno.h> 
#include <unistd.h>   //close 
#include <arpa/inet.h>    //close 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros 
#include "interface.h"
#include <vector>
#include <string>
#include <iostream>
#include <map>
#include <thread>
#include <fcntl.h>
#include <mutex>

#define TRUE   1 
#define FALSE  0 
#define PORT 8888 

using namespace std;

void create_socket(sockaddr_in &chat_address, int &mast_socket, int port, map<int, int>& connInfo);
void handle_client(int client_socket, int id);
void handle_room(int room_socket, int roomSeed, string roomKey);
void show_message(string message, int id, string roomKey);

enum roomStatus {
    OPEN,
    CLOSE
};

struct terminal {
    int id;
    string name;
    int socket;
    thread th;
};

struct roomTerminal {
    int id;
    string roomName;
    int socket;
    thread th;
    roomStatus status;
};


map<string, int> chatroom;
bool roomOpen = false;
bool acceptConn = false;
map<int, int> connInfo;
vector<terminal> clients;
vector<roomTerminal> rooms;
int seed, roomSeed = 0;
int new_port = 1024;
int master_room_socket, room_socket;
int master_socket, client_socket;
mutex cout_mtx, clients_mtx, room_mtx;
struct sockaddr_in address, chat_address;

int main(int argc, char* argv[])
{
    int opt = TRUE;
    int master_socket, new_socket, client_socket,
        max_clients = 10, activity, i, valread, sd, master_room_socket, new_room_socket, new_length, room_sockets[10], sd_room, room_activity, valread_room;
    int max_sd, max_room_sd;

    char buffer[1025];  //data buffer of 1K 
    char msg[1025];
    char message[MAX_DATA];

    //create a master socket 
    if ((master_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    //set master socket to allow multiple connections , 
    //this is just a good habit, it will work without this 
    /*if (setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt,
        sizeof(opt)) < 0)
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }*/

    //type of socket created 
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    //bind the socket to localhost port 8888 
    if (bind(master_socket, (struct sockaddr*)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    printf("Listener on port %d \n", PORT);

    //try to specify maximum of 3 pending connections for the master socket 
    if (listen(master_socket, 5) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    //accept the incoming connection 
    struct sockaddr_in client;
    unsigned int addrlen = sizeof(client);
    puts("Waiting for connections ...");

    while (true)
    {
        //clear the socket set 
        if ((client_socket = accept(master_socket, (struct sockaddr*)&address, (socklen_t*)&addrlen)) == -1) { perror("accept error: "); exit(-1); }
        seed++;
        thread t(handle_client, client_socket, seed);
        lock_guard<mutex> guard(clients_mtx);
        clients.push_back({ seed, string("Anonymous"),client_socket,(move(t)) });

    }

    for (int i = 0; i < clients.size(); i++) {
        if (clients[i].th.joinable()) clients[i].th.join();
    }
    close(master_socket);
    return 0;
}



void create_socket(sockaddr_in &chat_address, int &mas_socket, int port, map<int, int> &connInfo) {

    if ((mas_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
       // exit(EXIT_FAILURE);
    }

    chat_address.sin_family = AF_INET;
    chat_address.sin_addr.s_addr = INADDR_ANY;
    chat_address.sin_port = htons(port);

    //bind the socket to localhost port 8888 
    if (bind(mas_socket, (struct sockaddr*)&chat_address, sizeof(chat_address)) < 0)
    {
        perror("bind failed");
        //exit(EXIT_FAILURE);
    }

    if (listen(mas_socket, 3) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    connInfo.insert(pair<int, int>(port, mas_socket));

    cout << "port number is " << port << endl;

}

void handle_client(int client_socket, int id) {

    char buffer[1025];
    //strcpy(buffer, msg);

    string roomKey;
    string deleteKey;
    while (!roomOpen) {
        memset(&buffer, 0, sizeof(buffer));
        int bytesRecv = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytesRecv <= 0)
            return;

        string room = buffer;
        string response = "";
        if (strncmp(buffer, "C", 1) == 0) {
            bool success = true;
            room.erase(0, 1);
            //room.erase(2, 3);
            for (std::map<string, int>::iterator it = chatroom.begin(); it != chatroom.end(); ++it) {
                if (it->first == room) {
                    success = false;
                    break;
                }
            }
            if (success) {
                response = to_string(client_socket) + " " + to_string(ntohs(address.sin_port));
                strcpy((char*)&buffer, response.c_str());
                //cout << temp << endl;
                //temp = temp.substr(temp.find("CREATE "));
                chatroom.insert(pair<string, int>(room, new_port));
                create_socket(chat_address, master_room_socket, new_port, connInfo);
                new_port++;
            }
            send(client_socket, buffer, strlen(buffer), 0);
        }
        else if (strncmp(buffer, "J", 1) == 0) {

            bool found = false;
            room.erase(0, 1);
            //room.erase(2, 3);
            for (std::map<string, int>::iterator it = chatroom.begin(); it != chatroom.end(); ++it) {
                if (it->first == room) {
                    roomKey = it->first;
                    found = true;
                    break;
                }
            }
            if (found) {
                auto iter = chatroom.find(roomKey);
                response = to_string(iter->second);
                strcpy((char*)&buffer, response.c_str());
                for (std::map<int, int>::iterator it = connInfo.begin(); it != connInfo.end(); ++it) {
                    if (it->first == iter->second) {
                        master_room_socket = it->second;
                        break;
                    }
                }
                roomOpen = true;
            }
            send(client_socket, buffer, strlen(buffer), 0);
        }

        else if (strncmp(buffer, "L", 1) == 0) {

            bool found = false;
            for (std::map<string, int>::iterator it = chatroom.begin(); it != chatroom.end(); ++it) {
                response = response += (it->first + ",");
            }
            if (response == "") {
                response = "Empty";
            }
            strcpy((char*)&buffer, response.c_str());
            send(client_socket, buffer, strlen(buffer), 0);
        }

        else if (strncmp(buffer, "D", 1) == 0) {
            bool found = false;
            room.erase(0, 1);
            //room.erase(2, 3);
            response = "0";
            for (std::map<string, int>::iterator it = chatroom.begin(); it != chatroom.end(); ++it) {
                if (it->first == room) {
                    deleteKey = it->first;
                    found = true;
                    break;
                }
            }
            if (found) {
                char closingMessage[MAX_DATA] = "Warning: the chatting room is going to be closed...";
                //char closingChar[MAX_DATA] = closingMessage.c_str();
                auto iter = chatroom.find(deleteKey);
                for (std::map<int, int>::iterator it = connInfo.begin(); it != connInfo.end(); ++it) {
                    if (it->first == iter->second) {
                        int temp_socket = it->second;
                        for (int i = 0; i < rooms.size(); i++) {
                            if (rooms[i].roomName == deleteKey && rooms[i].status == OPEN) {
                                send(rooms[i].socket, closingMessage, sizeof(closingMessage), 0);
                                rooms[i].status = CLOSE;
                                rooms[i].id = 0;
                                close(rooms[i].socket);
                                rooms[i].socket = 0;
                                rooms[i].roomName = "";
                            }
                        }
                        break;
                    }
                }
                chatroom.erase(iter);
                response = "1";
                strcpy((char*)&buffer, response.c_str());
            }
            send(client_socket, buffer, strlen(buffer), 0);
            break;
        }
    }

    struct sockaddr_in roomAddr;
    unsigned int roomAddrLen = sizeof(roomAddr);

    if (roomOpen) {

        if ((room_socket = accept(master_room_socket, (struct sockaddr*)&roomAddr, (socklen_t*)&roomAddrLen)) == -1) { perror("accept error: "); exit(-1); }
        roomSeed++;
        thread thr(handle_room, room_socket, roomSeed, roomKey);
        lock_guard<mutex> roomGuard(room_mtx);
        rooms.push_back({ roomSeed, roomKey, room_socket, (move(thr)), OPEN });
        roomOpen = false;
    }
    for (int i = 0; i < rooms.size(); i++) {
        if (rooms[i].th.joinable()) rooms[i].th.join();
    }
}

void handle_room (int room_socket, int roomId, string roomKey) {
    char buffer[1025];

    while (true) {
        memset(&buffer, 0, sizeof(buffer));
        int bytesRecv = recv(room_socket, buffer, sizeof(buffer), 0);
        if (bytesRecv <= 0)
            return;
        show_message(string(buffer), roomId, roomKey);
    }
}

void show_message(string message, int sender_id, string roomKey) {
    char temp[MAX_DATA];
    strcpy(temp, message.c_str());
    for (int i = 0; i < rooms.size(); i++) {
        if (rooms[i].id != sender_id && rooms[i].roomName == roomKey) {
            send(rooms[i].socket, temp, sizeof(temp), 0);
        }
    }
}
