
add_executable(astari_opium opium.cpp)
set_target_properties(astari_opium PROPERTIES OUTPUT_NAME opium)
target_link_libraries(astari_opium PRIVATE astari_static)
target_include_directories(astari_opium PRIVATE ${PROJECT_SOURCE_DIR})
install(TARGETS astari_opium DESTINATION bin)