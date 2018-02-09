package require cpptcl_example_functions

proc pow {x y} {
   expr "$x ** $y"
}

hello
array set name {first "The" last "Dude"}
helloArray name
array set address {state "TX" city "Houston"}
helloArray2 name address
helloProc
