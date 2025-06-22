#ifndef exception_i
#define exception_i

%{
#include <TMQUtils/error.h>
%}

// Include SWIG's standard string handling
%include <std_string.i>

// Language-specific exception handling using SWIG preprocessor macros
#ifdef SWIGCSHARP
%typemap(throws, canthrow=1) TMQ::TMQException %{
    SWIG_CSharpSetPendingException(SWIG_CSharpApplicationException, $1.str().c_str());
    return $null;
%}
#endif

#ifdef SWIGPYTHON
%typemap(throws) TMQ::TMQException %{
    PyErr_SetString(PyExc_RuntimeError, $1.str().c_str());
    SWIG_fail;
%}
#endif

#endif