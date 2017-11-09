set ext [info sharedlibextension]
load "build/libcpptcl_module_one$ext"
hello
set first "The"
set last "Dude"
helloVar first last
