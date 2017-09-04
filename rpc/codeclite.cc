#include "codeclite.h"
#include "tcpserver.h"
#include "ev_endian.h"
#include "google-inl.h"

#include <google/protobuf/message.h>
#include <zlib.h>


using namespace evrpc;
using namespace evrpc::sockets;

namespace
{
int ProtobufVersionCheck()
{
  GOOGLE_PROTOBUF_VERIFY_VERSION;
  return 0;
}
int __attribute__ ((unused)) dummy = ProtobufVersionCheck();
}


namespace evrpc
{


void ProtobufCodecLite::send(Conn* conn,
                             const ::google::protobuf::Message& message)
{
//  int res = 0;
  struct evbuffer *ebuf = evbuffer_new();
  if (fillEmptyBuffer(ebuf, message) < 0)
  {
    LOG(ERROR) << "send error:"<<evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR());
  }
  if (conn->addBufToWriteBuffer(ebuf) != 0)
  {
    LOG(ERROR) << "send error:"<<evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR());
  }
  evbuffer_free(ebuf);
}

int ProtobufCodecLite::fillEmptyBuffer(struct evbuffer* buf,
                                        const google::protobuf::Message& message)
{
  int res = 0;
  if ((res = evbuffer_add(buf, tag_.c_str(), tag_.size())) < 0)
    return res;

  int byte_size = serializeToBuffer(message, buf);

  int32_t checkSum = checksum(buf, byte_size+tag_.size());
  int32_t len = sockets::hostToNetwork32(checkSum);
  if ((res = evbuffer_add(buf, &len, sizeof len)) < 0)
    return res;
  //  assert(buf->readableBytes() == tag_.size() + byte_size + kChecksumLen);
  len = sockets::hostToNetwork32(static_cast<int32_t>(tag_.size() + byte_size + kChecksumLen));
  if( (res = evbuffer_prepend(buf, &len, sizeof len)) < 0)
    return res;
  return 0;
}


int ProtobufCodecLite::serializeToBuffer(const google::protobuf::Message& message, struct evbuffer* buf)
{
  GOOGLE_DCHECK(message.IsInitialized()) << InitializationErrorMessage("serialize", message);

  struct evbuffer_iovec vec;
  uint32_t byte_size = message.ByteSize();
  /*int */evbuffer_reserve_space(buf, byte_size+kChecksumLen, &vec, 1);
  uint8_t *start = static_cast<uint8_t*>(vec.iov_base);
  uint8_t *end = message.SerializeWithCachedSizesToArray(start);
  vec.iov_len = end - start;
  if ( vec.iov_len != byte_size)
  {
    ByteSizeConsistencyError(byte_size, message.ByteSize(), static_cast<int>(end - start));
  }
  /*int */evbuffer_commit_space(buf, &vec, 1);

  return vec.iov_len;
}


void ProtobufCodecLite::onMessage(Conn* conn)
{
  LOG(INFO) << "received message len:"<<conn->getReadBufferLen();
  while (conn->getReadBufferLen() >= static_cast<uint32_t>(kMinMessageLen+kHeaderLen))
  {
    int32_t len = 0;
    conn->readBuffer(reinterpret_cast<char *>(&len), sizeof(len));
    len = asInt32(reinterpret_cast<char *>(&len));
    if (len > kMaxMessageLen || (uint32_t)len < kMinMessageLen)
    {
      errorCallback_(conn, kInvalidLength);/*change*/
      break;
    }
    else if (conn->getReadBufferLen()>= (static_cast<uint32_t>(len)))
    {
      if (rawCb_ && !rawCb_(conn, len))
      {
        evbuffer_drain(conn->getReadBuffer(), len);
        continue;
      }
      MessagePtr message(prototype_->New());
      // FIXME: can we move deserialization & callback to other thread?
      ErrorCode errorCode = parse(conn, len, message.get());
      if (errorCode == kNoError)
      {
        // FIXME: try { } catch (...) { }
        messageCallback_(conn, message);
        evbuffer_drain(conn->getReadBuffer(), len);
      }
      else
      {
        errorCallback_(conn, errorCode);
        break;
      }
    }
    else
    {
      break;
    }
  }
}

