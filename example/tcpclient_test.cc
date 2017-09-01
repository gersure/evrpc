/*************************************************************************
  > File Name: tcpserver_test.cpp
  > Author: changfa.zheng
  > Mail: changfa.zheng@ele.me
  > Created Time: 2017年09月01日 星期五 11时50分38秒
 ************************************************************************/

#include<iostream>
#include "tcpclient.h"
#include <assert.h>
#include <thread>

using namespace std;
using namespace evrpc;

void lenprint(char *buf, size_t len)
{
  string str(len+1, 'a');
  const char *data = str.c_str();
  memcpy(const_cast<char *>(data), buf, len);
  cout << str << endl;
}


class TcpServerImpl : noncopyable
{
  public:
    TcpServerImpl() {}
    ~TcpServerImpl() {}

    static void onMessage(Conn *conn)
    {
      char buffer[1024] = {};
      //cout << "recv data from client-->"<<endl;

      int len=0;
      len = conn->getReadBufferLen();

      for(int i =0; i< len/1024+1; i++){
        conn->readBuffer(buffer, (i*1023)>len ? (i%1023):1023);
        cout<< "\t\t\tserver " << i << ":" << buffer << endl;
      }

      std::this_thread::sleep_for(std::chrono::seconds(2));
    }



    static void send(Conn *conn)
    {
      string str("****************************!");
      conn->addToWriteBuffer(const_cast<char *>(str.c_str()), str.size());

      std::this_thread::sleep_for(std::chrono::seconds(2));
    }


    static void onAccept(Conn *conn) {
      cout << "client connect:"<<endl;
      cout << "fd:" << conn->getFd() << "\tthread id:" << conn->getThread()->thread_id << endl;
      string str("*************************!");
      conn->addToWriteBuffer(const_cast<char *>(str.c_str()), str.size());
    }



    static void onError(Conn *conn, short event)
    {
      cout << "client closed : " << endl;
      cout << "fd:" << conn->getFd() << "\tthread id:" << conn->getThread()->thread_id << endl;
    }

  private:
    //    static char buffer[1024];
};


int main(int argc, char **argv)
{
  TcpClient client("127.0.0.1", 8009);

  client.setConnectionCallback(&TcpServerImpl::onAccept);
  client.setReadCallback(&TcpServerImpl::onMessage);
  client.setWriteCallback(&TcpServerImpl::send);
  client.setEventCallback(&TcpServerImpl::onError);

  client.startRun();


  return 0;
}
