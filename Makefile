.PHONY: all configure build install test

configure:
	cmake -B ./build -S . -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=./install

build: configure
	make -C build -j32
	
install: configure
	make -C build -j32 install
	
test: build
	make -C build test
