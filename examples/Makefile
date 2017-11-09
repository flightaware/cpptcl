COMPILER = clang++
CXXFLAGS = 
MODULEFLAGS = -shared
INCLUDES = -I.. -I/usr/local/include/tcl8.6
LIBDIRS = -L/usr/local/lib
LIBS = -ltclstub8.6

OUTPUTS = example2 example6

all : ${OUTPUTS}

example2 : example2.cc ../cpptcl.cc
	${COMPILER} example2.cc ../cpptcl.cc -o $@ ${CXXFLAGS} ${INCLUDES} ${LIBDIRS} ${LIBS}

example6 : example6.cc ../cpptcl.cc
	${COMPILER} example6.cc ../cpptcl.cc -o $@ ${CXXFLAGS} ${INCLUDES} ${LIBDIRS} ${LIBS}

clean :
	rm -f ${OUTPUTS}
