
make_astari_package(parse)

install(FILES ${PROJECT_SOURCE_DIR}/parser.plo DESTINATION lib/astari-pl)
target_compile_definitions(parse
  PUBLIC PARSER_OBJFILE_PATH=\"${CMAKE_INSTALL_PREFIX}/lib/astari-pl/parser.plo\")