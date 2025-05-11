%module market_test
%{
#include <TMQMarket/market_test.h>
%}

// Make declspec macro a no-op for SWIG
#define TMQMarket_API

%include "stdint.i"
%include "typemaps.i"
%include "cpointer.i"
%include "std_string.i"
%include "std_except.i"
%include <windows.i>

%include "../../inc/market_test.h"