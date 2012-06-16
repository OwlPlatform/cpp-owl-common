SET(SourceFiles
  aggregator_solver_protocol.cpp
  netbuffer.cpp
  sample_data.cpp
  sensor_aggregator_protocol.cpp
  simple_sockets.cpp
  world_model_protocol.cpp
)

add_library (LibOwl SHARED ${SourceFiles})
SET_TARGET_PROPERTIES(LibOwl PROPERTIES VERSION ${LibOwl_VERSION_MAJOR}.${LibOwl_VERSION_MINOR})
SET_TARGET_PROPERTIES(LibOwl PROPERTIES SOVERSION ${LibOwl_VERSION_MAJOR}.${LibOwl_VERSION_MINOR})

INSTALL(TARGETS LibOwl DESTINATION lib${LIB_SUFFIX})

