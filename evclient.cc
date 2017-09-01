#include "evrpc.pb.h"
#include "rpcchannel.h"
#include "codeclite.h"
#include "rpccodec.h"
#include "tcpclient.h"

#include <glog/logging.h>
#include <stdio.h>
#include <unistd.h>

using namespace evrpc;
using namespace std;

class RpcClient : noncopyable
{
  public:
    RpcClient(std::string ip, int port = 8009)
      : client_(ip, port),
        channel_(new RpcChannel),
        stub_(get_pointer(channel_))
    {
      client_.setConnectionCallback(
            std::bind(&RpcClient::onConnection, this,std::placeholders::_1));
      client_.setReadCallback(
            std::bind(&RpcChannel::onMessage, get_pointer(channel_), std::placeholders::_1));
      // client_.enableRetry();
    }

    void connect()
    {
      client_.startRun();
    }

  private:
    void onConnection(Conn* conn)
    {
      //channel_.reset(new RpcChannel(conn));
      channel_->setConnection(conn);
      evrpc::PutRequest request;
      evrpc::Site *sit = request.add_sites();
      sit->set_key("key test 1");
      sit->set_value("value test 1");
      evrpc::PutResult* result = new evrpc::PutResult;
      stub_.Put(NULL, &request, result, google::protobuf::NewCallback(this, &RpcClient::solved, result));
    }

    void solved(evrpc::PutResult* resp)
    {
      cout << "solved:\n" << resp->DebugString().c_str();
      client_.quit(NULL);
    }

    TcpClient client_;
    RpcChannelPtr channel_;
    evrpc::EkvCmd_Stub stub_;
};

int main(int argc, char* argv[])
{
  cout << "pid = " << getpid();
  if (argc > 1)
  {
    RpcClient rpcClient("127.0.0.1", 8009);
    rpcClient.connect();
  }
  else
  {
    printf("Usage: %s host_ip\n", argv[0]);
  }
  google::protobuf::ShutdownProtobufLibrary();
}