bool ProtobufCodecLite::parseFromBuffer(const unsigned char * buf, int len, google::protobuf::Message* message)
{
  return message->ParseFromArray(buf, len);
}

namespace
{
  const std::string kNoErrorStr = "NoError";
  const std::string kInvalidLengthStr = "InvalidLength";
  const std::string kCheckSumErrorStr = "CheckSumError";
  const std::string kInvalidNameLenStr = "InvalidNameLen";
  const std::string kUnknownMessageTypeStr = "UnknownMessageType";
  const std::string kParseErrorStr = "ParseError";
  const std::string kUnknownErrorStr = "UnknownError";
}

const std::string& ProtobufCodecLite::errorCodeToString(ErrorCode errorCode)
{
  switch (errorCode)
  {
    case kNoError:
      return kNoErrorStr;
    case kInvalidLength:
      return kInvalidLengthStr;
    case kCheckSumError:
      return kCheckSumErrorStr;
    case kInvalidNameLen:
      return kInvalidNameLenStr;
    case kUnknownMessageType:
      return kUnknownMessageTypeStr;
    case kParseError:
      return kParseErrorStr;
    default:
      return kUnknownErrorStr;
  }
}

void ProtobufCodecLite::defaultErrorCallback(Conn* conn,
                                             ErrorCode errorCode)
{
  //LOG(DEBUF)<< "ProtobufCodecLite::defaultErrorCallback - " << errorCodeToString(errorCode);
  if (conn)
  {
    bufferevent_free(conn->getBufferevent());
    conn->getThread()->connect_queue.deleteConn(conn);
  }
}

int32_t ProtobufCodecLite::asInt32(const char* buf)
{
  int32_t be32 = 0;
  ::memcpy(&be32, buf, sizeof(be32));
  return sockets::networkToHost32(be32);
}

int32_t ProtobufCodecLite::checksum(struct evbuffer* buf, int len)
{
  int dlen = 0;
  uLong adler = adler32(1L, Z_NULL, 0);
  int n = evbuffer_peek(buf, len, NULL, NULL, 0);
  evbuffer_iovec *vec = new evbuffer_iovec[n];
  int res = evbuffer_peek(buf, len, NULL, vec, n);
  assert( res == n);
  for (int i = 0; i < n; i++){
    dlen += vec[i].iov_len;
    adler = adler32(adler, static_cast<const Bytef*>(vec[i].iov_base), vec[i].iov_len);
  }
  delete [] vec;
//  LOG(INFO) << "Len : "<< dlen << "checksum : "<<adler;

  return static_cast<int32_t>(adler);
}

bool ProtobufCodecLite::validateChecksum(const Bytef* buf, int len)
{
  int32_t expectedCheckSum = asInt32(reinterpret_cast<const char *>(buf) + len - kChecksumLen);
  uLong adler = adler32(1L, Z_NULL, 0);
  adler = adler32(adler, (buf), len-kChecksumLen);
//  LOG(INFO) << "Len : "<< len-kChecksumLen << "checksum : "<<adler;

  return (static_cast<int32_t>(adler)) == (expectedCheckSum);
}

ProtobufCodecLite::ErrorCode ProtobufCodecLite::parse(Conn* conn,
                                                      int len,
                                                      ::google::protobuf::Message* message)
{
  ErrorCode error = kNoError;
  evbuffer *readBuffer = conn->getReadBuffer();
  //  std::vector<char> tag(tag_.size());
  unsigned char *data = evbuffer_pullup(readBuffer, len);
  if (data == NULL){
    //
  }
/*
  struct evbuffer_iovec vec ;
  res = evbuffer_peek(readBuffer, len, NULL, &vec, 1);
  assert( res == 1);
*/
  if (validateChecksum(static_cast<const Bytef*>(data), len))
  {
    if (memcmp(data, tag_.data(), tag_.size()) == 0)
    {
      // parse from buffer
      int32_t dataLen = len - kChecksumLen - static_cast<int>(tag_.size());
      if (parseFromBuffer(data+tag_.size(), dataLen,  message))
      {
        error = kNoError;
      }
      else
      {
        error = kParseError;
      }
    }
    else
    {
      error = kUnknownMessageType;
    }
  }
  else
  {
    error = kCheckSumError;
  }

  return error;
}

}
