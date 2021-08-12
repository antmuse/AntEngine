# Build test app

#set(PRO_NAME "AntTest-${APP_BTYPE}")
set(PRO_NAME "AntTest")
set(PRO_VERSION "1.0.0.${BUILD_NUMBER}")
set(PRO_BRIEF "${PRO_NAME} is a C++ test app.")

#file(GLOB_RECURSE SRC_NET RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cpp)
aux_source_directory("${CMAKE_CURRENT_SOURCE_DIR}" SRC_TEST)

link_directories(${LIBRARY_OUTPUT_PATH})

add_executable(${PRO_NAME} ${SRC_TEST})

#libs
if (${APP_OS} MATCHES "Linux")
    target_link_libraries(${PRO_NAME} pthread)
    target_link_libraries(${PRO_NAME} "crypto")
    target_link_libraries(${PRO_NAME} "ssl")
else()
    target_link_libraries(${PRO_NAME} shlwapi)
    target_link_libraries(${PRO_NAME} ws2_32)
    target_link_libraries(${PRO_NAME} "libcrypto")
    target_link_libraries(${PRO_NAME} "libssl")
endif()
target_link_libraries(${PRO_NAME} debug "AntEngine-${APP_BTYPE}D")
target_link_libraries(${PRO_NAME} optimized "AntEngine-${APP_BTYPE}")
#CMAKE_INTDIR

include_directories(
    ${PROJECT_SOURCE_DIR}/Depend/http_parser
    ${PROJECT_SOURCE_DIR}/Depend/jsoncpp
    ${PROJECT_SOURCE_DIR}/Depend/openssl/include
    ${PROJECT_SOURCE_DIR}/Include
    ${PROJECT_SOURCE_DIR}/Include/${APP_OS}
    ${PROJECT_SOURCE_DIR}/Include/Net
    ${PROJECT_SOURCE_DIR}/Include/Net/HTTP
    ${PROJECT_SOURCE_DIR}/Include/Net/RedisClient
    ${PROJECT_SOURCE_DIR}/Source/Test
    )


install(DIRECTORY ${PROJECT_SOURCE_DIR}/include
    DESTINATION .
    COMPONENT headers
    )


set_target_properties(${PRO_NAME} PROPERTIES DEBUG_POSTFIX "-${APP_BTYPE}D")
set_target_properties(${PRO_NAME} PROPERTIES RELEASE_POSTFIX "-${APP_BTYPE}")
