all:
	rm -rf build
	mkdir build
	(cd build; cmake ..; make)

install: all
	(cd build; make install)

.PHONY: modules

modules:
	(cd modules/example_functions; mkdir build; cd build; cmake -DCPPTCL_SOURCE_DIR=$PWD/../../../ ..; make)

clean:
	rm -rf build build_xcode modules/example_functions/build

test:	all modules
	/usr/bin/env LD_LIBRARY_PATH=./build TCLLIBPATH=./modules/example_functions/build tclsh tests/cpptcl_module_one.tcl 

format:
	find . -name \*.h -o -name \*.cc | xargs clang-format-4.0 -style=file  -i

xcode:
	rm -rf build_xcode
	mkdir build_xcode
	(cd build_xcode; cmake -G Xcode ..)
	exec open build_xcode/cpptcl.xcodeproj
