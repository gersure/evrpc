#ifndef RPC_RPCSERVER_H
#define RPC_RPCSERVER_H

#include "tcpserver.h"
#include <map>

namespace google {
namespace protobuf {

class Service;

}  // namespace protobuf
}  // namespace google

namespace evrpc
{

class RpcServer
{
  public:
    RpcServer(int threads, const std::string& ip, int port);

    void registerService(::google::protobuf::Service*);
    void start();

  private:
    void onConnection(Conn* conn);

    // void onMessage(const Conn* conn );

    TcpServer server_;
    std::map<std::string, ::google::protobuf::Service*> services_;
};

}


#endif  // RPC_RPCSERVER_H
