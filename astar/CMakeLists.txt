
make_astari_package(astar_impl)

file(GLOB_RECURSE astar_src)
add_executable(astar main.cpp ${astar_src})
target_link_libraries(astar PRIVATE astari_static)
target_include_directories(astar PRIVATE ${PROJECT_SOURCE_DIR})
install(TARGETS astar DESTINATION bin)