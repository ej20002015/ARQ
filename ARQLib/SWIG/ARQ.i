%module(directors="1") ARQLib

%feature("nspace", 1);

%include "ARQUtils/exception.i"
%include "ARQUtils/time.i"
%include "ARQCore/mktdata_entities.i"
%include "ARQMarket/market.i"
%include "ARQMarket/managed_market.i"