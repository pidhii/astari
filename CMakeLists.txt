cmake_minimum_required(VERSION 3.15)
project(astari VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)


set(astari_packages "")
set(astari_objects "")

macro(register_astari_package package_name)
  # Library
  list(APPEND astari_packages ${package_name})
  set(astari_packages ${astari_packages} PARENT_SCOPE)
  # Objects
  list(APPEND astari_objects $<TARGET_OBJECTS:${package_name}>)
  set(astari_objects ${astari_objects} PARENT_SCOPE)
endmacro()

macro(make_astari_package package_name)
  # Object library
  file(GLOB_RECURSE package_src src/*.cpp)
  if (package_src)
    add_library(${package_name} OBJECT ${package_src})
    set_target_properties(${package_name} PROPERTIES POSITION_INDEPENDENT_CODE ON)
    target_include_directories(${package_name}
      PUBLIC
        ${PROJECT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
    )
    # Register package
    register_astari_package(${package_name})
  endif ()
  # Install headers
  file(GLOB package_headers *.hpp *.inl)
  if (package_headers)
    file(RELATIVE_PATH package_prefix ${PROJECT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
    install (FILES ${package_headers} DESTINATION include/astari/${package_prefix})
  endif ()
endmacro()

# include_directories(${PROJECT_SOURCE_DIR})
add_subdirectory(pl)

message(STATUS "astari packages: ${astari_packages}")

add_library(astari_shared SHARED ${astari_objects})
install (TARGETS astari_shared DESTINATION lib)

set_target_properties(astari_shared PROPERTIES OUTPUT_NAME astari)