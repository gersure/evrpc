#cmake file for ev server
#author changfa.zheng

add_custom_command(OUTPUT evrpc.pb.cc evrpc.pb.h
  COMMAND protoc ARGS --cpp_out . ${CMAKE_CURRENT_SOURCE_DIR}/evrpc.proto -I${CMAKE_CURRENT_SOURCE_DIR}
  DEPENDS evrpc.proto
  VERBATIM )
add_custom_target(evserver_proto DEPENDS evrpc.pb.cc evrpc.pb.h)

INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/include)
INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/rpc)
INCLUDE_DIRECTORIES(${PROJECT_BINARY_DIR}/example)
INCLUDE_DIRECTORIES(${PROJECT_BINARY_DIR}/rpc)
LINK_DIRECTORIES(${LIBRARY_OUTPUT_DIRECTORY})


ADD_EXECUTABLE(tcpclient_test tcpclient_test.cc)
ADD_EXECUTABLE(tcpserver_test tcpserver_test.cc)

ADD_EXECUTABLE(ev_server evserver.cc evrpc.pb.cc)
ADD_EXECUTABLE(ev_client evclient.cc evrpc.pb.cc)

add_dependencies(ev_server evserver_proto)
add_dependencies(ev_client evserver_proto)

TARGET_LINK_LIBRARIES(tcpclient_test evrpc glog pthread event)
TARGET_LINK_LIBRARIES(tcpserver_test evrpc glog pthread event)
TARGET_LINK_LIBRARIES(ev_server evrpc z protobuf glog pthread event)
TARGET_LINK_LIBRARIES(ev_client evrpc z protobuf glog pthread event)





