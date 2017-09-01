#cmake file for ev server
#author changfa.zheng

#add_custom_command(OUTPUT rpc.pb.cc rpc.pb.h
#  COMMAND protoc ARGS --cpp_out . ${CMAKE_CURRENT_SOURCE_DIR}/rpc.proto -I${CMAKE_CURRENT_SOURCE_DIR}
#  DEPENDS rpc.proto
#  VERBATIM )

#add_custom_target(rpc_comp DEPENDS rpc.pb.cc rpc.pb.h)



INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/include)
INCLUDE_DIRECTORIES(${PROJECT_BINARY_DIR}/rpc)
LINK_DIRECTORIES(${LIBRARY_OUTPUT_DIRECTORY})


ADD_EXECUTABLE(tcpclient_test tcpclient_test.cc)
ADD_EXECUTABLE(tcpserver_test tcpserver_test.cc)

#add_dependencies(evrpc rpc_comp)
#依赖的库文件
TARGET_LINK_LIBRARIES(tcpclient_test evrpc glog pthread event)
TARGET_LINK_LIBRARIES(tcpserver_test evrpc glog pthread event)
