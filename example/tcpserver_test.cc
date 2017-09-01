/*************************************************************************
	> File Name: tcpserver_test.cpp
	> Author: changfa.zheng
	> Mail: changfa.zheng@ele.me
	> Created Time: 2017年09月01日 星期五 11时50分38秒
 ************************************************************************/

#include<iostream>
#include "tcpserver.h"
#include <assert.h>
#include <thread>

using namespace std;
using namespace evrpc;

void lenprint(char *buf, size_t len)
{
  string str(len+1, 'a');
  str.clear();
  const char *data = str.c_str();
  memcpy(const_cast<char *>(data), buf, len);
  cout << "\t\t\tclient :" << str << endl;
}


class TcpServerImpl : noncopyable
{
  public:
    TcpServerImpl() {}
    ~TcpServerImpl() {}

    static void onMessage(Conn *conn)
    {
      char buffer[1024] = {};
      int len=0, res = 0;
      len = conn->getReadBufferLen();

      /***********************************************
       * test evbuffer_peek;
       * **********************************************/
      evbuffer *evbuf= conn->getReadBuffer();
      int n = evbuffer_peek(evbuf, len, NULL, NULL, 0);
      evbuffer_iovec *vec = new evbuffer_iovec[n];
      res = evbuffer_peek(evbuf, len, NULL, vec, n);
      assert( res == n);

      for (int i = 0; i < n; i++){
        lenprint((char *)(vec[i].iov_base), vec[i].iov_len);
      }
      delete [] vec;


      /***********************************************
       * test evbuffer_peek;
       * **********************************************/
      for(int i =0; i< len/1024+1; i++){
        res = conn->readBuffer(buffer, (i*1023)>len ? (i%1023):1023);
        cout<< "\t\t\tbuf " << i << ":" << buffer << endl;
      }
//      string str("server send message success!");
//      conn->addToWriteBuffer(const_cast<char *>(str.c_str()), str.size());
      std::this_thread::sleep_for(std::chrono::seconds(2));

    }



    static void send(Conn *conn)
    {
      string str("************************!");
      conn->addToWriteBuffer(const_cast<char *>(str.c_str()), str.size());

      std::this_thread::sleep_for(std::chrono::seconds(3));
    }


    static void onAccept(Conn *conn)
    {
      cout << "client connect:"<<endl;
      cout << "fd:" << conn->getFd() << "\tthread id:" << conn->getThread()->thread_id << endl;
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
  TcpServer server(4, "127.0.0.1", 8009);

  server.setConnectionCallback(&TcpServerImpl::onAccept);
  server.setReadCallback(&TcpServerImpl::onMessage);
  server.setWriteCallback(&TcpServerImpl::send);
  server.setEventCallback(&TcpServerImpl::onError);

  server.startRun();


  return 0;
}
