
make_astari_package(builtins)

file(GLOB tests t/*.pl)
string(REPLACE ";" " " tests "${tests}")
add_test(pl/builtins bash -c "${CMAKE_BINARY_DIR}/astari_main ${tests} <<<'t.'")