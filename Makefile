CPPTCL_LIBRARY = build/libcpptcl.so
MODULE_PATH = modules/example_functions
MODULE_LIBRARY = $(MODULE_PATH)/build/libcpptcl_example_functions.so

all: $(MODULE_LIBRARY) 

build/Makefile: CMakeLists.txt
	(mkdir -p build; cd build; cmake ..)

$(CPPTCL_LIBRARY): build/Makefile
	(cd build; make)

install: all
	(cd build; make install)

test: all
	(cd build; make test)

$(MODULE_LIBRARY): $(CPPTCL_LIBRARY) $(MODULE_PATH)/build/Makefile
	(cd $(MODULE_PATH)/build; make)

$(MODULE_PATH)/build/Makefile: $(MODULE_PATH)/CMakeLists.txt
	(cd $(MODULE_PATH); mkdir -p build)
	(cd $(MODULE_PATH)/build; cmake -DCPPTCL_SOURCE_DIR=$$PWD/../../../ ..)

clean:
	rm -rf build build_xcode modules/example_functions/build

test_tcl: all
	/usr/bin/env LD_LIBRARY_PATH=./build TCLLIBPATH=./modules/example_functions/build tclsh tests/cpptcl_module_one.tcl 

format:
	find . -name \*.h -o -name \*.cc | xargs clang-format-4.0 -style=file  -i

xcode:
	rm -rf build_xcode
	mkdir build_xcode
	(cd build_xcode; cmake -G Xcode ..)
	exec open build_xcode/cpptcl.xcodeproj
