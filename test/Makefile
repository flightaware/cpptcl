all:	build/Makefile
	(cd build; make)

build/Makefile:
	cmake -Bbuild -H.

clean:
	rm -rf build

test:	all
	(cd build; ctest)
