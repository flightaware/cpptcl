  
[[prev](goodies.html)][[top](index.html)][[next](compiling.html)]  

#### Error Handling  

All Tcl errors, conversion errors and others that may take place when calling Tcl code from C++ are translated and reported as exceptions of the `tcl_error` type:

<div style="margin-left: 40px;">`namespace Tcl`  
`{`  
`     class tcl_error : public std::runtime_error`  
`     {`  
`          // ...`  
`     };`  
`}`  
</div>

This means that the simplest framework for handling such errors may look like:

<div style="margin-left: 40px;">`int main()`  
`{`  
`     try`  
`     {`  
`           interpreter i;`  
`           // ...`  
`     }`  
`     catch (tcl_error const &e)`  
`     {`  
`          // some Tcl error`  
`     }`  
`     catch (exception const &e)`  
`     {`  
`          // some other error`  
`     }`  
`}`  
</div>

On the other hand, when calling C++ from Tcl, all C++ exceptions are translated and presented as regular Tcl errors, with error message taken from the C++ exception, if it was from the `std::exception` family. For all other C++ exceptions, the `"Unknown error."` message is returned.

[[prev](goodies.html)][[top](index.html)][[next](compiling.html)]  

* * *

Copyright © 2004-2006, Maciej Sobczak  

* * *

Copyright © 2017-2018, FlightAware LLC
