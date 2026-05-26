#pragma once

%{
#include <ARQUtils/error.h>
#include <stdexcept>
%}

%include "exception.i"

// This directive automatically wraps EVERY generated SWIG wrapper function inside a standard try/catch block
%exception {
    try
    {
        $action
    } 
    catch (const ARQ::ARQException& e)
    {
        SWIG_exception( SWIG_RuntimeError, e.str().c_str() );
    } 
    catch (const std::exception& e)
    {
        SWIG_exception( SWIG_SystemError, e.what() );
    } 
    catch (...)
    {
        SWIG_exception( SWIG_UnknownError, "Unknown C++ exception thrown" );
    }
}