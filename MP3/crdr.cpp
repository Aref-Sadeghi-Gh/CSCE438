#include <ctime>

#include <google/protobuf/timestamp.pb.h>
#include <google/protobuf/duration.pb.h>

#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <stdlib.h>
#include <unistd.h>
#include <random>
#include <google/protobuf/util/time_util.h>
#include <grpc++/grpc++.h>
#include <chrono>
#include <thread>

#include "sns.grpc.pb.h"

using google::protobuf::Timestamp;
using google::protobuf::Duration;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::ServerReaderWriter;
using grpc::ServerWriter;
using grpc::Status;
using csce438::Message;
using csce438::ListReply;
using csce438::Request;
using csce438::Reply;
using csce438::SNSService;

using namespace std;

struct Client {
  std::string username;
  bool connected = true;
  int following_file_size = 0;
  std::vector<Client*> client_followers;
  std::vector<Client*> client_following;
  ServerReaderWriter<Message, Message>* stream = 0;
  bool operator==(const Client& c1) const{
    return (username == c1.username);
  }
};

struct Server_ {
  std::string type;
  std::string id;
  std::string port;
  std::string hostname;
  chrono::time_point<std::chrono::system_clock> start = chrono::system_clock::now();
  bool heartbeat = true;
  //int following_file_size = 0;
  std::vector<Client*> client_managed;
  //ServerReaderWriter<Message, Message>* stream = 0;
  bool operator==(const Server_& s1) const{
    return (id == s1.id);
  }
};

struct Synchronizer {
  std::string id;
  std::string port;
  bool connected = true;
  //int following_file_size = 0;
  std::vector<Client*> client_managed;
  //ServerReaderWriter<Message, Message>* stream = 0;
  bool operator==(const Synchronizer& s1) const{
    return (id == s1.id);
  }
};

//Vector that stores every server that has been created
std::vector<Server_> server_db;

//Vector that stores every synchronizers that has been created
std::vector<Synchronizer> sync_db;

//Vector that stores every client that has been created
std::vector<Client> client_db;

int num_servers = 0;

//Helper function used to find a Client object given its username
int find_user(std::string username){
  int index = 0;
  for(Client c : client_db){
    if(c.username == username)
      return index;
    index++;
  }
  return -1;
}

int find_server(std::string id, string type){
  int index = 0;
  for(Server_ s : server_db){
    if(s.id == id && s.type == type)
      return index;
    index++;
  }
  return -1;
}

int find_sync(std::string id){
  int index = 0;
  for(Synchronizer s : sync_db){
    if(s.id == id)
      return index;
    index++;
  }
  return -1;
}

int find_client_server(string username){
  int index = 0;
  for(Server_ s : server_db){
    for (int i = 0; i < s.client_managed.size(); i++){
      if (s.client_managed.at(i)->username == username && s.type == "Master")
        return index;
    }
    index++;
  }
  return -1;
}

int find_master(std::string id){
  int index = 0;
  for(Server_ s : server_db){
    if(s.id == id && s.type == "Master")
      return index;
    index++;
  }
  return -1;
}

int find_slave(std::string id){
  int index = 0;
  for(Server_ s : server_db){
    if(s.id == id && s.type == "Slave")
      return index;
    index++;
  }
  return -1;
}

