SET(IncludeFiles
  aggregator_solver_protocol.hpp
  netbuffer.hpp
  sample_data.hpp
  sensor_aggregator_protocol.hpp
  simple_sockets.hpp
  world_model_protocol.hpp
)

#Need to install all of the include files
INSTALL(FILES ${IncludeFiles} DESTINATION ${INCLUDE_INSTALL_DIR})
