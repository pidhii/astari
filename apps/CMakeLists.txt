
add_executable(astari_pl astari-pl.cpp)
set_target_properties(astari_pl PROPERTIES OUTPUT_NAME astari-pl)
target_link_libraries(astari_pl PRIVATE astari_static)
target_include_directories(astari_pl PRIVATE ${PROJECT_SOURCE_DIR})
install(TARGETS astari_pl DESTINATION bin)