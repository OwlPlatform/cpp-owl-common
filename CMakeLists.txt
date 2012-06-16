cmake_minimum_required (VERSION 2.6)
project (LibOwlC++)

# The version number.
set (LibOwl_VERSION_MAJOR 1)
set (LibOwl_VERSION_MINOR 0)

set(CMAKE_CXX_FLAGS "-O2 -std=c++0x -Wall -Wextra -Wno-sign-compare")

#SUBDIRS(src)
#add_subdirectory ("${PROJECT_SOURCE_DIR}/src")

include_directories ("${LibOwlC++_SOURCE_DIR}/include")
add_subdirectory (src)

SET (LIB_SUFFIX "" CACHE STRING "Define suffix of library directory name (32/64)" )
SET (LIB_DESTINATION "${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX}" CACHE STRING "Library directory name")
## the following are directories where stuff will be installed to
SET(INCLUDE_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/include/owl/" CACHE PATH "The subdirectory to the header prefix")
SET(PKGCONFIG_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX}/pkgconfig/" CACHE STRING "Base directory for pkgconfig files")

#add_library (LibOwl SHARED ${IncludeFiles} ${SourceFiles})

#$(LIBDIR)/libowl.a: $(LIBDIR)/sensor_aggregator_protocol.o $(LIBDIR)/aggregator_solver_protocol.o $(LIBDIR)/world_model_protocol.o $(LIBDIR)/netbuffer.o $(LIBDIR)/sample_data.o $(LIBDIR)/simple_sockets.o
