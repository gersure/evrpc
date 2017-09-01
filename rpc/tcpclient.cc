#include "tcpclient.h"
#include "tcpserver.h"
#include <errno.h>

using namespace::evrpc;

TcpClient::TcpClient(const std::string& ip,int port)
  :read_cb_(NULL), write_cb_(NULL), connect_cb_(NULL), event_cb_(NULL),
    port_(port), ip_(ip), main_base_()
{
  main_base_.thread_id = pthread_self();
  conn_.thread_ = &main_base_;
  main_base_.base = event_base_new();
  main_base_.tcp_client = this;
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
  if (events & BEV_EVENT_EOF){
    LOG(INFO) <<"Connected closed."<<events;
  }else if (events & BEV_EVENT_READING){
    LOG(INFO) <<"error when reading."<<events;
  }else if (events & BEV_EVENT_WRITING){
    LOG(INFO) <<"error when writing."<<events;
  }else if (events & BEV_EVENT_TIMEOUT){
    LOG(INFO) <<"time out event."<<events;
  }else if (events & BEV_EVENT_ERROR){
    LOG(INFO) <<"error when operator."<<events;
  }else if (events & BEV_EVENT_CONNECTED){
    LOG(INFO) <<"connected success."<<events;
    Conn *conn = (Conn*)data;
    conn->fd_ = bufferevent_getfd(bev);
    conn->readBuf_ = bufferevent_get_input(bev);
    conn->writeBuf_ = bufferevent_get_output(bev);

    if(conn->getThread()->tcp_client->event_cb_)
      conn->getThread()->tcp_client->connect_cb_(conn);
    return;
  }else if (EVUTIL_SOCKET_ERROR() == 115){
    LOG(INFO) <<"info::::::is not connected."<<events;
    return;
  }else{
    LOG(INFO) <<"some unknow error!" << events;
  }

  LOG(ERROR) <<EVUTIL_SOCKET_ERROR()<< "\t" << evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR());

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
  errno = 0;
  conn_.bev_ = bufferevent_socket_new(this->main_base_.base, -1, BEV_OPT_CLOSE_ON_FREE);
  if (!conn_.bev_){ /*change*/}

  bufferevent_setcb(conn_.bev_,bufferReadCb,bufferWriteCb,bufferEventCb, &conn_);
  bufferevent_enable(conn_.bev_, EV_WRITE|EV_READ);

  if ((bufferevent_socket_connect(conn_.bev_, (struct sockaddr *)(&sin), sizeof(sin))) < 0)
  {
    bufferevent_free(conn_.bev_);
    LOG(ERROR) << "connect to " << ip_ << "error !";
    return;
  }

//  conn_->readBuf_ = bufferevent_get_input(conn_.bev_);
//  conn_->writeBuf_ = bufferevent_get_output(conn_.bev_);
  event_base_dispatch(main_base_.base);

  printf("Finished\n");
}

void TcpClient::quit(timeval *tv)
{
  event_base_loopexit(main_base_.base,tv);
}

