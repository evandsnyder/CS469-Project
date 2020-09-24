cmake_minimum_required(VERSION 3.10)

project(CS469_Project)

FIND_PACKAGE(PkgConfig REQUIRED)
PKG_CHECK_MODULES(GTK3 REQUIRED gtk+-3.0)
PKG_CHECK_MODULES(SQLITE3 REQUIRED sqlite3)
PKG_CHECK_MODULES(GMOD REQUIRED gmodule-2.0)

# Setup CMake to use GTK+, tell the compiler where to look for headers
# and to the linker where to look for libraries
INCLUDE_DIRECTORIES(${GTK3_INCLUDE_DIRS})
LINK_DIRECTORIES(${GTK3_LIBRARY_DIRS})

INCLUDE_DIRECTORIES(${SQLITE3_INCLUDE_DIRS})
LINK_DIRECTORIES(${SQLITE3_LIBRARY_DIRS})

# Add other flags to the compiler
ADD_DEFINITIONS(${GTK3_CFLAGS_OTHER})

#add_executable(dbserver dbserver/server.c)
#target_link_libraries(dbserver ${SQLITE3_LIBRARIES})

add_executable(inventoryserver inventoryserver/server.c inventoryserver/network.h inventoryserver/network.c)
target_link_libraries(inventoryserver ${SQLITE3_LIBRARIES})


add_executable(client client/client.c client/login_window.c client/login_window.h client/network.h client/network.c)
target_link_libraries(client ${GTK3_LIBRARIES} ${GMOD_LIBRARIES})
target_link_options(client PRIVATE -rdynamic)

add_executable(datastore datastore/datastore.c)