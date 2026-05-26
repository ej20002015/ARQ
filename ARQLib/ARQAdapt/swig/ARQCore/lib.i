#pragma once

%{
#include <ARQAdapt/ARQCore/ad_lib.h>
%}

%include <std_string.i>
%include <std_vector.i>

%rename("LibGuard") ARQ::LibGuard_Adapter;

%include "inc/ARQCore/ad_lib.h"