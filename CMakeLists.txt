cmake_minimum_required(VERSION 3.13.0 FATAL_ERROR)
project(AntEngine)

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set(APP_OS Linux)
    set(DEBUG_FLAGS "-g -O0 -Wall -DEBUG")
    set(RELEASE_FLAGS "-O3")
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set(APP_OS Windows)
    set(DEBUG_FLAGS "")
    set(RELEASE_FLAGS "")
    add_definitions(-DUNICODE -D_UNICODE)
else()
    set(APP_OS NULL)
    message(STATUS "other platform: ${CMAKE_SYSTEM_NAME}")
endif()

# if(WIN32)
# 	message(STATUS "Now is windows")
# elseif(APPLE)
# 	message(STATUS "Now is Apple systens.")
# elseif(UNIX)
# 	message(STATUS "Now is UNIX-like OS's.")
# endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(APP_BTYPE 64)
else()
    set(APP_BTYPE 32)
endif()

if($ENV{BUILD_NUMBER})
    set(BUILD_NUMBER $ENV{BUILD_NUMBER})
else()
    set(BUILD_NUMBER 0)
endif()

set(CMAKE_CXX_FLAGS ${RELEASE_FLAGS})
set(CMAKE_C_FLAGS ${RELEASE_FLAGS})
set(CMAKE_CXX_FLAGS_DEBUG ${DEBUG_FLAGS})
set(CMAKE_CXX_FLAGS_DEBUG ${GCOV_FLAGS})
set(CMAKE_C_FLAGS_DEBUG ${DEBUG_FLAGS})
set(CMAKE_C_FLAGS_DEBUG ${GCOV_FLAGS})
set(CMAKE_EXE_LINKER_FLAGS_DEBUG ${GCOV_FLAGS})
set(EXECUTABLE_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/Bin")
set(LIBRARY_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/Lib/${APP_OS}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_SOURCE_DIR}/Bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/Bin) 
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${PROJECT_SOURCE_DIR}/Lib/${APP_OS}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${PROJECT_SOURCE_DIR}/Lib/${APP_OS}")

#库文件的后缀名
set(CMAKE_DEBUG_POSTFIX   "-${APP_BTYPE}D")
set(CMAKE_RELEASE_POSTFIX "-${APP_BTYPE}")


# TODO OP check how this works on multiple configurations generators
# available configurations in multiple configurations enabled generators
set(CMAKE_CONFIGURATION_TYPES Debug Release)

message("Src path = ${PROJECT_SOURCE_DIR}")
message("Build path = ${PROJECT_BINARY_DIR}")
message("Bin path = ${EXECUTABLE_OUTPUT_PATH}")
message("Lib path = ${LIBRARY_OUTPUT_PATH}")
message("Build type = ${APP_BTYPE}")
message("OS type = ${APP_OS}")

# find_package(OpenSSL REQUIRED)

add_subdirectory(Source)
add_subdirectory(Source/Test)

# Licensing and stuff like that
set(LICENSE_FILE ${PROJECT_SOURCE_DIR}/LICENSE-MIT)

# CMAKE_INSTALL_PREFIX
install(FILES
    ${README_FILE}
    ${LICENSE_FILE}
    DESTINATION .
    )

