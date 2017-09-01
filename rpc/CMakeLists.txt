#cmake file for ev server
#author changfa.zheng

add_custom_command(OUTPUT rpc.pb.cc rpc.pb.h
  COMMAND protoc ARGS --cpp_out . ${CMAKE_CURRENT_SOURCE_DIR}/rpc.proto -I${CMAKE_CURRENT_SOURCE_DIR}
  DEPENDS rpc.proto 
  VERBATIM )

add_custom_target(rpc_comp DEPENDS rpc.pb.cc rpc.pb.h)



INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/include)
INCLUDE_DIRECTORIES(${PROJECT_BINARY_DIR}/rpc)

#LINK_DIRECTORIES(
#    ./
#    )


#获得一个目录下所有源文件（不包括头文件）到变量SRCS
#AUX_SOURCE_DIRECTORY(dirname SRCS)
#AUX_SOURCE_DIRECTORY(./ SOURCE_FILES)

set(SOURCE_FILES 
  codeclite.cc
  rpcchannel.cc
  rpccodec.cc
  rpcserver.cc
  tcpserver.cc
  tcpclient.cc
  rpc.pb.cc)

ADD_LIBRARY(evrpc ${SOURCE_FILES})
#生成静态链接库libname.a
#ADD_LIBRARY(name $SOURCE_FILES)

add_dependencies(evrpc rpc_comp)
#依赖的库文件
TARGET_LINK_LIBRARIES(evrpc glog pthread event)

install(TARGETS evrpc DESTINATION ${PROJECT_BINARY_DIR}/lib/)

SET(HEADERS
    codeclite.h
    google-inl.h
    rpcchannel.h
    rpccodec.h
    rpcserver.h
    tcpserver.h
    tcpclient.h
    ${PROJECT_BINARY_DIR}/rpc/rpc.pb.h
    )

install(FILES ${HEADERS} DESTINATION ${PROJECT_BINARY_DIR}/include/rpc/)
