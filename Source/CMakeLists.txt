# Build lib

#set(PRO_NAME "AntEngine-${APP_BTYPE}")
set(PRO_NAME "AntEngine")
set(PRO_VERSION "1.0.0.${BUILD_NUMBER}")
set(PRO_BRIEF "${PRO_NAME} is a C++ lib.")

#file(GLOB_RECURSE SRC_NET RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cpp)
aux_source_directory("${CMAKE_CURRENT_SOURCE_DIR}" SRC_CORE)
aux_source_directory("${CMAKE_CURRENT_SOURCE_DIR}/Net" SRC_NET)
aux_source_directory("${CMAKE_CURRENT_SOURCE_DIR}/Net/HTTP" SRC_HTTP)
aux_source_directory("${CMAKE_CURRENT_SOURCE_DIR}/Script" SRC_SCRIPT)
aux_source_directory("${CMAKE_CURRENT_SOURCE_DIR}/Net/RedisClient" SRC_REDIS)
aux_source_directory("${CMAKE_CURRENT_SOURCE_DIR}/${APP_OS}" SRC_OS)
aux_source_directory("${PROJECT_SOURCE_DIR}/Depend/http_parser" SRC_HTTPARSER)
aux_source_directory("${PROJECT_SOURCE_DIR}/Depend/jsoncpp" SRC_JSON)

link_directories(${LIBRARY_OUTPUT_PATH})

add_library(${PRO_NAME} STATIC 
    ${SRC_NET}
    ${SRC_HTTP}
    ${SRC_REDIS}
    ${SRC_OS}
    ${SRC_CORE}
    ${SRC_JSON}
    ${SRC_SCRIPT}
    ${SRC_HTTPARSER}
    )

include_directories(
    ${PROJECT_SOURCE_DIR}/Depend/http_parser
    ${PROJECT_SOURCE_DIR}/Depend/jsoncpp
    ${PROJECT_SOURCE_DIR}/Depend/lua/src
    ${PROJECT_SOURCE_DIR}/Depend/MySQL
    ${PROJECT_SOURCE_DIR}/Depend/MySQL/mysql
    ${PROJECT_SOURCE_DIR}/Depend/openssl/include
    ${PROJECT_SOURCE_DIR}/Include
    ${PROJECT_SOURCE_DIR}/Include/${APP_OS}
    ${PROJECT_SOURCE_DIR}/Include/Net
    ${PROJECT_SOURCE_DIR}/Include/Net/HTTP
    ${PROJECT_SOURCE_DIR}/Include/Net/RedisClient
    )


install(DIRECTORY ${PROJECT_SOURCE_DIR}/include
    DESTINATION .
    COMPONENT headers
    )
