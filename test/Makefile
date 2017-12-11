COMPILER = clang++
CXXFLAGS = -Wall -Wno-long-long -pedantic -std=c++14
TESTFLAGS = 
INCLUDES = -I.. -I/usr/local/include/tcl8.6
LIBDIRS = -L/usr/local/lib
LIBS = -ltcl86 -ltclstub86

OUTPUTS = test1 test3 test4 test5 test6

all : build dotest

build : ${OUTPUTS}

test1 : test1.cc cpptcl.o
	${COMPILER} test1.cc cpptcl.o -o $@ ${CXXFLAGS} ${TESTFLAGS} ${INCLUDES} ${LIBDIRS} ${LIBS}

test2.so : test2.cc cpptcl.o
	${COMPILER} -shared -fPIC test2.cc cpptcl.o -o $@ ${CXXFLAGS} ${TESTFLAGS} ${INCLUDES}

test3 : test3.cc cpptcl.o
	${COMPILER} test3.cc cpptcl.o -o $@ ${CXXFLAGS} ${TESTFLAGS} ${INCLUDES} ${LIBDIRS} ${LIBS}

test4 : test4.cc cpptcl.o
	${COMPILER} test4.cc cpptcl.o -o $@ ${CXXFLAGS} ${TESTFLAGS} ${INCLUDES} ${LIBDIRS} ${LIBS}

test5 : test5.cc cpptcl.o
	${COMPILER} test5.cc cpptcl.o -o $@ ${CXXFLAGS} ${TESTFLAGS} ${INCLUDES} ${LIBDIRS} ${LIBS}

test6 : test6.cc cpptcl.o
	${COMPILER} test6.cc cpptcl.o -o $@ ${CXXFLAGS} ${TESTFLAGS} ${INCLUDES} ${LIBDIRS} ${LIBS}


cpptcl.o : ../cpptcl.cc ../cpptcl/cpptcl.h
	${COMPILER} ../cpptcl.cc -c -o $@ ${CXXFLAGS} ${TESTFLAGS} ${INCLUDES}


dotest :
	./test1
	tclsh8.6 test2.tcl
	./test3
	./test4
	./test5
	./test6

clean :
	rm -f ${OUTPUTS} cpptcl.o
