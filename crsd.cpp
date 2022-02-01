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
#include <unistd.h>
#include <fcntl.h>

#define TRUE   1 
#define FALSE  0 
#define PORT 8888 

using namespace std;

void create_socket(sockaddr_in &chat_address, int &mast_socket, int port, map<int, int>& connInfo, map<int, sockaddr_in>& addr_sock);

int main(int argc, char* argv[])
{
    int opt = TRUE;
    int master_socket, addrlen, new_socket, client_socket[10],
        max_clients = 10, activity, i, valread, sd, master_room_socket, new_room_socket, new_length, new_port, room_sockets[10], sd_room, room_activity, valread_room;
    int max_sd, max_room_sd;
    struct sockaddr_in address, chat_address;
    map<string, int> chatroom;
    new_port = 1024;
    bool roomOpen = false;
    bool acceptConn = false;
    map<int, int> connInfo;
    map<int, sockaddr_in> addr_sock;

    char buffer[1025];  //data buffer of 1K 
    char msg[1025];
    char message[MAX_DATA];

    //set of socket descriptors 
    fd_set readfds, readfds_room;

    //a message 
    //char* message = "ECHO Daemon v1.0 \r\n";

    //initialise all client_socket[] to 0 so not checked 
    for (i = 0; i < max_clients; i++)
    {
        client_socket[i] = 0;
        room_sockets[i] = 0;
    }

    //create a master socket 
    if ((master_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    //set master socket to allow multiple connections , 
    //this is just a good habit, it will work without this 
    if (setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt,
        sizeof(opt)) < 0)
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

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
    if (listen(master_socket, 3) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    //accept the incoming connection 
    addrlen = sizeof(address);
    puts("Waiting for connections ...");

    while (TRUE)
    {
        //clear the socket set 
        FD_ZERO(&readfds);

        //add master socket to set 
        FD_SET(master_socket, &readfds);
        max_sd = master_socket;

        //add child sockets to set 
        for (i = 0; i < max_clients; i++)
        {
            //socket descriptor 
            sd = client_socket[i];

            //if valid socket descriptor then add to read list 
            if (sd > 0)
                FD_SET(sd, &readfds);

            //highest file descriptor number, need it for the select function 
            if (sd > max_sd)
                max_sd = sd;
        }

        //wait for an activity on one of the sockets , timeout is NULL , 
        //so wait indefinitely 
        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if ((activity < 0) && (errno != EINTR))
        {
            printf("select error");
        }

        //If something happened on the master socket , 
        //then its an incoming connection 
        if (FD_ISSET(master_socket, &readfds))
        {
            if ((new_socket = accept(master_socket,
                (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0)
            {
                perror("accept");
                exit(EXIT_FAILURE);
            }

            //inform user of socket number - used in send and receive commands 
           // printf("New connection , socket fd is %d , ip is : %s , port : %d \n",
             //   new_socket, inet_ntoa(address.sin_addr), ntohs(address.sin_port));

            //send new connection greeting message 
            /*if( send(new_socket, message, strlen(message), 0) != strlen(message) )
            {
                perror("send");
            }

            puts("Welcome message sent successfully");  */

            //add new socket to array of sockets 
            for (i = 0; i < max_clients; i++)
            {
                //if position is empty 
                if (client_socket[i] == 0)
                {
                    client_socket[i] = new_socket;
                    //printf("Adding to list of sockets as %d\n", i);

                    break;
                }
            }
        }

        //else its some IO operation on some other socket
        for (i = 0; i < max_clients; i++)
        {
            sd = client_socket[i];

            if (FD_ISSET(sd, &readfds))
            {
                //Check if it was for closing , and also read the 
                //incoming message 
                if ((valread = recv(sd, (char*)&msg, strlen(msg), 0)) == 0)
                {
                    //Somebody disconnected , get his details and print 
                    getpeername(sd, (struct sockaddr*)&address, \
                        (socklen_t*)&addrlen);
                    printf("Host disconnected , ip %s , port %d \n",
                        inet_ntoa(address.sin_addr), ntohs(address.sin_port));

                    //Close the socket and mark as 0 in list for reuse 
                    close(sd);
                    client_socket[i] = 0;
                }

                //Echo back the message that came in 
                else
                {
                    //set the string terminating NULL byte on the end 
                    //of the data read
                    memset(&buffer, 0, sizeof(buffer));
                    strcpy(buffer, msg);

                    string temp = buffer;
                    temp.erase(0, 1);
                    temp.erase(2, 3);
                    string key;
                    if (strncmp(buffer, "C", 1) == 0) {
                        bool success = true;
                        for (std::map<string, int>::iterator it = chatroom.begin(); it != chatroom.end(); ++it) {
                            if (it->first == temp) {
                                success = false;
                                break;
                            }
                        }
                        if (success) {
                            string tempBuf = to_string(sd) + " " + to_string(ntohs(address.sin_port));
                            strcpy((char*)&buffer, tempBuf.c_str());
                            //cout << temp << endl;
                            //temp = temp.substr(temp.find("CREATE "));
                            chatroom.insert(pair<string, int>(temp, new_port));
                            create_socket(chat_address, master_room_socket, new_port, connInfo, addr_sock);
                            new_port++;
                        }
                        //send(sd, buffer, strlen(buffer), 0);
                    }
                    else if (strncmp(buffer, "J", 1) == 0) {

                        bool found = false;
                        for (std::map<string, int>::iterator it = chatroom.begin(); it != chatroom.end(); ++it) {
                            if (it->first == temp) {
                                key = it->first;
                                found = true;
                                break;
                            }
                        }
                        if (found) {
                            auto iter = chatroom.find(key);
                            string response = to_string(iter->second);
                            strcpy((char*)&buffer, response.c_str());
                            for (std::map<int, int>::iterator it = connInfo.begin(); it != connInfo.end(); ++it) {
                                if (it->first == iter->second) {
                                    master_room_socket = it->second;
                                    break;
                                }
                            }
                            for (std::map<int, sockaddr_in>::iterator it = addr_sock.begin(); it != addr_sock.end(); ++it) {
                                if (it->first == master_room_socket) {
                                    chat_address = it->second;
                                    break;
                                }
                            }
                            roomOpen = true;
                        }
                        //send(sd, buffer, strlen(buffer), 0);
                    }

                    else if (strncmp(buffer, "L", 1) == 0) {

                        bool found = false;
                        string rooms = "";
                        for (std::map<string, int>::iterator it = chatroom.begin(); it != chatroom.end(); ++it) {
                            rooms += (it->first + ",");
                        }
                        strcpy((char*)&buffer, rooms.c_str());
                        //send(sd, buffer, strlen(buffer), 0);
                    }

                    else if (strncmp(buffer, "D", 1) == 0) {
                        bool found = false;
                        string response = "0";
                        for (std::map<string, int>::iterator it = chatroom.begin(); it != chatroom.end(); ++it) {
                            if (it->first == temp) {
                                key = it->first;
                                found = true;
                                break;
                            }
                        }
                        if (found) {
                            auto iter = chatroom.find(key);
                            chatroom.erase(iter);
                            response = "1";
                            strcpy((char*)&buffer, response.c_str());
                        }
                        //send(sd, buffer, strlen(buffer), 0);
                    }
                    send(sd, buffer, strlen(buffer), 0);
                }
            }
        }
        if (roomOpen) {

            FD_ZERO(&readfds_room);

            //add master socket to set 
            FD_SET(master_room_socket, &readfds_room);
            max_room_sd = master_room_socket;

            //add child sockets to set 
            for (i = 0; i < max_clients; i++)
            {
                //socket descriptor 
                sd_room = room_sockets[i];

                //if valid socket descriptor then add to read list 
                if (sd_room > 0)
                    FD_SET(sd_room, &readfds_room);

                //highest file descriptor number, need it for the select function 
                if (sd_room > max_room_sd)
                    max_room_sd = sd_room;
            }
            //wait for an activity on one of the sockets , timeout is NULL , 
            //so wait indefinitely 

            room_activity = select(max_room_sd + 1, &readfds_room, NULL, NULL, NULL);

            if ((room_activity < 0) && (errno != EINTR))
            {
                printf("select error");
            }

            //If something happened on the master socket , 
            //then its an incoming connection 
            if (FD_ISSET(master_room_socket, &readfds_room))
            {
                if ((new_room_socket = accept(master_room_socket,
                    (struct sockaddr*)&chat_address, (socklen_t*)&new_length)) < 0)
                {
                    perror("accept");
                    exit(EXIT_FAILURE);
                }

                //inform user of socket number - used in send and receive commands 
                    printf("New connection to chatroom , socket fd is %d , ip is : %s , port : %d \n",
                        new_room_socket, inet_ntoa(chat_address.sin_addr), ntohs(chat_address.sin_port));

                //send new connection greeting message 
                /*if( send(new_socket, message, strlen(message), 0) != strlen(message) )
                {
                    perror("send");
                }

                puts("Welcome message sent successfully");  */

                //add new socket to array of sockets 
                for (i = 0; i < max_clients; i++)
                {
                    //if position is empty 
                    if (room_sockets[i] == 0)
                    {
                        room_sockets[i] = new_room_socket;
                        //printf("Adding to list of sockets as %d\n", i);

                        break;
                    }
                }
            }

            //else its some IO operation on some other socket
            for (i = 0; i < max_clients; i++)
            {
                sd_room = room_sockets[i];

                if (FD_ISSET(sd_room, &readfds_room))
                {
                    //Check if it was for closing , and also read the 
                    //incoming message 
                    if ((valread_room = recv(sd_room, (char*)&message, strlen(message), 0)) == 0)
                    {
                        //Somebody disconnected , get his details and print 
                        getpeername(sd_room, (struct sockaddr*)&chat_address, \
                            (socklen_t*)&new_length);
                        printf("Host disconnected , ip %s , port %d \n",
                            inet_ntoa(chat_address.sin_addr), ntohs(chat_address.sin_port));

                        //Close the socket and mark as 0 in list for reuse 
                        close(sd_room);
                        room_sockets[i] = 0;
                    }
                    else {
                        //message[valread] = '\0';
                        send(sd_room, (char*)&message, strlen(message), 0);
                    }
                }
            }
        }
    }

    return 0;
}


void create_socket(sockaddr_in &chat_address, int &mas_socket, int port, map<int, int> &connInfo, map<int, sockaddr_in> &addr_sock) {

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
    addr_sock.insert(pair<int, sockaddr_in>(mas_socket, chat_address));

    cout << "port number is " << port << endl;

}
