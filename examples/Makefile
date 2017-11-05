COMPILER = g++
CXXFLAGS = -Wall -Wno-long-long -pedantic
MODULEFLAGS = -shared
INCLUDES = -I.. -I/usr/local/include/tcl8.6 -I/usr/local/include/boost
LIBDIRS = -L/usr/local/lib
LIBS = -ltcl84

OUTPUTS = mymodule1.so example2 mymodule3.so mymodule4.so mymodule5.so example6 mymodule7.so

all : ${OUTPUTS}


mymodule1.so : example1.cc ../cpptcl.cc
	${COMPILER} ${MODULEFLAGS} example1.cc ../cpptcl.cc -o $@ ${CXXFLAGS} ${INCLUDES}

example2 : example2.cc ../cpptcl.cc
	${COMPILER} example2.cc ../cpptcl.cc -o $@ ${CXXFLAGS} ${INCLUDES} ${LIBDIRS} ${LIBS}

mymodule3.so : example3.cc ../cpptcl.cc
	${COMPILER} ${MODULEFLAGS} example3.cc ../cpptcl.cc -o $@ ${CXXFLAGS} ${INCLUDES}

mymodule4.so : example4.cc ../cpptcl.cc
	${COMPILER} ${MODULEFLAGS} example4.cc ../cpptcl.cc -o $@ ${CXXFLAGS} ${INCLUDES}

mymodule5.so : example5.cc ../cpptcl.cc
	${COMPILER} ${MODULEFLAGS} example5.cc ../cpptcl.cc -o $@ ${CXXFLAGS} ${INCLUDES}

example6 : example6.cc ../cpptcl.cc
	${COMPILER} example6.cc ../cpptcl.cc -o $@ ${CXXFLAGS} ${INCLUDES} ${LIBDIRS} ${LIBS}

mymodule7.so : example7.cc ../cpptcl.cc
	${COMPILER} ${MODULEFLAGS} example7.cc ../cpptcl.cc -o $@ ${CXXFLAGS} ${INCLUDES}


clean :
	rm -f ${OUTPUTS}
