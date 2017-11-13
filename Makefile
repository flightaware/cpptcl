all:
	rm -rf build
	mkdir build
	(cd build; cmake ..; make)

clean:
	rm -rf build build_xcode

test:	all
	tclsh tests/cpptcl_module_one.tcl 

format:
	find . -name \*.h -o -name \*.cc | xargs clang-format-4.0 -style=file  -i

xcode:
	rm -rf build_xcode
	mkdir build_xcode
	(cd build_xcode; cmake -G Xcode ..)
	exec open build_xcode/cpptcl.xcodeproj
