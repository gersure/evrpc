#ifndef RPC_RPCCODEC_H
#define RPC_RPCCODEC_H


#include "codeclite.h"

namespace evrpc
{

class Buffer;
class TcpConnection;
typedef std::shared_ptr<Conn *> TcpConnectionPtr;

class RpcMessage;
typedef std::shared_ptr<RpcMessage> RpcMessagePtr;
extern const char rpctag[];// = "RPC0";

// wire format
//
// Field     Length  Content
//
// size      4-byte  N+8
// "RPC0"    4-byte
// payload   N-byte
// checksum  4-byte  adler32 of "RPC0"+payload
//

typedef ProtobufCodecLiteT<RpcMessage, rpctag> RpcCodec;

}

#endif  // MUDUO_NET_PROTORPC_RPCCODEC_H
