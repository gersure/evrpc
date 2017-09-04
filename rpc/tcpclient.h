#ifndef RPC_TCPCLIENT_H
#define RPC_TCPCLIENT_H

#include "tcpserver.h"

namespace evrpc
{


class TcpClient: public noncopyable
{
public:

    TcpClient(const std::string& ip,int port = 0);
    ~TcpClient();

    void startRun();

    void quit(timeval *tv);

    // read enough data then call ReadCallback
    void setReadCallback(const DataCallback& cb)
    {
        read_cb_ =cb;
    }
    // write enough data the call WriteCallback
    void setWriteCallback(const DataCallback& cb)
    {
        write_cb_ = cb;
    }
    // add a new conn then call ConnectCallback
    void setConnectionCallback(const DataCallback& cb)
    {
        connect_cb_ = cb;
    }
    // error then call EventCallback
    void setEventCallback(const EventCallback& cb)
    {
        event_cb_ = cb;
    }

private:
    static void* threadProcess(void *arg);
    static void notifyHandler(int fd,short which,void* arg);


    static void acceptCb(evconnlistener* listener,evutil_socket_t fd,sockaddr* sa,int socklen,void *user_data);

    static void bufferReadCb(struct bufferevent* bev,void* data);
    static void bufferWriteCb(struct bufferevent* bev,void* data);
    static void bufferEventCb(struct bufferevent* bev,short events,void* data);

    DataCallback read_cb_;
    DataCallback write_cb_;
    DataCallback connect_cb_;
    EventCallback event_cb_;

    int port_;
    std::string ip_;
    Conn        conn_;
    LibeventThread main_base_;
};


}
#endif // TCPCLIENT_H
