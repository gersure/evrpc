#include "tcpserver.h"

using namespace::evrpc;

TcpServer::TcpServer(int count, const std::string& ip,int port)
    :read_cb_(NULL), write_cb_(NULL), connect_cb_(NULL), event_cb_(NULL),
    thread_count_(count), port_(port), ip_(ip), main_base_(new LibeventThread),
    threads_(new LibeventThread[thread_count_])
{
    main_base_->thread_id = pthread_self();
    main_base_->base = event_base_new();
    if (!main_base_->base)
        throw EkvNetErr("main thread event_base_new error!");

    for(int i=0;i< thread_count_;i++)
    {
        setupThread(&threads_[i]);
    }
    LOG(INFO) << "TcpServer init success";
}

TcpServer::~TcpServer()
{
    quit(NULL);
    event_base_free(main_base_->base);
    for(int i =0;i< thread_count_;i++)
    {
        event_base_free(threads_[i].base);
    }
    delete main_base_;
    delete[] threads_;
    LOG(INFO)<< "TcpServer dtor";
}
void TcpServer::setupThread(LibeventThread* thread)
{
    thread->tcp_server = this;
    thread->base = event_base_new();
    if(!thread->base)
        throw EkvNetErr("child thread event_base_new error!");
    int fds[2];
    if(pipe(fds))
        throw EkvNetErr("child thread pipe create error!");
    thread->notify_send_fd = fds[1];
    thread->notify_recv_fd = fds[0];

    event_set(&thread->notify_event,thread->notify_recv_fd,EV_READ|EV_PERSIST,notifyHandler,thread);
    event_base_set(thread->base,&thread->notify_event);
    if(event_add(&thread->notify_event,0) == -1)
        throw EkvNetErr("child thread add event error!");
}

void TcpServer::notifyHandler(int fd,short which,void* arg)
{
    LibeventThread * thread = (LibeventThread*)arg;

    int pipefd = thread->notify_recv_fd;
    evutil_socket_t confd;
    if(-1 == read(pipefd,&confd,sizeof(evutil_socket_t)))
    {
        LOG(ERROR)<<"pipe read error";
        return ;
    }
    if(EXIT_CODE == confd)
    {
        event_base_loopbreak(thread->base);
        LOG(INFO) << "notify pipe recv EXIT_CODE base loopbreak";
        return;
    }

    struct bufferevent *bev;
    bev = bufferevent_socket_new(thread->base,confd,BEV_OPT_CLOSE_ON_FREE);
    if(!bev)
    {
        LOG(ERROR)<<"bufferevent create with evutil_socket_t  error";
        event_base_loopbreak(thread->base);
        return;
    }

    // insert new connect to the thread's connection queue
    Conn *conn = thread->connect_queue.insertConn(confd,thread);

    conn->bev_ = bev;
    conn->thread_ = thread;
    conn->readBuf_ = bufferevent_get_input(bev);
    conn->writeBuf_ = bufferevent_get_output(bev);

    bufferevent_setcb(bev,bufferReadCb,bufferWriteCb,bufferEventCb,conn);
    bufferevent_enable(bev,EV_READ);
    bufferevent_enable(bev,EV_WRITE);

    if(thread->tcp_server->connect_cb_)
        thread->tcp_server->connect_cb_(conn);
}

void* TcpServer::threadProcess(void *arg)
{
    LibeventThread *thread = (LibeventThread*)arg;
    LOG(INFO)<<"thread "<<thread->thread_id <<" started!";
    event_base_dispatch(thread->base);
    return NULL;
}
void TcpServer::bufferReadCb(struct bufferevent* bev,void* data)
{
    Conn* conn = (Conn*)data;
   /* conn->read_buf_ = bufferevent_get_input(bev);
    conn->write_buf_ = bufferevent_get_output(bev);
    */
    LOG(INFO)<<"have data to read";

    if(conn->getThread()->tcp_server->read_cb_)
        conn->getThread()->tcp_server->read_cb_(conn);
}
void TcpServer::bufferWriteCb(struct bufferevent* bev,void* data)
{
    Conn* conn = (Conn*)data;
    conn->readBuf_ = bufferevent_get_input(bev);
    conn->writeBuf_ = bufferevent_get_output(bev);
    LOG(INFO)<<"have data to send";
    if(conn->getThread()->tcp_server->write_cb_)
        conn->getThread()->tcp_server->write_cb_(conn);
}
void TcpServer::bufferEventCb(struct bufferevent* bev,short events,void* data)
{
    LOG(ERROR) <<"some error happened."<<events;
    Conn *conn = (Conn*)data;
    if(conn->getThread()->tcp_server->event_cb_)
        conn->getThread()->tcp_server->event_cb_(conn,events);
    conn->getThread()->connect_queue.deleteConn(conn);
    bufferevent_free(bev);
}
void TcpServer::startRun()
{
    evconnlistener *listener;

    sockaddr_in sin;
    memset(&sin,0,sizeof(sin));

    sin.sin_family = AF_INET;
    sin.sin_port = htons(port_);
    if(ip_ != std::string())
    {
        if(inet_pton(AF_INET,ip_.c_str(),&sin.sin_addr) <= 0)
            throw EkvNetErr("sokcet ip error!");
    }

    listener = evconnlistener_new_bind(main_base_->base,acceptCb,(void*)this,LEV_OPT_REUSEABLE|LEV_OPT_CLOSE_ON_FREE,-1,(sockaddr*)&sin,sizeof(sockaddr_in));
    if(NULL == listener)
        throw EkvNetErr("create listener error!");

    for(int i=0;i<thread_count_;i++)
    {
        if (pthread_create(&threads_[i].thread_id,NULL,threadProcess,(void*)&threads_[i]))
            throw EkvNetErr(std::string("pthread_create error:").append(strerror(errno)));
    }

    event_base_dispatch(main_base_->base);

    evconnlistener_free(listener);
}

void TcpServer::quit(timeval *tv)
{
    int contant = EXIT_CODE;
    for(int i=0;i<thread_count_;i++)
    {
        write(threads_[i].notify_send_fd,&contant,sizeof(int));
    }

    event_base_loopexit(main_base_->base,tv);
}
void TcpServer::acceptCb(evconnlistener* listener,evutil_socket_t fd,sockaddr* sa,int socklen,void *user_data)
{
    TcpServer *server = (TcpServer*) user_data;
    unsigned long  num = 0 ;
    for(int i=0; i< server->thread_count_; i++){
        num = server->threads_[num].connect_queue.getCount() > server->threads_[i].connect_queue.getCount() ? i : num;
    }
    write(server->threads_[num].notify_send_fd,&fd,sizeof(evutil_socket_t));
}

