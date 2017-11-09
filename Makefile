all:
	rm -rf build
	mkdir build
	(cd build; cmake ..; make)

xcode:
	rm -rf build_xcode
	mkdir build_xcode
	(cd build_xcode; cmake -G Xcode ..)
	exec open build_xcode/cpptcl.xcodeproj
