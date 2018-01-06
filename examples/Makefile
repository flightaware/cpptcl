OUTPUTS = build/example2 build/example6

all : ${OUTPUTS}

build/Makefile:
	mkdir -p build
	(cd build; cmake ..)

build/example2 : build/Makefile example2.cc
	(cd build; make example2)

build/example6 : build/Makefile example6.cc
	(cd build; make example6)

clean :
	rm -rf build
