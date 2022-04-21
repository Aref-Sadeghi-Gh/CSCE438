Design Documents:

I started working on the assignment by reading the requirements. For the relationship between servers and coordinators, each server's 
information is being stored in the coordinator's vector of Servers. The has two threads, one for listening to clients' requests, and 
the other for sending heartbeats to the coordinator to indicate and it is alive. The coordinator listens has two threads too. One that 
listens to new connections and requests, and the other that listens to heartbeats and updates server's information. If the Master server 
dies during the connection and fails to send heartbeats, then the Master server becomes slave and the Slave server becomes Master and 
starts communicating with the clients once they reconnect.

Clients first connect to the coordinator and once a live server with the same ID is found, they are connected to that server which must 
be of type "Master." Clients send all their requests to the server.

Synchronizers connect to a cluster. If a cluster does not exist, they just do not connect. After connection, they keep track of the 
following/followers of the clients in the cluster and let other synchronizers know if there is a change to the following/followers of 
one of their clients. Synchronizers only communicate through the coordinator and process Follow and Timeline.