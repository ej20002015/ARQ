#pragma once

%include <stdint.i>
%include <std_string.i>
%include <std_vector.i>
%include <std_map.i>
%include <std_shared_ptr.i>

// Strings
%nspacemove(ARQ::Collections) std::vector<std::string>;
%template(StringVector) std::vector<std::string>;

// Primitives
%nspacemove(ARQ::Collections) std::vector<int32_t>;
%template(Int32Vector)  std::vector<int32_t>;
%nspacemove(ARQ::Collections) std::vector<int64_t>;
%template(Int64Vector)  std::vector<int64_t>;
%nspacemove(ARQ::Collections) std::vector<double>;
%template(DoubleVector) std::vector<double>;