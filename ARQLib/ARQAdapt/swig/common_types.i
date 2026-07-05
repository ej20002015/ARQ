#pragma once

%include <stdint.i>
%include <std_string.i>
%include <std_string_view.i>
%include <std_vector.i>
%include <std_map.i>
%include <std_shared_ptr.i>

// For std::optional
#define SWIG_STD_OPTIONAL_DEFAULT_TYPES
#ifdef SWIGCSHARP
#define SWIG_STD_OPTIONAL_USE_NULLABLE_REFERENCE_TYPES
%include "std/std_optional_csharp.i"
#endif
#ifdef SWIGPYTHON
%include "std/std_optional_python.i"
#endif

// Strings
%nspacemove(ARQ::Collections) std::vector<std::string>;
%template(StringVector) std::vector<std::string>;
%nspacemove(ARQ::Collections) std::vector<std::string_view>;
%template(StringViewVector) std::vector<std::string_view>;

// Primitives
%nspacemove(ARQ::Collections) std::vector<int32_t>;
%template(Int32Vector)  std::vector<int32_t>;
%nspacemove(ARQ::Collections) std::vector<int64_t>;
%template(Int64Vector)  std::vector<int64_t>;
%nspacemove(ARQ::Collections) std::vector<double>;
%template(DoubleVector) std::vector<double>;