class SNSServiceImpl final : public SNSService::Service {
    Status Login(ServerContext* context, const Request* request, Reply* reply) override {
        
        if (request->arguments(0) == "NONE"){
            Client c;
            int user_index = find_user(request->username());
            int server_index = find_master(request->username());
            if (num_servers == 0 || server_index < 0){
                reply->set_msg("No server to connect to");
                return Status::OK;
            }
            //srand(time(NULL));
            //int randServer = (rand() % num_servers);
            if(user_index < 0){
              c.username = request->username();
              client_db.push_back(c);
              Server_ *server = &server_db[server_index];
              reply->add_arguments(server->port);
              server->client_managed.push_back(&c);
              reply->set_msg("Login Successful! connecte to port " + server->port);
              ofstream ofs;
              std::string filename = "clients.txt";
              ofs.open(filename, fstream::app);
              ofs << c.username << std::endl;
              ofstream _ofs("server" + server->id + "_clients.txt");
              _ofs << c.username << endl;
            }
            else{ 
                Client *user = &client_db[user_index];
                int master_index = find_master(request->username());
                Server_ *master_ = &server_db[master_index];
                if(user->connected){
                  reply->add_arguments(master_->port);
                  reply->set_msg("Login Successful! connecte to port " + master_->port);
                  //cout << "right after sending port number: " << master_->port << endl;
                }
                else{
                    std::string msg = "Welcome Back " + user->username;
                    reply->add_arguments(master_->port);
        	          reply->set_msg(msg);
                    user->connected = true;
                }
            }
        }
        
        else if (request->arguments(0) == "SYNC"){
            Synchronizer s;
            int syncIndex = find_sync(request->username());
            s.id = request->username();
            s.port = request->arguments(1);
            int serverIndex = find_master(s.id);
            if (serverIndex < 0){
              reply->set_msg("No server to connect to");
            }
            else if (syncIndex < 0){
              sync_db.push_back(s);
              Server_ *server = &server_db[serverIndex];
              if (!server->heartbeat)
                reply->set_msg("Server is not valid.");
              else {
                reply->set_msg("Connected to the cluster with server port " + server->port);
                reply->add_arguments(server->port);
              }
            }
            else {
                Synchronizer *sync = &sync_db[syncIndex];
                if (sync->connected)
                    reply->set_msg("Synchronizer is not valid.");
                else {
                    reply->set_msg("Welcome back syhnchronizer: " + s.id);
                    s.connected = true;
                }
            }
            
        }
        
        else if (request->arguments(0) == "HEARTBEAT"){
          if (request->arguments(2) == "Master"){
            int masterIndex = find_master(request->username());
            Server_ *master = &server_db[masterIndex];
            master->start = chrono::system_clock::now();
            string server_type = "Master";
            reply->add_arguments(server_type);
          }
          else if (request->arguments(2) == "Slave"){
            string server_type = "Slave";
            int masterIndex = find_master(request->username());
            if (masterIndex >= 0){
              chrono::time_point<std::chrono::system_clock> end = chrono::system_clock::now();
              Server_ *master = &server_db[masterIndex];
              if (chrono::duration_cast<chrono::seconds>(end - master->start).count() > 20){
                server_type = "Master";
                master->type = "Slave";
              }
            }
            int slaveIndex = find_slave(request->username());
            Server_ *slave = &server_db[slaveIndex];
            slave->start = chrono::system_clock::now();
            reply->add_arguments(server_type);
          }
          reply->set_msg("heartbeat received");
        }
        
        else {
            Server_ s;
            int serverIndex = find_server(request->username(), request->arguments(1));
            std::string id = request->username();
            std::string serverPort = request->arguments(0);
            std::string type = request->arguments(1);
            if (serverIndex < 0){
                s.port = serverPort;
                s.id = id;
                s.type = type;
                server_db.push_back(s);
                num_servers++;
                std::cout << "Server ready to accept connection on port: " + serverPort << std::endl;
            }
            else {
              Server_ *server = &server_db[serverIndex];
                if (server->heartbeat)
                    reply->set_msg("Server is not valid.");
                else {
                    reply->set_msg("Server is back and ready to accept connection on port: " + serverPort);
                    server->heartbeat = true;
                }
            }
        }
        return Status::OK;
    }
    
    Status Follow(ServerContext* context, const Request* request, Reply* reply) override {
      string username = request->arguments(0);
      int index_server = find_client_server(username);
      string sync_id = server_db[index_server].id;
      string port = server_db[index_server].port;
      reply->add_arguments(port);
      reply->add_arguments(sync_id);
      reply->set_msg("successfully found the coordinator for client " + username);
    }
};

void check_for_heartbeat(){
  while (true){
    chrono::time_point<std::chrono::system_clock> end = chrono::system_clock::now();
    for (Server_ s : server_db){
      if (chrono::duration_cast<chrono::seconds>(end - s.start).count() > 20 && s.type == "Master"){
        int newMaster_index = find_slave(s.id);
        if (newMaster_index < 0)
          continue;
        s.heartbeat = false;
        s.type = "Slave";
        Server_ *newMaster = &server_db[newMaster_index];
        newMaster->type = "Master";
      }
    }
    sleep(2);
  }
}

void RunCoordinator(std::string port_no) {
  std::string coordinator_address = "0.0.0.0:"+port_no;
  SNSServiceImpl service;

  ServerBuilder builder;
  builder.AddListeningPort(coordinator_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Coordinator listening on " << coordinator_address << std::endl;

  server->Wait();
}

int main(int argc, char** argv)
{
	std::string port = "3010";
  int opt = 0;
  while ((opt = getopt(argc, argv, "p:")) != -1){
    switch(opt) {
      case 'p':
          port = optarg;break;
      default:
	  std::cerr << "Invalid Command Line Argument\n";
    }
  }
  std::thread t1(RunCoordinator, port);
  std::thread t2(check_for_heartbeat);
  
  std::thread t_server = move(t1);
	std::thread t_update = move(t2);

	if (t_server.joinable())
		t_server.join();
	if (t_update.joinable())
		t_update.join();
		
  return 0;
}