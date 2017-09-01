#ifndef RPC_CODEC_LITE_H__
#define RPC_CODEC_LITE_H__


#include <memory>
#include <type_traits>
#include "tcpserver.h"
#include <zlib.h>

namespace google
{
namespace protobuf
{
class Message;
}
}

namespace evrpc
{

typedef std::shared_ptr<google::protobuf::Message> MessagePtr;

// wire format
//
// Field     Length  Content
//
// size      4-byte  M+N+4
// tag       M-byte  could be "RPC0", etc.
// payload   N-byte
// checksum  4-byte  adler32 of tag+payload
//
// This is an internal class, you should use ProtobufCodecT instead.
class ProtobufCodecLite : noncopyable
{
 public:
  const static int kHeaderLen = sizeof(int32_t);
  const static int kChecksumLen = sizeof(int32_t);
  const static int kMaxMessageLen = 64*1024*1024; // same as codec_stream.h kDefaultTotalBytesLimit

  enum ErrorCode
  {
    kNoError = 0,
    kInvalidLength,
    kCheckSumError,
    kInvalidNameLen,
    kUnknownMessageType,
    kParseError,
  };

  // return false to stop parsing protobuf message
  typedef std::function<bool (Conn*, int32_t)> RawMessageCallback;

  typedef std::function<void (Conn*, const MessagePtr&p)> ProtobufMessageCallback;

  typedef std::function<void (Conn*, ErrorCode)> ErrorCallback;

  ProtobufCodecLite(const ::google::protobuf::Message* prototype,
                    std::string tagArg,
                    const ProtobufMessageCallback& messageCb,
                    const RawMessageCallback& rawCb = RawMessageCallback(),
                    const ErrorCallback& errorCb = defaultErrorCallback)
    : prototype_(prototype),
      tag_(tagArg),
      messageCallback_(messageCb),
      rawCb_(rawCb),
      errorCallback_(errorCb),
      kMinMessageLen(tagArg.size() + kChecksumLen)
  {
  }

  virtual ~ProtobufCodecLite() { }

  const std::string& tag() const { return tag_; }


  void send(Conn* conn,
            const ::google::protobuf::Message& message);
  void onMessage(Conn* conn);
  bool parseFromBuffer(const unsigned char * buf, int len,
                       google::protobuf::Message* message);;


  int serializeToBuffer(const google::protobuf::Message& message,
                        struct evbuffer* buf);
  static const std::string& errorCodeToString(ErrorCode errorCode);

  // public for unit tests
  ErrorCode parse(Conn* conn, int len, ::google::protobuf::Message* message);
  void fillEmptyBuffer(struct evbuffer* buf, const google::protobuf::Message& message);




  static int32_t checksum(struct evbuffer* buf, int len);
  static bool validateChecksum(const Bytef* buf, int len);
  static int32_t asInt32(const char* buf);
  static void defaultErrorCallback(Conn*, ErrorCode);

 private:
  const ::google::protobuf::Message* prototype_;
  const std::string tag_;
  ProtobufMessageCallback messageCallback_;
  RawMessageCallback rawCb_;
  ErrorCallback errorCallback_;
  const int kMinMessageLen;
};

template<typename MSG, const char* TAG, typename CODEC=ProtobufCodecLite>  // TAG must be a variable with external linkage, not a string literal
class ProtobufCodecLiteT
{
  static_assert(std::is_base_of<ProtobufCodecLite, CODEC>::value, "CODEC should be derived from ProtobufCodecLite");
 public:
  typedef std::shared_ptr<MSG> ConcreteMessagePtr;
  typedef std::function<void (Conn*, const ConcreteMessagePtr&)> ProtobufMessageCallback;
  typedef ProtobufCodecLite::RawMessageCallback RawMessageCallback;
  typedef ProtobufCodecLite::ErrorCallback ErrorCallback;

  explicit ProtobufCodecLiteT(const ProtobufMessageCallback& messageCb,
                              const RawMessageCallback& rawCb = RawMessageCallback(),
                              const ErrorCallback& errorCb = ProtobufCodecLite::defaultErrorCallback)
    : messageCallback_(messageCb),
      codec_(&MSG::default_instance(),
             TAG,
             std::bind(&ProtobufCodecLiteT::onRpcMessage, this, std::placeholders::_1,std::placeholders::_2),
             rawCb,
             errorCb)
  {
  }

  const std::string& tag() const { return codec_.tag(); }

  void send(Conn* conn,
            const MSG& message)
  {
    codec_.send(conn, message);
  }

  void onMessage(Conn* conn)
  {
    codec_.onMessage(conn);
  }

  // internal
  void onRpcMessage(Conn* conn,
                    const MessagePtr& message)
  {
    messageCallback_(conn, ::down_pointer_cast<MSG>(message));
  }

  void fillEmptyBuffer(struct evbuffer* buf, const google::protobuf::Message& message)
  {
    codec_.fillEmptyBuffer(buf, message);
  }

 private:
  ProtobufMessageCallback messageCallback_;
  CODEC codec_;
};

}

#endif  // RPC_CODEC_LITE_H__
