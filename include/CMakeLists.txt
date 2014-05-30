SET(IncludeFiles
  aggregator_solver_protocol.hpp
  netbuffer.hpp
  sample_data.hpp
  sensor_aggregator_protocol.hpp
  simple_sockets.hpp
  world_model_protocol.hpp
  message_receiver.hpp
  grail_types.hpp
  temporarily_unavailable.hpp
)

#Need to install all of the include files
SET(INCLUDE_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/include/owl/" CACHE PATH "The subdirectory to the header prefix")
INSTALL(FILES ${IncludeFiles} PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ DESTINATION ${INCLUDE_INSTALL_DIR})

