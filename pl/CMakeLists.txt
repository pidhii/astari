
make_astari_package(pl)

add_subdirectory(core)
add_subdirectory(misc)
add_subdirectory(obj)
add_subdirectory(coding)
add_subdirectory(parse)

set(astari_packages ${astari_packages} PARENT_SCOPE)
set(astari_objects ${astari_objects} PARENT_SCOPE)