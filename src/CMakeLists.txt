SET(SourceFiles
  aggregator_solver_protocol.cpp
  netbuffer.cpp
  sample_data.cpp
  sensor_aggregator_protocol.cpp
  simple_sockets.cpp
  world_model_protocol.cpp
  message_receiver.cpp
)

add_library (Owl SHARED ${SourceFiles})
set (FULLVERSION ${LibOwl_VERSION_MAJOR}.${LibOwl_VERSION_MINOR}.${LibOwl_VERSION_REVISION})
SET_TARGET_PROPERTIES(Owl PROPERTIES VERSION ${FULLVERSION} SOVERSION ${FULLVERSION})

SET(CMAKE_INSTALL_LIBDIR lib${LIB_SUFFIX} CACHE PATH "Output directory for libraries")
INSTALL(TARGETS Owl DESTINATION ${CMAKE_INSTALL_LIBDIR})
