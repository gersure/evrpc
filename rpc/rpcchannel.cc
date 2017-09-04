#include "rpcchannel.h"
#include "rpc.pb.h"
#include <google/protobuf/descriptor.h>

using namespace evrpc;

RpcChannel::RpcChannel()
  : codec_(std::bind(&RpcChannel::onRpcMessage, this, std::placeholders::_1,std::placeholders::_2)),
    services_(NULL)
{
  LOG(INFO) << "RpcChannel::Constructor -- " << this;
}

RpcChannel::RpcChannel(Conn* conn)
  : codec_(std::bind(&RpcChannel::onRpcMessage, this,std::placeholders::_1,std::placeholders::_2)),
    conn_(conn),
    services_(NULL)
{
  LOG(INFO) << "RpcChannel::Constructor -- " << this;
}

RpcChannel::~RpcChannel()
{
  LOG(INFO) << "RpcChannel::Destructor - " << this;
  for (std::map<int64_t, OutstandingCall>::iterator it = outstandings_.begin(); it != outstandings_.end(); ++it)
  {
    OutstandingCall out = it->second;
    delete out.response;
    delete out.done;
  }
}

  // Call the given method of the remote service.  The signature of this
  // procedure looks the same as Service::CallMethod(), but the requirements
  // are less strict in one important way:  the request and response objects
  // need not be of any specific class as long as their descriptors are
  // method->input_type() and method->output_type().
void RpcChannel::CallMethod(const ::google::protobuf::MethodDescriptor* method,
                            google::protobuf::RpcController* controller,
                            const ::google::protobuf::Message* request,
                            ::google::protobuf::Message* response,
                            ::google::protobuf::Closure* done)
{
  RpcMessage message;
  message.set_type(REQUEST);
  int64_t id = id_.incrementAndGet();
  message.set_id(id);
  message.set_service(method->service()->full_name());
  message.set_method(method->name());
  message.set_request(request->SerializeAsString()); // FIXME: error check

  OutstandingCall out = { response, done };
  {
  std::lock_guard<std::mutex> lk(mutex_);
  outstandings_[id] = out;
  }
  codec_.send(conn_, message);
}

void RpcChannel::onMessage(Conn* conn)
{
  codec_.onMessage(conn);
}

void RpcChannel::onRpcMessage(Conn* conn, const RpcMessagePtr& messagePtr)
{
  assert(conn == conn_);
  //printf("%s\n", message.DebugString().c_str());
  RpcMessage& message = *messagePtr;
  if (message.type() == RESPONSE)
  {
    int64_t id = message.id();
    assert(message.has_response() || message.has_error());

    OutstandingCall out = { NULL, NULL };

    {
      std::lock_guard<std::mutex> lk(mutex_);
      std::map<int64_t, OutstandingCall>::iterator it = outstandings_.find(id);
      if (it != outstandings_.end())
      {
        out = it->second;
        outstandings_.erase(it);
      }
    }

    if (out.response)
    {
      std::unique_ptr<google::protobuf::Message> d(out.response);
      if (message.has_response())
      {
        out.response->ParseFromString(message.response());
      }
      if (out.done)
      {
        out.done->Run();
      }
    }
  }
  else if (message.type() == REQUEST)
  {
    // FIXME: extract to a function
    ErrorCode error = WRONG_PROTO;
    if (services_)
    {
      std::map<std::string, google::protobuf::Service*>::const_iterator it = services_->find(message.service());
      if (it != services_->end())
      {
        google::protobuf::Service* service = it->second;
        assert(service != NULL);
        const google::protobuf::ServiceDescriptor* desc = service->GetDescriptor();
        const google::protobuf::MethodDescriptor* method
          = desc->FindMethodByName(message.method());
        if (method)
        {
          std::unique_ptr<google::protobuf::Message> request(service->GetRequestPrototype(method).New());
          if (request->ParseFromString(message.request()))
          {
            google::protobuf::Message* response = service->GetResponsePrototype(method).New();
            // response is deleted in doneCallback
            int64_t id = message.id();
            service->CallMethod(method, NULL, get_pointer(request), response,
                                NewCallback(this, &RpcChannel::doneCallback, response, id));
            error = NO_ERROR;
          }
          else
          {
            error = INVALID_REQUEST;
          }
        }
        else
        {
          error = NO_METHOD;
        }
      }
      else
      {
        error = NO_SERVICE;
      }
    }
    else
    {
      error = NO_SERVICE;
    }
    if (error != NO_ERROR)
    {
      RpcMessage response;
      response.set_type(RESPONSE);
      response.set_id(message.id());
      response.set_error(error);
      codec_.send(conn_, response);
    }
  }
  else if (message.type() == ERROR)
  {
  }
}

void RpcChannel::doneCallback(::google::protobuf::Message* response, int64_t id)
{
  std::unique_ptr<google::protobuf::Message> d(response);
  RpcMessage message;
  message.set_type(RESPONSE);
  message.set_id(id);
  message.set_response(response->SerializeAsString()); // FIXME: error check
  codec_.send(conn_, message);
}

