#include <ctime>

#include <google/protobuf/timestamp.pb.h>
#include <google/protobuf/duration.pb.h>

#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <stdlib.h>
#include <unistd.h>
#include <google/protobuf/util/time_util.h>
#include <grpc++/grpc++.h>

#include "sns.grpc.pb.h"

using google::protobuf::Timestamp;
using google::protobuf::Duration;
using grpc::ClientContext;
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

std::vector<Client> client_db;

vector<string> clusterClients;

int find_user(std::string username){
  int index = 0;
  for(int i = 0; i < clusterClients.size(); i++){
    if(username == clusterClients.at(i))
      return index;
    index++;
  }
  return -1;
}

class SNSServiceImplt final : public SNSService::Service { 
    Status Follow(ServerContext* context, const Request* request, Reply* reply) override {
    std::string username1 = request->username();
    std::string username2 = request->arguments(0);
    int join_index = find_user(username2);
    if(join_index < 0 || username1 == username2)
      reply->set_msg("Join Failed -- Invalid Username");
    else{
      Client *user1 = &client_db[find_user(username1)];
      Client *user2 = &client_db[join_index];
      if(std::find(user1->client_following.begin(), user1->client_following.end(), user2) != user1->client_following.end()){
	      reply->set_msg("Join Failed -- Already Following User");
        return Status::OK;
      }
      user1->client_following.push_back(user2);
      user2->client_followers.push_back(user1);
      reply->set_msg("Follow Successful");
    }
    return Status::OK; 
  }

  /*Status UnFollow(ServerContext* context, const Request* request, Reply* reply) override {
    std::string username1 = request->username();
    std::string username2 = request->arguments(0);
    int leave_index = find_user(username2);
    if(leave_index < 0 || username1 == username2)
      reply->set_msg("Leave Failed -- Invalid Username");
    else{
      Client *user1 = &client_db[find_user(username1)];
      Client *user2 = &client_db[leave_index];
      if(std::find(user1->client_following.begin(), user1->client_following.end(), user2) == user1->client_following.end()){
	reply->set_msg("Leave Failed -- Not Following User");
        return Status::OK;
      }
      user1->client_following.erase(find(user1->client_following.begin(), user1->client_following.end(), user2)); 
      user2->client_followers.erase(find(user2->client_followers.begin(), user2->client_followers.end(), user1));
      reply->set_msg("Leave Successful");
    }
    return Status::OK;
  }*/
};

void establishConnection(std::string host, std::string portCoor, std::string id, std::string syncPort){
  std::string connection_address = host + ":" + portCoor;
  std::unique_ptr<SNSService::Stub> stub_ = std::unique_ptr<SNSService::Stub>(SNSService::NewStub(
             grpc::CreateChannel(
                  connection_address, grpc::InsecureChannelCredentials())));
                  
  ClientContext context;
  Request request;
  Reply reply;
  
  request.set_username(id);
  request.add_arguments("SYNC");
  request.add_arguments(syncPort);
  
  Status status = stub_->Login(&context, request, &reply);
  if (!status.ok()){
    std::cout << "Failed to connect to the coordinator" << std::endl;
    exit(1);
  }
  std::cout << reply.msg() << std::endl;
}

void check_for_updates(string host, string portCoor, string id){
  while (true){
    ifstream ifs("server" + id + "_clients.txt");
    if (!ifs.is_open()){}
    else {
      while(!ifs.eof()){
        string username = "";
        getline(ifs, username);
        bool alreadyInCluster = false;
        int user_index = find_user(username);
        if (user_index < 0){
          Client c;
          c.username = username;
          client_db.push_back(c);
          clusterClients.push_back(username);
        }
      }
      for (int i = 0; i < clusterClients.size(); i++) {
        ifstream _ifs(clusterClients.at(i) + "_following.txt");
        if (!_ifs.is_open()){}
        else {
          while (!_ifs.eof()){
            string clientName = "";
            getline(_ifs, clientName);
            int index_user = find_user(clientName);
            if (index_user < 0) {
              ClientContext context;
              Request request;
              Reply reply;
              
              request.add_arguments(clientName);
              std::string connection_address = host + ":" + portCoor;
              std::unique_ptr<SNSService::Stub> stub_ = std::unique_ptr<SNSService::Stub>(SNSService::NewStub(
              grpc::CreateChannel(connection_address, grpc::InsecureChannelCredentials())));
              Status status = stub_->Follow(&context, request, &reply);
              if (!status.ok()){
                std::cout << "Failed to connect to the coordinator" << std::endl;
                exit(1);
              }
              std::cout << reply.msg() << std::endl;
              string sync_connection = host + ":" + reply.arguments(0);
              stub_ = std::unique_ptr<SNSService::Stub>(SNSService::NewStub(
              grpc::CreateChannel(connection_address, grpc::InsecureChannelCredentials())));
              ClientContext context_;
              Request request_;
              Reply reply_;
              
              request_.set_username(clusterClients.at(i));
              request_.add_arguments(clientName);
              Status status_ = stub_->Follow(&context_, request_, &reply_);
            }
          }
        }
      }
    }
    sleep(30);
  }
}

int main(int argc, char** argv)
{
	std::string host = "0.0.0.0";
  std::string portCoor = "3010";
  std::string id = "1";
  std::string syncPort = "3000";
  int opt = 0;
  while ((opt = getopt(argc, argv, "h:p:i:s:")) != -1){
    switch(opt) {
      case 'h':
          host = optarg;break;
      case 'p':
          portCoor = optarg;break;
      case 'i':
          id = optarg;break;
      case 's':
          syncPort = optarg;break;
      default:
	  std::cerr << "Invalid Command Line Argument\n";
    }
  }
  
  establishConnection(host, portCoor, id, syncPort);
  check_for_updates(host, portCoor, id);
}