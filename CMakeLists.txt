cmake_minimum_required (VERSION 2.6)
project (BackupConnection)
set (BackupConnection_VERSION_MAJOR 1)
set (BackupConnection_VERSION_MINOR 0)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive -Wextra -Werror")

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(IPTC REQUIRED libiptc)
pkg_check_modules(CURL REQUIRED libcurl)

add_executable(BackupConnection connmgr.cpp)
target_link_libraries(BackupConnection ${IPTC_LIBRARIES})
target_link_libraries(BackupConnection ${CURL_LIBRARIES})
target_link_libraries (BackupConnection Threads::Threads)
