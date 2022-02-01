In order to complete the programming assignment, I first plan on focusing on creating the server side in the 
crsd.cpp file. In this file I will make the master socket, bind, and after that start listening for new 
connection and then accepting new connections. To handle the difference between a new client and requests from 
the same client I will make use of the select() function. in the client side, I will implement the three 
functions. In the connect_to() function, I will create a socket and try connecting to the server. After that I 
will accept command requests that start with one of the four commands. If a valid one is found, then I will 
notify the server and the server will either make a new chat room, delete one, list them all, or join the client 
to one of the already existing chatrooms. 