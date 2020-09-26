cmake_minimum_required(VERSION 3.10)

project(CS469_Project)

FIND_PACKAGE(PkgConfig REQUIRED)
PKG_CHECK_MODULES(GTK3 REQUIRED gtk+-3.0)
PKG_CHECK_MODULES(SQLITE3 REQUIRED sqlite3)
PKG_CHECK_MODULES(GMOD REQUIRED gmodule-2.0)
PKG_CHECK_MODULES(OPENSSL REQUIRED openssl)
find_package(Threads REQUIRED)

# Setup CMake to use GTK+, tell the compiler where to look for headers
# and to the linker where to look for libraries
INCLUDE_DIRECTORIES(${GTK3_INCLUDE_DIRS})
LINK_DIRECTORIES(${GTK3_LIBRARY_DIRS})

INCLUDE_DIRECTORIES(${SQLITE3_INCLUDE_DIRS})
LINK_DIRECTORIES(${SQLITE3_LIBRARY_DIRS})

# Add other flags to the compiler
ADD_DEFINITIONS(${GTK3_CFLAGS_OTHER})

# add_executable(inventoryserver inventoryserver/server.c inventoryserver/network.c)
add_executable(server inventoryserver/server.c inventoryserver/network.h inventoryserver/network.c inventoryserver/queue.h inventoryserver/queue.c)
target_link_libraries(server ${SQLITE3_LIBRARIES} ${OPENSSL_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} crypt)
# SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")


#add_executable(clientApp client/client.c client/login_window.c client/login_window.h client/network.h client/network.c)
#target_link_libraries(clientApp ${GTK3_LIBRARIES} ${OPENSSL_LIBRARIES} ${GMOD_LIBRARIES} "-rdynamic")
add_executable(clientApp client/client.c client/network.h client/network.c)
target_link_libraries(clientApp ${OPENSSL_LIBRARIES})

add_executable(backupserver datastore/datastore.c)

add_executable(user_mgr user_mgr.c)
target_link_libraries(user_mgr ${SQLITE3_LIBRARIES} crypt)