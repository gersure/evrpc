#cmake file for ev server
#author changfa.zheng

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)


#project name
PROJECT(ev_server)

#execute_process(COMMAND protoc --cpp_out . ${CMAKE_CURRENT_SOURCE_DIR}/rpc.proto -I${CMAKE_CURRENT_SOURCE_DIR})

#add_custom_command(OUTPUT evrpc.pb.cc evrpc.pb.h
#  COMMAND protoc ARGS --cpp_out . ${CMAKE_CURRENT_SOURCE_DIR}/evrpc.proto -I${CMAKE_CURRENT_SOURCE_DIR}
#  DEPENDS evrpc.proto
#  VERBATIM )
#add_custom_target(evserver_proto DEPENDS evrpc.pb.cc evrpc.pb.h)


set(CMAKE_CXX_COMPILER "g++")
SET(CMAKE_CXX_FLAGS "-std=c++11")
SET(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -finline-limit=1000 -DNDEBUG")
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

#SET(CMAKE_BUILD_TYPE Release)
SET(CMAKE_BUILD_TYPE DEBUG)

#SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ./bin)



#INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/include)
#INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/rpc)
#INCLUDE_DIRECTORIES(${PROJECT_BINARY_DIR})
#LINK_DIRECTORIES(${LIBRARY_OUTPUT_DIRECTORY})

#message(STATUS ${PROJECT_BINARY_DIR})
#message(STATUS ${PROJECT_SOURCE_DIR})

add_subdirectory(rpc)
add_subdirectory(example)

#获得一个目录下所有源文件（不包括头文件）到变量SRCS
#AUX_SOURCE_DIRECTORY(dirname SRCS)
#AUX_SOURCE_DIRECTORY(./ SOURCE_FILES)


#生成静态链接库libname.a
#ADD_LIBRARY(name $SOURCE_FILES)


#ADD_EXECUTABLE(ev_server evserver.cc evrpc.pb.cc)
#ADD_EXECUTABLE(ev_client evclient.cc evrpc.pb.cc)
#add_dependencies(ev_server evserver_proto)
#add_dependencies(ev_client evserver_proto)

#依赖的库文件
#TARGET_LINK_LIBRARIES(ev_server evrpc z protobuf glog pthread event)
#TARGET_LINK_LIBRARIES(ev_client evrpc z protobuf glog pthread event)

SET(HEADERS
    include/atomic.h
    include/ev_endian.h
    include/util.h
    )

install(FILES ${HEADERS} DESTINATION ${PROJECT_BINARY_DIR}/include/)

