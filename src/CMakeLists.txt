SET(SourceFiles
  aggregator_solver_protocol.cpp
  netbuffer.cpp
  sample_data.cpp
  sensor_aggregator_protocol.cpp
  simple_sockets.cpp
  world_model_protocol.cpp
  message_receiver.cpp
  grail_types.cpp
)

add_library (owl-common SHARED ${SourceFiles})
set (FULLVERSION ${LibOwl_VERSION_MAJOR}.${LibOwl_VERSION_MINOR}.${LibOwl_VERSION_REVISION})
SET_TARGET_PROPERTIES(owl-common PROPERTIES VERSION ${FULLVERSION} SOVERSION ${FULLVERSION})

SET(CMAKE_INSTALL_LIBDIR lib${LIB_SUFFIX} CACHE PATH "Output directory for libraries")
INSTALL(TARGETS owl-common DESTINATION ${CMAKE_INSTALL_LIBDIR})
