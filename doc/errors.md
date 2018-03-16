[[prev](goodies.md)][[top](index.md)][[next](compiling.md)]  

#### Error Handling  

All Tcl errors, conversion errors and others that may take place when calling Tcl code from C++ are translated and reported as exceptions of the tcl_error type:

```
namespace Tcl  
{  
     class tcl_error : public std::runtime_error  
     {  
          // ...  
     };  
}  
```

This means that the simplest framework for handling such errors may look like:

```
int main()  
{  
     try  
     {  
           interpreter i;  
           // ...  
     }  
     catch (tcl_error const &e)  
     {  
          // some Tcl error  
     }  
     catch (exception const &e)  
     {  
          // some other error  
     }  
}  
```

On the other hand, when calling C++ from Tcl, all C++ exceptions are translated and presented as regular Tcl errors, with error message taken from the C++ exception, if it was from the std::exception family. For all other C++ exceptions, the "Unknown error." message is returned.

[[prev](goodies.md)][[top](index.md)][[next](compiling.md)]  

* * *

Copyright © 2004-2006, Maciej Sobczak  

* * *

Copyright © 2017-2018, FlightAware LLC
