%module ARQLib

#ifdef SWIGCSHARP
%feature("nspace", 1);
#endif

%typemap(csclassmodifiers) SWIGTYPE "public partial class"

%include "defines.i"
%include "swig/common_types.i"
%include "swig/exceptions.i"

%include "swig/ARQUtils/id.i"
%include "swig/ARQUtils/time.i"
%include "swig/ARQUtils/sys.i"

%include "swig/ARQCore/lib.i"
%include "swig/ARQCore/semantic_format.i"
%include "swig/ARQCore/physical_type.i"
%include "swig/ARQCore/refdata_entities.i"
%include "swig/ARQCore/refdata_meta.i"
%include "swig/ARQCore/refdata_repository.i"