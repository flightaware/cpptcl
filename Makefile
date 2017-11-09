all:
	rm -rf build
	mkdir build
	(cd build; cmake ..; make)

xcode:
	rm -rf build
	mkdir build
	(cd build; cmake -G Xcode ..)
	exec open build/cpptcl.xcodeproj
