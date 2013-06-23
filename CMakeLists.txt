build_subdir(${CMAKE_CURRENT_SOURCE_DIR})

set(DATA irc2sql.example.conf)
install(FILES ${DATA}
  DESTINATION ${CONF_DIR}
)

set(DATA geoipupdate)
install (FILES ${DATA}
  DESTINATION ${BIN_DIR}
)
