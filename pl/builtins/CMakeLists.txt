
make_astari_package(builtins)


file(GLOB_RECURSE plfiles ${CMAKE_CURRENT_SOURCE_DIR}/src/*.pl)
foreach (plfilepath ${plfiles})
  get_filename_component(targetname ${plfilepath} NAME_WE)
  get_filename_component(filename ${plfilepath} NAME)
  set(plofile "${filename}o")
  add_custom_command(
    OUTPUT ${plofile}
    COMMAND env ASTARI_PARSER_OBJFILE_PATH=${CMAKE_SOURCE_DIR}/parser.plo $<TARGET_FILE:astari_plo> ${plfilepath} ${plofile}
    MAIN_DEPENDENCY ${plfilepath}
    DEPENDS astari_plo
    VERBATIM)
  add_custom_target(${targetname} ALL DEPENDS ${plofile})
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${plofile} DESTINATION lib/astari-pl)
  target_compile_definitions(builtins
    PRIVATE "PLO_PATH_${targetname}=\"${CMAKE_INSTALL_PREFIX}/lib/astari-pl/${plofile}\"")
endforeach ()


file(GLOB tests t/*.pl)
string(REPLACE ";" " " tests "${tests}")
add_test(pl/builtins bash -c "${CMAKE_BINARY_DIR}/apps/astari-pl ${tests} <<<'t.'")