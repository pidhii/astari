.PHONY: all configure build install test

configure:
	cmake -B ./build -S . -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=./install

build: configure
	$(MAKE) -C build
	
install: configure
	$(MAKE) -C build install
	
test: install
	$(MAKE) -C build test
