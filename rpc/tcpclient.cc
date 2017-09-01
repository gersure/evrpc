#include "tcpclient.h"
#include "tcpserver.h"

using namespace::evrpc;

TcpClient::TcpClient(const std::string& ip,int port)
    :read_cb_(NULL), write_cb_(NULL), connect_cb_(NULL), event_cb_(NULL),
    port_(port), ip_(ip), main_base_()
{
    main_base_.thread_id = pthread_self();
    conn_.thread_ = &main_base_;
    main_base_.base = event_base_new();
    if (!main_base_.base)
        throw EkvNetErr("main thread event_base_new error!");

    LOG(INFO) << "TcpClient init success";
}

TcpClient::~TcpClient()
{
    quit(NULL);
    event_base_free(main_base_.base);
    LOG(INFO)<< "TcpServer dtor";
}

void TcpClient::bufferReadCb(struct bufferevent* bev,void* data)
{
    Conn* conn = (Conn*)data;
    conn->readBuf_ = bufferevent_get_input(bev);
    conn->writeBuf_ = bufferevent_get_output(bev);

    LOG(INFO)<<"have data to read";

    if(conn->getThread()->tcp_client->read_cb_)
        conn->getThread()->tcp_client->read_cb_(conn);
}
void TcpClient::bufferWriteCb(struct bufferevent* bev,void* data)
{
    Conn* conn = (Conn*)data;
    conn->readBuf_ = bufferevent_get_input(bev);
    conn->writeBuf_ = bufferevent_get_output(bev);
    LOG(INFO)<<"have data to send";
    if(conn->getThread()->tcp_client->write_cb_)
        conn->getThread()->tcp_client->write_cb_(conn);
}
void TcpClient::bufferEventCb(struct bufferevent* bev,short events,void* data)
{
    LOG(ERROR) <<"some error happened."<<events;
    Conn *conn = (Conn*)data;
    if(conn->getThread()->tcp_client->event_cb_)
        conn->getThread()->tcp_client->event_cb_(conn,events);
    conn->getThread()->connect_queue.deleteConn(conn);
    bufferevent_free(bev);
}
void TcpClient::startRun()
{  
  sockaddr_in sin;
  memset(&sin,0,sizeof(sin));

  sin.sin_family = AF_INET;
  sin.sin_port = htons(port_);
  if(ip_ != std::string())
  {
      if(inet_pton(AF_INET,ip_.c_str(),&sin.sin_addr) <= 0)
          throw EkvNetErr("sokcet ip error!");
  }

  conn_.bev_ = bufferevent_socket_new(this->main_base_.base, -1, BEV_OPT_CLOSE_ON_FREE);
  if (!conn_.bev_){ /*change*/}

  if ((conn_.fd_ = bufferevent_socket_connect(conn_.bev_, (struct sockaddr *)(&sin), sizeof(sin))) < 0)
  {
    bufferevent_free(conn_.bev_);
    LOG(ERROR) << "connect to " << ip_ << "error !";
    return;
  }

  bufferevent_setcb(conn_.bev_,bufferReadCb,bufferWriteCb,bufferEventCb, &conn_);
  bufferevent_enable(conn_.bev_, EV_READ | EV_WRITE);

//  conn_->readBuf_ = bufferevent_get_input(conn_.bev_);
//  conn_->writeBuf_ = bufferevent_get_output(conn_.bev_);
  event_base_dispatch(main_base_.base);

  printf("Finished\n");
}

void TcpClient::quit(timeval *tv)
{
    event_base_loopexit(main_base_.base,tv);
}

