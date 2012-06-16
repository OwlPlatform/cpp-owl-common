SET(SourceFiles
  aggregator_solver_protocol.cpp
  netbuffer.cpp
  sample_data.cpp
  sensor_aggregator_protocol.cpp
  simple_sockets.cpp
  world_model_protocol.cpp
)

add_library (Owl SHARED ${SourceFiles})
SET_TARGET_PROPERTIES(Owl PROPERTIES VERSION ${LibOwl_VERSION_MAJOR}.${LibOwl_VERSION_MINOR}
                                     SOVERSION ${LibOwl_VERSION_MAJOR}.${LibOwl_VERSION_MINOR})

SET(CMAKE_INSTALL_LIBDIR lib CACHE PATH "Output directory for libraries")
INSTALL(TARGETS Owl DESTINATION ${CMAKE_INSTALL_LIBDIR})
