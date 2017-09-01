#ifndef _EKV_SERVER_H
#define _EKV_SERVER_H


#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <glog/logging.h>

#include <stdexcept>
#include <string>
#include <functional>

#include <event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <event2/util.h>
#include <event2/event.h>

#include "util.h"


namespace evrpc
{

class TcpClient;



class EkvNetErr : public std::runtime_error
{
public:
    EkvNetErr(const std::string &what_arg):std::runtime_error(what_arg){}
};

class TcpServer;
class ConnQueue;
class Conn;
struct LibeventThread;


typedef std::function<void(Conn*)> DataCallback;
typedef std::function<void(Conn*,short)> EventCallback;

class Conn : public noncopyable
{
    friend class ConnQueue;
    friend class TcpServer;
    friend class TcpClient;
public:
    Conn(int fd=0) :fd_(fd),thread_(NULL),bev_(NULL),readBuf_(NULL),
        writeBuf_(NULL),pre_(NULL),next_(NULL) { }

    LibeventThread* getThread() {return thread_;}
    int getFd() {return fd_;}
    bufferevent* getBufferevent() {return bev_;}
    uint32_t getReadBufferLen()  { return evbuffer_get_length(readBuf_); }
    evbuffer* getReadBuffer() { return readBuf_; }
    int readBuffer(char* buffer,int len) { return evbuffer_remove(readBuf_,buffer,len); }
    int copyBuffer(char *buffer, int len) { return evbuffer_copyout(readBuf_, buffer, len); }
    uint32_t getWriteBufferLen() { return evbuffer_get_length(writeBuf_); }
    int addToWriteBuffer(char *buffer, int len) { LOG(INFO)<<"add write buffer"; return evbuffer_add(writeBuf_, buffer, len); }
    int addBufToWriteBuffer(evbuffer *buf) { return evbuffer_add_buffer(writeBuf_, buf); }
    void moveBufferReadToWrite() { evbuffer_add_buffer(writeBuf_, readBuf_); }
private:
    int fd_;
    LibeventThread* thread_;
    bufferevent *bev_;
    evbuffer *readBuf_;
    evbuffer *writeBuf_;

    Conn *pre_;
    Conn *next_;
};

class ConnQueue
{
public:
    ConnQueue() : total(0), head_(new Conn(0)), tail_(new Conn(0))
    {
        head_->pre_ = tail_->next_ = NULL;
        head_->next_ = tail_; tail_->pre_ = head_;
    }
    ~ConnQueue()
    {
        Conn *tcur,*tnext; tcur = head_;
        while(tcur != NULL) {
            tnext = tcur->next_;
            delete tcur;
            tcur = tnext;
        }
    }
    unsigned long getCount() { return total; }
    Conn* insertConn(int fd,LibeventThread* t)
    {
        Conn *c = new Conn(fd);
        c->thread_ = t;
        Conn *next = head_->next_;

        c->pre_ = head_;
        c->next_ = head_->next_;
        head_->next_ = c;
        next->pre_ = c;
        total++;
        return c;
    }
    void deleteConn(Conn* conn)
    {
        conn->pre_->next_ = conn->next_;
        conn->next_->pre_ = conn->pre_;
        delete conn;
        total--;
    }
private:
    unsigned long total;
    Conn* head_;
    Conn* tail_;
};

struct LibeventThread
{
    pthread_t thread_id;
    struct event_base *base;
    struct event notify_event;
    int notify_send_fd;
    int notify_recv_fd;
    ///TODO:
    /// not use this.
    /// every loop have one queue,but not use

    ConnQueue connect_queue;
    //queue<conn> connect_queue;
    union {
        TcpServer *tcp_server;
        TcpClient *tcp_client;
    };
    //vector<TcpServer> tcp_connect;
};


class TcpServer: public noncopyable
{
public:

    static const int EXIT_CODE = -1;

    TcpServer(int count, const std::string& ip,int port = 0);
    ~TcpServer();

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
    void setupThread(LibeventThread* thread);
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

    int thread_count_;
    int port_;
    std::string ip_;
    LibeventThread* main_base_;
    LibeventThread* threads_;


};

}

#endif // EKV_SERVER_H
