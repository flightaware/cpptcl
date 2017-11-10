set ext [info sharedlibextension]
load "build/libcpptcl_module_one$ext"
hello
set first "The"
set last "Dude"
helloVar first last
array set name {first "The" last "Dude"}
helloArray name
array set address {state "TX" city "Houston"}
helloArray2 name address
