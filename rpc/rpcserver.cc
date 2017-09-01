#include "rpcserver.h"
#include "tcpserver.h"

#include "rpcchannel.h"

#include <google/protobuf/descriptor.h>
#include <google/protobuf/service.h>

using namespace evrpc;

RpcServer::RpcServer(int threads,
                     const std::string& ip, int port)
  : server_(threads, ip, port)
{
  server_.setReadCallback(
      std::bind(&RpcServer::onConnection, this, std::placeholders::_1));
//   server_.setMessageCallback(
//       std::bind(&RpcServer::onMessage, this, _1, _2, _3));
}

void RpcServer::registerService(google::protobuf::Service* service)
{
  const google::protobuf::ServiceDescriptor* desc = service->GetDescriptor();
  services_[desc->full_name()] = service;
}

void RpcServer::start()
{
  server_.startRun();
}

void RpcServer::onConnection(Conn* conn)
{
  LOG(INFO)<<"Socketfd:"<<conn->getFd()<<"\tthreadid:"<<conn->getThread()->thread_id<<std::endl;

  RpcChannelPtr channel(new RpcChannel(conn));
  channel->setServices(&services_);
  server_.setReadCallback(std::bind(&RpcChannel::onMessage, get_pointer(channel), std::placeholders::_1));

}

// void RpcServer::onMessage(const Conn* conn)
// {
//   RpcChannelPtr& channel = boost::any_cast<RpcChannelPtr&>(conn->getContext());
//   channel->onMessage(conn, buf, time);
// }

